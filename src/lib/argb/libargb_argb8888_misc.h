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

/**
 * Sampling algorithms,
 * A pixel goes from 0 to 1, with its center placed on the top-left corner,
 * so a value of 0.9 is still inside the pixel
 */
/* A bilinear sampling */
static inline uint32_t argb8888_sample_good(uint32_t *data, size_t stride, int sw,
		int sh, Eina_F16p16 xx, Eina_F16p16 yy)
{
	Eina_F16p16 sww;
	Eina_F16p16 shh;
	Eina_F16p16 minone;

	sww = eina_f16p16_int_from(sw) + EINA_F16P16_ONE;
	shh = eina_f16p16_int_from(sh) + EINA_F16P16_ONE;
	minone = eina_f16p16_int_from(-1);

	if (xx < sww && yy < shh && xx >= minone && yy >= minone)
	{
		int x, y;
		uint32_t p0 = 0, p1 = 0, p2 = 0, p3 = 0;

		/* we use the int_to given that it floors the value
		 * floor(-0.8) = -1, so we always pick the left-top pixel first
		 */
		x = eina_f16p16_int_to(xx);
		y = eina_f16p16_int_to(yy);

		/* pick the coord */
		data = argb8888_at(data, stride, x, y);

		if ((x > -1) && (y > - 1) && (x < sw) && (y < sh))
			p0 = *data;

		if ((y > -1) && (y < sh) && ((x + 1) < sw))
			p1 = *(data + 1);

		if ((y + 1) < sh)
		{
			if ((x > -1) && (x < sw))
				p2 = *((uint32_t *)((uint8_t *)data + stride));
			if ((x + 1) < sw)
				p3 = *((uint32_t *)((uint8_t *)data + stride) + 1);
		}

		if (p0 | p1 | p2 | p3)
		{
			uint16_t ax, ay;

			/* to make it be in the 1 - 256 range , note that for
			 * negative values, the fracc already does the two's complement
			 * and thus the value is correct
			 */
			ax = 1 + (eina_f16p16_fracc_get(xx) >> 8);
			ay = 1 + (eina_f16p16_fracc_get(yy) >> 8);

			p0 = argb8888_interp_256(ax, p1, p0);
			p2 = argb8888_interp_256(ax, p3, p2);
			p0 = argb8888_interp_256(ay, p2, p0);
		}
		return p0;
	}
	else
		return 0;
}

/*
 * Just pick the nearest pixel based on x,y
 */
static inline uint32_t argb8888_sample_fast(uint32_t *data, size_t stride, int sw,
		int sh, Eina_F16p16 xx, Eina_F16p16 yy)
{
	int x, y;

	x = eina_f16p16_int_to(xx);
	y = eina_f16p16_int_to(yy);
	if (x < sw && y < sh && x >= 0 && y >= 0)
	{
		uint32_t *ret;

		ret = argb8888_at(data, stride, x, y);
		return *ret;
	}
	else
		return 0;
}

#endif
