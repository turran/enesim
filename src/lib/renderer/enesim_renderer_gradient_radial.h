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
#ifndef ENESIM_RENDERER_GRADIENT_RADIAL_H_
#define ENESIM_RENDERER_GRADIENT_RADIAL_H_

/**
 * @file
 * @listgroup{Enesim_Renderer_Gradient_Radial}
 */

/**
 * @defgroup Enesim_Renderer_Gradient_Radial Radial
 * @brief Radial gradient @inherits{Enesim_Renderer_Gradient}
 * @ingroup Enesim_Renderer_Gradient
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_gradient_radial_new(void);
EAPI void enesim_renderer_gradient_radial_center_x_set(Enesim_Renderer *r, double center_x);
EAPI double enesim_renderer_gradient_radial_center_x_get(Enesim_Renderer *r);
EAPI void enesim_renderer_gradient_radial_center_y_set(Enesim_Renderer *r, double center_y);
EAPI double enesim_renderer_gradient_radial_center_y_get(Enesim_Renderer *r);
EAPI void enesim_renderer_gradient_radial_center_set(Enesim_Renderer *r, double center_x, double center_y);
EAPI void enesim_renderer_gradient_radial_center_get(Enesim_Renderer *r, 
							double *center_x, double *center_y);

EAPI void enesim_renderer_gradient_radial_focus_x_set(Enesim_Renderer *r, double focus_x);
EAPI double enesim_renderer_gradient_radial_focus_x_get(Enesim_Renderer *r);
EAPI void enesim_renderer_gradient_radial_focus_y_set(Enesim_Renderer *r, double focus_y);
EAPI double enesim_renderer_gradient_radial_focus_y_get(Enesim_Renderer *r);
EAPI void enesim_renderer_gradient_radial_focus_set(Enesim_Renderer *r, double focus_x, double focus_y);
EAPI void enesim_renderer_gradient_radial_focus_get(Enesim_Renderer *r, 
							double *focus_x, double *focus_y);

EAPI void enesim_renderer_gradient_radial_radius_set(Enesim_Renderer *r, double radius);
EAPI double enesim_renderer_gradient_radial_radius_get(Enesim_Renderer *r);

/**
 * @}
 */

#endif
