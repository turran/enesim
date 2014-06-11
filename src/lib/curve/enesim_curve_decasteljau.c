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

/* Approach to the Loop & Blinn algorithm found on
 * http://research.microsoft.com/en-us/um/people/cloop/LoopBlinn05.pdf
 * Also with some ideas taken from WebCore code
 */
#include "enesim_private.h"
#include "enesim_path.h"

#include "enesim_path_private.h"
#include "enesim_curve_private.h"
#include "enesim_curve_decasteljau_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_curve_decasteljau_cubic_at(const Enesim_Path_Cubic *c, double t,
		Enesim_Path_Cubic *left, Enesim_Path_Cubic *right)
{
	double mx, my;

	mx = ((1 - t) * c->ctrl_x0) + (t * c->ctrl_x1);
	my = ((1 - t) * c->ctrl_y0) + (t * c->ctrl_y1);

	/* left */
	left->start_x = c->start_x;
	left->start_y = c->start_y;
	left->ctrl_x0 = ((1 - t) * c->start_x) + (t * c->ctrl_x0);
	left->ctrl_y0 = ((1 - t) * c->start_y) + (t * c->ctrl_y0);
	left->ctrl_x1 = ((1 - t) * left->ctrl_x0) + (t * mx);
	left->ctrl_y1 = ((1 - t) * left->ctrl_y0) + (t * my);

	/* right */
	right->end_y = c->end_y;
	right->end_x = c->end_x;
	right->ctrl_x1 = ((1 - t) * c->ctrl_x1) + (t * c->end_x);
	right->ctrl_y1 = ((1 - t) * c->ctrl_y1) + (t * c->end_y);
	right->ctrl_x0 = ((1 - t) * mx) + (t * right->ctrl_x1);
	right->ctrl_y0 = ((1 - t) * my) + (t * right->ctrl_y1);

	/* common */
	left->end_x = ((1 - t) * left->ctrl_x1) + (t * right->ctrl_x0);
	left->end_y = ((1 - t) * left->ctrl_y1) + (t * right->ctrl_y0);
	right->start_x = left->end_x;
	right->start_y = left->end_y;
}

void enesim_curve_decasteljau_cubic_mid(const Enesim_Path_Cubic *c,
		Enesim_Path_Cubic *left, Enesim_Path_Cubic *right)
{
	double mx, my;

	mx = (c->ctrl_x0 + c->ctrl_x1) * 0.5;
	my = (c->ctrl_y0 + c->ctrl_y1) * 0.5;

	/* left */
	left->start_x = c->start_x;
	left->start_y = c->start_y;
	left->ctrl_x0 = (c->start_x + c->ctrl_x0) * 0.5;
	left->ctrl_y0 = (c->start_y + c->ctrl_y0) * 0.5;
	left->ctrl_x1 = (left->ctrl_x0 + mx) * 0.5;
	left->ctrl_y1 = (left->ctrl_y0 + my) * 0.5;

	/* right */
	right->end_y = c->end_y;
	right->end_x = c->end_x;
	right->ctrl_x1 = (c->ctrl_x1 + c->end_x) * 0.5;
	right->ctrl_y1 = (c->ctrl_y1 + c->end_y) * 0.5;
	right->ctrl_x0 = (mx + right->ctrl_x1) * 0.5;
	right->ctrl_y0 = (my + right->ctrl_y1) * 0.5;

	/* common */
	left->end_x = (left->ctrl_x1 + right->ctrl_x0) * 0.5;
	left->end_y = (left->ctrl_y1 + right->ctrl_y0) * 0.5;
	right->start_x = left->end_x;
	right->start_y = left->end_y;
}

void enesim_curve_decasteljau_quadratic_at(const Enesim_Path_Quadratic *c, double t,
		Enesim_Path_Quadratic *left, Enesim_Path_Quadratic *right)
{
	/* left */
	left->start_x = c->start_x;
	left->start_y = c->start_y;
	left->ctrl_x = ((1 - t) * c->start_x) + (t * c->ctrl_x);
	left->ctrl_y = ((1 - t) * c->start_y) + (t * c->ctrl_y);

	/* right */
	right->end_x = c->end_x;
	right->end_y = c->end_y;
	right->ctrl_x = ((1 - t) * c->ctrl_x) + (t * c->end_x);
	right->ctrl_y = ((1 - t) * c->ctrl_y) + (t * c->end_y);

	/* common */
	left->end_x = ((1 - t) * left->ctrl_x) + (t * right->ctrl_x);
	left->end_y = ((1 - t) * left->ctrl_y) + (t * right->ctrl_y);
	right->start_x = left->end_x;
	right->start_y = left->end_y;
}

void enesim_curve_decasteljau_quadratic_mid(const Enesim_Path_Quadratic *c,
		Enesim_Path_Quadratic *left, Enesim_Path_Quadratic *right)
{
	/* left */
	left->start_x = c->start_x;
	left->start_y = c->start_y;
	left->ctrl_x = (c->start_x + c->ctrl_x) * 0.5;
	left->ctrl_y = (c->start_y + c->ctrl_y) * 0.5;

	/* right */
	right->end_x = c->end_x;
	right->end_y = c->end_y;
	right->ctrl_x = (c->ctrl_x + c->end_x) * 0.5;
	right->ctrl_y = (c->ctrl_y + c->end_y) * 0.5;

	/* common */
	left->end_x = (left->ctrl_x + right->ctrl_x) * 0.5;
	left->end_y = (left->ctrl_y + right->ctrl_y) * 0.5;
	right->start_x = left->end_x;
	right->start_y = left->end_y;
}
/** @endcond */
