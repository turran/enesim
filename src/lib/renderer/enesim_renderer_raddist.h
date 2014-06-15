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
#ifndef ENESIM_RENDERER_RADDIST_H_
#define ENESIM_RENDERER_RADDIST_H_

/**
 * @file
 * @listgroup{Enesim_Renderer_Raddist}
 */

/**
 * @defgroup Enesim_Renderer_Raddist Radial Distortion
 * @brief Filters a source image using a barrel distortion @inherits{Enesim_Renderer}
 * @ingroup Enesim_Renderer
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_raddist_new(void);
EAPI void enesim_renderer_raddist_radius_set(Enesim_Renderer *r, double radius);
EAPI double enesim_renderer_raddist_radius_get(Enesim_Renderer *r);

EAPI void enesim_renderer_raddist_factor_set(Enesim_Renderer *r, double factor);
EAPI double enesim_renderer_raddist_factor_get(Enesim_Renderer *r);

EAPI void enesim_renderer_raddist_source_surface_set(Enesim_Renderer *r, Enesim_Surface *src);
EAPI Enesim_Surface * enesim_renderer_raddist_source_surface_get(Enesim_Renderer *r);

EAPI void enesim_renderer_raddist_x_set(Enesim_Renderer *r, double ox);
EAPI void enesim_renderer_raddist_y_set(Enesim_Renderer *r, double oy);

/**
 * @}
 */

#endif
