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
#ifndef ENESIM_RENDERER_PATH_ABSTRACT_H
#define ENESIM_RENDERER_PATH_ABSTRACT_H

/* TODO later instead of inheriting from a shape, just inherit from
 * a renderer and pass the state directly
 */
typedef struct _Enesim_Renderer_Path_Abstract_State
{
	Eina_List *commands;
	Eina_Bool changed;
} Enesim_Renderer_Path_Abstract_State;

typedef void (*Enesim_Renderer_Path_Abstract_Commands_Set_Cb)(Enesim_Renderer *r, const Eina_List *commands);
typedef void (*Enesim_Renderer_Path_Abstract_State_Set_Cb)(Enesim_Renderer *r, const Enesim_Renderer_State *state);
typedef void (*Enesim_Renderer_Path_Abstract_Shape_State_Set_Cb)(Enesim_Renderer *r, const Enesim_Renderer_Shape_State *state);
typedef void (*Enesim_Renderer_Path_Abstract_Path_State_Set_Cb)(Enesim_Renderer *r, const Enesim_Renderer_Path_Abstract_State *state);

typedef struct _Enesim_Renderer_Path_Abstract_Descriptor {
	unsigned int version;
	Enesim_Renderer_Base_Name_Get_Cb base_name_get;
	Enesim_Renderer_Delete_Cb free;
	Enesim_Renderer_Bounds_Get_Cb bounds_get;
	Enesim_Renderer_Features_Get features_get;
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
	Enesim_Renderer_Shape_Features_Get_Cb shape_features_get;
	/* path related functions */
	Enesim_Renderer_Path_Abstract_Commands_Set_Cb commands_set;
	Enesim_Renderer_Path_Abstract_Path_State_Set_Cb path_state_set;
	Enesim_Renderer_Path_Abstract_Shape_State_Set_Cb shape_state_set;
	Enesim_Renderer_Path_Abstract_State_Set_Cb state_set;
} Enesim_Renderer_Path_Abstract_Descriptor;

Enesim_Renderer * enesim_renderer_path_abstract_new(Enesim_Renderer_Path_Abstract_Descriptor *descriptor, void *data);
void enesim_renderer_path_abstract_commands_set(Enesim_Renderer *r, const Eina_List *commands);
void * enesim_renderer_path_abstract_data_get(Enesim_Renderer *r);

/* abstract implementations */
Enesim_Renderer * enesim_renderer_path_enesim_new(void);
#if BUILD_CAIRO
Enesim_Renderer * enesim_renderer_path_cairo_new(void);
#endif

#endif
