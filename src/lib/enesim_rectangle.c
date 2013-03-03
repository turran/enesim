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

#include "enesim_rectangle.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

EAPI void enesim_rectangle_union(Enesim_Rectangle *rr1, Enesim_Rectangle *rr2,
		Enesim_Rectangle *dst)
{
	double l1, t1, r1, b1;
	double l2, t2, r2, b2;

	l1 = rr1->x;
	t1 = rr1->y;
	r1 = rr1->x + rr1->w;
	b1 = rr1->y + rr1->h;

	l2 = rr2->x;
	t2 = rr2->y;
	r2 = rr2->x + rr2->w;
	b2 = rr2->y + rr2->h;

	if (l1 > l2)
		dst->x = l2;
	else
		dst->x = l1;

	if (t1 > t2)
		dst->y = t2;
	else
		dst->y = t1;

	if (r1 > r2)
		dst->w = r1 - dst->x;
	else
		dst->w = r2 - dst->x;

	if (b1 > b2)
		dst->h = b1 - dst->y;
	else
		dst->h = b2 - dst->y;
}


