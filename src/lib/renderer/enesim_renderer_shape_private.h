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
#ifndef _ENESIM_RENDERER_SHAPE_PRIVATE_H_
#define _ENESIM_RENDERER_SHAPE_PRIVATE_H_

#define ENESIM_RENDERER_SHAPE_DESCRIPTOR enesim_renderer_shape_descriptor_get()
#define ENESIM_RENDERER_SHAPE_CLASS(k) ENESIM_OBJECT_CLASS_CHECK(k,		\
		Enesim_Renderer_Shape_Class, ENESIM_RENDERER_SHAPE_DESCRIPTOR)
#define ENESIM_RENDERER_SHAPE_CLASS_GET(o) ENESIM_RENDERER_SHAPE_CLASS(		\
		ENESIM_OBJECT_INSTANCE_CLASS(o));
#define ENESIM_RENDERER_SHAPE(o) ENESIM_OBJECT_INSTANCE_CHECK(o, 		\
		Enesim_Renderer_Shape, ENESIM_RENDERER_SHAPE_DESCRIPTOR)

typedef struct _Enesim_Renderer_Shape_Dash
{
	Eina_List *l;
	Eina_Bool changed;
} Enesim_Renderer_Shape_Dash;

typedef struct _Enesim_Renderer_Shape_State
{
	struct {
		struct {
			Enesim_Color color;
			Enesim_Renderer *r;
			double weight;
			Enesim_Renderer_Shape_Stroke_Location location;
			Enesim_Renderer_Shape_Stroke_Cap cap;
			Enesim_Renderer_Shape_Stroke_Join join;
		} stroke;

		struct {
			Enesim_Color color;
			Enesim_Renderer *r;
			Enesim_Renderer_Shape_Fill_Rule rule;
		} fill;
		Enesim_Renderer_Shape_Draw_Mode draw_mode;
	} current, past;
	Enesim_List *dashes;
	Eina_Bool changed;
} Enesim_Renderer_Shape_State;

typedef struct _Enesim_Renderer_Shape
{
	Enesim_Renderer parent;
	/* properties */
	Enesim_Renderer_Shape_State state;
	/* private */
	/* interface */
} Enesim_Renderer_Shape;


typedef void (*Enesim_Renderer_Shape_Features_Get_Cb)(Enesim_Renderer *r, Enesim_Renderer_Shape_Feature *features);
typedef Eina_Bool (*Enesim_Renderer_Shape_Geometry_Get_Cb)(Enesim_Renderer *r, Enesim_Rectangle *geometry);

typedef struct _Enesim_Renderer_Shape_Class {
	Enesim_Renderer_Class parent;

	Enesim_Renderer_Has_Changed_Cb has_changed;
	/* software based functions */
	Enesim_Renderer_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	/* opencl based functions */
	Enesim_Renderer_OpenCL_Setup opencl_setup;
	Enesim_Renderer_OpenCL_Kernel_Setup opencl_kernel_setup;
	Enesim_Renderer_OpenCL_Cleanup opencl_cleanup;
	/* opengl based functions */
	Enesim_Renderer_OpenGL_Setup opengl_setup;
	Enesim_Renderer_OpenGL_Cleanup opengl_cleanup;
	/* shape functions */
	Enesim_Renderer_Shape_Features_Get_Cb features_get;
	Enesim_Renderer_Shape_Geometry_Get_Cb geometry_get;
} Enesim_Renderer_Shape_Class;

Enesim_Object_Descriptor * enesim_renderer_shape_descriptor_get(void);
void enesim_renderer_shape_state_commit(Enesim_Renderer *r);
Eina_Bool enesim_renderer_shape_state_has_changed(Enesim_Renderer *r);
const Enesim_Renderer_Shape_State * enesim_renderer_shape_state_get(
		Enesim_Renderer *r);
void enesim_renderer_shape_propagate(Enesim_Renderer *r, Enesim_Renderer *to);

#endif

