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
#include "enesim_quad.h"
#include "enesim_matrix.h"

#include "enesim_matrix_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_cramer_3x3(double x, double y, double z, Enesim_Matrix *m,
		double *a, double *ax, double *ay, double *az)
{
	Enesim_Matrix cof;

	enesim_matrix_cofactor(m, &cof);
	/* D0 = det(a1, a2, a3) */
	*a = (MATRIX_XX(m) * MATRIX_XX(&cof)) +
			(MATRIX_YX(m) * MATRIX_YX(&cof)) +
			(MATRIX_ZX(m) * MATRIX_ZX(&cof));
	/* D1 = det(y, a2, a3) */
	*ax = (x * MATRIX_XX(&cof)) + (y * MATRIX_YX(&cof)) +
			(z * MATRIX_ZX(&cof));
	/* D2 = det(a1, y, a3) */
	*ay = (x * MATRIX_XY(&cof)) + (y * MATRIX_YY(&cof)) +
			(z * MATRIX_ZY(&cof));
	/* D3 = det(a1, a2, y) */
	*az = (x * MATRIX_XZ(&cof)) + (y * MATRIX_YZ(&cof)) +
			(z * MATRIX_ZZ(&cof));

	/* xi = Di/D0 */
}
/** @endcond */