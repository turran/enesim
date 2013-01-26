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
	} stroke;

	struct {
		Enesim_Color color;
		Enesim_Renderer *r;
		Enesim_Shape_Fill_Rule rule;
	} fill;
	Enesim_Shape_Draw_Mode draw_mode;
} Enesim_Renderer_Shape_State;

typedef void (*Enesim_Renderer_Shape_Sw_Draw)(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y,
		unsigned int len,
		void *data);

typedef void (*Enesim_Renderer_Shape_Bounds)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstate[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *bounds);
typedef void (*Enesim_Renderer_Shape_Destination_Bounds)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstate[ENESIM_RENDERER_STATES],
		Eina_Rectangle *bounds);

typedef Eina_Bool (*Enesim_Renderer_Shape_Sw_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstate[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Shape_Sw_Draw *draw,
		Enesim_Error **error);
typedef Eina_Bool (*Enesim_Renderer_Shape_OpenCL_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstate[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		const char **program_name, const char **program_source,
		size_t *program_length,
		Enesim_Error **error);
typedef Eina_Bool (*Enesim_Renderer_Shape_OpenGL_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstate[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Error **error);

typedef void (*Enesim_Renderer_Shape_Feature_Get)(Enesim_Renderer *r, Enesim_Shape_Feature *features);

typedef struct _Enesim_Renderer_Shape_Descriptor {
	/* common */
	Enesim_Renderer_Name name;
	Enesim_Renderer_Delete free;
	Enesim_Renderer_Shape_Bounds bounds;
	Enesim_Renderer_Shape_Destination_Bounds destination_bounds;
	Enesim_Renderer_Flags flags;
	Enesim_Renderer_Hints_Get hints_get;
	Enesim_Renderer_Inside is_inside;
	Enesim_Renderer_Damage damage;
	Enesim_Renderer_Has_Changed has_changed;
	Enesim_Renderer_Shape_Feature_Get feature_get;
	/* software based functions */
	Enesim_Renderer_Shape_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	/* opencl based functions */
	Enesim_Renderer_Shape_OpenCL_Setup opencl_setup;
	Enesim_Renderer_OpenCL_Kernel_Setup opencl_kernel_setup;
	Enesim_Renderer_OpenCL_Cleanup opencl_cleanup;
	/* opengl based functions */
	Enesim_Renderer_OpenGL_Initialize opengl_initialize;
	Enesim_Renderer_Shape_OpenGL_Setup opengl_setup;
	Enesim_Renderer_OpenGL_Cleanup opengl_cleanup;
} Enesim_Renderer_Shape_Descriptor;

Enesim_Renderer * enesim_renderer_shape_new(Enesim_Renderer_Shape_Descriptor *descriptor, void *data);
void * enesim_renderer_shape_data_get(Enesim_Renderer *r);
void enesim_renderer_shape_cleanup(Enesim_Renderer *r, Enesim_Surface *s);
Eina_Bool enesim_renderer_shape_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Error **error);

#if 0
/* TODO properties callbacks */
typedef void (*Enesim_Renderer_Shape_Stroke_Color_Set_Cb)(void *p, Enesim_Color color);
typedef Enesim_Color (*Enesim_Renderer_Shape_Stroke_Color_Get_Cb)(void *p);
typedef void (*Enesim_Renderer_Shape_Stroke_Renderer_Set_Cb)(void *p, Enesim_Renderer *r);
typedef Enesim_Renderer *(*Enesim_Renderer_Shape_Stroke_Renderer_Get_Cb)(void *p);
typedef void (*Enesim_Renderer_Shape_Stroke_Weight_Set_Cb)(void *p, double weight);
typedef double (*Enesim_Renderer_Shape_Stroke_Weight_Get_Cb)(void *p);
typedef void (*Enesim_Renderer_Shape_Stroke_Location_Set_Cb)(void *p, Enesim_Shape_Stroke_Location l);
typedef Enesim_Shape_Stroke_Location (*Enesim_Renderer_Shape_Stroke_Location_Get_Cb)(void *p);
typedef void (*Enesim_Renderer_Shape_Stroke_Cap_Set_Cb)(void *p, Enesim_Shape_Stroke_Cap l);
typedef Enesim_Shape_Stroke_Cap (*Enesim_Renderer_Shape_Stroke_Cap_Get_Cb)(void *p);
typedef void (*Enesim_Renderer_Shape_Stroke_Join_Set_Cb)(void *p, Enesim_Shape_Stroke_Join l);
typedef Enesim_Shape_Stroke_Join (*Enesim_Renderer_Shape_Stroke_Join_Get_Cb)(void *p);

typedef void (*Enesim_Renderer_Shape_Fill_Color_Set_Cb)(void *p, Enesim_Color color);
typedef Enesim_Color (*Enesim_Renderer_Shape_Fill_Color_Get_Cb)(void *p);
typedef void (*Enesim_Renderer_Shape_Fill_Renderer_Set_Cb)(void *p, Enesim_Renderer *r);
typedef Enesim_Renderer *(*Enesim_Renderer_Shape_Fill_Renderer_Get_Cb)(void *p);
typedef void (*Enesim_Renderer_Shape_Fill_Rule_Set_Cb)(void *p, Enesim_Shape_Fill_Rule rule);
typedef Enesim_Shape_Fill_Rule (*Enesim_Renderer_Shape_Fill_Rule_Get_Cb)(void *p);

typedef void (*Enesim_Renderer_Shape_Draw_Mode_Set_Cb)(void *p, Enesim_Shape_Draw_Mode mode);
typedef Enesim_Shape_Draw_Mode (*Enesim_Renderer_Shape_Draw_Mode_Get_Cb)(void *p);

EAPI Eina_Bool enesim_renderer_shape_state_changed(Enesim_Renderer_Flag flags, Enesim_Renderer_Shape_State *curr, Enesim_Renderer_Shape_State *prev);
EAPI void enesim_renderer_shape_state_update(Enesim_Renderer_Flag flags, Enesim_Renderer_Shape_State *curr, Enesim_Renderer_Shape_State *prev);
#endif

#endif

