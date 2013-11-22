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
#ifndef ENESIM_RENDERER_PATH_ENESIM_H_
#define ENESIM_RENDERER_PATH_ENESIM_H_

#include "enesim_private.h"
#include "libargb.h"

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_path.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_path.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#ifdef BUILD_OPENGL
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
#endif

#include "enesim_path_private.h"
#include "enesim_list_private.h"
#include "enesim_buffer_private.h"
#include "enesim_surface_private.h"
#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_path_abstract_private.h"
#include "enesim_vector_private.h"
#include "enesim_rasterizer_private.h"
#include "enesim_curve_private.h"
#include "enesim_path_generator_private.h"

/* the gl implementations */
#ifdef BUILD_OPENGL
#include "enesim_renderer_path_enesim_gl_tesselator_private.h"
#endif

#define ENESIM_RENDERER_PATH_ENESIM(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Path_Enesim,					\
		enesim_renderer_path_enesim_descriptor_get())

typedef struct _Enesim_Renderer_Path_Enesim
{
	Enesim_Renderer_Path_Abstract parent;
	/* properties */
	/* private */
	Enesim_Path *path;
	Enesim_Path_Generator *stroke_path;
	Enesim_Path_Generator *strokeless_path;
	Enesim_Path_Generator *dashed_path;
#if BUILD_OPENGL
	Enesim_Renderer_Path_Enesim_OpenGL gl;
#endif
	/* TODO put the below data into a path_sw struct */
	Enesim_Figure *fill_figure;
	Enesim_Figure *stroke_figure;
	/* external properties that require a path generation */
	Enesim_Matrix last_matrix;
	Enesim_Renderer_Shape_Stroke_Join last_join;
	Enesim_Renderer_Shape_Stroke_Cap last_cap;
	double last_stroke_weight;
	/* internal stuff */
	Enesim_Renderer *bifigure;
	Eina_Bool changed : 1;
	Eina_Bool path_changed : 1;
	Eina_Bool dashes_changed : 1;
	Eina_Bool generated : 1;
} Enesim_Renderer_Path_Enesim;

typedef struct _Enesim_Renderer_Path_Enesim_Class
{
	Enesim_Renderer_Path_Abstract_Class parent;
} Enesim_Renderer_Path_Enesim_Class;


#endif

