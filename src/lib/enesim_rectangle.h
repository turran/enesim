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
#ifndef ENESIM_RECTANGLE_H_
#define ENESIM_RECTANGLE_H_

#include <math.h>

/**
 * @file
 * @ender_group{Enesim_Rectangle}
 */

/**
 * @defgroup Enesim_Rectangle Rectangle
 * @ingroup Enesim_Basic
 * @brief Rectangle definition and operations
 * @{
 */

/** Helper macro for printf formatting */
#define ENESIM_RECTANGLE_FORMAT "g %g - %gx%g"
/** Helper macro for printf formatting arg */
#define ENESIM_RECTANGLE_ARGS(r) (r)->x, (r)->y, (r)->w, (r)->h

/** Floating point rectangle */
typedef struct _Enesim_Rectangle
{
	double x; /**< The top left X coordinate */
	double y; /**< The top left Y coordinate */
	double w; /**< The width */
	double h; /**< The height */
} Enesim_Rectangle;

static inline void enesim_rectangle_normalize(Enesim_Rectangle *r, Eina_Rectangle *dst)
{
	dst->x = floor(r->x);
	dst->y = floor(r->y);
	dst->w = ceil(r->x - dst->x + r->w);
	dst->h = ceil(r->y - dst->y + r->h);
}

static inline Eina_Bool enesim_rectangle_is_inside(Enesim_Rectangle *r, double x, double y)
{
	if ((x - r->x >= 0) && (y - r->y >= 0) && (r->x + r->w - x >= 0) && (r->y + r->h - y >= 0))
		return EINA_TRUE;
	else
		return EINA_FALSE;
}

static inline void enesim_rectangle_coords_from(Enesim_Rectangle *r, double x, double y, double w, double h)
{
	r->x = x;
	r->y = y;
	r->w = w;
	r->h = h;
}

EAPI void enesim_rectangle_union(Enesim_Rectangle *rr1, Enesim_Rectangle *rr2,
		Enesim_Rectangle *dst);

/**
 * @}
 */
#endif
