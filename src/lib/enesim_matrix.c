/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2010 Jorge Luis Zapata
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

#include "enesim_eina.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"

#include "enesim_matrix_private.h"
/* TODO add a fixed point matrix too, to speed up the matrix_rotate sin/cos */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/* FIXME make this function on API */
static inline void _quad_dump(Enesim_Quad *q)
{
	printf("Q = %f %f, %f %f, %f %f, %f %f\n", QUAD_X0(q), QUAD_Y0(q), QUAD_X1(q), QUAD_Y1(q), QUAD_X2(q), QUAD_Y2(q), QUAD_X3(q), QUAD_Y3(q));
}

/*
 * In the range [-pi pi]
 * (4/pi)*x - ((4/(pi*pi))*x*abs(x))
 * http://www.devmaster.net/forums/showthread.php?t=5784
 */
#define EXTRA_PRECISION
static double _sin(double x)
{
	const double B = 4/M_PI;
	const double C = -4/(M_PI*M_PI);

	double y = (B * x) + (C * x * fabsf(x));

#ifdef EXTRA_PRECISION
	//  const float Q = 0.775;
	const double P = 0.225;

	y = P * (y * fabsf(y) - y) + y; // Q * y + P * y * abs(y)
#endif
	return y;
}

static double _cos(double x)
{
	x += M_PI_2;

	if (x > M_PI)   // Original x > pi/2
	{
	    x -= 2 * M_PI;   // Wrap: cos(x) = cos(x - 2 pi)
	}

	return _sin(x);
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Return the type of the given floating point matrix.
 *
 * @param m The floating point matrix.
 * @return The type of the matrix.
 *
 * This function returns the type of the matrix @p m. No check is done
 * on @p m.
 */
EAPI Enesim_Matrix_Type enesim_matrix_type_get(const Enesim_Matrix *m)
{
	if ((MATRIX_ZX(m) != 0) || (MATRIX_ZY(m) != 0) || (MATRIX_ZZ(m) != 1))
		return ENESIM_MATRIX_PROJECTIVE;
	else
	{
		if ((MATRIX_XX(m) == 1) && (MATRIX_XY(m) == 0) && (MATRIX_XZ(m) == 0) &&
				(MATRIX_YX(m) == 0) && (MATRIX_YY(m) == 1) && (MATRIX_YZ(m) == 0))
			return ENESIM_MATRIX_IDENTITY;
		else
			return ENESIM_MATRIX_AFFINE;
	}
}

/**
 * @brief Return the type of the given fixed point matrix.
 *
 * @param m The fixed point matrix.
 * @return The type of the matrix.
 *
 * This function returns the type of the matrix @p m. No check is done
 * on @p m.
 */
EAPI Enesim_Matrix_Type enesim_f16p16_matrix_type_get(const Enesim_F16p16_Matrix *m)
{
	if ((MATRIX_ZX(m) != 0) || (MATRIX_ZY(m) != 0) || (MATRIX_ZZ(m) != 65536))
		return ENESIM_MATRIX_PROJECTIVE;
	else
	{
		if ((MATRIX_XX(m) == 65536) && (MATRIX_XY(m) == 0) && (MATRIX_XZ(m) == 0) &&
				(MATRIX_YX(m) == 0) && (MATRIX_YY(m) == 65536) && (MATRIX_YZ(m) == 0))
			return ENESIM_MATRIX_IDENTITY;
		else
			return ENESIM_MATRIX_AFFINE;
	}
}

/**
 * @brief Set the values of the coefficients of the given floating
 * point matrix.
 *
 * @param m The floating point matrix.
 * @param a The first coefficient value.
 * @param b The second coefficient value.
 * @param c The third coefficient value.
 * @param d The fourth coefficient value.
 * @param e The fifth coefficient value.
 * @param f The sixth coefficient value.
 * @param g The seventh coefficient value.
 * @param h The heighth coefficient value.
 * @param i The nineth coefficient value.
 *
 * This function sets the values of the coefficients of the matrix
 * @p m. No check is done on @p m.
 *
 * @see enesim_matrix_values_get()
 */
EAPI void enesim_matrix_values_set(Enesim_Matrix *m, double a, double b, double c,
		double d, double e, double f, double g, double h, double i)
{
	MATRIX_XX(m) = a;
	MATRIX_XY(m) = b;
	MATRIX_XZ(m) = c;
	MATRIX_YX(m) = d;
	MATRIX_YY(m) = e;
	MATRIX_YZ(m) = f;
	MATRIX_ZX(m) = g;
	MATRIX_ZY(m) = h;
	MATRIX_ZZ(m) = i;
}

/**
 * @brief Get the values of the coefficients of the given floating
 * point matrix.
 *
 * @param m The floating point matrix.
 * @param a The first coefficient value.
 * @param b The second coefficient value.
 * @param c The third coefficient value.
 * @param d The fourth coefficient value.
 * @param e The fifth coefficient value.
 * @param f The sixth coefficient value.
 * @param g The seventh coefficient value.
 * @param h The heighth coefficient value.
 * @param i The nineth coefficient value.
 *
 * This function gets the values of the coefficients of the matrix
 * @p m. No check is done on @p m.
 *
 * @see enesim_matrix_values_set()
 */
EAPI void enesim_matrix_values_get(const Enesim_Matrix *m, double *a, double *b, double *c,
		double *d, double *e, double *f, double *g, double *h, double *i)
{
	if (a) *a = MATRIX_XX(m);
	if (b) *b = MATRIX_XY(m);
	if (c) *c = MATRIX_XZ(m);
	if (d) *d = MATRIX_YX(m);
	if (e) *e = MATRIX_YY(m);
	if (f) *f = MATRIX_YZ(m);
	if (g) *g = MATRIX_ZX(m);
	if (h) *h = MATRIX_ZY(m);
	if (i) *i = MATRIX_ZZ(m);
}

/**
 * @brief Get the values of the coefficients of the given fixed
 * point matrix.
 *
 * @param m The fixed point matrix.
 * @param a The first coefficient value.
 * @param b The second coefficient value.
 * @param c The third coefficient value.
 * @param d The fourth coefficient value.
 * @param e The fifth coefficient value.
 * @param f The sixth coefficient value.
 * @param g The seventh coefficient value.
 * @param h The heighth coefficient value.
 * @param i The nineth coefficient value.
 *
 * This function gets the values of the coefficients of the matrix
 * @p m. No check is done on @p m.
 *
 * @see enesim_matrix_values_set()
 */
EAPI void enesim_matrix_fixed_values_get(const Enesim_Matrix *m, Eina_F16p16 *a,
		Eina_F16p16 *b, Eina_F16p16 *c, Eina_F16p16 *d, Eina_F16p16 *e,
		Eina_F16p16 *f, Eina_F16p16 *g, Eina_F16p16 *h, Eina_F16p16 *i)
{
	if (a) *a = eina_f16p16_double_from(MATRIX_XX(m));
	if (b) *b = eina_f16p16_double_from(MATRIX_XY(m));
	if (c) *c = eina_f16p16_double_from(MATRIX_XZ(m));
	if (d) *d = eina_f16p16_double_from(MATRIX_YX(m));
	if (e) *e = eina_f16p16_double_from(MATRIX_YY(m));
	if (f) *f = eina_f16p16_double_from(MATRIX_YZ(m));
	if (g) *g = eina_f16p16_double_from(MATRIX_ZX(m));
	if (h) *h = eina_f16p16_double_from(MATRIX_ZY(m));
	if (i) *i = eina_f16p16_double_from(MATRIX_ZZ(m));
}

/**
 * @brief Transform the given floating point matrix to the given fixed
 * point matrix.
 *
 * @param m The floating point matrix.
 * @param fm The fixed point matrix.
 *
 * This function transforms the floating point matrix @p m to a fixed
 * point matrix and store the coefficients into the fixed point matrix
 * @p fm.
 */
EAPI void enesim_matrix_f16p16_matrix_to(const Enesim_Matrix *m,
		Enesim_F16p16_Matrix *fm)
{
	enesim_matrix_fixed_values_get(m, &fm->xx, &fm->xy, &fm->xz,
			&fm->yx, &fm->yy, &fm->yz,
			&fm->zx, &fm->zy, &fm->zz);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_matrix_point_transform(const Enesim_Matrix *m, double x, double y, double *xr, double *yr)
{
	double xrr, yrr;

	if (!MATRIX_ZX(m) && !MATRIX_ZY(m))
	{
		xrr = (x * MATRIX_XX(m) + y * MATRIX_XY(m) + MATRIX_XZ(m));
		yrr = (x * MATRIX_YX(m) + y * MATRIX_YY(m) + MATRIX_YZ(m));
	}
	else
	{
		xrr = (x * MATRIX_XX(m) + y * MATRIX_XY(m) + MATRIX_XZ(m)) /
			(x * MATRIX_ZX(m) + y * MATRIX_ZY(m) + MATRIX_ZZ(m));
		yrr = (x * MATRIX_YX(m) + y * MATRIX_YY(m) + MATRIX_YZ(m)) /
			(x * MATRIX_ZX(m) + y * MATRIX_ZY(m) + MATRIX_ZZ(m));
	}

	if (xr) *xr = xrr;
	if (yr) *yr = yrr;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_matrix_eina_rectangle_transform(const Enesim_Matrix *m, const Eina_Rectangle *r, const Enesim_Quad *q)
{
	enesim_matrix_point_transform(m, r->x, r->y, &((Enesim_Quad *)q)->x0, &((Enesim_Quad *)q)->y0);
	enesim_matrix_point_transform(m, r->x + r->w, r->y, &((Enesim_Quad *)q)->x1, &((Enesim_Quad *)q)->y1);
	enesim_matrix_point_transform(m, r->x + r->w, r->y + r->h, &((Enesim_Quad *)q)->x2, &((Enesim_Quad *)q)->y2);
	enesim_matrix_point_transform(m, r->x, r->y + r->h, &((Enesim_Quad *)q)->x3, &((Enesim_Quad *)q)->y3);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_matrix_rectangle_transform(const Enesim_Matrix *m, const Enesim_Rectangle *r, const Enesim_Quad *q)
{
	enesim_matrix_point_transform(m, r->x, r->y, &((Enesim_Quad *)q)->x0, &((Enesim_Quad *)q)->y0);
	enesim_matrix_point_transform(m, r->x + r->w, r->y, &((Enesim_Quad *)q)->x1, &((Enesim_Quad *)q)->y1);
	enesim_matrix_point_transform(m, r->x + r->w, r->y + r->h, &((Enesim_Quad *)q)->x2, &((Enesim_Quad *)q)->y2);
	enesim_matrix_point_transform(m, r->x, r->y + r->h, &((Enesim_Quad *)q)->x3, &((Enesim_Quad *)q)->y3);
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_matrix_adjoint(const Enesim_Matrix *m, Enesim_Matrix *a)
{
	double a11, a12, a13, a21, a22, a23, a31, a32, a33;

	/* cofactor */
	a11 = (MATRIX_YY(m) * MATRIX_ZZ(m)) - (MATRIX_YZ(m) * MATRIX_ZY(m));
	a12 = -1 * ((MATRIX_YX(m) * MATRIX_ZZ(m)) - (MATRIX_YZ(m) * MATRIX_ZX(m)));
	a13 = (MATRIX_YX(m) * MATRIX_ZY(m)) - (MATRIX_YY(m) * MATRIX_ZX(m));

	a21 = -1 * ((MATRIX_XY(m) * MATRIX_ZZ(m)) - (MATRIX_XZ(m) * MATRIX_ZY(m)));
	a22 = (MATRIX_XX(m) * MATRIX_ZZ(m)) - (MATRIX_XZ(m) * MATRIX_ZX(m));
	a23 = -1 * ((MATRIX_XX(m) * MATRIX_ZY(m)) - (MATRIX_XY(m) * MATRIX_ZX(m)));

	a31 = (MATRIX_XY(m) * MATRIX_YZ(m)) - (MATRIX_XZ(m) * MATRIX_YY(m));
	a32 = -1 * ((MATRIX_XX(m) * MATRIX_YZ(m)) - (MATRIX_XZ(m) * MATRIX_YX(m)));
	a33 = (MATRIX_XX(m) * MATRIX_YY(m)) - (MATRIX_XY(m) * MATRIX_YX(m));
	/* transpose */
	MATRIX_XX(a) = a11;
	MATRIX_XY(a) = a21;
	MATRIX_XZ(a) = a31;

	MATRIX_YX(a) = a12;
	MATRIX_YY(a) = a22;
	MATRIX_YZ(a) = a32;

	MATRIX_ZX(a) = a13;
	MATRIX_ZY(a) = a23;
	MATRIX_ZZ(a) = a33;

}

/**
 * @brief Return the determinant of the given matrix.
 *
 * @param m The matrix.
 * @return The determinant.
 *
 * This function returns the determinant of the matrix @p m. No check
 * is done on @p m.
 */
EAPI double enesim_matrix_determinant(const Enesim_Matrix *m)
{
	double det;

	det = MATRIX_XX(m) * ((MATRIX_YY(m) * MATRIX_ZZ(m)) - (MATRIX_YZ(m) * MATRIX_ZY(m)));
	det -= MATRIX_XY(m) * ((MATRIX_YX(m) * MATRIX_ZZ(m)) - (MATRIX_YZ(m) * MATRIX_ZX(m)));
	det += MATRIX_XZ(m) * ((MATRIX_YX(m) * MATRIX_ZY(m)) - (MATRIX_YY(m) * MATRIX_ZX(m)));

	return det;
}

/**
 * @brief Divide the given matrix by the given scalar.
 *
 * @param m The matrix.
 * @param scalar The scalar number.
 *
 * This function divides the matrix @p m by @p scalar. No check
 * is done on @p m.
 */
EAPI void enesim_matrix_divide(Enesim_Matrix *m, double scalar)
{
	MATRIX_XX(m) /= scalar;
	MATRIX_XY(m) /= scalar;
	MATRIX_XZ(m) /= scalar;

	MATRIX_YX(m) /= scalar;
	MATRIX_YY(m) /= scalar;
	MATRIX_YZ(m) /= scalar;

	MATRIX_ZX(m) /= scalar;
	MATRIX_ZY(m) /= scalar;
	MATRIX_ZZ(m) /= scalar;
}

/**
 * @brief Compute the inverse of the given matrix.
 *
 * @param m The matrix to inverse.
 * @param m2 The inverse matrix.
 *
 * This function inverse the matrix @p m and stores the result in
 * @p m2. No check is done on @p m or @p m2. If @p m can not be
 * invertible, then @p m2 is set to the identity matrix.
 */
EAPI void enesim_matrix_inverse(const Enesim_Matrix *m, Enesim_Matrix *m2)
{
	double scalar;

	/* determinant */
	scalar = enesim_matrix_determinant(m);
	if (!scalar)
	{
		enesim_matrix_identity(m2);
		return;
	}
	/* do its adjoint */
	enesim_matrix_adjoint(m, m2);
	/* divide */
	enesim_matrix_divide(m2, scalar);
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_matrix_compose(const Enesim_Matrix *m1, const Enesim_Matrix *m2, Enesim_Matrix *dst)
{
	double a11, a12, a13, a21, a22, a23, a31, a32, a33;

	a11 = (MATRIX_XX(m1) * MATRIX_XX(m2)) + (MATRIX_XY(m1) * MATRIX_YX(m2)) + (MATRIX_XZ(m1) * MATRIX_ZX(m2));
	a12 = (MATRIX_XX(m1) * MATRIX_XY(m2)) + (MATRIX_XY(m1) * MATRIX_YY(m2)) + (MATRIX_XZ(m1) * MATRIX_ZY(m2));
	a13 = (MATRIX_XX(m1) * MATRIX_XZ(m2)) + (MATRIX_XY(m1) * MATRIX_YZ(m2)) + (MATRIX_XZ(m1) * MATRIX_ZZ(m2));

	a21 = (MATRIX_YX(m1) * MATRIX_XX(m2)) + (MATRIX_YY(m1) * MATRIX_YX(m2)) + (MATRIX_YZ(m1) * MATRIX_ZX(m2));
	a22 = (MATRIX_YX(m1) * MATRIX_XY(m2)) + (MATRIX_YY(m1) * MATRIX_YY(m2)) + (MATRIX_YZ(m1) * MATRIX_ZY(m2));
	a23 = (MATRIX_YX(m1) * MATRIX_XZ(m2)) + (MATRIX_YY(m1) * MATRIX_YZ(m2)) + (MATRIX_YZ(m1) * MATRIX_ZZ(m2));

	a31 = (MATRIX_ZX(m1) * MATRIX_XX(m2)) + (MATRIX_ZY(m1) * MATRIX_YX(m2)) + (MATRIX_ZZ(m1) * MATRIX_ZX(m2));
	a32 = (MATRIX_ZX(m1) * MATRIX_XY(m2)) + (MATRIX_ZY(m1) * MATRIX_YY(m2)) + (MATRIX_ZZ(m1) * MATRIX_ZY(m2));
	a33 = (MATRIX_ZX(m1) * MATRIX_XZ(m2)) + (MATRIX_ZY(m1) * MATRIX_YZ(m2)) + (MATRIX_ZZ(m1) * MATRIX_ZZ(m2));

	MATRIX_XX(dst) = a11;
	MATRIX_XY(dst) = a12;
	MATRIX_XZ(dst) = a13;
	MATRIX_YX(dst) = a21;
	MATRIX_YY(dst) = a22;
	MATRIX_YZ(dst) = a23;
	MATRIX_ZX(dst) = a31;
	MATRIX_ZY(dst) = a32;
	MATRIX_ZZ(dst) = a33;
}

/**
 * @brief Check whether the two given matrices are equal or not.
 *
 * @param m1 The first matrix.
 * @param m2 The second matrix.
 * @return EINA_TRUE if the two matrices are equal, 0 otherwise.
 *
 * This function return EINA_TRUE if thematrices @p m1 and @p m2 are
 * equal, EINA_FALSE otherwise. No check is done on the matrices.
 */
EAPI Eina_Bool enesim_matrix_is_equal(const Enesim_Matrix *m1, const Enesim_Matrix *m2)
{
	if (m1->xx != m2->xx ||
			m1->xy != m2->xy ||
			m1->xz != m2->xz ||
			m1->yx != m2->yx ||
			m1->yy != m2->yy ||
			m1->yz != m2->yz ||
			m1->zx != m2->zx ||
			m1->zy != m2->zy ||
			m1->zz != m2->zz)
		return EINA_FALSE;
	return EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_f16p16_matrix_compose(const Enesim_F16p16_Matrix *m1,
		const Enesim_F16p16_Matrix *m2, Enesim_F16p16_Matrix *dst)
{
	Eina_F16p16 a11, a12, a13, a21, a22, a23, a31, a32, a33;

	a11 = eina_f16p16_mul(MATRIX_XX(m1), MATRIX_XX(m2)) +
			eina_f16p16_mul(MATRIX_XY(m1), MATRIX_YX(m2)) +
			eina_f16p16_mul(MATRIX_XZ(m1), MATRIX_ZX(m2));
	a12 = eina_f16p16_mul(MATRIX_XX(m1), MATRIX_XY(m2)) +
			eina_f16p16_mul(MATRIX_XY(m1), MATRIX_YY(m2)) +
			eina_f16p16_mul(MATRIX_XZ(m1), MATRIX_ZY(m2));
	a13 = eina_f16p16_mul(MATRIX_XX(m1), MATRIX_XZ(m2)) +
			eina_f16p16_mul(MATRIX_XY(m1), MATRIX_YZ(m2)) +
			eina_f16p16_mul(MATRIX_XZ(m1), MATRIX_ZZ(m2));

	a21 = eina_f16p16_mul(MATRIX_YX(m1), MATRIX_XX(m2)) +
			eina_f16p16_mul(MATRIX_YY(m1), MATRIX_YX(m2)) +
			eina_f16p16_mul(MATRIX_YZ(m1), MATRIX_ZX(m2));
	a22 = eina_f16p16_mul(MATRIX_YX(m1), MATRIX_XY(m2)) +
			eina_f16p16_mul(MATRIX_YY(m1), MATRIX_YY(m2)) +
			eina_f16p16_mul(MATRIX_YZ(m1), MATRIX_ZY(m2));
	a23 = eina_f16p16_mul(MATRIX_YX(m1), MATRIX_XZ(m2)) +
			eina_f16p16_mul(MATRIX_YY(m1), MATRIX_YZ(m2)) +
			eina_f16p16_mul(MATRIX_YZ(m1), MATRIX_ZZ(m2));

	a31 = eina_f16p16_mul(MATRIX_ZX(m1), MATRIX_XX(m2)) +
			eina_f16p16_mul(MATRIX_ZY(m1), MATRIX_YX(m2)) +
			eina_f16p16_mul(MATRIX_ZZ(m1), MATRIX_ZX(m2));
	a32 = eina_f16p16_mul(MATRIX_ZX(m1), MATRIX_XY(m2)) +
			eina_f16p16_mul(MATRIX_ZY(m1), MATRIX_YY(m2)) +
			eina_f16p16_mul(MATRIX_ZZ(m1), MATRIX_ZY(m2));
	a33 = eina_f16p16_mul(MATRIX_ZX(m1), MATRIX_XZ(m2)) +
			eina_f16p16_mul(MATRIX_ZY(m1), MATRIX_YZ(m2)) +
			eina_f16p16_mul(MATRIX_ZZ(m1), MATRIX_ZZ(m2));

	MATRIX_XX(dst) = a11;
	MATRIX_XY(dst) = a12;
	MATRIX_XZ(dst) = a13;
	MATRIX_YX(dst) = a21;
	MATRIX_YY(dst) = a22;
	MATRIX_YZ(dst) = a23;
	MATRIX_ZX(dst) = a31;
	MATRIX_ZY(dst) = a32;
	MATRIX_ZZ(dst) = a33;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_matrix_translate(Enesim_Matrix *m, double tx, double ty)
{
	MATRIX_XX(m) = 1;
	MATRIX_XY(m) = 0;
	MATRIX_XZ(m) = tx;
	MATRIX_YX(m) = 0;
	MATRIX_YY(m) = 1;
	MATRIX_YZ(m) = ty;
	MATRIX_ZX(m) = 0;
	MATRIX_ZY(m) = 0;
	MATRIX_ZZ(m) = 1;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_matrix_scale(Enesim_Matrix *m, double sx, double sy)
{
	MATRIX_XX(m) = sx;
	MATRIX_XY(m) = 0;
	MATRIX_XZ(m) = 0;
	MATRIX_YX(m) = 0;
	MATRIX_YY(m) = sy;
	MATRIX_YZ(m) = 0;
	MATRIX_ZX(m) = 0;
	MATRIX_ZY(m) = 0;
	MATRIX_ZZ(m) = 1;
}
/**
 * FIXME fix this, is incredible slow.
 */
EAPI void enesim_matrix_rotate(Enesim_Matrix *m, double rad)
{
	double c, s;
#if 0
	c = cosf(rad);
	s = sinf(rad);
#else
	/* normalize the angle between -pi,pi */
	rad = fmod(rad + M_PI, 2 * M_PI) - M_PI;
	c = _cos(rad);
	s = _sin(rad);
#endif

	MATRIX_XX(m) = c;
	MATRIX_XY(m) = -s;
	MATRIX_XZ(m) = 0;
	MATRIX_YX(m) = s;
	MATRIX_YY(m) = c;
	MATRIX_YZ(m) = 0;
	MATRIX_ZX(m) = 0;
	MATRIX_ZY(m) = 0;
	MATRIX_ZZ(m) = 1;
}

/**
 * @brief Set the given floating point matrix to the identity matrix.
 *
 * @param m The floating point matrix to set
 *
 * This function sets @p m to the identity matrix. No check is done on
 * @p m.
 */
EAPI void enesim_matrix_identity(Enesim_Matrix *m)
{
	MATRIX_XX(m) = 1;
	MATRIX_XY(m) = 0;
	MATRIX_XZ(m) = 0;
	MATRIX_YX(m) = 0;
	MATRIX_YY(m) = 1;
	MATRIX_YZ(m) = 0;
	MATRIX_ZX(m) = 0;
	MATRIX_ZY(m) = 0;
	MATRIX_ZZ(m) = 1;
}

/**
 * @brief Set the given fixed point matrix to the identity matrix.
 *
 * @param m The fixed point matrix to set
 *
 * This function sets @p m to the identity matrix. No check is done on
 * @p m.
 */
EAPI void enesim_f16p16_matrix_identity(Enesim_F16p16_Matrix *m)
{
	MATRIX_XX(m) = 65536;
	MATRIX_XY(m) = 0;
	MATRIX_XZ(m) = 0;
	MATRIX_YX(m) = 0;
	MATRIX_YY(m) = 65536;
	MATRIX_YZ(m) = 0;
	MATRIX_ZX(m) = 0;
	MATRIX_ZY(m) = 0;
	MATRIX_ZZ(m) = 65536;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_quad_eina_rectangle_to(const Enesim_Quad *q,
		Eina_Rectangle *r)
{
	double xmin, ymin, xmax, ymax;
	/* FIXME this code is very ugly, for sure there must be a better
	 * implementation */
	xmin = QUAD_X0(q) < QUAD_X1(q) ? QUAD_X0(q) : QUAD_X1(q);
	xmin = xmin < QUAD_X2(q) ? xmin : QUAD_X2(q);
	xmin = xmin < QUAD_X3(q) ? xmin : QUAD_X3(q);

	ymin = QUAD_Y0(q) < QUAD_Y1(q) ? QUAD_Y0(q) : QUAD_Y1(q);
	ymin = ymin < QUAD_Y2(q) ? ymin : QUAD_Y2(q);
	ymin = ymin < QUAD_Y3(q) ? ymin : QUAD_Y3(q);

	xmax = QUAD_X0(q) > QUAD_X1(q) ? QUAD_X0(q) : QUAD_X1(q);
	xmax = xmax > QUAD_X2(q) ? xmax : QUAD_X2(q);
	xmax = xmax > QUAD_X3(q) ? xmax : QUAD_X3(q);

	ymax = QUAD_Y0(q) > QUAD_Y1(q) ? QUAD_Y0(q) : QUAD_Y1(q);
	ymax = ymax > QUAD_Y2(q) ? ymax : QUAD_Y2(q);
	ymax = ymax > QUAD_Y3(q) ? ymax : QUAD_Y3(q);

	r->x = lround(xmin);
	r->w = lround(xmax) - r->x;
	r->y = lround(ymin);
	r->h = lround(ymax) - r->y;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_quad_rectangle_to(const Enesim_Quad *q,
		Enesim_Rectangle *r)
{
	double xmin, ymin, xmax, ymax;
	/* FIXME this code is very ugly, for sure there must be a better
	 * implementation */
	xmin = QUAD_X0(q) < QUAD_X1(q) ? QUAD_X0(q) : QUAD_X1(q);
	xmin = xmin < QUAD_X2(q) ? xmin : QUAD_X2(q);
	xmin = xmin < QUAD_X3(q) ? xmin : QUAD_X3(q);

	ymin = QUAD_Y0(q) < QUAD_Y1(q) ? QUAD_Y0(q) : QUAD_Y1(q);
	ymin = ymin < QUAD_Y2(q) ? ymin : QUAD_Y2(q);
	ymin = ymin < QUAD_Y3(q) ? ymin : QUAD_Y3(q);

	xmax = QUAD_X0(q) > QUAD_X1(q) ? QUAD_X0(q) : QUAD_X1(q);
	xmax = xmax > QUAD_X2(q) ? xmax : QUAD_X2(q);
	xmax = xmax > QUAD_X3(q) ? xmax : QUAD_X3(q);

	ymax = QUAD_Y0(q) > QUAD_Y1(q) ? QUAD_Y0(q) : QUAD_Y1(q);
	ymax = ymax > QUAD_Y2(q) ? ymax : QUAD_Y2(q);
	ymax = ymax > QUAD_Y3(q) ? ymax : QUAD_Y3(q);

	r->x = xmin;
	r->w = xmax - r->x;
	r->y = ymin;
	r->h = ymax - r->y;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_quad_rectangle_from(Enesim_Quad *q,
		const Eina_Rectangle *r)
{
	QUAD_X0(q) = r->x;
	QUAD_Y0(q) = r->y;
	QUAD_X1(q) = r->x + r->w;
	QUAD_Y1(q) = r->y;
	QUAD_X2(q) = r->x + r->w;
	QUAD_Y2(q) = r->y + r->h;
	QUAD_X3(q) = r->x;
	QUAD_Y3(q) = r->y + r->h;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_quad_coords_get(const Enesim_Quad *q, double *qx0, double *qy0,
		double *qx1, double *qy1, double *qx2, double *qy2, double *qx3,
		double *qy3)
{
	if (qx0) *qx0 = q->x0;
	if (qy0) *qy0 = q->y0;
	if (qx1) *qx1 = q->x1;
	if (qy1) *qy1 = q->y1;
	if (qx2) *qx2 = q->x2;
	if (qy2) *qy2 = q->y2;
	if (qx3) *qx3 = q->x3;
	if (qy3) *qy3 = q->y3;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_quad_coords_set(Enesim_Quad *q, double qx0, double qy0,
		double qx1, double qy1, double qx2, double qy2, double qx3,
		double qy3)
{
	QUAD_X0(q) = qx0;
	QUAD_Y0(q) = qy0;
	QUAD_X1(q) = qx1;
	QUAD_Y1(q) = qy1;
	QUAD_X2(q) = qx2;
	QUAD_Y2(q) = qy2;
	QUAD_X3(q) = qx3;
	QUAD_Y3(q) = qy3;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Eina_Bool enesim_matrix_square_quad_to(Enesim_Matrix *m,
		const Enesim_Quad *q)
{
	double ex = QUAD_X0(q) - QUAD_X1(q) + QUAD_X2(q) - QUAD_X3(q); // x0 - x1 + x2 - x3
	double ey = QUAD_Y0(q) - QUAD_Y1(q) + QUAD_Y2(q) - QUAD_Y3(q); // y0 - y1 + y2 - y3

	/* paralellogram */
	if (!ex && !ey)
	{
		/* create the affine matrix */
		MATRIX_XX(m) = QUAD_X1(q) - QUAD_X0(q);
		MATRIX_XY(m) = QUAD_X2(q) - QUAD_X1(q);
		MATRIX_XZ(m) = QUAD_X0(q);

		MATRIX_YX(m) = QUAD_Y1(q) - QUAD_Y0(q);
		MATRIX_YY(m) = QUAD_Y2(q) - QUAD_Y1(q);
		MATRIX_YZ(m) = QUAD_Y0(q);

		MATRIX_ZX(m) = 0;
		MATRIX_ZY(m) = 0;
		MATRIX_ZZ(m) = 1;

		return EINA_TRUE;
	}
	else
	{
		double dx1 = QUAD_X1(q) - QUAD_X2(q); // x1 - x2
		double dx2 = QUAD_X3(q) - QUAD_X2(q); // x3 - x2
		double dy1 = QUAD_Y1(q) - QUAD_Y2(q); // y1 - y2
		double dy2 = QUAD_Y3(q) - QUAD_Y2(q); // y3 - y2
		double den = (dx1 * dy2) - (dx2 * dy1);

		if (!den)
			return EINA_FALSE;

		MATRIX_ZX(m) = ((ex * dy2) - (dx2 * ey)) / den;
		MATRIX_ZY(m) = ((dx1 * ey) - (ex * dy1)) / den;
		MATRIX_ZZ(m) = 1;
		MATRIX_XX(m) = QUAD_X1(q) - QUAD_X0(q) + (MATRIX_ZX(m) * QUAD_X1(q));
		MATRIX_XY(m) = QUAD_X3(q) - QUAD_X0(q) + (MATRIX_ZY(m) * QUAD_X3(q));
		MATRIX_XZ(m) = QUAD_X0(q);
		MATRIX_YX(m) = QUAD_Y1(q) - QUAD_Y0(q) + (MATRIX_ZX(m) * QUAD_Y1(q));
		MATRIX_YY(m) = QUAD_Y3(q) - QUAD_Y0(q) + (MATRIX_ZY(m) * QUAD_Y3(q));
		MATRIX_YZ(m) = QUAD_Y0(q);

		return EINA_TRUE;
	}
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Eina_Bool enesim_matrix_quad_square_to(Enesim_Matrix *m,
		const Enesim_Quad *q)
{
	Enesim_Matrix tmp;

	/* compute square to quad */
	if (!enesim_matrix_square_quad_to(&tmp, q))
		return EINA_FALSE;

	enesim_matrix_inverse(&tmp, m);
	/* make the projective matrix always have 1 on zz */
	if (MATRIX_ZZ(m) != 1)
	{
		enesim_matrix_divide(m, MATRIX_ZZ(m));
	}

	return EINA_TRUE;
}
/**
 * Creates a projective matrix that maps a quadrangle to a quadrangle
 */
EAPI Eina_Bool enesim_matrix_quad_quad_to(Enesim_Matrix *m,
		const Enesim_Quad *src, const Enesim_Quad *dst)
{
	Enesim_Matrix tmp;

	/* TODO check that both are actually quadrangles */
	if (!enesim_matrix_quad_square_to(m, src))
		return EINA_FALSE;
	if (!enesim_matrix_square_quad_to(&tmp, dst))
		return EINA_FALSE;
	enesim_matrix_compose(&tmp, m, m);

	return EINA_TRUE;
}
