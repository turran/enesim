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
#include "enesim_private.h"

#include "enesim_main.h"
#include "enesim_path.h"

#include "enesim_path_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static void _cubic_flatten(double x0, double y0, double ctrl_x0,
		double ctrl_y0, double ctrl_x, double ctrl_y,
		double x, double y, double tolerance,
		Enesim_Path_Vertex_Add vertex_add, void *data)
{
	double x01, y01, x23, y23;
	double xa, ya, xb, yb, xc, yc;

	x01 = (x0 + x) / 2;
	y01 = (y0 + y) / 2;
	x23 = (ctrl_x0 + ctrl_x) / 2;
	y23 = (ctrl_y0 + ctrl_y) / 2;

	if ((((x01 - x23) * (x01 - x23)) + ((y01 - y23) * (y01 - y23))) <= tolerance)
	{
		vertex_add(x, y, data);
		return;
	}

	x01 = (x0 + ctrl_x0) / 2;
	y01 = (y0 + ctrl_y0) / 2;
	xc = x23;
	yc = y23;
	x23 = (x + ctrl_x) / 2;
	y23 = (y + ctrl_y) / 2;
	xa = (x01 + xc) / 2;
	ya = (y01 + yc) / 2;
	xb = (x23 + xc) / 2;
	yb = (y23 + yc) / 2;
	xc = (xa + xb) / 2;
	yc = (ya + yb) / 2;

	_cubic_flatten(x0, y0, x01, y01, xa, ya, xc, yc, tolerance, vertex_add, data);
	_cubic_flatten(xc, yc, xb, yb, x23, y23, x, y, tolerance, vertex_add, data);
}

static void _quadratic_flatten(double x0, double y0, double ctrl_x,
		double ctrl_y, double x, double y, double tolerance,
		Enesim_Path_Vertex_Add vertex_add, void *data)
{
	double x1, y1, x01, y01;

	/* TODO should we check here that the x,y == ctrl_x, ctrl_y? or on the caller */
	x01 = (x0 + x) / 2;
	y01 = (y0 + y) / 2;
	if ((((x01 - ctrl_x) * (x01 - ctrl_x)) + ((y01 - ctrl_y) * (y01
			- ctrl_y))) <= tolerance)
	{
		vertex_add(x, y, data);
		return;
	}

	x0 = (ctrl_x + x0) / 2;
	y0 = (ctrl_y + y0) / 2;
	x1 = (ctrl_x + x) / 2;
	y1 = (ctrl_y + y) / 2;
	x01 = (x0 + x1) / 2;
	y01 = (y0 + y1) / 2;

	_quadratic_flatten(x, y, x0, y0, x01, y01, tolerance, vertex_add, data);
	_quadratic_flatten(x01, y01, x1, y1, x, y, tolerance, vertex_add, data);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_path_quadratic_cubic_to(Enesim_Path_Quadratic *q,
		Enesim_Path_Cubic *c)
{
	/* same start and end points */
	c->end_x = q->end_x;
	c->start_x = q->start_x;
	c->end_y = q->end_y;
	c->start_y = q->start_y;
	/* create the new control points */
	c->ctrl_x0 = q->start_x + (2.0/3.0 * (q->ctrl_x - q->start_x));
	c->ctrl_x1 = q->end_x + (2.0/3.0 * (q->ctrl_x - q->end_x));

	c->ctrl_y0 = q->start_y + (2.0/3.0 * (q->ctrl_y - q->start_y));
	c->ctrl_y1 = q->end_y + (2.0/3.0 * (q->ctrl_y - q->end_y));
}

void enesim_path_quadratic_flatten(Enesim_Path_Quadratic *thiz,
		double tolerance, Enesim_Path_Vertex_Add vertex_add,
		void *data)
{
	_quadratic_flatten(thiz->start_x, thiz->start_y, thiz->ctrl_x,
			thiz->ctrl_y, thiz->end_x, thiz->end_y,
			tolerance, vertex_add, data);
}

void enesim_path_cubic_flatten(Enesim_Path_Cubic *thiz,
		double tolerance, Enesim_Path_Vertex_Add vertex_add,
		void *data)
{
	/* force one initial subdivision */
	double x01 = (thiz->start_x + thiz->ctrl_x0) / 2;
	double y01 = (thiz->start_y + thiz->ctrl_y0) / 2;
	double xc = (thiz->ctrl_x0 + thiz->ctrl_x1) / 2;
	double yc = (thiz->ctrl_y0 + thiz->ctrl_y1) / 2;
	double x23 = (thiz->end_x + thiz->ctrl_x1) / 2;
	double y23 = (thiz->end_y + thiz->ctrl_y1) / 2;
	double xa = (x01 + xc) / 2;
	double ya = (y01 + yc) / 2;
	double xb = (x23 + xc) / 2;
	double yb = (y23 + yc) / 2;

	xc = (xa + xb) / 2;
	yc = (ya + yb) / 2;
	_cubic_flatten(thiz->start_x, thiz->start_y, x01, y01, xa, ya, xc, yc,
			tolerance, vertex_add, data);
	_cubic_flatten(xc, yc, xb, yb, x23, y23, thiz->end_x, thiz->end_y,
			tolerance, vertex_add, data);
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

