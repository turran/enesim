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
 * r = (c0 - c1) * a + c1
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
 *
 */
static inline uint32_t argb8888_mul_256(uint16_t a, uint32_t c)
{
	return  ( (((((c) >> 8) & 0x00ff00ff) * (a)) & 0xff00ff00) +
	(((((c) & 0x00ff00ff) * (a)) >> 8) & 0x00ff00ff) );
}

/*
 *
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

static inline uint32_t argb8888_sample_good(uint32_t *data, size_t stride, int sw,
		int sh, Eina_F16p16 xx, Eina_F16p16 yy, int x, int y)
{
	if (x < sw && y < sh && x >= 0 && y >= 0)
	{
		uint32_t p0 = 0, p1 = 0, p2 = 0, p3 = 0;

		data = argb8888_at(data, stride, x, y);

		if ((x > -1) && (y > - 1))
			p0 = *data;

		if ((y > -1) && ((x + 1) < sw))
			p1 = *(data + 1);

		if ((y + 1) < sh)
		{
			if (x > -1)
				p2 = *((uint32_t *)((uint8_t *)data + stride));
			if ((x + 1) < sw)
				p3 = *((uint32_t *)((uint8_t *)data + stride) + 1);
		}

		if (p0 | p1 | p2 | p3)
		{
			uint16_t ax, ay;

			ax = 1 + ((xx & 0xffff) >> 8);
			ay = 1 + ((yy & 0xffff) >> 8);

			p0 = argb8888_interp_256(ax, p1, p0);
			p2 = argb8888_interp_256(ax, p3, p2);
			p0 = argb8888_interp_256(ay, p2, p0);
		}
		return p0;
	}
	else
		return 0;
}


#endif
