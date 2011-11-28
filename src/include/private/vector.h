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
#ifndef VECTOR_H_
#define VECTOR_H_

typedef struct _Enesim_F16p16_Point
{
	Eina_F16p16 x;
	Eina_F16p16 y;
} Enesim_F16p16_Point;

typedef struct _Enesim_F16p16_Line
{
	Eina_F16p16 a;
	Eina_F16p16 b;
	Eina_F16p16 c;
} Enesim_F16p16_Line;

/* p0 the initial point
 * vx the x direction
 * vy the y direction
 */
static inline void enesim_f16p16_line_f16p16_direction_from(Enesim_F16p16_Line *l,
		Enesim_F16p16_Point *p0, Eina_F16p16 vx, Eina_F16p16 vy)
{
	l->a = vy;
	l->b = -vx;
	l->c = eina_f16p16_mul(vx, p0->y) - eina_f16p16_mul(vy, p0->x);
}

static inline Eina_F16p16 enesim_f16p16_line_affine_setup(Enesim_F16p16_Line *l,
		Eina_F16p16 xx, Eina_F16p16 yy)
{
	Eina_F16p16 ret;

	ret = eina_f16p16_mul(l->a, xx) + eina_f16p16_mul(l->b, yy) + l->c;
	return ret;
}

#endif
