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
#ifndef _PATH_H
#define _PATH_H

typedef void (*Enesim_Path_Begin)(void *data);
typedef void (*Enesim_Path_Polygon_Add)(void *data);
typedef void (*Enesim_Path_Polygon_Close)(Eina_Bool close, void *data);
typedef void (*Enesim_Path_Done)(void *data);

typedef struct _Enesim_Path_Descriptor
{
	Enesim_Curve_Vertex_Add vertex_add;
	Enesim_Path_Polygon_Add polygon_add;
	Enesim_Path_Polygon_Close polygon_close;
	Enesim_Path_Begin path_begin;
	Enesim_Path_Done path_done;
} Enesim_Path_Descriptor;

typedef struct _Enesim_Path
{
	Enesim_Path_Descriptor *descriptor;
	Enesim_Curve_State st;
	Enesim_Figure *figure;
	const Enesim_Matrix *gm;
	double scale_x;
	double scale_y;
	void *data;
} Enesim_Path;

Enesim_Path * enesim_path_new(Enesim_Path_Descriptor *descriptor, void *data);
void enesim_path_free(Enesim_Path *thiz);
void enesim_path_figure_set(Enesim_Path *thiz, Enesim_Figure *figure);
void enesim_path_transformation_set(Enesim_Path *thiz, const Enesim_Matrix *matrix);
void enesim_path_scale_set(Enesim_Path *thiz, double scale_x, double scale_y);
void * enesim_path_data_get(Enesim_Path *thiz);
void enesim_path_generate(Enesim_Path *thiz, Eina_List *commands);

Enesim_Path * enesim_path_strokeless_new(void);

Enesim_Path * enesim_path_full_new(void);
void enesim_path_full_stroke_figure_set(Enesim_Path *thiz, Enesim_Figure *stroke);
void enesim_path_full_stroke_cap_set(Enesim_Path *thiz, Enesim_Shape_Stroke_Cap cap);
void enesim_path_full_stroke_join_set(Enesim_Path *thiz, Enesim_Shape_Stroke_Join join);
void enesim_path_full_stroke_weight_set(Enesim_Path *thiz, double sw);

Enesim_Path * enesim_path_dashed_new(void);
void enesim_path_dashed_stroke_figure_set(Enesim_Path *thiz, Enesim_Figure *stroke);
void enesim_path_dashed_stroke_cap_set(Enesim_Path *thiz, Enesim_Shape_Stroke_Cap cap);
void enesim_path_dashed_stroke_join_set(Enesim_Path *thiz, Enesim_Shape_Stroke_Join join);
void enesim_path_dashed_stroke_weight_set(Enesim_Path *thiz, double sw);
void enesim_path_dashed_stroke_dash_set(Enesim_Path *thiz, const Eina_List *dashes);

#endif
