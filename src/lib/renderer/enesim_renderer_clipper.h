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
#ifndef ENESIM_RENDERER_CLIPPER_H_
#define ENESIM_RENDERER_CLIPPER_H_

/**
 * @defgroup Enesim_Renderer_Clipper_Group Clipper
 * @brief Renderer that clips a source renderer
 * @ingroup Enesim_Renderer_Group
 * @{
 */

EAPI Enesim_Renderer * enesim_renderer_clipper_new(void);
EAPI void enesim_renderer_clipper_clipped_set(Enesim_Renderer *r,
		Enesim_Renderer *clipped);
EAPI Enesim_Renderer * enesim_renderer_clipper_clipped_get(Enesim_Renderer *r);
EAPI void enesim_renderer_clipper_x_set(Enesim_Renderer *r, int x);
EAPI int enesim_renderer_clipper_x_get(Enesim_Renderer *r);
EAPI void enesim_renderer_clipper_y_set(Enesim_Renderer *r, int y);
EAPI int enesim_renderer_clipper_y_get(Enesim_Renderer *r);
EAPI void enesim_renderer_clipper_position_set(Enesim_Renderer *r, int x, int y);
EAPI void enesim_renderer_clipper_position_get(Enesim_Renderer *r, int *x, int *y);
EAPI void enesim_renderer_clipper_width_set(Enesim_Renderer *r,
		int width);
EAPI int enesim_renderer_clipper_width_get(Enesim_Renderer *r);

EAPI void enesim_renderer_clipper_height_set(Enesim_Renderer *r,
		int height);
EAPI int enesim_renderer_clipper_height_get(Enesim_Renderer *r);
EAPI void enesim_renderer_clipper_size_set(Enesim_Renderer *r, int w, int h);
EAPI void enesim_renderer_clipper_size_get(Enesim_Renderer *r, int *w, int *h);

/**
 * @}
 */

#endif

