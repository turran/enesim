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
#ifndef ENESIM_RENDERER_CIRCLE_H_
#define ENESIM_RENDERER_CIRCLE_H_

/**
 * @file
 * @listgroup{Enesim_Renderer_Circle}
 */

/**
 * @defgroup Enesim_Renderer_Circle Circle
 * @brief Circle renderer @inherits{Enesim_Renderer_Shape}
 * @ingroup Enesim_Renderer_Shape
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_circle_new(void);

EAPI void enesim_renderer_circle_x_set(Enesim_Renderer *r, double x);
EAPI double enesim_renderer_circle_x_get(Enesim_Renderer *r);

EAPI void enesim_renderer_circle_y_set(Enesim_Renderer *r, double y);
EAPI double enesim_renderer_circle_y_get(Enesim_Renderer *r);

EAPI void enesim_renderer_circle_center_set(Enesim_Renderer *r, double x, double y);
EAPI void enesim_renderer_circle_center_get(Enesim_Renderer *r, double *x, double *y);

EAPI void enesim_renderer_circle_radius_set(Enesim_Renderer *r, double radius);
EAPI double enesim_renderer_circle_radius_get(Enesim_Renderer *r);
/**
 * @}
 */

#endif
