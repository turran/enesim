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
/* TODO whenever we append/prepend a new point, calculate the boundings */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
Enesim_Point * _point_new(double x, double y)
{
	Enesim_Point *p;

	p = calloc(1, sizeof(Enesim_Point));
	p->x = x;
	p->y = y;

	return p;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Polygon * enesim_polygon_new(void)
{
	Enesim_Polygon *p;

	p = calloc(1, sizeof(Enesim_Polygon));
	return p;
}

void enesim_polygon_point_append(Enesim_Polygon *ep, Enesim_Point *p)
{
	Enesim_Point *new_point;

	new_point = _point_new(p->x, p->y);
	ep->points = eina_list_append(ep->points, new_point);
}

void enesim_polygon_point_prepend(Enesim_Polygon *ep, Enesim_Point *p)
{
	Enesim_Point *new_point;

	new_point = _point_new(p->x, p->y);
	ep->points = eina_list_prepend(ep->points, new_point);
}

void enesim_polygon_boundings(Enesim_Polygon *ep, Enesim_Rectangle *bounds)
{

}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

