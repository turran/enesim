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
#ifndef ENESIM_RENDERER_PATTERN_H_
#define ENESIM_RENDERER_PATTERN_H_

/**
 * @file
 * @listgroup{Enesim_Renderer_Pattern}
 */

/**
 * @defgroup Enesim_Renderer_Pattern Pattern
 * @brief Renderer that repeats an area of another renderer to form a pattern @inherits{Enesim_Renderer}
 * @ingroup Enesim_Renderer
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_pattern_new(void);
EAPI void enesim_renderer_pattern_source_renderer_set(Enesim_Renderer *r, Enesim_Renderer *source);
EAPI Enesim_Renderer * enesim_renderer_pattern_source_renderer_get(Enesim_Renderer *r);
EAPI void enesim_surface_pattern_source_surface_set(Enesim_Renderer *r, Enesim_Surface *source);
EAPI Enesim_Surface * enesim_renderer_pattern_source_surface_get(Enesim_Renderer *r);
EAPI void enesim_renderer_pattern_repeat_mode_set(Enesim_Renderer *r, Enesim_Repeat_Mode mode);
EAPI Enesim_Repeat_Mode enesim_renderer_pattern_repeat_mode_get(Enesim_Renderer *r);

/**
 * @}
 */

#endif

