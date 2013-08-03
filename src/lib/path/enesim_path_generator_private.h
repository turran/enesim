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
#ifndef _ENESIM_PATH_GENERATOR_PRIVATE_H
#define _ENESIM_PATH_GENERATOR_PRIVATE_H

/* The generator generates a figure from a path */

typedef void (*Enesim_Path_Delete)(void *data);
typedef void (*Enesim_Path_Begin)(void *data);
typedef void (*Enesim_Path_Polygon_Add)(void *data);
typedef void (*Enesim_Path_Polygon_Close)(Eina_Bool close, void *data);
typedef void (*Enesim_Path_Done)(void *data);

typedef struct _Enesim_Path_Descriptor
{
	Enesim_Path_Delete free;
	Enesim_Curve_Vertex_Add vertex_add;
	Enesim_Path_Polygon_Add polygon_add;
	Enesim_Path_Polygon_Close polygon_close;
	Enesim_Path_Begin path_begin;
	Enesim_Path_Done path_done;
} Enesim_Path_Descriptor;

typedef struct _Enesim_Path_Generator
{
	Enesim_Path_Descriptor *descriptor;
	Enesim_Curve_State st;
	const Enesim_Matrix *gm;
	double scale_x;
	double scale_y;
	Enesim_Figure *figure;
	Enesim_Figure *stroke_figure;
	Enesim_Renderer_Shape_Stroke_Cap cap;
	Enesim_Renderer_Shape_Stroke_Join join;
	double sw;
	const Eina_List *dashes;
	void *data;
} Enesim_Path_Generator;

Enesim_Path_Generator * enesim_path_generator_new(Enesim_Path_Descriptor *descriptor, void *data);
void enesim_path_generator_free(Enesim_Path_Generator *thiz);
void enesim_path_generator_figure_set(Enesim_Path_Generator *thiz, Enesim_Figure *figure);
void enesim_path_generator_transformation_set(Enesim_Path_Generator *thiz, const Enesim_Matrix *matrix);
void enesim_path_generator_scale_set(Enesim_Path_Generator *thiz, double scale_x, double scale_y);
void enesim_path_generator_stroke_dash_set(Enesim_Path_Generator *thiz, const Eina_List *dashes);
void enesim_path_generator_stroke_figure_set(Enesim_Path_Generator *thiz, Enesim_Figure *stroke);
void enesim_path_generator_stroke_cap_set(Enesim_Path_Generator *thiz, Enesim_Renderer_Shape_Stroke_Cap cap);
void enesim_path_generator_stroke_join_set(Enesim_Path_Generator *thiz, Enesim_Renderer_Shape_Stroke_Join join);
void enesim_path_generator_stroke_weight_set(Enesim_Path_Generator *thiz, double sw);

void * enesim_path_generator_data_get(Enesim_Path_Generator *thiz);
void enesim_path_generator_generate(Enesim_Path_Generator *thiz, Eina_List *commands);

Enesim_Path_Generator * enesim_path_generator_strokeless_new(void);
Enesim_Path_Generator * enesim_path_generator_stroke_new(void);
Enesim_Path_Generator * enesim_path_generator_stroke_dashless_new(void);
Enesim_Path_Generator * enesim_path_generator_dashed_new(void);

#endif
