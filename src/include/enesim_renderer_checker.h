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
#ifndef ENESIM_RENDERER_CHECKER_H_
#define ENESIM_RENDERER_CHECKER_H_

/**
 * @defgroup Enesim_Renderer_Checker_Group Checker
 * @ingroup Enesim_Renderer_Group
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_checker_new(void);
EAPI void enesim_renderer_checker_even_color_set(Enesim_Renderer *r, Enesim_Color color);
EAPI void enesim_renderer_checker_even_color_get(Enesim_Renderer *r, Enesim_Color *color);
EAPI void enesim_renderer_checker_odd_color_set(Enesim_Renderer *r, Enesim_Color color);
EAPI void enesim_renderer_checker_odd_color_get(Enesim_Renderer *r, Enesim_Color *color);
EAPI void enesim_renderer_checker_width_set(Enesim_Renderer *r, int width);
EAPI void enesim_renderer_checker_width_get(Enesim_Renderer *r, int *width);
EAPI void enesim_renderer_checker_height_set(Enesim_Renderer *r, int height);
EAPI void enesim_renderer_checker_height_get(Enesim_Renderer *r, int *height);

/**
 * @}
 */

#endif
