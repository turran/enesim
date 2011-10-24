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
#ifndef ENESIM_MATRIX_H_
#define ENESIM_MATRIX_H_
/**
 * @defgroup Enesim_Matrix_Group Matrix
 * @{
 * @todo
 * - Create all this Macros
 */
#define ENESIM_MATRIX_XX(m) m[0]

typedef enum
{
	ENESIM_MATRIX_IDENTITY,
	ENESIM_MATRIX_AFFINE,
	ENESIM_MATRIX_PROJECTIVE,
	ENESIM_MATRIX_TYPES
} Enesim_Matrix_Type;

typedef struct _Enesim_Matrix
{
	double xx, xy, xz;
	double yx, yy, yz;
	double zx, zy, zz;
} Enesim_Matrix; /**< Floating point matrix handler */

typedef struct _Enesim_F16p16_Matrix
{
	Eina_F16p16 xx, xy, xz;
	Eina_F16p16 yx, yy, yz;
	Eina_F16p16 zx, zy, zz;
} Enesim_F16p16_Matrix; /**< Fixed point matrix handler */

typedef struct _Enesim_Quad
{
	double x0, y0;
	double x1, y1;
	double x2, y2;
	double x3, y3;
} Enesim_Quad; /**< Quadrangle handler */

EAPI Enesim_Matrix_Type enesim_matrix_type_get(const Enesim_Matrix *m);
EAPI void enesim_matrix_values_set(Enesim_Matrix *m, double a, double b, double c,
		double d, double e, double f, double g, double h, double i);
EAPI void enesim_matrix_values_get(Enesim_Matrix *m, double *a, double *b,
		double *c, double *d, double *e, double *f, double *g, double *h,
		double *i);
EAPI void enesim_matrix_fixed_values_get(const Enesim_Matrix *m, Eina_F16p16 *a,
		Eina_F16p16 *b, Eina_F16p16 *c, Eina_F16p16 *d, Eina_F16p16 *e,
		Eina_F16p16 *f, Eina_F16p16 *g, Eina_F16p16 *h, Eina_F16p16 *i);
EAPI void enesim_matrix_f16p16_matrix_to(const Enesim_Matrix *m,
		Enesim_F16p16_Matrix *fm);

EAPI void enesim_matrix_compose(Enesim_Matrix *m1, Enesim_Matrix *m2,
		Enesim_Matrix *dst);
EAPI void enesim_matrix_translate(Enesim_Matrix *t, double tx, double ty);
EAPI void enesim_matrix_scale(Enesim_Matrix *t, double sx, double sy);
EAPI void enesim_matrix_rotate(Enesim_Matrix *t, double rad);
EAPI void enesim_matrix_identity(Enesim_Matrix *t);

EAPI double enesim_matrix_determinant(Enesim_Matrix *m);
EAPI void enesim_matrix_divide(Enesim_Matrix *m, double scalar);
EAPI void enesim_matrix_inverse(Enesim_Matrix *m, Enesim_Matrix *m2);
EAPI void enesim_matrix_adjoint(Enesim_Matrix *m, Enesim_Matrix *a);

EAPI void enesim_matrix_point_transform(Enesim_Matrix *m, double x, double y, double *xr, double *yr);
EAPI void enesim_matrix_eina_rectangle_transform(Enesim_Matrix *m, Eina_Rectangle *r, Enesim_Quad *q);
EAPI void enesim_matrix_rectangle_transform(Enesim_Matrix *m, Enesim_Rectangle *r, Enesim_Quad *q);

EAPI Eina_Bool enesim_matrix_quad_quad_to(Enesim_Matrix *m, Enesim_Quad *src, Enesim_Quad *dst);
EAPI Eina_Bool enesim_matrix_square_quad_to(Enesim_Matrix *m, Enesim_Quad *q);
EAPI Eina_Bool enesim_matrix_quad_square_to(Enesim_Matrix *m, Enesim_Quad *q);

EAPI void enesim_f16p16_matrix_compose(Enesim_F16p16_Matrix *m1,
		Enesim_F16p16_Matrix *m2, Enesim_F16p16_Matrix *dst);
EAPI Enesim_Matrix_Type enesim_f16p16_matrix_type_get(const Enesim_F16p16_Matrix *m);

EAPI void enesim_quad_rectangle_to(Enesim_Quad *q,
		Enesim_Rectangle *r);
EAPI void enesim_quad_eina_rectangle_to(Enesim_Quad *q,
		Eina_Rectangle *r);
EAPI void enesim_quad_rectangle_from(Enesim_Quad *q,
		Eina_Rectangle *r);
EAPI void enesim_quad_coords_set(Enesim_Quad *q, double x1, double y1, double x2,
		double y2, double x3, double y3, double x4, double y4);
EAPI void enesim_quad_coords_get(Enesim_Quad *q, double *x1, double *y1,
		double *x2, double *y2, double *x3, double *y3, double *x4,
		double *y4);
/**
 * @}
 */
#endif /*ENESIM_MATRIX_H_*/
