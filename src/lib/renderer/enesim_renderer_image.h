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
#ifndef ENESIM_RENDERER_IMAGE_H_
#define ENESIM_RENDERER_IMAGE_H_

/**
 * @defgroup Enesim_Renderer_Image Image
 * @brief Image based renderer
 * @ingroup Enesim_Renderer
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_image_new(void);
EAPI void enesim_renderer_image_x_set(Enesim_Renderer *r, double x);
EAPI double enesim_renderer_image_x_get(Enesim_Renderer *r);

EAPI void enesim_renderer_image_y_set(Enesim_Renderer *r, double y);
EAPI double enesim_renderer_image_y_get(Enesim_Renderer *r);

EAPI void enesim_renderer_image_position_set(Enesim_Renderer *r, double x, double y);
EAPI void enesim_renderer_image_position_get(Enesim_Renderer *r, double *x, double *y);

EAPI void enesim_renderer_image_width_set(Enesim_Renderer *r, double w);
EAPI double enesim_renderer_image_width_get(Enesim_Renderer *r);

EAPI void enesim_renderer_image_height_set(Enesim_Renderer *r, double h);
EAPI double enesim_renderer_image_height_get(Enesim_Renderer *r);

EAPI void enesim_renderer_image_size_set(Enesim_Renderer *r, double w, double h);
EAPI void enesim_renderer_image_size_get(Enesim_Renderer *r, double *w, double *h);

EAPI void enesim_renderer_image_source_surface_set(Enesim_Renderer *r, Enesim_Surface *src);
EAPI Enesim_Surface * enesim_renderer_image_source_surface_get(Enesim_Renderer *r);

EAPI void enesim_renderer_image_damage_add(Enesim_Renderer *r, const Eina_Rectangle *area);

/**
 * @}
 */

#endif

