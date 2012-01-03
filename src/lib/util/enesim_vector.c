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
#include "Enesim.h"
#include "enesim_private.h"
#include "float.h"
/* TODO whenever we append/prepend a new point, calculate the boundings */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static inline void _polygon_update_bounds(Enesim_Polygon *ep, Enesim_Point *p)
{
	if (p->x > ep->xmax) ep->xmax = p->x;
	if (p->y > ep->ymax) ep->ymax = p->y;
	if (p->x < ep->xmin) ep->xmin = p->x;
	if (p->y < ep->ymin) ep->ymin = p->y;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Point * enesim_point_new(void)
{
	Enesim_Point *thiz;

	thiz = calloc(1, sizeof(Enesim_Point));
	return thiz;
}

Enesim_Point * enesim_point_new_from_coords(double x, double y)
{
	Enesim_Point *thiz;

	thiz = enesim_point_new();
	thiz->x = x;
	thiz->y = y;

	return thiz;
}

Enesim_Polygon * enesim_polygon_new(void)
{
	Enesim_Polygon *p;

	p = calloc(1, sizeof(Enesim_Polygon));
	p->xmax = p->ymax = -DBL_MAX;
	p->xmin = p->ymin = DBL_MAX;
	return p;
}

void enesim_polygon_point_append_from_coords(Enesim_Polygon *thiz, double x, double y)
{
	Enesim_Point *p;

	p = enesim_point_new_from_coords(x, y);
	enesim_polygon_point_append(thiz, p);
}

void enesim_polygon_point_append(Enesim_Polygon *thiz, Enesim_Point *p)
{
	thiz->points = eina_list_append(thiz->points, p);
	_polygon_update_bounds(thiz, p);
}

void enesim_polygon_point_prepend(Enesim_Polygon *thiz, Enesim_Point *p)
{
	thiz->points = eina_list_prepend(thiz->points, p);
	_polygon_update_bounds(thiz, p);
}

int enesim_polygon_point_count(Enesim_Polygon *thiz)
{
	return eina_list_count(thiz->points);
}

void enesim_polygon_clear(Enesim_Polygon *thiz)
{

}

void enesim_polygon_close(Enesim_Polygon *thiz, Eina_Bool close)
{
	thiz->closed = close;
}

void enesim_polygon_boundings(Enesim_Polygon *ep, Enesim_Rectangle *bounds)
{

}

Enesim_Figure * enesim_figure_new(void)
{
	Enesim_Figure *thiz;

	thiz = calloc(1, sizeof(Enesim_Figure));
	return thiz;
}

void enesim_figure_delete(Enesim_Figure *thiz)
{

}

void enesim_figure_polygon_append(Enesim_Figure *thiz, Enesim_Polygon *p)
{

}

int enesim_figure_polygon_count(Enesim_Figure *thiz)
{
	return eina_list_count(thiz->polygons);
}

void enesim_figure_clear(Enesim_Figure *thiz)
{

}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

