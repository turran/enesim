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

#if BUILD_OPENCL
#include "Enesim_OpenCL.h"
#endif

#include "enesim_opencl_private.h"
#include "enesim_buffer_private.h"
#include "enesim_surface_private.h"
#include "enesim_renderer_private.h"

#define ENESIM_LOG_DEFAULT enesim_log_renderer
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/*----------------------------------------------------------------------------*
 *                               Helpers                                      *
 *----------------------------------------------------------------------------*/
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
#if 0
	if (rdata->context)
	{
		clReleaseContext(rdata->context);
		rdata->context = NULL;
	}
#endif
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

static void _compile_error(cl_program program, cl_device_id device)
{
	char *build_log;
	size_t log_size;

	/* to know the size of the log */
	clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
	build_log = malloc(log_size + 1);
	/* now the log itself */
	clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, build_log, NULL);
	build_log[log_size] = '\0';
	ERR("Compilation error log: %s", build_log);
	free(build_log);
}

static Eina_Bool enesim_renderer_opencl_compile_kernel(
	Enesim_Renderer_OpenCL_Data *thiz, Enesim_Renderer *r,
	const char *source_name, const char *source, size_t source_size,
	const char *kernel_name)
{
	Enesim_OpenCL_Context *cdata;
	Enesim_OpenCL_Context_Program *cprogram;
	cl_int cl_err;
	cl_kernel kernel;

	cdata = (Enesim_OpenCL_Context *)thiz->context;
	if (!cdata->header)
	{
		cl_program hdr_program;
		size_t code_size;
		const char *code;

		/* create the header program, no need to compile it */
		code = 
		#include "enesim_main_opencl_private.cl"
		#include "enesim_renderer_shape_opencl_private.cl"
		#include "enesim_coord_opencl_private.cl"
		#include "enesim_color_opencl_private.cl"
		#include "enesim_color_blend_opencl_private.cl"
		;
		code_size = strlen(code);
		hdr_program = clCreateProgramWithSource(thiz->context->context,
				1, &code, &code_size, &cl_err);
		if (cl_err != CL_SUCCESS)
		{
			ERR("Can not create application header (err: %d)",
					cl_err);
			return EINA_FALSE;
		}
		cprogram = enesim_opencl_context_program_new();
		cprogram->program = hdr_program;
		cdata->header = cprogram;
	}

	if (!cdata->lib)
	{
		cl_program lib_program, program;
		size_t code_size;
		const char *code;

		/* compile the common library */
		code = 
		#include "enesim_coord_opencl.cl"
		#include "enesim_color_opencl.cl"
		#include "enesim_color_blend_opencl.cl"
		;
		code_size = strlen(code);
		program = clCreateProgramWithSource(thiz->context->context, 1,
				&code, &code_size, &cl_err);
		if (cl_err != CL_SUCCESS)
		{
			ERR("Can not create application library (err: %d)",
					cl_err);
			return EINA_FALSE;
		}
		cl_err = clCompileProgram(program, 1, &thiz->device,
				"-cl-std=CL2.0", 0, NULL, NULL, NULL, NULL);
		if (cl_err != CL_SUCCESS)
		{
			ERR("Can not compile application library (err: %d)",
					cl_err);
			_compile_error(program, thiz->device);
			return EINA_FALSE;

		}
		lib_program = clLinkProgram(thiz->context->context, 1,
				&thiz->device, "-create-library", 1, &program,
				NULL, NULL, &cl_err);
		if (cl_err != CL_SUCCESS)
		{
			ERR("Can not link application library (err: %d)",
					cl_err);
			_compile_error(program, thiz->device);
			return EINA_FALSE;
		}
		cprogram = enesim_opencl_context_program_new();
		cprogram->program = lib_program;
		cdata->lib = cprogram;
		clReleaseProgram(program);
	}

	cprogram = eina_hash_find(cdata->programs, source_name);
	if (!cprogram)
	{
		cprogram = enesim_opencl_context_program_new();
		eina_hash_add(cdata->programs, source_name, cprogram);
	}
	if (!cprogram->program)
	{
		const char *header_name = "enesim_opencl.h";
		cl_program program;
		cl_program programs[2];

		programs[0] = clCreateProgramWithSource(thiz->context->context,
				1, &source, &source_size, &cl_err);
		programs[1] = cdata->lib->program;

		if (cl_err != CL_SUCCESS)
		{
			ERR("Can not create program for renderer %s (err: %d)", r->name, cl_err);
			return EINA_FALSE;
		}

		/* build the program */
		cl_err = clCompileProgram(programs[0], 1, &thiz->device,
				"-cl-std=CL2.0", 1, &cdata->header->program,
				&header_name, NULL, NULL);
		if (cl_err != CL_SUCCESS)
		{
			ERR("Can not build program for renderer %s (err: %d)",
					r->name, cl_err);
			_compile_error(programs[0], thiz->device);
			return EINA_FALSE;
		}
		program = clLinkProgram(thiz->context->context, 1,
				&thiz->device, NULL, 2, programs, NULL, NULL,
				&cl_err);
		if (cl_err != CL_SUCCESS)
		{
			ERR("Can not link program for renderer %s (err: %d)",
					r->name, cl_err);
			_compile_error(programs[0], thiz->device);
			_compile_error(programs[1], thiz->device);
			return EINA_FALSE;
		}

		clReleaseProgram(programs[0]);
		cprogram->program = program;
	}
	kernel = clCreateKernel(cprogram->program, kernel_name, &cl_err);
	if (cl_err != CL_SUCCESS)
	{
		ERR("Can not create kernel '%s' for renderer %s (err: %d)",
				kernel_name, r->name, cl_err);
		return EINA_FALSE;
	}
	thiz->kernel = kernel;
	thiz->kernel_name = kernel_name;
	return EINA_TRUE;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Eina_Bool enesim_renderer_opencl_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Log **error)
{
	Enesim_Renderer_Class *klass;
	Eina_Bool ret = EINA_FALSE;

	klass = ENESIM_RENDERER_CLASS_GET(r);
	if (klass->opencl_setup)
	{
		ret = klass->opencl_setup(r, s, rop, error);
	}
	return ret;
}

void enesim_renderer_opencl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS_GET(r);
	if (klass->opencl_cleanup)
	{
		klass->opencl_cleanup(r, s);
	}
}

void enesim_renderer_opencl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, const Eina_Rectangle *area, int x, int y)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS_GET(r);
	if (klass->opencl_draw)
	{
		klass->opencl_draw(r, s, rop, area, x, y);
	}
}

/*----------------------------------------------------------------------------*
 *                          Default implementation                            *
 *----------------------------------------------------------------------------*/
Eina_Bool enesim_renderer_opencl_setup_default(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Log **error)
{
	Enesim_Renderer_OpenCL_Data *rdata;
	Enesim_Buffer_OpenCL_Data *sdata;
	Enesim_Renderer_Class *klass;
	Eina_Bool ret;
	cl_int cl_rop = rop;
	const char *source = NULL;
	const char *source_name = NULL;
	const char *kernel_name = NULL;
	size_t source_size = 0;

	klass = ENESIM_RENDERER_CLASS_GET(r);
	sdata = enesim_surface_backend_data_get(s);

	/* on the sw version we set the fill function, here we need to set
	 * the string of the program so the generic code can compile it
	 * we need a way to get a uniqueid of the program too to not compile
	 * it every time, something like a token
	 */
	if (!klass->opencl_kernel_get)
		return EINA_FALSE;

	ret = klass->opencl_kernel_get(r, s, rop, &source_name, &source,
			&source_size, &kernel_name);
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
		rdata->mode = ENESIM_RENDERER_OPENCL_KERNEL_MODE_PIXEL;
		enesim_renderer_backend_data_set(r, ENESIM_BACKEND_OPENCL, rdata);
	}
	else
	{
		Eina_Bool release = EINA_FALSE;

		if (rdata->context != sdata->context)
			release = EINA_TRUE;
		if (rdata->device != sdata->device)
			release = EINA_TRUE;
		if (rdata->kernel_name != kernel_name)
			release = EINA_TRUE;

		if (release)
			_release(rdata);
	}
	if (!rdata->context)
	{
		rdata->context = sdata->context;
	}
	if (!rdata->device)
	{
		rdata->device = sdata->device;
		clRetainDevice(rdata->device);
	}
	if (!rdata->kernel)
	{
		if (!enesim_renderer_opencl_compile_kernel(rdata, r, source_name,
				source, source_size, kernel_name))
			return EINA_FALSE;
	}

	clSetKernelArg(rdata->kernel, 1, sizeof(cl_int), &cl_rop);
	/* now setup the kernel on the renderer side */
	if (klass->opencl_kernel_setup)
	{
		if (!klass->opencl_kernel_setup(r, s, 2, &rdata->mode))
		{
			ERR("Cannot setup the kernel for renderer %s", r->name);
			return EINA_FALSE;
		}
	}

	return EINA_TRUE;
}

void enesim_renderer_opencl_cleanup_default(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS_GET(r);
	if (klass->opencl_kernel_cleanup)
	{
		klass->opencl_kernel_cleanup(r, s);
	}
}

void enesim_renderer_opencl_draw_default(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, const Eina_Rectangle *area, int x, int y)
{
	Enesim_Renderer_OpenCL_Data *rdata;
	Enesim_Buffer_OpenCL_Data *sdata;
	cl_int cl_err;
	size_t local_ws[2];
	size_t global_ws[2];
	size_t max_local;
	cl_uint ndim = 2;

	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENCL);
	sdata = enesim_surface_backend_data_get(s);
	clGetDeviceInfo(sdata->device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(size_t), &max_local, NULL);
	switch (rdata->mode)
	{
		case ENESIM_RENDERER_OPENCL_KERNEL_MODE_PIXEL:
		local_ws[0] = 16; /* Number of work-items per work-group */
		local_ws[1] = 16; /* Number of work-items per work-group */

		global_ws[0] = _roundup(local_ws[0], area->w);
		global_ws[1] = _roundup(local_ws[1], area->h);
		ndim = 2;
		break;

		case ENESIM_RENDERER_OPENCL_KERNEL_MODE_HSPAN:
		local_ws[0] = 16; /* Number of work-items per work-group */
		global_ws[0] = _roundup(local_ws[0], area->h);
		ndim = 1;
		break;
	}

	/* only on the final moment add the surface, we should not use the
	 * surface done at setup, otherwise the renderers can not use a
	 * temporary surface to draw inner renderers
	 */
	clSetKernelArg(rdata->kernel, 0, sizeof(cl_mem), &sdata->mem[0]);
	/* launch it!!! */
	cl_err = clEnqueueNDRangeKernel(sdata->queue, rdata->kernel, ndim, NULL, global_ws, NULL, 0, NULL, NULL);
	if (cl_err != CL_SUCCESS)
	{
		ERR("Can not enqueue kernel for renderer %s (err: %d)",
				r->name, cl_err);
	}
}

void enesim_renderer_opencl_init(void)
{
}

void enesim_renderer_opencl_shutdown(void)
{
}

void enesim_renderer_opencl_free(Enesim_Renderer *r)
{
	Enesim_Renderer_OpenCL_Data *cl_data;

	cl_data = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENCL);
	if (!cl_data)
		return;
	_release(cl_data);
	free(cl_data);
}

void enesim_renderer_opencl_kernel_transformation_add(Enesim_Renderer *r,
		int *argc)
{
	Enesim_Renderer_OpenCL_Data *rdata;
	Enesim_Matrix matrix, inv;
	cl_float cl_matrix[9];
	cl_mem cl_mmatrix;

	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENCL);
	enesim_renderer_transformation_get(r, &matrix);
	enesim_matrix_inverse(&matrix, &inv);
	cl_matrix[0] = inv.xx; cl_matrix[1] = inv.xy; cl_matrix[2] = inv.xz;
	cl_matrix[3] = inv.yx; cl_matrix[4] = inv.yy; cl_matrix[5] = inv.yz;
	cl_matrix[6] = inv.zx; cl_matrix[7] = inv.zy; cl_matrix[8] = inv.zz;
	cl_mmatrix = clCreateBuffer(rdata->context->context,
			CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
			sizeof(cl_matrix), &cl_matrix, NULL);
	clSetKernelArg(rdata->kernel, (*argc)++, sizeof(cl_mem), (void *)&cl_mmatrix);
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

