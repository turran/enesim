/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2011 Jorge Luis Zapata
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
#include "enesim_error.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"

#include "enesim_buffer_private.h"
#include "enesim_renderer_private.h"

#if BUILD_OPENCL
#include "Enesim_OpenCL.h"
#endif
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static Eina_Hash *_program_lut = NULL;

/* smallest multiple of local_size bigger than num */
static size_t _roundup(size_t local_size, size_t num)
{
	size_t ret = local_size;
	while (ret < num)
		ret += local_size;
	return ret;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Eina_Bool enesim_renderer_opencl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Error **error)
{
	Enesim_Renderer_OpenCL_Data *rdata;
	Enesim_Buffer_OpenCL_Data *data;
	Eina_Bool ret;
	cl_program program;
	cl_int cl_err;
	const char *source = NULL;
	const char *source_name = NULL;
	size_t source_size = 0;

	data = enesim_surface_backend_data_get(s);

	/* on the sw version we set the fill function, here we need to set
	 * the string of the program so the generic code can compile it
	 * we need a way to get a uniqueid of the program too to not compile
	 * it every time, something like a token
	 */
	if (!r->descriptor.opencl_setup) return EINA_FALSE;
	ret = r->descriptor.opencl_setup(r, states, s, &source_name, &source, &source_size, error);
	printf("loading the opencl description %s\n", source);
	if (!ret) return EINA_FALSE;

	program = clCreateProgramWithSource(data->context, 1, &source, &source_size, &cl_err);
	if (cl_err != CL_SUCCESS)
	{
		printf("1 cl_err %d\n", cl_err);
		exit(1);
	}

	/* build the program */
	cl_err = clBuildProgram(program, 1, &data->device, NULL, NULL, NULL);
	if (cl_err != CL_SUCCESS)
	{
		char *build_log;
		size_t log_size;

		/* to know the size of the log */
		clGetProgramBuildInfo(program, data->device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
		build_log = malloc(log_size + 1);
		/* now the log itself */
		clGetProgramBuildInfo(program, data->device, CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
		build_log[log_size] = '\0';
		printf("log %s\n", build_log);
		free(build_log);
		return EINA_FALSE;
	}

	/* create it! */
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENCL);
	if (!rdata)
	{
		rdata = calloc(1, sizeof(Enesim_Renderer_OpenCL_Data));
		enesim_renderer_backend_data_set(r, ENESIM_BACKEND_OPENCL, rdata);
	}
	rdata->kernel = clCreateKernel(program, source_name, &cl_err);
	if (cl_err != CL_SUCCESS)
	{
		printf("3 cl_err %d\n", cl_err);
		r->backend_data[ENESIM_BACKEND_OPENCL] = NULL;
		free(rdata);
		return EINA_FALSE;
	}
	printf("everything ok\n");

	return EINA_TRUE;
}

void enesim_renderer_opencl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	//clReleaseKernel(vector_add_k);
}

void enesim_renderer_opencl_draw(Enesim_Renderer *r, Enesim_Surface *s, Eina_Rectangle *area,
		int x, int y)
{
	Enesim_Renderer_OpenCL_Data *rdata;
	Enesim_Buffer_OpenCL_Data *sdata;
	cl_int error;
	size_t local_ws[2];
	size_t global_ws[2];
	size_t max_local;

	printf("inside!!\n");
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENCL);
	sdata = enesim_surface_backend_data_get(s);
	error = clSetKernelArg(rdata->kernel, 0, sizeof(cl_mem), &sdata->mem);
	assert(error == CL_SUCCESS);
	/* now setup the kernel on the renderer side */
	if (r->descriptor.opencl_kernel_setup)
	{
		if (!r->descriptor.opencl_kernel_setup(r, s))
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
	printf("area %d %d [%d %d] [%d %d] %d\n", area->w, area->h, global_ws[0], global_ws[1], local_ws[0], local_ws[1], max_local);
	/* launch it!!! */
	error = clEnqueueNDRangeKernel(sdata->queue, rdata->kernel, 2, NULL, global_ws, local_ws, 0, NULL, NULL);
	if (error != CL_SUCCESS)
	{
		printf("4 error %d\n", error);
		exit(1);
	}
}

void enesim_renderer_opencl_init(void)
{
	/* maybe we should add a hash to match between kernel name and program */
}

void enesim_renderer_opencl_shutdown(void)
{

}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

