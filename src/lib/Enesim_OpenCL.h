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
#ifndef ENESIM_OPENCL_H_
#define ENESIM_OPENCL_H_

#include "CL/cl.h"

#define ENESIM_OPENCL_CODE(...) #__VA_ARGS__

typedef struct _Enesim_Renderer_OpenCL_Context_Data Enesim_Renderer_OpenCL_Context_Data;

typedef struct _Enesim_Buffer_OpenCL_Data
{
	const Enesim_Renderer_OpenCL_Context_Data *context;
	cl_mem *mem;
	cl_device_id device;
	cl_command_queue queue;
	unsigned int num_mems;
} Enesim_Buffer_OpenCL_Data;

EAPI Enesim_Pool * enesim_pool_opencl_new(void);
EAPI Enesim_Pool * enesim_pool_opencl_new_platform_from(cl_platform_id platform_id, cl_device_type device_type);
EAPI Enesim_Pool * enesim_pool_opencl_new_device_from(cl_device_id device_id);

EAPI Enesim_Buffer * enesim_buffer_new_opencl_pool_and_data_from(Enesim_Buffer_Format f,
		uint32_t w, uint32_t h, Enesim_Pool *pool,
		Enesim_Buffer_OpenCL_Data *backend_data);

#ifdef ENESIM_OPENGL_H
EAPI Enesim_Buffer * enesim_pool_opencl_new_buffer_opengl_from(
		Enesim_Pool *p, Enesim_Format f, uint32_t w, uint32_t h,
		Enesim_Buffer_OpenGL_Data *gl_data);
#endif

#endif
