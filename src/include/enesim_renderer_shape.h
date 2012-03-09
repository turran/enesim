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
#ifndef ENESIM_RENDERER_SHAPE_H_
#define ENESIM_RENDERER_SHAPE_H_

/**
 * @defgroup Enesim_Renderer_Shape_Group Shapes
 * @ingroup Enesim_Renderer_Group
 * @{
 */

typedef enum _Enesim_Shape_Feature
{
	ENESIM_SHAPE_FLAG_FILL_RENDERER 	= (1 << 0),
	ENESIM_SHAPE_FLAG_STROKE_RENDERER	= (1 << 1),
} Enesim_Shape_Feature;

typedef enum _Enesim_Shape_Draw_Mode
{
	ENESIM_SHAPE_DRAW_MODE_FILL 	= (1 << 0),
	ENESIM_SHAPE_DRAW_MODE_STROKE	= (1 << 1),
} Enesim_Shape_Draw_Mode;

typedef enum _Enesim_Shape_Stroke_Location
{
	ENESIM_SHAPE_STROKE_INSIDE,
	ENESIM_SHAPE_STROKE_OUTSIDE,
	ENESIM_SHAPE_STROKE_CENTER,
} Enesim_Shape_Stroke_Location;

#define ENESIM_SHAPE_DRAW_MODE_STROKE_FILL (ENESIM_SHAPE_DRAW_MODE_FILL | ENESIM_SHAPE_DRAW_MODE_STROKE)

typedef enum _Enesim_Shape_Stroke_Cap
{
	ENESIM_CAP_BUTT,
	ENESIM_CAP_ROUND,
	ENESIM_CAP_SQUARE,
	ENESIM_SHAPE_STROKE_CAPS,
} Enesim_Shape_Stroke_Cap;

typedef enum _Enesim_Shape_Stroke_Join
{
	ENESIM_JOIN_MITER,
	ENESIM_JOIN_ROUND,
	ENESIM_JOIN_BEVEL,
	ENESIM_SHAPE_STROKE_JOINS,
} Enesim_Shape_Stroke_Join;

typedef enum _Enesim_Shape_Fill_Rule
{
    ENESIM_SHAPE_FILL_RULE_NON_ZERO,
    ENESIM_SHAPE_FILL_RULE_EVEN_ODD,
} Enesim_Shape_Fill_Rule;

EAPI void enesim_renderer_shape_feature_get(Enesim_Renderer *r, Enesim_Shape_Feature *features);

/* stroke properties */
EAPI void enesim_renderer_shape_stroke_color_set(Enesim_Renderer *r, Enesim_Color stroke_color);
EAPI void enesim_renderer_shape_stroke_color_get(Enesim_Renderer *r, Enesim_Color *color);
EAPI void enesim_renderer_shape_stroke_renderer_set(Enesim_Renderer *r, Enesim_Renderer *o);
EAPI void enesim_renderer_shape_stroke_renderer_get(Enesim_Renderer *r, Enesim_Renderer **o);
EAPI void enesim_renderer_shape_stroke_weight_set(Enesim_Renderer *r, double weight);
EAPI void enesim_renderer_shape_stroke_weight_get(Enesim_Renderer *r, double *weight);
EAPI void enesim_renderer_shape_stroke_location_set(Enesim_Renderer *r, Enesim_Shape_Stroke_Location location);
EAPI void enesim_renderer_shape_stroke_location_get(Enesim_Renderer *r, Enesim_Shape_Stroke_Location *location);
EAPI void enesim_renderer_shape_stroke_cap_set(Enesim_Renderer *r, Enesim_Shape_Stroke_Cap cap);
EAPI void enesim_renderer_shape_stroke_cap_get(Enesim_Renderer *r, Enesim_Shape_Stroke_Cap *cap);
EAPI void enesim_renderer_shape_stroke_join_set(Enesim_Renderer *r, Enesim_Shape_Stroke_Join join);
EAPI void enesim_renderer_shape_stroke_join_get(Enesim_Renderer *r, Enesim_Shape_Stroke_Join *join);
EAPI void enesim_renderer_shape_stroke_renderer_set(Enesim_Renderer *r, Enesim_Renderer *o);
EAPI void enesim_renderer_shape_stroke_renderer_get(Enesim_Renderer *r, Enesim_Renderer **o);

/* fill properties */
EAPI void enesim_renderer_shape_fill_color_set(Enesim_Renderer *r, Enesim_Color fill_color);
EAPI void enesim_renderer_shape_fill_color_get(Enesim_Renderer *r, Enesim_Color *fill_color);
EAPI void enesim_renderer_shape_fill_renderer_set(Enesim_Renderer *r, Enesim_Renderer *f);
EAPI void enesim_renderer_shape_fill_renderer_get(Enesim_Renderer *r, Enesim_Renderer **fill);
EAPI void enesim_renderer_shape_fill_rule_set(Enesim_Renderer *r, Enesim_Shape_Fill_Rule rule);
EAPI void enesim_renderer_shape_fill_rule_get(Enesim_Renderer *r, Enesim_Shape_Fill_Rule *rule);

/* stroke and/or fill */
EAPI void enesim_renderer_shape_draw_mode_set(Enesim_Renderer *r, Enesim_Shape_Draw_Mode draw_mode);
EAPI void enesim_renderer_shape_draw_mode_get(Enesim_Renderer *r, Enesim_Shape_Draw_Mode *draw_mode);
/**
 * @}
 */

#endif
