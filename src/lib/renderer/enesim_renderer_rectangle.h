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
#ifndef ENESIM_RENDERER_RECTANGLE_H_
#define ENESIM_RENDERER_RECTANGLE_H_

/**
 * @defgroup Enesim_Renderer_Rectangle Rectangle
 * @brief Rectangle renderer
 * @ingroup Enesim_Renderer_Shape
 * @{
 */

EAPI Enesim_Renderer * enesim_renderer_rectangle_new(void);
EAPI void enesim_renderer_rectangle_x_set(Enesim_Renderer *r, double x);
EAPI double enesim_renderer_rectangle_x_get(Enesim_Renderer *r);

EAPI void enesim_renderer_rectangle_y_set(Enesim_Renderer *r, double y);
EAPI double enesim_renderer_rectangle_y_get(Enesim_Renderer *r);

EAPI void enesim_renderer_rectangle_position_set(Enesim_Renderer *r, double x, double y);
EAPI void enesim_renderer_rectangle_position_get(Enesim_Renderer *r, double *x, double *y);

EAPI void enesim_renderer_rectangle_width_set(Enesim_Renderer *p, double w);
EAPI double enesim_renderer_rectangle_width_get(Enesim_Renderer *p);

EAPI void enesim_renderer_rectangle_height_set(Enesim_Renderer *p, double h);
EAPI double enesim_renderer_rectangle_height_get(Enesim_Renderer *p);

EAPI void enesim_renderer_rectangle_size_set(Enesim_Renderer *p, double w, double h);
EAPI void enesim_renderer_rectangle_size_get(Enesim_Renderer *p, double *w, double *h);

EAPI void enesim_renderer_rectangle_corner_radius_x_set(Enesim_Renderer *p, double rx);
EAPI double enesim_renderer_rectangle_corner_radius_x_get(Enesim_Renderer *p);

EAPI void enesim_renderer_rectangle_corner_radius_y_set(Enesim_Renderer *p, double ry);
EAPI double enesim_renderer_rectangle_corner_radius_y_get(Enesim_Renderer *p);

EAPI void enesim_renderer_rectangle_corner_radii_set(Enesim_Renderer *p, double rx, double ry);
EAPI void enesim_renderer_rectangle_corner_radii_get(Enesim_Renderer *p, double *rx, double *ry);

EAPI void enesim_renderer_rectangle_corners_set(Enesim_Renderer *p, Eina_Bool tl, Eina_Bool tr, Eina_Bool bl, Eina_Bool br);
EAPI void enesim_renderer_rectangle_corners_get(Enesim_Renderer *p, Eina_Bool *tl, Eina_Bool *tr, Eina_Bool *bl, Eina_Bool *br);

EAPI void enesim_renderer_rectangle_top_left_corner_set(Enesim_Renderer *r, Eina_Bool rounded);
EAPI Eina_Bool enesim_renderer_rectangle_top_left_corner_get(Enesim_Renderer *r);

EAPI void enesim_renderer_rectangle_top_right_corner_set(Enesim_Renderer *r, Eina_Bool rounded);
EAPI Eina_Bool enesim_renderer_rectangle_top_right_corner_get(Enesim_Renderer *r);

EAPI void enesim_renderer_rectangle_bottom_left_corner_set(Enesim_Renderer *r, Eina_Bool rounded);
EAPI Eina_Bool enesim_renderer_rectangle_bottom_left_corner_get(Enesim_Renderer *r);

EAPI void enesim_renderer_rectangle_bottom_right_corner_set(Enesim_Renderer *r, Eina_Bool rounded);
EAPI Eina_Bool enesim_renderer_rectangle_bottom_right_corner_get(Enesim_Renderer *r);

/**
 * @}
 */

#endif
