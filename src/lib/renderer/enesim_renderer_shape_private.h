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
#ifndef SHAPE_H_
#define SHAPE_H_

typedef struct _Enesim_Renderer_Shape_State2
{
	struct {
		struct {
			Enesim_Color color;
			Enesim_Renderer *r;
			double weight;
			Enesim_Shape_Stroke_Location location;
			Enesim_Shape_Stroke_Cap cap;
			Enesim_Shape_Stroke_Join join;
		} stroke;

		struct {
			Enesim_Color color;
			Enesim_Renderer *r;
			Enesim_Shape_Fill_Rule rule;
		} fill;
		Enesim_Shape_Draw_Mode draw_mode;
	} current, past;
	Eina_List *stroke_dashes;

	Eina_Bool stroke_dashes_changed;
	Eina_Bool changed;
} Enesim_Renderer_Shape_State2;

/* common shape renderer functions */
typedef struct _Enesim_Renderer_Shape_State
{
	struct {
		Enesim_Color color;
		Enesim_Renderer *r;
		double weight;
		Enesim_Shape_Stroke_Location location;
		Enesim_Shape_Stroke_Cap cap;
		Enesim_Shape_Stroke_Join join;
		Eina_List *dashes;
	} stroke;

	struct {
		Enesim_Color color;
		Enesim_Renderer *r;
		Enesim_Shape_Fill_Rule rule;
	} fill;
	Enesim_Shape_Draw_Mode draw_mode;
} Enesim_Renderer_Shape_State;


typedef void (*Enesim_Renderer_Shape_Feature_Get_Cb)(Enesim_Renderer *r, Enesim_Shape_Feature *features);

typedef struct _Enesim_Renderer_Shape_Descriptor {
	unsigned int version;
	Enesim_Renderer_Base_Name_Get_Cb base_name_get;
	Enesim_Renderer_Delete_Cb free;
	Enesim_Renderer_Bounds_Get_Cb bounds_get;
	Enesim_Renderer_Destination_Bounds_Get_Cb destination_bounds_get;
	Enesim_Renderer_Flags_Get_Cb flags_get;
	Enesim_Renderer_Is_Inside_Cb is_inside;
	Enesim_Renderer_Damages_Get_Cb damages_get;
	Enesim_Renderer_Has_Changed_Cb has_changed;
	/* software based functions */
	Enesim_Renderer_Sw_Hints_Get_Cb sw_hints_get;
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
	/* shape related functions */
	Enesim_Renderer_Shape_Feature_Get_Cb feature_get;
} Enesim_Renderer_Shape_Descriptor;

Enesim_Renderer * enesim_renderer_shape_new(Enesim_Renderer_Shape_Descriptor *descriptor, void *data);
void * enesim_renderer_shape_data_get(Enesim_Renderer *r);

#endif

