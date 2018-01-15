/* ENESIM - Drawing Library
 * Copyright (C) 2007-2018 Jorge Luis Zapata
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
#ifndef ENESIM_OPENCL_PRIVATE_H_
#define ENESIM_OPENCL_PRIVATE_H_

#include "CL/cl.h"

typedef struct _Enesim_Renderer_OpenCL_Context_Program
{
	cl_program program;
	char *program_name;
	Eina_Bool cached;
} Enesim_Renderer_OpenCL_Context_Program;

struct _Enesim_Renderer_OpenCL_Context_Data
{
	cl_context context;
	char *name;
	Enesim_Renderer_OpenCL_Context_Program *lib;
	Enesim_Renderer_OpenCL_Context_Program *header;
	Eina_Hash *programs;
};

void enesim_opencl_init(void);
void enesim_opencl_shutdown(void);

Enesim_Renderer_OpenCL_Context_Program *
enesim_renderer_opencl_context_program_new(void);
void enesim_renderer_opencl_context_program_free(
		Enesim_Renderer_OpenCL_Context_Program *thiz);
Enesim_Renderer_OpenCL_Context_Data *
enesim_renderer_opencl_context_data_new(void);

void enesim_renderer_opencl_context_data_free(
		Enesim_Renderer_OpenCL_Context_Data *thiz);

Enesim_Renderer_OpenCL_Context_Data * enesim_renderer_opencl_context_data_get(
		cl_device_id device);

#endif
