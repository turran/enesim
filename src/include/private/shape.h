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

typedef void (*Enesim_Renderer_Shape_Boundings)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstate[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *boundings);
typedef void (*Enesim_Renderer_Shape_Destination_Boundings)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstate[ENESIM_RENDERER_STATES],
		Eina_Rectangle *boundings);

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
		Enesim_Renderer_OpenGL_Draw *define_geomery,
		Enesim_Renderer_OpenGL_Shader_Setup *shader_setup,
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***shaders,
		Enesim_Error **error);

typedef void (*Enesim_Renderer_Shape_Feature_Get)(Enesim_Renderer *r, Enesim_Shape_Feature *features);

typedef struct _Enesim_Renderer_Shape_Descriptor {
	/* common */
	Enesim_Renderer_Name name;
	Enesim_Renderer_Delete free;
	Enesim_Renderer_Shape_Boundings boundings;
	Enesim_Renderer_Shape_Destination_Boundings destination_boundings;
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

#endif

