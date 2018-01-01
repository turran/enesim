/* ENESIM - Drawing Library
 * Copyright (C) 2007-2017 Jorge Luis Zapata
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
#ifndef ENESIM_RENDERER_CL_PRIVATE_H_
#define ENESIM_RENDERER_CL_PRIVATE_H_

#if BUILD_OPENCL
#include "Enesim_OpenCL.h"
#endif

typedef enum _Enesim_Renderer_OpenCL_Kernel_Mode
{
	ENESIM_RENDERER_OPENCL_KERNEL_MODE_PIXEL,
	ENESIM_RENDERER_OPENCL_KERNEL_MODE_HSPAN,
} Enesim_Renderer_OpenCL_Kernel_Mode;

#if BUILD_OPENCL
typedef struct _Enesim_Renderer_OpenCL_Data
{
	/* to cache the kernel_get */
	cl_context context;
	cl_device_id device;
	cl_kernel kernel;
	const char *kernel_name;
	Enesim_Renderer_OpenCL_Kernel_Mode mode;
} Enesim_Renderer_OpenCL_Data;

#endif

void enesim_renderer_opencl_init(void);
void enesim_renderer_opencl_shutdown(void);
void enesim_renderer_opencl_free(Enesim_Renderer *r);

Eina_Bool enesim_renderer_opencl_setup(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, Enesim_Log **error);
void enesim_renderer_opencl_cleanup(Enesim_Renderer *r, Enesim_Surface *s);
void enesim_renderer_opencl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, const Eina_Rectangle *area, int x, int y);

Eina_Bool enesim_renderer_opencl_setup_default(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, Enesim_Log **error);
void enesim_renderer_opencl_cleanup_default(Enesim_Renderer *r, Enesim_Surface *s);
void enesim_renderer_opencl_draw_default(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, const Eina_Rectangle *area, int x, int y);

#endif
