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
#ifndef ENESIM_RENDERER_MAP_QUAD_H_
#define ENESIM_RENDERER_MAP_QUAD_H_

/**
 * @file
 * @ender_group{Enesim_Renderer_Map_Quad}
 */

/**
 * @defgroup Enesim_Renderer_Map_Quad Map_Quad
 * @brief Map_Quad based renderer @ender_inherits{Enesim_Renderer}
 * @ingroup Enesim_Renderer
 * @{
 */

EAPI Enesim_Renderer * enesim_renderer_map_quad_new(void);
EAPI void enesim_renderer_map_quad_source_surface_set(Enesim_Renderer *r,
		Enesim_Surface *src);
EAPI Enesim_Surface * enesim_renderer_map_quad_source_surface_get(
		Enesim_Renderer *r);
EAPI void enesim_renderer_map_quad_vertex_color_set(Enesim_Renderer *r,
		int index, Enesim_Argb color);
EAPI Enesim_Argb enesim_renderer_map_quad_vertex_color_get(Enesim_Renderer *r,
		int index);
EAPI void enesim_renderer_map_quad_vertex_position_set(Enesim_Renderer *r,
		int index, double x, double y);
EAPI void enesim_renderer_map_quad_vertex_position_get(Enesim_Renderer *r,
		int index, double *x, double *y);

/**
 * @}
 */

#endif


