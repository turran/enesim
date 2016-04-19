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
#ifndef ENESIM_FIGURE_PRIVATE_H_
#define ENESIM_FIGURE_PRIVATE_H_

typedef struct _Enesim_Polygon
{
	Eina_List *points;
	double threshold;
	double xmax;
	double xmin;
	double ymax;
	double ymin;
	Eina_Bool closed;
} Enesim_Polygon;

typedef struct _Enesim_Figure
{
	Eina_List *polygons;
	int ref;
	double xmax;
	double xmin;
	double ymax;
	double ymin;
	double d;
	int changed;
	int bounds_generated;
	int distance_generated;
} Enesim_Figure;
/*----------------------------------------------------------------------------*
 *                                 Polygon                                    *
 *----------------------------------------------------------------------------*/

Enesim_Polygon * enesim_polygon_new(void);
void enesim_polygon_delete(Enesim_Polygon *thiz);
int enesim_polygon_point_count(Enesim_Polygon *thiz);
void enesim_polygon_point_append_from_coords(Enesim_Polygon *thiz, double x, double y);
void enesim_polygon_point_prepend_from_coords(Enesim_Polygon *thiz, double x, double y);
void enesim_polygon_clear(Enesim_Polygon *thiz);
void enesim_polygon_close(Enesim_Polygon *thiz, Eina_Bool close);
void enesim_polygon_merge(Enesim_Polygon *thiz, Enesim_Polygon *to_merge);
Eina_Bool enesim_polygon_bounds(const Enesim_Polygon *thiz, double *xmin, double *ymin, double *xmax, double *ymax);
void enesim_polygon_threshold_set(Enesim_Polygon *p, double threshold);
void enesim_polygon_dump(Enesim_Polygon *thiz);

/*----------------------------------------------------------------------------*
 *                                  Figure                                    *
 *----------------------------------------------------------------------------*/
Enesim_Figure * enesim_figure_new(void);
int enesim_figure_polygon_count(Enesim_Figure *thiz);
void enesim_figure_polygon_append(Enesim_Figure *thiz, Enesim_Polygon *p);
void enesim_figure_polygon_remove(Enesim_Figure *thiz, Enesim_Polygon *p);
void enesim_figure_dump(Enesim_Figure *thiz);
void enesim_figure_change(Enesim_Figure *thiz);
int enesim_figure_changed(Enesim_Figure *thiz);
void enesim_figure_reset(Enesim_Figure *thiz);

#endif
