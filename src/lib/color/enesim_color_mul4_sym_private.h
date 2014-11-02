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
#ifndef _ENESIM_COLOR_MUL4_SYM_PRIVATE_H
#define _ENESIM_COLOR_MUL4_SYM_PRIVATE_H

/*============================================================================*
 *                              Span operations                               *
 *============================================================================*/
static inline void enesim_color_mul4_sym_sp_none_color_none(uint32_t *d,
		unsigned int len, uint32_t color)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint32_t r;

		r = enesim_color_mul4_sym(color, *d);
		*d++ = r;
	}
}

#endif
