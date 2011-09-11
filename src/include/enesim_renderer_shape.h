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
typedef enum _Enesim_Shape_Draw_Mode
{
	ENESIM_SHAPE_DRAW_MODE_FILL,
	ENESIM_SHAPE_DRAW_MODE_STROKE,
	ENESIM_SHAPE_DRAW_MODE_STROKE_FILL,
} Enesim_Shape_Draw_Mode;

EAPI void enesim_renderer_shape_stroke_weight_set(Enesim_Renderer *r, double weight);
EAPI void enesim_renderer_shape_stroke_weight_get(Enesim_Renderer *r, double *weight);
EAPI void enesim_renderer_shape_stroke_color_set(Enesim_Renderer *r, Enesim_Color stroke_color);
EAPI void enesim_renderer_shape_stroke_color_get(Enesim_Renderer *r, Enesim_Color *color);
EAPI void enesim_renderer_shape_stroke_renderer_set(Enesim_Renderer *r, Enesim_Renderer *o);
EAPI void enesim_renderer_shape_stroke_renderer_get(Enesim_Renderer *r, Enesim_Renderer **o);
EAPI void enesim_renderer_shape_fill_color_set(Enesim_Renderer *r, Enesim_Color fill_color);
EAPI void enesim_renderer_shape_fill_color_get(Enesim_Renderer *r, Enesim_Color *fill_color);
EAPI void enesim_renderer_shape_fill_renderer_set(Enesim_Renderer *r, Enesim_Renderer *f);
EAPI void enesim_renderer_shape_fill_renderer_get(Enesim_Renderer *r, Enesim_Renderer **fill);
EAPI void enesim_renderer_shape_draw_mode_set(Enesim_Renderer *r, Enesim_Shape_Draw_Mode draw_mode);
EAPI void enesim_renderer_shape_draw_mode_get(Enesim_Renderer *r, Enesim_Shape_Draw_Mode *draw_mode);
/**
 * @}
 */

#endif
