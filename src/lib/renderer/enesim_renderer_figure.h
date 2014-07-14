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
#ifndef ENESIM_RENDERER_FIGURE_H_
#define ENESIM_RENDERER_FIGURE_H_

/**
 * @file
 * @ender_group{Enesim_Renderer_Figure}
 */

/**
 * @defgroup Enesim_Renderer_Figure Figure
 * @brief A figure composed of polygons renderer @ender_inherits{Enesim_Renderer_Shape}
 * @ingroup Enesim_Renderer_Shape
 * @{
 */

EAPI Enesim_Renderer * enesim_renderer_figure_new(void);
EAPI void enesim_renderer_figure_polygon_add(Enesim_Renderer *p);
EAPI void enesim_renderer_figure_polygon_vertex_add(Enesim_Renderer *p, double x, double y);
EAPI void enesim_renderer_figure_polygon_close(Enesim_Renderer *p);
EAPI void enesim_renderer_figure_clear(Enesim_Renderer *p);
/**
 * @}
 */

#endif
