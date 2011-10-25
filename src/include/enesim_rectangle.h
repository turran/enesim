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
#ifndef ENESIM_RECTANGLE_H_
#define ENESIM_RECTANGLE_H_
/**
 * @defgroup Enesim_Rectangle_Group Rectangle
 * @{
 * @todo
 */

#define ENESIM_RECTANGLE_FORMAT "g %g - %gx%g"
#define ENESIM_RECTANGLE_ARGS(r) (r)->x, (r)->y, (r)->w, (r)->h

typedef struct _Enesim_Rectangle
{
	double x;
	double y;
	double w;
	double h;
} Enesim_Rectangle;

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

/**
 * @}
 */
#endif
