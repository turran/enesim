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
#ifndef LIBARGB_ARGB8888_FILL_H
#define LIBARGB_ARGB8888_FILL_H
/*============================================================================*
 *                             Point operations                               *
 *============================================================================*/
static inline void argb8888_pt_none_color_none_fill(uint32_t *d, uint32_t s __UNUSED__,
		uint32_t color, uint32_t m __UNUSED__)
{
	*d = color;
}
static inline void argb8888_pt_none_color_argb8888_fill(uint32_t *d,
		uint32_t s __UNUSED__, uint32_t color, uint32_t m)
{
	uint16_t a = m >> 24;
	switch (a)
	{
		case 0:
		break;

		case 255:
		*d = color;
		break;

		default:
		*d = argb8888_interp_256(a + 1, color, *d);
		break;
	}
}

static inline void argb8888_pt_argb8888_none_argb8888_fill(uint32_t *d,
		uint32_t s, uint32_t color __UNUSED__, uint32_t m)
{
	uint16_t a = m >> 24;
	switch (a)
	{
		case 0:
		break;

		case 255:
		*d = s;
		break;

		default:
		*d = argb8888_interp_256(a + 1, s, *d);
		break;
	}
}

static inline void argb8888_pt_argb8888_none_none_fill(uint32_t *d,
		uint32_t s, uint32_t color __UNUSED__, uint32_t m __UNUSED__)
{
	*d = s;
}

static inline void argb8888_pt_argb8888_color_none_fill(uint32_t *d,
		uint32_t s, uint32_t color, uint32_t m __UNUSED__)
{
	*d = argb8888_mul4_sym(color, s);
}

/*============================================================================*
 *                              Span operations                               *
 *============================================================================*/

/** "fill" functions - do a copy shaped by a mask **/
/** dst = src*color*ma + dst*(1 - ma) **/
/** no mask or src is equivalent to them being 1 everywhere **/
static inline void argb8888_sp_none_color_none_fill(uint32_t *d, uint32_t len,
		uint32_t *s __UNUSED__, uint32_t color, uint32_t *m __UNUSED__)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		*d = color;
		d++;
	}
}
static inline void argb8888_sp_argb8888_none_none_fill(uint32_t *d, uint32_t len,
		uint32_t *s, uint32_t color __UNUSED__, uint32_t *m __UNUSED__)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		*d = *s;
		d++;
		s++;
	}
}

static inline void argb8888_sp_argb8888_color_none_fill(uint32_t *d, uint32_t len,
		uint32_t *s, uint32_t color, uint32_t *m __UNUSED__)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint32_t c = *s;
		switch (c)
		{
			case 0:
			*d = 0;
			break;

			case 0xffffffff:
			*d = color;
			break;

			default:
			*d = argb8888_mul4_sym(color, c);
			break;
		}
		d++;
		s++;
	}
}

static inline void argb8888_sp_none_color_argb8888_fill(uint32_t *d, uint32_t len,
		uint32_t *s __UNUSED__, uint32_t color, uint32_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t a = *m >> 24;
		switch (a)
		{
			case 0:
			break;

			case 255:
			*d = color;
			break;

			default:
			*d = argb8888_interp_256(a + 1, color, *d);
			break;
		}
		d++;
		m++;
	}
}

static inline void argb8888_sp_none_none_argb8888_fill(uint32_t *d, uint32_t len,
		uint32_t *s __UNUSED__, uint32_t color __UNUSED__, uint32_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t a = *m >> 24;
		switch (a)
		{
			case 0:
			break;

			case 255:
			*d = 0xffffffff;
			break;

			default:
			*d = argb8888_interp_256(a + 1, 0xffffffff, *d);
			break;
		}
		d++;
		m++;
	}
}

static inline void argb8888_sp_argb8888_none_argb8888_fill(uint32_t *d,
		uint32_t len, uint32_t *s, uint32_t color __UNUSED__,
		uint32_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t a = *m >> 24;
		switch (a)
		{
			case 0:
			break;

			case 255:
			*d = *s;
			break;

			default:
			*d = argb8888_interp_256(a + 1, *s, *d);
			break;
		}
		m++;
		s++;
		d++;
	}
}

static inline void argb8888_sp_argb8888_color_argb8888_fill(uint32_t *d,
		uint32_t len, uint32_t *s, uint32_t color, uint32_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t a = *m >> 24;
		switch (a)
		{
			case 0:
			break;

			case 255:
			*d = argb8888_mul4_sym(color, *s);
			break;

			default:
			{
				uint32_t _tmp_color = argb8888_mul4_sym(color, *s);
				*d = argb8888_interp_256(a + 1, _tmp_color, *d);
			}
			break;
		}
		m++;
		s++;
		d++;
	}

}

static inline void argb8888_sp_none_color_a8_fill(uint32_t *d, uint32_t len,
		uint32_t *s __UNUSED__, uint32_t color, uint8_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t a = *m;
		switch (a)
		{
			case 0:
			break;

			case 255:
			*d = color;
			break;

			default:
			*d = argb8888_interp_256(a + 1, color, *d);
			break;
		}
		d++;
		m++;
	}
}

/** "copy" functions - do a straight copy **/
/** dst = src*color*ma **/
/** case of no mask agrees with the fill versions, so omitted **/
static inline void argb8888_sp_argb8888_color_argb8888_copy(uint32_t *d,
		uint32_t len, uint32_t *s, uint32_t color, uint32_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t a = *m >> 24;
		switch (a)
		{
			case 0:
			*d = 0;
			break;

			case 255:
			*d = argb8888_mul4_sym(color, *s);
			break;

			default:
			{
				uint32_t _tmp_color = argb8888_mul4_sym(color, *s);
				*d = argb8888_mul_sym(a, _tmp_color);
			}
			break;
		}
		m++;
		s++;
		d++;
	}

}

static inline void argb8888_sp_argb8888_none_argb8888_copy(uint32_t *d,
		uint32_t len, uint32_t *s, uint32_t color __UNUSED__, uint32_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t a = *m >> 24;
		switch (a)
		{
			case 0:
			*d = 0;
			break;

			case 255:
			*d = *s;
			break;

			default:
			*d = argb8888_mul_sym(a, *s);
			break;
		}
		m++;
		s++;
		d++;
	}

}

static inline void argb8888_sp_none_color_argb8888_copy(uint32_t *d, uint32_t len,
		uint32_t *s __UNUSED__, uint32_t color, uint32_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t a = *m >> 24;
		switch (a)
		{
			case 0:
			*d = 0;
			break;

			case 255:
			*d = color;
			break;

			default:
			*d = argb8888_mul_sym(a, color);
			break;
		}
		d++;
		m++;
	}
}

static inline void argb8888_sp_none_none_argb8888_copy(uint32_t *d, uint32_t len,
		uint32_t *s __UNUSED__, uint32_t color __UNUSED__, uint32_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		// really should be result = ma<<24|ma<<16|ma<<8|ma
		// but this is much faster and works for us
		*d = *m;
		d++;
		m++;
	}
}


#if 0
static void argb8888_sp_pixel_fill_argb8888_mmx(Enesim_Surface_Data *d,
		uint32_t len, Enesim_Surface_Data *s,
		Enesim_Surface_Pixel *color, Enesim_Surface_Data *m)
{
	uint32_t *stmp = s->plane0, *dtmp = d->plane0;
	uint32_t *end;
	int l = 0;

	l = (len / 16);
	end = d->plane0 + (len - (len % 16));
	while (dtmp < end)
	{
		mmx_t m0, m1, m2, m3, m4, m5, m6, m7;

		m0 = *((__m64 *)stmp);
		m1 = *((__m64 *)(stmp + 2));
		m2 = *((__m64 *)(stmp + 4));
		m3 = *((__m64 *)(stmp + 6));
		m4 = *((__m64 *)(stmp + 8));
		m5 = *((__m64 *)(stmp + 10));
		m6 = *((__m64 *)(stmp + 12));
		m7 = *((__m64 *)(stmp + 14));
		*(__m64 *)dtmp = m0;
		*(__m64 *)(dtmp + 2) = m1;
		*(__m64 *)(dtmp + 4) = m2;
		*(__m64 *)(dtmp + 6) = m3;
		*(__m64 *)(dtmp + 8) = m4;
		*(__m64 *)(dtmp + 10) = m5;
		*(__m64 *)(dtmp + 12) = m6;
		*(__m64 *)(dtmp + 14) = m7;

		stmp += 16;
		dtmp += 16;
	}
	_mm_empty();
}
#endif

#endif /* LIBARGB_ARGB8888_FILL_H */
