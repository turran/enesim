/* LIBARGB - ARGB helper functions
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
#ifndef LIBARGB_ARGB8888_MUL4_SYM_H
#define LIBARGB_ARGB8888_MUL4_SYM_H

/*
 * [a1 r1 g1 b1], [a2 r2 g2 b2] => [a1*a2 r1*r2 g1*g2 b1*b2]
 */
static inline uint32_t argb8888_mul4_sym(uint32_t c1, uint32_t c2)
{
	return ( ((((((c1) >> 16) & 0xff00) * (((c2) >> 16) & 0xff00)) + 0xff0000) & 0xff000000) +
	   ((((((c1) >> 8) & 0xff00) * (((c2) >> 16) & 0xff)) + 0xff00) & 0xff0000) +
	   ((((((c1) & 0xff00) * ((c2) & 0xff00)) + 0xff00) >> 16) & 0xff00) +
	   (((((c1) & 0xff) * ((c2) & 0xff)) + 0xff) >> 8) );
}

/*============================================================================*
 *                             Point operations                               *
 *============================================================================*/
/*============================================================================*
 *                              Span operations                               *
 *============================================================================*/
static inline void argb8888_none_color_none_mul4_sym(uint32_t *d,
		unsigned int len, uint32_t *s EINA_UNUSED,
		uint32_t color, uint32_t *m EINA_UNUSED)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint32_t r;

		r = argb8888_mul4_sym(color, *d);
		*d++ = r;
	}
}

#endif
