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
#ifndef _COORD_H
#define _COORD_H

/* FIXME in order to reach 1.0 we need to rmeove every occurence
 * of eina_f16p16 and use our own implementation internally
 * still need to figure out how to handle exported values
 * like Enesim_Matrix_F16p16
 */

#define Enesim_Coord Eina_F16p16

#define enesim_coord_subpixel_get 0
#define enesim_coord_double_from
#define enesim_coord_double_to
#define enesim_coord_int_to
#define enesim_coord_int_from

/* Helper functions needed by other renderers */
static inline Eina_F16p16 enesim_point_f16p16_transform(Eina_F16p16 x, Eina_F16p16 y,
		Eina_F16p16 cx, Eina_F16p16 cy, Eina_F16p16 cz)
{
	return eina_f16p16_mul(cx, x) + eina_f16p16_mul(cy, y) + cz;
}

void enesim_coord_identity_setup(Eina_F16p16 *fpx, Eina_F16p16 *fpy,
		int x, int y, double pre_x, double pre_y);
void enesim_coord_affine_setup(Eina_F16p16 *fpx, Eina_F16p16 *fpy,
		int x, int y, double pre_x, double pre_y, 
		const Enesim_F16p16_Matrix *matrix);
void enesim_coord_projective_setup(Eina_F16p16 *fpx, Eina_F16p16 *fpy,
		Eina_F16p16 *fpz, int x, int y, double pre_x, double pre_y,
		const Enesim_F16p16_Matrix *matrix);

#endif
