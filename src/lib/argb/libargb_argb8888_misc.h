/* LIBARGB - ARGB helper functions
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
#ifndef LIBARGB_ARGB8888_MISC_H
#define LIBARGB_ARGB8888_MISC_H

/*
 * r = (c0 - c1) * a + c1
 * a = 0 => r = c1
 * a = 1 => r = c0
 */
static inline uint32_t argb8888_interp_256(uint16_t a, uint32_t c0, uint32_t c1)
{
	/* Let's disable this until we find the reason of the fp exception */
#if LIBARGB_MMX
	mmx_t rc0, rc1;
	mmx_t ra, ra255;
	uint32_t res;

	ra = a2v_mmx(a);
	ra255 = a2v_mmx(255);
	rc0 = c2v_mmx(c0);
	rc1 = c2v_mmx(c1);

	rc0 = _mm_sub_pi16(rc0, rc1);
	rc0 = _mm_mullo_pi16(ra, rc0);
	rc0 = _mm_srli_pi16(rc0, 8);
	rc0 = _mm_add_pi16(rc0, rc1);
	rc0 = _mm_and_si64(rc0, ra255);
	rc0 = _mm_packs_pu16(rc0, rc0);
	res = _mm_cvtsi64_si32(rc0);
	_mm_empty();

	return res;

#else
	return ( (((((((c0) >> 8) & 0xff00ff) - (((c1) >> 8) & 0xff00ff)) * (a))
			+ ((c1) & 0xff00ff00)) & 0xff00ff00) +
			(((((((c0) & 0xff00ff) - ((c1) & 0xff00ff)) * (a)) >> 8)
			+ ((c1) & 0xff00ff)) & 0xff00ff) );
#endif
}

/*
 * ret = (c0 - c1) * a + c1
 * a = 0 => r = c1
 * a = 1 => r = c0
 */
static inline uint32_t argb8888_interp_65536(uint32_t a, uint32_t c0, uint32_t c1)
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
static inline uint32_t argb8888_mul_256(uint16_t a, uint32_t c)
{
	return  ( (((((c) >> 8) & 0x00ff00ff) * (a)) & 0xff00ff00) +
	(((((c) & 0x00ff00ff) * (a)) >> 8) & 0x00ff00ff) );
}

/*
 * ret = [(c.a * a), (c.r * a), (c.g * a), (c.b * a)]
 * Where a is on the 1 - 65536 range
 */
static inline uint32_t argb8888_mul_65536(uint32_t a, uint32_t c)
{
	return ( ((((c >> 16) & 0xff00) * a) & 0xff000000) +
		((((c >> 16) & 0xff) * a) & 0xff0000) +
		((((c & 0xff00) * a) >> 16) & 0xff00) +
		((((c & 0xff) * a) >> 16) & 0xff) );
}

/*
 *
 */
static inline uint32_t argb8888_mul_sym(uint16_t a, uint32_t c)
{
	return ( (((((c) >> 8) & 0x00ff00ff) * (a) + 0xff00ff) & 0xff00ff00) +
	   (((((c) & 0x00ff00ff) * (a) + 0xff00ff) >> 8) & 0x00ff00ff) );
}

static inline uint32_t * argb8888_at(uint32_t *data, size_t stride, int x, int y)
{
	return (uint32_t *)((uint8_t *)data + (stride * y)) + x;
}

/*
 * Get the luminance based on the function
 * ret = (c.r * 0.2125) + (c.g * 0.7154) + (c.b * 0.0721)
 */
static inline uint8_t argb8888_lum_get(uint32_t c)
{
	uint16_t a;

	a = ((55 * (c & 0xff0000)) >> 24) + ((184 * (c & 0xff00)) >> 16) +
			((19 * (c & 0xff)) >> 8);
	return a;
}

#endif
