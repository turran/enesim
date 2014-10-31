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
#ifndef _ENESIM_COLOR_PRIVATE_H
#define _ENESIM_COLOR_PRIVATE_H

static inline uint8_t enesim_color_alpha_get(uint32_t plane0)
{
	return (plane0 >> 24);
}

static inline uint8_t enesim_color_red_get(uint32_t plane0)
{
	return ((plane0 >> 16) & 0xff);
}

static inline uint8_t enesim_color_green_get(uint32_t plane0)
{
	return ((plane0 >> 8) & 0xff);
}

static inline uint8_t enesim_color_blue_get(uint32_t plane0)
{
	return (plane0 & 0xff);
}

static inline void enesim_color_from_components(uint32_t *plane0, uint8_t a, uint8_t r,
		uint8_t g, uint8_t b)
{
	*plane0 = (a << 24) | (r << 16) | (g << 8) | b;
}

static inline void enesim_color_to_components(uint32_t plane0, uint8_t *a, uint8_t *r,
		uint8_t *g, uint8_t *b)
{
	if (a) *a = enesim_color_alpha_get(plane0);
	if (r) *r = enesim_color_red_get(plane0);
	if (g) *g = enesim_color_green_get(plane0);
	if (b) *b = enesim_color_blue_get(plane0);
}

/*
 * r = (c0 - c1) * a + c1
 * a = 0 => r = c1
 * a = 1 => r = c0
 */
static inline uint32_t enesim_color_interp_256(uint16_t a, uint32_t c0, uint32_t c1)
{
	return ( (((((((c0) >> 8) & 0xff00ff) - (((c1) >> 8) & 0xff00ff)) * (a))
			+ ((c1) & 0xff00ff00)) & 0xff00ff00) +
			(((((((c0) & 0xff00ff) - ((c1) & 0xff00ff)) * (a)) >> 8)
			+ ((c1) & 0xff00ff)) & 0xff00ff) );
}

/*
 * ret = (c0 - c1) * a + c1
 * a = 0 => r = c1
 * a = 1 => r = c0
 */
static inline uint32_t enesim_color_interp_65536(uint32_t a, uint32_t c0, uint32_t c1)
{
	return ( ((((((c0 >> 16) & 0xff00) - ((c1 >> 16) & 0xff00)) * a) +
		(c1 & 0xff000000)) & 0xff000000) +
		((((((c0 >> 16) & 0xff) - ((c1 >> 16) & 0xff)) * a) +
		(c1 & 0xff0000)) & 0xff0000) +
		((((((c0 & 0xff00) - (c1 & 0xff00)) * a) >> 16) +
		(c1 & 0xff00)) & 0xff00) +
		((((((c0 & 0xff) - (c1 & 0xff)) * a) >> 16) +
		(c1 & 0xff)) & 0xff) );
}

/*
 * ret = [(c.a * a), (c.r * a), (c.g * a), (c.b * a)]
 * Where a is on the 1 - 256 range
 */
static inline uint32_t enesim_color_mul_256(uint16_t a, uint32_t c)
{
	return  ( (((((c) >> 8) & 0x00ff00ff) * (a)) & 0xff00ff00) +
	(((((c) & 0x00ff00ff) * (a)) >> 8) & 0x00ff00ff) );
}

/*
 * ret = [(c.a * a), (c.r * a), (c.g * a), (c.b * a)]
 * Where a is on the 1 - 65536 range
 */
static inline uint32_t enesim_color_mul_65536(uint32_t a, uint32_t c)
{
	return ( ((((c >> 16) & 0xff00) * a) & 0xff000000) +
		((((c >> 16) & 0xff) * a) & 0xff0000) +
		((((c & 0xff00) * a) >> 16) & 0xff00) +
		((((c & 0xff) * a) >> 16) & 0xff) );
}

/*
 *
 */
static inline uint32_t enesim_color_mul_sym(uint16_t a, uint32_t c)
{
	return ( (((((c) >> 8) & 0x00ff00ff) * (a) + 0xff00ff) & 0xff00ff00) +
	   (((((c) & 0x00ff00ff) * (a) + 0xff00ff) >> 8) & 0x00ff00ff) );
}

static inline uint32_t * enesim_color_at(uint32_t *data, size_t stride, int x, int y)
{
	return (uint32_t *)((uint8_t *)data + (stride * y)) + x;
}

/*
 * Get the luminance based on the function
 * ret = (c.r * 0.2125) + (c.g * 0.7154) + (c.b * 0.0721)
 */
static inline uint8_t enesim_color_lum_get(uint32_t c)
{
	uint16_t a;

	a = ((55 * (c & 0xff0000)) >> 24) + ((184 * (c & 0xff00)) >> 16) +
			((19 * (c & 0xff)) >> 8);
	return a;
}

static inline void enesim_color_blend(uint32_t *dplane0, uint16_t a, uint32_t splane0)
{
	*dplane0 = splane0 + enesim_color_mul_256(a, *dplane0);
}

static inline void enesim_color_fill(uint32_t *dplane0, uint32_t splane0)
{
	*dplane0 = splane0;
}

#endif
