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
#ifndef ENESIM_MATRIX_H_
#define ENESIM_MATRIX_H_

#include "enesim_quad.h"

/**
 * @file
 * @ender_group{Enesim_Matrix_Type}
 * @ender_group{Enesim_Matrix_F16p16}
 * @ender_group{Enesim_Matrix}
 */

/**
 * @defgroup Enesim_Matrix_Type Matrices type
 * @ingroup Enesim_Basic
 * @brief Matrix types
 * @{
 */
typedef enum _Enesim_Matrix_Type
{
	ENESIM_MATRIX_TYPE_IDENTITY, /**< Identity matrix type */
	ENESIM_MATRIX_TYPE_AFFINE, /**< Affine matrix type */
	ENESIM_MATRIX_TYPE_PROJECTIVE, /**< Projective matrix type */
} Enesim_Matrix_Type;

/**< The total number of matrix types */
#define ENESIM_MATRIX_TYPE_LAST (ENESIM_MATRIX_TYPE_PROJECTIVE + 1)

/**
 * @}
 * @defgroup Enesim_Matrix_F16p16 Matrices in fixed point
 * @ingroup Enesim_Basic
 * @brief Fixed point matrices operations
 * @{
 */

/**
 * @ender_name{enesim.matrix_f16p16}
 * Fixed point matrix handler
 */
typedef struct _Enesim_Matrix_F16p16
{
	Eina_F16p16 xx; /**< xx in x' = (x * xx) + (y * xy) + xz */
	Eina_F16p16 xy; /**< xy in x' = (x * xx) + (y * xy) + xz */
	Eina_F16p16 xz; /**< xz in x' = (x * xx) + (y * xy) + xz */

	Eina_F16p16 yx; /**< yx in y' = (x * yx) + (y * yy) + yz */
	Eina_F16p16 yy; /**< yy in y' = (x * yx) + (y * yy) + yz */
	Eina_F16p16 yz; /**< yz in y' = (x * yx) + (y * yy) + yz */

	Eina_F16p16 zx; /**< zx in z' = (x * zx) + (y * zy) + zz */
	Eina_F16p16 zy; /**< zy in z' = (x * zx) + (y * zy) + zz */
	Eina_F16p16 zz; /**< zz in z' = (x * zx) + (y * zy) + zz */
} Enesim_Matrix_F16p16;


EAPI void enesim_matrix_f16p16_identity(Enesim_Matrix_F16p16 *m);
EAPI void enesim_matrix_f16p16_compose(const Enesim_Matrix_F16p16 *m1,
		const Enesim_Matrix_F16p16 *m2, Enesim_Matrix_F16p16 *dst);
EAPI Enesim_Matrix_Type enesim_matrix_f16p16_type_get(const Enesim_Matrix_F16p16 *m);


/**
 * @}
 * @defgroup Enesim_Matrix Matrices in floating point
 * @ingroup Enesim_Basic
 * @brief Matrix definition and operations
 * @{
 */

/** Helper macro for printf formatting */
#define ENESIM_MATRIX_FORMAT "f %f %f | %f %f %f | %f %f %f"
/** Helper macro for printf formatting arg */
#define ENESIM_MATRIX_ARGS(m) (m)->xx, (m)->xy, (m)->xz, 	\
		(m)->yx, (m)->yy, (m)->yz,			\
		(m)->zx, (m)->zy, (m)->zz


/**
 * Floating point matrix handler
 */
typedef struct _Enesim_Matrix
{
	double xx; /**< xx in x' = (x * xx) + (y * xy) + xz */
	double xy; /**< xy in x' = (x * xx) + (y * xy) + xz */
	double xz; /**< xz in x' = (x * xx) + (y * xy) + xz */

	double yx; /**< yx in y' = (x * yx) + (y * yy) + yz */
	double yy; /**< yy in y' = (x * yx) + (y * yy) + yz */
	double yz; /**< yz in y' = (x * yx) + (y * yy) + yz */

	double zx; /**< zx in z' = (x * zx) + (y * zy) + zz */
	double zy; /**< zy in z' = (x * zx) + (y * zy) + zz */
	double zz; /**< zz in z' = (x * zx) + (y * zy) + zz */
} Enesim_Matrix;

EAPI Enesim_Matrix_Type enesim_matrix_type_get(const Enesim_Matrix *m);
EAPI void enesim_matrix_values_set(Enesim_Matrix *m, double a, double b, double c,
		double d, double e, double f, double g, double h, double i);
EAPI void enesim_matrix_values_get(const Enesim_Matrix *m, double *a, double *b,
		double *c, double *d, double *e, double *f, double *g, double *h,
		double *i);
EAPI void enesim_matrix_fixed_values_get(const Enesim_Matrix *m, Eina_F16p16 *a,
		Eina_F16p16 *b, Eina_F16p16 *c, Eina_F16p16 *d, Eina_F16p16 *e,
		Eina_F16p16 *f, Eina_F16p16 *g, Eina_F16p16 *h, Eina_F16p16 *i);
EAPI void enesim_matrix_matrix_f16p16_to(const Enesim_Matrix *m,
		Enesim_Matrix_F16p16 *fm);

EAPI Eina_Bool enesim_matrix_is_equal(const Enesim_Matrix *m1, const Enesim_Matrix *m2);
EAPI void enesim_matrix_compose(const Enesim_Matrix *m1, const Enesim_Matrix *m2,
		Enesim_Matrix *dst);
EAPI void enesim_matrix_translate(Enesim_Matrix *t, double tx, double ty);
EAPI void enesim_matrix_scale(Enesim_Matrix *t, double sx, double sy);
EAPI void enesim_matrix_rotate(Enesim_Matrix *t, double rad);
EAPI void enesim_matrix_identity(Enesim_Matrix *t);

EAPI double enesim_matrix_determinant(const Enesim_Matrix *m);
EAPI void enesim_matrix_divide(Enesim_Matrix *m, double scalar);
EAPI void enesim_matrix_inverse(const Enesim_Matrix *m, Enesim_Matrix *m2);
EAPI void enesim_matrix_transpose(const Enesim_Matrix *m, Enesim_Matrix *a);
EAPI void enesim_matrix_cofactor(const Enesim_Matrix *m, Enesim_Matrix *a);
EAPI void enesim_matrix_adjoint(const Enesim_Matrix *m, Enesim_Matrix *a);

EAPI void enesim_matrix_point_transform(const Enesim_Matrix *m, double x, double y, double *xr, double *yr);
EAPI void enesim_matrix_eina_rectangle_transform(const Enesim_Matrix *m, const Eina_Rectangle *r, const Enesim_Quad *q);
EAPI void enesim_matrix_rectangle_transform(const Enesim_Matrix *m, const Enesim_Rectangle *r, const Enesim_Quad *q);

EAPI Eina_Bool enesim_matrix_quad_quad_map(Enesim_Matrix *m, const Enesim_Quad *src, const Enesim_Quad *dst);
EAPI Eina_Bool enesim_matrix_square_quad_map(Enesim_Matrix *m, const Enesim_Quad *q);
EAPI Eina_Bool enesim_matrix_quad_square_map(Enesim_Matrix *m, const Enesim_Quad *q);


/**
 * @}
 */
#endif /*ENESIM_MATRIX_H_*/
