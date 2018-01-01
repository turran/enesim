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
#ifndef ENESIM_COORD_PRIVATE_H_
#define ENESIM_COORD_PRIVATE_H_

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

static inline Eina_F16p16 enesim_coord_repeat(Eina_F16p16 c, Eina_F16p16 len)
{
	if (c < 0 || c >= len)
	{
		c = c % len;
		if (c < 0)
			c += len;
	}
	return c;
}

static inline Eina_F16p16 enesim_coord_reflect(Eina_F16p16 c, Eina_F16p16 len)
{
	Eina_F16p16 len2;

	len2 = eina_f16p16_mul(len, eina_f16p16_int_from(2));
	c = c % len2;
	if (c < 0) c += len2;
	if (c >= len) c = len2 - c - 1;

	return c;
}

static inline Eina_F16p16 enesim_coord_pad(Eina_F16p16 c, Eina_F16p16 len)
{
	if (c < 0)
		c = 0;
	else if (c >= len)
		c = len - 1;
	return c;
}

void enesim_coord_identity_setup(Eina_F16p16 *fpx, Eina_F16p16 *fpy,
		int x, int y, double pre_x, double pre_y);
void enesim_coord_affine_setup(Eina_F16p16 *fpx, Eina_F16p16 *fpy,
		int x, int y, double pre_x, double pre_y, 
		const Enesim_Matrix_F16p16 *matrix);
void enesim_coord_projective_setup(Eina_F16p16 *fpx, Eina_F16p16 *fpy,
		Eina_F16p16 *fpz, int x, int y, double pre_x, double pre_y,
		const Enesim_Matrix_F16p16 *matrix);

uint32_t enesim_coord_sample_good_restrict(uint32_t *data, size_t stride, int sw,
		int sh, Eina_F16p16 xx, Eina_F16p16 yy);
uint32_t enesim_coord_sample_good_repeat(uint32_t *data, size_t stride, int sw,
		int sh, Eina_F16p16 xx, Eina_F16p16 yy);
uint32_t enesim_coord_sample_good_reflect(uint32_t *data, size_t stride, int sw,
		int sh, Eina_F16p16 xx, Eina_F16p16 yy);
uint32_t enesim_coord_sample_fast(uint32_t *data, size_t stride, int sw,
		int sh, Eina_F16p16 xx, Eina_F16p16 yy);

#endif
