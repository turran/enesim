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
#ifndef ENESIM_RENDERER_SIMPLE_PRIVATE_H_
#define ENESIM_RENDERER_SIMPLE_PRIVATE_H_

typedef struct _Enesim_Renderer_Simple_Descriptor
{
	Enesim_Renderer_Base_Name_Get_Cb name_get;
	Enesim_Renderer_Delete free;
	Enesim_Renderer_Bounds_Get_Cb bounds_get;
	Enesim_Renderer_Destination_Bounds_Get_Cb destination_bounds_get;
	Enesim_Renderer_Flags_Get_Cb flags_get;
	Enesim_Renderer_Sw_Hints_Get_Cb hints_get;
	Enesim_Renderer_Is_Inside_Cb is_inside;
	Enesim_Renderer_Damages_Get_Cb damages_get;
	Enesim_Renderer_Has_Changed_Cb has_changed;
	/* software based functions */
	Enesim_Renderer_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	/* opencl based functions */
	Enesim_Renderer_OpenCL_Setup opencl_setup;
	Enesim_Renderer_OpenCL_Kernel_Setup opencl_kernel_setup;
	Enesim_Renderer_OpenCL_Cleanup opencl_cleanup;
	/* opengl based functions */
	Enesim_Renderer_OpenGL_Initialize opengl_initialize;
	Enesim_Renderer_OpenGL_Setup opengl_setup;
	Enesim_Renderer_OpenGL_Cleanup opengl_cleanup;
} Enesim_Renderer_Simple_Descriptor;

Enesim_Renderer * enesim_renderer_simple_new(
		Enesim_Renderer_Simple_Descriptor *descriptor, void *data);
void * enesim_renderer_simple_data_get(Enesim_Renderer *r);
void enesim_renderer_simple_state_get(Enesim_Renderer *r,
		const Enesim_Renderer_State2 **state);
void enesim_renderer_simple_transformation_type_get(Enesim_Renderer *r,
		Enesim_Matrix_Type *type);

#endif
