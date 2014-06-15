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
#ifndef ENESIM_RENDERER_GRADIENT_LINEAR_H_
#define ENESIM_RENDERER_GRADIENT_LINEAR_H_

/**
 * @file
 * @listgroup{Enesim_Renderer_Gradient_Linear}
 */

/**
 * @defgroup Enesim_Renderer_Gradient_Linear Linear
 * @brief Linear gradient @inherits{Enesim_Renderer_Gradient}
 * @ingroup Enesim_Renderer_Gradient
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_gradient_linear_new(void);
EAPI void enesim_renderer_gradient_linear_x0_set(Enesim_Renderer *r, double x0);
EAPI double enesim_renderer_gradient_linear_x0_get(Enesim_Renderer *r);
EAPI void enesim_renderer_gradient_linear_y0_set(Enesim_Renderer *r, double y0);
EAPI double enesim_renderer_gradient_linear_y0_get(Enesim_Renderer *r);
EAPI void enesim_renderer_gradient_linear_x1_set(Enesim_Renderer *r, double x1);
EAPI double enesim_renderer_gradient_linear_x1_get(Enesim_Renderer *r);
EAPI void enesim_renderer_gradient_linear_y1_set(Enesim_Renderer *r, double y1);
EAPI double enesim_renderer_gradient_linear_y1_get(Enesim_Renderer *r);
EAPI void enesim_renderer_gradient_linear_position_set(Enesim_Renderer *r, double x0,
		double y0, double x1, double y1);
EAPI void enesim_renderer_gradient_linear_position_get(Enesim_Renderer *r, double *x0,
		double *y0, double *x1, double *y1);

/**
 * @}
 */

#endif
