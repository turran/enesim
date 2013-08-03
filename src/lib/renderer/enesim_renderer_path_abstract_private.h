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

#define ENESIM_RENDERER_PATH_ABSTRACT_DESCRIPTOR 				\
		enesim_renderer_path_abstract_descriptor_get()
#define ENESIM_RENDERER_PATH_ABSTRACT_CLASS(k) ENESIM_OBJECT_CLASS_CHECK(k,	\
		Enesim_Renderer_Path_Abstract_Class,				\
		ENESIM_RENDERER_PATH_ABSTRACT_DESCRIPTOR())
#define ENESIM_RENDERER_PATH_ABSTRACT_CLASS_GET(o)				\
		ENESIM_RENDERER_PATH_ABSTRACT_CLASS(				\
		ENESIM_OBJECT_INSTANCE_CLASS(o));
#define ENESIM_RENDERER_PATH_ABSTRACT(o) ENESIM_OBJECT_INSTANCE_CHECK(o, 	\
		Enesim_Renderer_Path_Abstract,					\
		ENESIM_RENDERER_PATH_ABSTRACT_DESCRIPTOR)


/* TODO later instead of inheriting from a shape, just inherit from
 * a renderer and pass the state directly
 */
typedef struct _Enesim_Renderer_Path_Abstract_State
{
	Eina_List *commands;
	Eina_Bool changed;
} Enesim_Renderer_Path_Abstract_State;

typedef struct _Enesim_Renderer_Path_Abstract
{
	Enesim_Renderer_Shape parent;
} Enesim_Renderer_Path_Abstract;

/* TODO Rename the commands and pass the path directly, so we can track a change */
typedef void (*Enesim_Renderer_Path_Abstract_Commands_Set_Cb)(Enesim_Renderer *r, const Eina_List *commands);
typedef void (*Enesim_Renderer_Path_Abstract_State_Set_Cb)(Enesim_Renderer *r, const Enesim_Renderer_State *state);
typedef void (*Enesim_Renderer_Path_Abstract_Shape_State_Set_Cb)(Enesim_Renderer *r, const Enesim_Renderer_Shape_State *state);
typedef void (*Enesim_Renderer_Path_Abstract_Path_State_Set_Cb)(Enesim_Renderer *r, const Enesim_Renderer_Path_Abstract_State *state);

typedef struct _Enesim_Renderer_Path_Abstract_Class {
	Enesim_Renderer_Shape_Class parent;
	Enesim_Renderer_Path_Abstract_Commands_Set_Cb commands_set;
	Enesim_Renderer_Path_Abstract_Path_State_Set_Cb path_state_set;
	Enesim_Renderer_Path_Abstract_Shape_State_Set_Cb shape_state_set;
	Enesim_Renderer_Path_Abstract_State_Set_Cb state_set;
} Enesim_Renderer_Path_Abstract_Class;

Enesim_Object_Descriptor * enesim_renderer_path_abstract_descriptor_get(void);
void enesim_renderer_path_abstract_commands_set(Enesim_Renderer *r, const Eina_List *commands);

/* abstract implementations */
Enesim_Renderer * enesim_renderer_path_enesim_new(void);
#if BUILD_CAIRO
Enesim_Renderer * enesim_renderer_path_cairo_new(void);
#endif

#endif
