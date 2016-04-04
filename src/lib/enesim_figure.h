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
#ifndef ENESIM_FIGURE_H_
#define ENESIM_FIGURE_H_

/**
 * @file
 * @ender_group_proto{Enesim_Figure}
 * @ender_group{Enesim_Figure}
 * @ender_group{Enesim_Figure_Definitions}
 */

/**
 * Figure Handle
 * @ingroup Enesim_Figure
 */
typedef struct _Enesim_Figure Enesim_Figure;

/**
 * @defgroup Enesim_Figure_Definitions Figure definitions
 * @ingroup Enesim_Figure
 * @{
 */

typedef double (*Enesim_Figure_Point_At_Cb)(const Enesim_Figure *thiz, double x, double y, double n, void *data);

/**
 * @}
 * @defgroup Enesim_Figure Figures
 * @ingroup Enesim_Basic
 * @brief Figure definition
 * @{
 */

EAPI Enesim_Figure * enesim_figure_new(void);
EAPI Enesim_Figure * enesim_figure_ref(Enesim_Figure *thiz);
EAPI void enesim_figure_unref(Enesim_Figure *thiz);
EAPI Eina_Bool enesim_figure_bounds(const Enesim_Figure *thiz, double *xmin, double *ymin, double *xmax, double *ymax);
EAPI void enesim_figure_clear(Enesim_Figure *thiz);

EAPI void enesim_figure_polygon_add(Enesim_Figure *thiz);
EAPI void enesim_figure_polygon_vertex_add(Enesim_Figure *thiz,
		double x, double y);
EAPI void enesim_figure_polygon_close(Enesim_Figure *thiz);
EAPI double enesim_figure_length_get(Enesim_Figure *thiz);

EAPI void enesim_figure_point_at(Enesim_Figure *thiz, double at,
		Enesim_Figure_Point_At_Cb cb, void *data);

/**
 * @}
 */

#endif

