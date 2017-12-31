/* ENESIM - Drawing Library
 * Copyright (C) 2007-2013 Jorge Luis Zapata
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#include "enesim_private.h"

#include <assert.h>

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_buffer_private.h"
#include "enesim_surface_private.h"
#include "enesim_renderer_private.h"

#if BUILD_OPENCL
#include "Enesim_OpenCL.h"
#endif

#define ENESIM_LOG_DEFAULT enesim_log_renderer
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
static Eina_Hash *_program_lut = NULL;

/* smallest multiple of local_size bigger than num */
static size_t _roundup(size_t local_size, size_t num)
{
	size_t ret = local_size;
	while (ret < num)
		ret += local_size;
	return ret;
}

static void _release(Enesim_Renderer_OpenCL_Data *rdata)
{
	if (rdata->context)
	{
		clReleaseContext(rdata->context);
		rdata->context = NULL;
	}
	if (rdata->device)
	{
		clReleaseDevice(rdata->device);
		rdata->device = NULL;
	}
	if (rdata->kernel)
	{
		clReleaseKernel(rdata->kernel);
		rdata->kernel = NULL;
	}
	if (rdata->kernel_name)
	{
		rdata->kernel_name = NULL;
	}
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Eina_Bool enesim_renderer_opencl_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Log **error)
{
	Enesim_Renderer_Class *klass;
	Enesim_Renderer_OpenCL_Data *rdata;
	Enesim_Buffer_OpenCL_Data *sdata;
	Eina_Bool ret;
	const char *source = NULL;
	const char *source_name = NULL;
	size_t source_size = 0;

	klass = ENESIM_RENDERER_CLASS_GET(r);
	sdata = enesim_surface_backend_data_get(s);

	/* on the sw version we set the fill function, here we need to set
	 * the string of the program so the generic code can compile it
	 * we need a way to get a uniqueid of the program too to not compile
	 * it every time, something like a token
	 */
	if (!klass->opencl_setup)
		return EINA_FALSE;

	ret = klass->opencl_setup(r, s, rop, &source_name, &source, &source_size, error);
	if (!ret)
	{
		ERR("Can not setup the renderer %s", r->name);
		return EINA_FALSE;
	}

	/* create it! */
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENCL);
	if (!rdata)
	{
		rdata = calloc(1, sizeof(Enesim_Renderer_OpenCL_Data));
		enesim_renderer_backend_data_set(r, ENESIM_BACKEND_OPENCL, rdata);
	}
	else
	{
		Eina_Bool release = EINA_FALSE;

		if (rdata->context != sdata->context)
			release = EINA_TRUE;
		if (rdata->device != sdata->device)
			release = EINA_TRUE;
		if (rdata->kernel_name != source_name)
			release = EINA_TRUE;

		if (release)
			_release(rdata);
	}
	if (!rdata->context)
	{
		rdata->context = sdata->context;
		clRetainContext(rdata->context);
	}
	if (!rdata->device)
	{
		rdata->device = sdata->device;
		clRetainDevice(rdata->device);
	}
	if (!rdata->kernel)
	{
		cl_int cl_err;
		cl_program program;

		program = clCreateProgramWithSource(rdata->context, 1, &source,
				&source_size, &cl_err);
		if (cl_err != CL_SUCCESS)
		{
			ERR("Can not create program for renderer %s (err: %d)", r->name, cl_err);
			return EINA_FALSE;
		}

		/* build the program */
		cl_err = clBuildProgram(program, 1, &rdata->device, "-cl-std=CL2.0", NULL,
				NULL);
		if (cl_err != CL_SUCCESS)
		{
			char *build_log;
			size_t log_size;

			/* to know the size of the log */
			clGetProgramBuildInfo(program, rdata->device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
			build_log = malloc(log_size + 1);
			/* now the log itself */
			clGetProgramBuildInfo(program, rdata->device, CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
			build_log[log_size] = '\0';
			ERR("Can not build program for renderer %s (err: %d, log: %s)",
					r->name, cl_err, build_log);
			free(build_log);
			return EINA_FALSE;
		}

		rdata->kernel = clCreateKernel(program, source_name, &cl_err);
		if (cl_err != CL_SUCCESS)
		{
			ERR("Can not create kernel for renderer %s (err: %d)",
					r->name, cl_err);
			return EINA_FALSE;
		}
		rdata->kernel_name = source_name;
	}

	return EINA_TRUE;
}

void enesim_renderer_opencl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	//clReleaseKernel(vector_add_k);
}

void enesim_renderer_opencl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, Eina_Rectangle *area, int x, int y)
{
	Enesim_Renderer_Class *klass;
	Enesim_Renderer_OpenCL_Data *rdata;
	Enesim_Buffer_OpenCL_Data *sdata;
	cl_int cl_err;
	size_t local_ws[2];
	size_t global_ws[2];
	size_t max_local;

	//printf("inside!!\n");
	klass = ENESIM_RENDERER_CLASS_GET(r);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENCL);
	sdata = enesim_surface_backend_data_get(s);
	clSetKernelArg(rdata->kernel, 0, sizeof(cl_mem), &sdata->mem);
	/* now setup the kernel on the renderer side */
	if (klass->opencl_kernel_setup)
	{
		if (!klass->opencl_kernel_setup(r, s))
		{
			printf("Cannot setup the kernel\n");
			return;
		}
	}
	clGetDeviceInfo(sdata->device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &max_local, NULL);
 	local_ws[0] = 16; /* Number of work-items per work-group */
 	local_ws[1] = 16; /* Number of work-items per work-group */

	global_ws[0] = _roundup(local_ws[0], area->w);
	global_ws[1] = _roundup(local_ws[1], area->h);
	/* launch it!!! */
	cl_err = clEnqueueNDRangeKernel(sdata->queue, rdata->kernel, 2, NULL, global_ws, NULL, 0, NULL, NULL);
	if (cl_err != CL_SUCCESS)
	{
		CRI("Can not enqueue kenrel for renderer %s", r->name);
	}
}

void enesim_renderer_opencl_init(void)
{
	/* maybe we should add a hash to match between kernel name and program */
}

void enesim_renderer_opencl_shutdown(void)
{

}

void enesim_renderer_opencl_free(Enesim_Renderer *r)
{
	Enesim_Renderer_OpenCL_Data *cl_data;

	cl_data = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENCL);
	if (!cl_data) return;
	_release(cl_data);
	free(cl_data);
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

