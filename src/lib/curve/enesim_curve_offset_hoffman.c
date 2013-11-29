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
#include "enesim_private.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_path.h"

#include "enesim_path_private.h"
#include "enesim_curve_private.h"
#include "enesim_matrix_private.h"
#include "enesim_cramer_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/* algorithm taken from http://www.fho-emden.de/~hoffmann/bezier18122002.pdf */
void enesim_curve_offset_hoffman(Enesim_Path_Cubic *c, Enesim_Path_Cubic *res,
		double offset, double err)
{
	Enesim_Matrix m;
	double s0x, s0y;
	double s3x, s3y;
	double ax, ay;
	double bx, by;
	double cx, cy;
	double Ax, Ay;
	double Bx, By;
	double Cx, Cy;
	double den;
	double dn;
	double n0x, n0y;
	double n3x, n3y;
	double q0x, q3x;
	double q0y, q3y;
	double dpx, dpy;
	double dqx, dqy;
	double pcx, pcy;
	double qcx, qcy;
	double ncx, ncy;
	double rcx, rcy;
	double y1, y2, y3;
	double De0, De1, De2, De3;
	double aDe0;
	double dk0, dk3;
	double k0, k3;
	int singular;

	s0x = c->ctrl_x0 - c->start_x;
	s0y = c->ctrl_y0 - c->start_y;
	s3x = c->end_x - c->ctrl_x1;
	s3y = c->end_y - c->ctrl_y1;

	ax = (3 * s0x) + (3 * s3x) - (2 * (c->end_x - c->start_x));
	ay = (3 * s0y) + (3 * s3y) - (2 * (c->end_y - c->start_y));
	bx = (-6 * s0x) + (-3 * s3x) + (3 * (c->end_x - c->start_x));
	by = (-6 * s0y) + (-3 * s3y) + (3 * (c->end_y - c->start_y));
	cx = 3 * s0x;
	cy = 3 * s0y;

	/* get the normals of the first and last point */
	den = hypot(s0x, s0y);
	n0x = -s0y / den;
	n0y = s0x / den;
	
	den = hypot(s3x, s3y);
	n3x = -s3y / den;
	n3y = s3x / den;

	/* shift in normal direction with offset offset */
	q0x = c->start_x + (offset * n0x);
	q0y = c->start_y + (offset * n0y);
	q3x = c->end_x + (offset * n3x);
	q3y = c->end_y + (offset * n3y);

	/* coefficients for curve Q */
	Ax = (3 * s0x) + (3 * s3x) - (2 * (q3x - q0x));
	Ay = (3 * s0y) + (3 * s3y) - (2 * (q3y - q0y));
	Bx = (-6 * s0x) + (-3 * s3x) + (3 * (q3x - q0x));
	By = (-6 * s0y) + (-3 * s3y) + (3 * (q3y - q0y));
	Cx = 3 * s0x;
	Cy = 3 * s0y;

	/* slopes at t = 0.5 */
	dpx = ax * 3/4 + bx + cx;
	dpy = ay * 3/4 + by + cy;
	dqx = Ax * 3/4 + Bx + Cx;
	dqy = Ay * 3/4 + By + Cy;

	/* midpoint at t = 0.5 */
	pcx = (ax * 0.125) + (bx * 0.250) + (cx * 0.5) + c->start_x;
	pcy = (ay * 0.125) + (by * 0.250) + (cy * 0.5) + c->start_y;

	qcx = (Ax * 0.125) + (Bx * 0.250) + (Cx * 0.5) + q0x;
	qcy = (Ay * 0.125) + (By * 0.250) + (Cy * 0.5) + q0y;

	/* the normal vectors */
	dn = hypot(dpx, dpy);
	ncx = -dpy/dn;
	ncy = dpx/dn;

	/* offset for midpoint (not exactly at t=0.5) */
	rcx = pcx + (offset * ncx);
	rcy = pcy + (offset * ncy);

	MATRIX_XX(&m) = s0x; MATRIX_XY(&m) = -s3x; MATRIX_XZ(&m) = (8/3.0) * dpx;
	MATRIX_YX(&m) = s0y; MATRIX_YY(&m) = -s3y; MATRIX_YZ(&m) = (8/3.0) * dpy;
	MATRIX_ZX(&m) = (s0x * ncx) + (s0y * ncy);
	MATRIX_ZY(&m) = (s3x * ncx) + (s3y * ncy);
	MATRIX_ZZ(&m) = 4 * (((s0x - s3x) * ncx) + ((s0y - s3y) * ncy));

	y1 = 8/3.0 * (rcx - qcx);
	y2 = 8/3.0 * (rcy - qcy);
	y3 = 4/3.0 * ((dqx * ncx) + (dqy * ncy));

	/* the first  method */
	enesim_cramer_3x3(y1, y2, y3, &m, &De0, &De1, &De2, &De3);
	/* xi = Di/D0 */
	singular = 0;
	aDe0 = fabs(De0);
	if (aDe0 <= 1.0)
	{
		/* any absolute division result should be less than max */
		if (fabs(De1) >= aDe0 * 0.9)
			 singular = 1;
		if (fabs(De2) >= aDe0 * 0.9)
			 singular = 1;
		if (fabs(De3) >= aDe0 * 0.45)
			 singular = 1;
	}
	/* A’A x = A’ y
	 * B x = z
	 */
	if (singular)
	{
		Enesim_Matrix mb;
		double z1, z2, z3;
		double w2 = err;

		MATRIX_XX(&mb) =
				(MATRIX_XX(&m) * MATRIX_XX(&m)) +
				(MATRIX_YX(&m) * MATRIX_YX(&m)) +
				(MATRIX_ZX(&m) * MATRIX_ZX(&m)) + w2;
		MATRIX_YY(&mb) =
				(MATRIX_XY(&m) * MATRIX_XY(&m)) +
				(MATRIX_YY(&m) * MATRIX_YY(&m)) +
				(MATRIX_ZY(&m) * MATRIX_ZY(&m)) + w2;
		MATRIX_ZZ(&mb) =
				(MATRIX_XZ(&m) * MATRIX_XZ(&m)) +
				(MATRIX_YZ(&m) * MATRIX_YZ(&m)) +
				(MATRIX_ZZ(&m) * MATRIX_ZZ(&m)) + w2;
		MATRIX_XY(&mb) =
				(MATRIX_XX(&m) * MATRIX_XY(&m)) +
				(MATRIX_YX(&m) * MATRIX_YY(&m)) +
				(MATRIX_ZX(&m) * MATRIX_ZY(&m));
		MATRIX_XZ(&mb) = 
				(MATRIX_XZ(&m) * MATRIX_XZ(&m)) +
				(MATRIX_YX(&m) * MATRIX_YZ(&m)) +
				(MATRIX_ZX(&m) * MATRIX_ZZ(&m));
		MATRIX_YZ(&mb) = 
				(MATRIX_XY(&m) * MATRIX_XZ(&m)) + 
				(MATRIX_YY(&m) * MATRIX_YZ(&m)) +
				(MATRIX_ZY(&m) * MATRIX_ZZ(&m));
		MATRIX_YX(&mb) = MATRIX_XY(&mb);
		MATRIX_ZX(&mb) = MATRIX_XZ(&mb);
		MATRIX_ZY(&mb) = MATRIX_YZ(&mb);

		z1 = (MATRIX_XX(&m) * y1) + (MATRIX_YX(&m) * y2) +
				(MATRIX_ZX(&m) * y3);
		z2 = (MATRIX_XY(&m) * y1) + (MATRIX_YY(&m) * y2) +
				(MATRIX_ZY(&m) * y3);
		z3 = (MATRIX_XZ(&m) * y1) + (MATRIX_YZ(&m) * y2) +
				(MATRIX_ZZ(&m) * y3);

		enesim_cramer_3x3(z1, z2, z3, &mb, &De0, &De1, &De2, &De3);
	}

	dk0 = De1 / De0;
	dk3 = De2 / De0;

	k0 = 1 + dk0;
	k3 = 1 + dk3;

	res->start_x = q0x;
	res->start_y = q0y;
	res->ctrl_x0 = q0x + (k0 * s0x);
	res->ctrl_y0 = q0y + (k0 * s0y);
	res->ctrl_x1 = q3x - (k3 * s3x);
	res->ctrl_y1 = q3y - (k3 * s3y);
	res->end_x = q3x;
	res->end_y = q3y;
}
