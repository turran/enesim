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
#ifndef RASTERIZER_H_
#define RASTERIZER_H_

#include "enesim_renderer_shape_private.h"

typedef struct _Enesim_Rasterizer_State
{
	Enesim_Renderer_State2 rstate;
	Enesim_Renderer_Shape_State2 sstate;
} Enesim_Rasterizer_State;

typedef void (*Enesim_Rasterizer_Figure_Set)(Enesim_Renderer *r, const Enesim_Figure *figure);

typedef struct _Enesim_Rasterizer_Descriptor
{
	/* inherited from the renderer */
	Enesim_Renderer_Base_Name_Get_Cb base_name_get;
	Enesim_Renderer_Delete free;
	Enesim_Renderer_Flags_Get_Cb flags_get;
	/* properties */
	Enesim_Renderer_Origin_X_Set_Cb origin_x_set;
	Enesim_Renderer_Origin_X_Get_Cb origin_x_get;
	Enesim_Renderer_Origin_Y_Set_Cb origin_y_set;
	Enesim_Renderer_Origin_Y_Get_Cb origin_y_get;
	Enesim_Renderer_Transformation_Set_Cb transformation_set;
	Enesim_Renderer_Transformation_Get_Cb transformation_get;
	Enesim_Renderer_Quality_Set_Cb quality_set;
	Enesim_Renderer_Quality_Get_Cb quality_get;
	/* software based functions */
	Enesim_Renderer_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	/* inherited from the shape */
	Enesim_Renderer_Shape_Feature_Get_Cb feature_get;
	Enesim_Renderer_Shape_Draw_Mode_Set_Cb draw_mode_set;
	Enesim_Renderer_Shape_Draw_Mode_Get_Cb draw_mode_get;
	Enesim_Renderer_Shape_Stroke_Color_Set_Cb stroke_color_set;
	Enesim_Renderer_Shape_Stroke_Color_Get_Cb stroke_color_get;
	Enesim_Renderer_Shape_Stroke_Renderer_Set_Cb stroke_renderer_set;
	Enesim_Renderer_Shape_Stroke_Renderer_Get_Cb stroke_renderer_get;
	Enesim_Renderer_Shape_Stroke_Weight_Set_Cb stroke_weight_set;
	Enesim_Renderer_Shape_Stroke_Weight_Get_Cb stroke_weight_get;
	Enesim_Renderer_Shape_Fill_Color_Set_Cb fill_color_set;
	Enesim_Renderer_Shape_Fill_Color_Get_Cb fill_color_get;
	Enesim_Renderer_Shape_Fill_Renderer_Set_Cb fill_renderer_set;
	Enesim_Renderer_Shape_Fill_Renderer_Get_Cb fill_renderer_get;
	Enesim_Renderer_Shape_Fill_Rule_Set_Cb fill_rule_set;
	Enesim_Renderer_Shape_Fill_Rule_Get_Cb fill_rule_get;
	/* our own interface */
	Enesim_Rasterizer_Figure_Set figure_set;
} Enesim_Rasterizer_Descriptor;

Enesim_Renderer * enesim_rasterizer_new(Enesim_Rasterizer_Descriptor *d, void *data);
void * enesim_rasterizer_data_get(Enesim_Renderer *r);
void enesim_rasterizer_figure_set(Enesim_Renderer *r, const Enesim_Figure *f);

Enesim_Renderer * enesim_rasterizer_basic_new(void);
void enesim_rasterizer_basic_vectors_get(Enesim_Renderer *r, int *nvectors,
		Enesim_F16p16_Vector **vectors);

Enesim_Renderer * enesim_rasterizer_bifigure_new(void);
void enesim_rasterizer_bifigure_over_figure_set(Enesim_Renderer *r, const Enesim_Figure *figure);

#endif
