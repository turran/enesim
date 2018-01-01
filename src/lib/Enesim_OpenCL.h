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

typedef struct _Enesim_Renderer_OpenCL_Data
{
	cl_context context;
	cl_device_id device;
	cl_kernel kernel;
	const char *kernel_name;
} Enesim_Renderer_OpenCL_Data;

typedef struct _Enesim_Buffer_OpenCL_Data
{
	cl_mem mem;
	cl_context context;
	cl_device_id device;
	cl_command_queue queue;
} Enesim_Buffer_OpenCL_Data;

EAPI Enesim_Pool * enesim_pool_opencl_new(void);

#endif
