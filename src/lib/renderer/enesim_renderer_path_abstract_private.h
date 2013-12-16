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
#ifndef ENESIM_RENDERER_PATH_ABSTRACT_H
#define ENESIM_RENDERER_PATH_ABSTRACT_H

#include "enesim_path_private.h"
#include "enesim_vector_private.h"
#include "enesim_curve_private.h"
#include "enesim_path_generator_private.h"

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


typedef struct _Enesim_Renderer_Path_Abstract
{
	Enesim_Renderer_Shape parent;
	/* some common helpers */
	Enesim_Path *path;
	Enesim_Path_Generator *stroke_path;
	Enesim_Path_Generator *strokeless_path;
	Enesim_Path_Generator *dashed_path;
	Enesim_Figure *fill_figure;
	Enesim_Figure *stroke_figure;
	/* external properties that require a path generation */
	Enesim_Matrix last_matrix;
	Enesim_Renderer_Shape_Stroke_Join last_join;
	Enesim_Renderer_Shape_Stroke_Cap last_cap;
	double last_stroke_weight;
	/* to keep track of the changes */
	int last_path_change;
	int last_dash_change;
	Eina_Bool generated : 1;
	Eina_Bool stroke_figure_used : 1;
} Enesim_Renderer_Path_Abstract;

typedef void (*Enesim_Renderer_Path_Abstract_Path_Set_Cb)(Enesim_Renderer *r, Enesim_Path *path);
typedef Eina_Bool (*Enesim_Renderer_Path_Abstract_Is_Available_Cb)(Enesim_Renderer *r);

typedef struct _Enesim_Renderer_Path_Abstract_Class {
	Enesim_Renderer_Shape_Class parent;
	Enesim_Renderer_Path_Abstract_Path_Set_Cb path_set;
	Enesim_Renderer_Path_Abstract_Is_Available_Cb is_available;
} Enesim_Renderer_Path_Abstract_Class;

Enesim_Object_Descriptor * enesim_renderer_path_abstract_descriptor_get(void);
void enesim_renderer_path_abstract_path_set(Enesim_Renderer *r, Enesim_Path *path);
Eina_Bool enesim_renderer_path_abstract_is_available(Enesim_Renderer *r);
void enesim_renderer_path_abstract_generate(Enesim_Renderer *r);
Eina_Bool enesim_renderer_path_abstract_needs_generate(Enesim_Renderer *r);
void enesim_renderer_path_abstract_cleanup(Enesim_Renderer *r);

/* abstract implementations */
Enesim_Renderer * enesim_renderer_path_enesim_new(void);
#if BUILD_OPENGL
Enesim_Renderer * enesim_renderer_path_nv_new(void);
Enesim_Renderer * enesim_renderer_path_tesselator_new(void);
#endif
#if BUILD_CAIRO
Enesim_Renderer * enesim_renderer_path_cairo_new(void);
#endif

#endif
