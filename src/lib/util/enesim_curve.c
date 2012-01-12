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
/* FIXME
 * the state last_ctrl_pt/last_pt is broken on the cubic and quadratic
 * we should have an indirection to keep track correctly of the state
 * If we fix it, looks like a new vertex is created that is on almost
 * the same place as the one before, and the path renderer (the basic
 * rasterizer really) complains at line 796:
 * _basic_sw_setup basic2 Length 0 < 0.00390625
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_curve_line_to(Enesim_Curve_State *state,
		double x, double y)
{
	state->vertex_add(x, y, state->data);
	state->last_ctrl_x = state->last_x;
	state->last_ctrl_y = state->last_y;
	state->last_x = x;
	state->last_y = y;
}

/* these subdiv approximations need to be done more carefully */
void enesim_curve_quadratic_to(Enesim_Curve_State *state,
		double ctrl_x, double ctrl_y,
		double x, double y)
{
	double x0, y0, x1, y1, x01, y01;
	double sm = 1 / 92.0;

	x0 = state->last_x;
	y0 = state->last_y;
	x01 = (x0 + x) / 2;
	y01 = (y0 + y) / 2;
	if ((((x01 - ctrl_x) * (x01 - ctrl_x)) + ((y01 - ctrl_y) * (y01
			- ctrl_y))) <= sm)
	{
		state->vertex_add(x, y, state->data);
		state->last_x = x;
		state->last_y = y;
		state->last_ctrl_x = ctrl_x;
		state->last_ctrl_y = ctrl_y;
		return;
	}

	x0 = (ctrl_x + x0) / 2;
	y0 = (ctrl_y + y0) / 2;
	x1 = (ctrl_x + x) / 2;
	y1 = (ctrl_y + y) / 2;
	x01 = (x0 + x1) / 2;
	y01 = (y0 + y1) / 2;

	enesim_curve_quadratic_to(state, x0, y0, x01, y01);
	state->last_x = x01;
	state->last_y = y01;
	state->last_ctrl_x = x0;
	state->last_ctrl_y = y0;

	enesim_curve_quadratic_to(state, x1, y1, x, y);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = x1;
	state->last_ctrl_y = y1;
}

void enesim_curve_squadratic_to(Enesim_Curve_State *state,
		double x, double y)
{
	double x0, y0, cx0, cy0;

	x0 = state->last_x;
	y0 = state->last_y;
	cx0 = state->last_ctrl_x;
	cy0 = state->last_ctrl_y;
	cx0 = (2 * x0) - cx0;
	cy0 = (2 * y0) - cy0;

	enesim_curve_quadratic_to(state, cx0, cy0, x, y);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = cx0;
	state->last_ctrl_y = cy0;
}

void enesim_curve_cubic_to(Enesim_Curve_State *state,
		double ctrl_x0, double ctrl_y0,
		double ctrl_x, double ctrl_y,
		double x, double y)
{
	double x0, y0, x01, y01, x23, y23;
	double xa, ya, xb, yb, xc, yc;
	double sm = 1 / 64.0;

	x0 = state->last_x;
	y0 = state->last_y;
	x01 = (x0 + x) / 2;
	y01 = (y0 + y) / 2;
	x23 = (ctrl_x0 + ctrl_x) / 2;
	y23 = (ctrl_y0 + ctrl_y) / 2;

	if ((((x01 - x23) * (x01 - x23)) + ((y01 - y23) * (y01 - y23))) <= sm)
	{
		state->vertex_add(x, y, state->data);
		state->last_x = x;
		state->last_y = y;
		state->last_ctrl_x = ctrl_x;
		state->last_ctrl_y = ctrl_y;
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

	enesim_curve_cubic_to(state, x01, y01, xa, ya, xc, yc);
	state->last_x = xc;
	state->last_y = yc;
	state->last_ctrl_x = xa;
	state->last_ctrl_y = ya;

	enesim_curve_cubic_to(state, xb, yb, x23, y23, x, y);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = x23;
	state->last_ctrl_y = y23;
}

void enesim_curve_scubic_to(Enesim_Curve_State *state,
		double ctrl_x, double ctrl_y,
		double x, double y)
{
	double x0, y0, cx0, cy0;

	x0 = state->last_x;
	y0 = state->last_y;
	cx0 = state->last_ctrl_x;
	cy0 = state->last_ctrl_y;
	cx0 = (2 * x0) - cx0;
	cy0 = (2 * y0) - cy0;

	enesim_curve_cubic_to(state, cx0, cy0, ctrl_x, ctrl_y, x, y);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = ctrl_x;
	state->last_ctrl_y = ctrl_y;
}

/* code adapted from the moonlight sources
 * we need to fix it to match c89
 */
void enesim_curve_arc_to(Enesim_Curve_State *state, double rx, double ry, double angle,
		unsigned char large, unsigned char sweep, double x, double y)
{
	double sx, sy;
	double cos_phi, sin_phi;
	double dx2, dy2;
	double x1p, y1p;
	double x1p2, y1p2;
	double rx2, ry2;
	double lambda;

	// some helpful stuff is available here:
	// http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes

	sx = state->last_x;
	sy = state->last_y;

	// if start and end points are identical, then no arc is drawn
	if ((fabs(x - sx) < (1 / 256.0)) && (fabs(y - sy) < (1 / 256.0)))
		return;

	// Correction of out-of-range radii, see F6.6.1 (step 2)
	rx = fabs(rx);
	ry = fabs(ry);
	if ((rx < 0.5) || (ry < 0.5))
	{
		enesim_curve_line_to(state, x, y);
		return;
	}

	angle = angle * M_PI / 180.0;
	cos_phi = cos(angle);
	sin_phi = sin(angle);
	dx2 = (sx - x) / 2.0;
	dy2 = (sy - y) / 2.0;
	x1p = cos_phi * dx2 + sin_phi * dy2;
	y1p = cos_phi * dy2 - sin_phi * dx2;
	x1p2 = x1p * x1p;
	y1p2 = y1p * y1p;
	rx2 = rx * rx;
	ry2 = ry * ry;
	lambda = (x1p2 / rx2) + (y1p2 / ry2);

	// Correction of out-of-range radii, see F6.6.2 (step 4)
	if (lambda > 1.0)
	{
		// see F6.6.3
		double lambda_root = sqrt(lambda);

		rx *= lambda_root;
		ry *= lambda_root;
		// update rx2 and ry2
		rx2 = rx * rx;
		ry2 = ry * ry;
	}

	double cxp, cyp, cx, cy;
	double c = (rx2 * ry2) - (rx2 * y1p2) - (ry2 * x1p2);

	// check if there is no possible solution
	// (i.e. we can't do a square root of a negative value)
	if (c < 0.0)
	{
		// scale uniformly until we have a single solution
		// (see F6.2) i.e. when c == 0.0
		double scale = sqrt(1.0 - c / (rx2 * ry2));
		rx *= scale;
		ry *= scale;
		// update rx2 and ry2
		rx2 = rx * rx;
		ry2 = ry * ry;

		// step 2 (F6.5.2) - simplified since c == 0.0
		cxp = 0.0;
		cyp = 0.0;
		// step 3 (F6.5.3 first part) - simplified since cxp and cyp == 0.0
		cx = 0.0;
		cy = 0.0;
	}
	else
	{
		// complete c calculation
		c = sqrt(c / ((rx2 * y1p2) + (ry2 * x1p2)));
		// inverse sign if Fa == Fs
		if (large == sweep)
		   c = -c;

		// step 2 (F6.5.2)
		cxp = c * ( rx * y1p / ry);
		cyp = c * (-ry * x1p / rx);

		// step 3 (F6.5.3 first part)
		cx = cos_phi * cxp - sin_phi * cyp;
		cy = sin_phi * cxp + cos_phi * cyp;
	}

	// step 3 (F6.5.3 second part) we now have the center point of the ellipse
	cx += (sx + x) / 2.0;
	cy += (sy + y) / 2.0;

	// step 4 (F6.5.4)
	// we dont' use arccos (as per w3c doc),
	// see http://www.euclideanspace.com/maths/algebra/vectors/angleBetween/index.htm
	// note: atan2 (0.0, 1.0) == 0.0
	double at = atan2(((y1p - cyp) / ry), ((x1p - cxp) / rx));
	double theta1 = (at < 0.0) ? 2.0 * M_PI + at : at;

	double nat = atan2(((-y1p - cyp) / ry), ((-x1p - cxp) / rx));
	double delta_theta = (nat < at) ? 2.0 * M_PI - at + nat : nat - at;

	if (sweep)
	{
		// ensure delta theta < 0 or else add 360 degrees
		if (delta_theta < 0.0)
			delta_theta += 2.0 * M_PI;
	}
	else
	{
		// ensure delta theta > 0 or else substract 360 degrees
		if (delta_theta > 0.0)
		    delta_theta -= 2.0 * M_PI;
	}

	// add several cubic bezier to approximate the arc
	// (smaller than 90 degrees)
	// we add one extra segment because we want something
	// smaller than 90deg (i.e. not 90 itself)
	int segments = (int) (fabs(delta_theta / M_PI_2)) + 1;
	double delta = delta_theta / segments;

	// http://www.stillhq.com/ctpfaq/2001/comp.text.pdf-faq-2001-04.txt (section 2.13)
	double bcp = 4.0 / 3 * (1 - cos(delta / 2)) / sin(delta / 2);

	double cos_phi_rx = cos_phi * rx;
	double cos_phi_ry = cos_phi * ry;
	double sin_phi_rx = sin_phi * rx;
	double sin_phi_ry = sin_phi * ry;

	double cos_theta1 = cos(theta1);
	double sin_theta1 = sin(theta1);

	int i;
	for (i = 0; i < segments; ++i)
	{
		// end angle (for this segment) = current + delta
		double theta2 = theta1 + delta;
		double cos_theta2 = cos(theta2);
		double sin_theta2 = sin(theta2);

		// first control point (based on start point sx,sy)
		double c1x = sx - bcp * (cos_phi_rx * sin_theta1 + sin_phi_ry * cos_theta1);
		double c1y = sy + bcp * (cos_phi_ry * cos_theta1 - sin_phi_rx * sin_theta1);

		// end point (for this segment)
		double ex = cx + (cos_phi_rx * cos_theta2 - sin_phi_ry * sin_theta2);
		double ey = cy + (sin_phi_rx * cos_theta2 + cos_phi_ry * sin_theta2);

		// second control point (based on end point ex,ey)
		double c2x = ex + bcp * (cos_phi_rx * sin_theta2 + sin_phi_ry * cos_theta2);
		double c2y = ey + bcp * (sin_phi_rx * sin_theta2 - cos_phi_ry * cos_theta2);

		enesim_curve_cubic_to(state, c1x, c1y, c2x, c2y, ex, ey);

		// next start point is the current end point (same for angle)
		sx = ex;
		sy = ey;
		theta1 = theta2;
		// avoid recomputations
		cos_theta1 = cos_theta2;
		sin_theta1 = sin_theta2;
	}
}
