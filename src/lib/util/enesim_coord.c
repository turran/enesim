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

#include "enesim_eina.h"

#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_coord_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_coord_identity_setup(Eina_F16p16 *fpx, Eina_F16p16 *fpy,
		int x, int y, double pre_x, double pre_y)
{
	Eina_F16p16 ox, oy;

	ox = eina_f16p16_double_from(pre_x);
	oy = eina_f16p16_double_from(pre_y);

	*fpx = eina_f16p16_int_from(x);
	*fpy = eina_f16p16_int_from(y);

	*fpx = eina_f16p16_sub(*fpx, ox);
	*fpy = eina_f16p16_sub(*fpy, oy);
}

/*
 * x' = (xx * x) + (xy * y) + xz;
 * y' = (yx * x) + (yy * y) + yz;
 */
void enesim_coord_affine_setup(Eina_F16p16 *fpx, Eina_F16p16 *fpy,
		int x, int y, double pre_x, double pre_y,
		const Enesim_F16p16_Matrix *matrix)
{
	Eina_F16p16 xx, yy;
	Eina_F16p16 ox, oy;

	ox = eina_f16p16_double_from(pre_x);
	oy = eina_f16p16_double_from(pre_y);

	xx = eina_f16p16_int_from(x);
	yy = eina_f16p16_int_from(y);

	*fpx = enesim_point_f16p16_transform(xx, yy, matrix->xx,
			matrix->xy, matrix->xz);
	*fpy = enesim_point_f16p16_transform(xx, yy, matrix->yx,
			matrix->yy, matrix->yz);

	*fpx = eina_f16p16_sub(*fpx, ox);
	*fpy = eina_f16p16_sub(*fpy, oy);
}

/*
 * x' = (xx * x) + (xy * y) + xz;
 * y' = (yx * x) + (yy * y) + yz;
 * z' = (zx * x) + (zy * y) + zz;
 */
void enesim_coord_projective_setup(Eina_F16p16 *fpx, Eina_F16p16 *fpy,
		Eina_F16p16 *fpz, int x, int y, double pre_x, double pre_y,
		const Enesim_F16p16_Matrix *matrix)
{
	Eina_F16p16 xx, yy;
	Eina_F16p16 ox, oy;

	ox = eina_f16p16_double_from(pre_x);
	oy = eina_f16p16_double_from(pre_y);

	xx = eina_f16p16_int_from(x);
	yy = eina_f16p16_int_from(y);
	*fpx = enesim_point_f16p16_transform(xx, yy, matrix->xx,
			matrix->xy, matrix->xz);
	*fpy = enesim_point_f16p16_transform(xx, yy, matrix->yx,
			matrix->yy, matrix->yz);
	*fpz = enesim_point_f16p16_transform(xx, yy, matrix->zx,
			matrix->zy, matrix->zz);

	*fpx = eina_f16p16_sub(*fpx, ox);
	*fpy = eina_f16p16_sub(*fpy, oy);
}
