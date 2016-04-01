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

#include <float.h>

#include "enesim_vector_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*----------------------------------------------------------------------------*
 *                              F16p16 Vector                                 *
 *----------------------------------------------------------------------------*/
Eina_Bool enesim_f16p16_vector_setup(Enesim_F16p16_Vector *thiz,
		Enesim_Point *p0, Enesim_Point *p1, double tolerance)
{
	double x0, y0, x1, y1;
	double x01, y01;
	double len;

	x0 = p0->x;
	y0 = p0->y;
	x1 = p1->x;
	y1 = p1->y;

	x01 = x1 - x0;
	y01 = y1 - y0;
	len = hypot(x01, y01);
	if (len < tolerance)
		return EINA_FALSE;

	thiz->a = eina_f16p16_double_from(-y01 / len);
	thiz->b = eina_f16p16_double_from(x01 / len);
	thiz->c = eina_f16p16_double_from(((y1 * x0) - (x1 * y0)) / len);
	thiz->xx0 = eina_f16p16_double_from(x0);
	thiz->yy0 = eina_f16p16_double_from(y0);
	thiz->xx1 = eina_f16p16_double_from(x1);
	thiz->yy1 = eina_f16p16_double_from(y1);

	if ((thiz->yy0 == thiz->yy1) || (thiz->xx0 == thiz->xx1))
		thiz->sgn = 0;
	else
	{
		thiz->sgn = 1;
		if (thiz->yy1 > thiz->yy0)
		{
			if (thiz->xx1 < thiz->xx0)
				thiz->sgn = -1;
		}
		else
		{
			 if (thiz->xx1 > thiz->xx0)
				thiz->sgn = -1;
		}
	}

	if (thiz->xx0 > thiz->xx1)
	{
		Eina_F16p16 xx0 = thiz->xx0;
		thiz->xx0 = thiz->xx1;
		thiz->xx1 = xx0;
	}

	if (thiz->yy0 > thiz->yy1)
	{
		Eina_F16p16 yy0 = thiz->yy0;
		thiz->yy0 = thiz->yy1;
		thiz->yy1 = yy0;
	}
	return EINA_TRUE;
}
/*----------------------------------------------------------------------------*
 *                                 Points                                     *
 *----------------------------------------------------------------------------*/

Enesim_Point * enesim_point_new(void)
{
	Enesim_Point *thiz;

	thiz = calloc(1, sizeof(Enesim_Point));
	return thiz;
}

Enesim_Point * enesim_point_new_from_coords(double x, double y, double z)
{
	Enesim_Point *thiz;

	thiz = enesim_point_new();
	enesim_point_coords_set(thiz, x, y, z);

	return thiz;
}

/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
