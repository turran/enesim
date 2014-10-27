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
#ifndef LIBARGB_ARGB8888_COPY_H
#define LIBARGB_ARGB8888_COPY_H
/*============================================================================*
 *                              Span operations                               *
 *============================================================================*/
/** "copy" functions - do a copy shaped by a mask **/
/** dst = src*color*ma + dst*(1 - ma) **/
/** case of no mask agrees with the copy versions, so omitted **/
static inline void argb8888_sp_none_color_argb8888_copy(uint32_t *d, uint32_t len,
		uint32_t *s EINA_UNUSED, uint32_t color, uint32_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t a;

		a = argb8888_alpha_get(*m);
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

static inline void argb8888_sp_none_none_argb8888_alpha_copy(uint32_t *d, uint32_t len,
		uint32_t *s EINA_UNUSED, uint32_t color EINA_UNUSED, uint32_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t a;

		a = argb8888_alpha_get(*m);
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

static inline void argb8888_sp_argb8888_none_argb8888_alpha_copy(uint32_t *d,
		uint32_t len, uint32_t *s, uint32_t color EINA_UNUSED, uint32_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t a;

		a = argb8888_alpha_get(*m);
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

static inline void argb8888_sp_argb8888_color_argb8888_alpha_copy(uint32_t *d,
		uint32_t len, uint32_t *s, uint32_t color, uint32_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t a;

		a = argb8888_alpha_get(*m);
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

static inline void argb8888_sp_none_color_a8_alpha_copy(uint32_t *d, uint32_t len,
		uint32_t *s EINA_UNUSED, uint32_t color, uint8_t *m)
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

static inline void argb8888_sp_argb8888_none_argb8888_luminance_copy(uint32_t *d,
		uint32_t len, uint32_t *s, uint32_t color EINA_UNUSED, uint32_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint32_t p = *m;
		switch (p)
		{
			case 0:
			break;

			case 0xffffffff:
			*d = *s;
			break;

			default:
			{
				uint16_t ma;

				ma = 1 + argb8888_lum_get(p);
				*d = argb8888_interp_256(ma, *s, *d);
			}
			break;
		}
		m++;
		s++;
		d++;
	}
}

static inline void argb8888_sp_argb8888_color_argb8888_luminance_copy(uint32_t *d,
		uint32_t len, uint32_t *s, uint32_t color, uint32_t *m)
{
	uint32_t *end = d + len;
	uint16_t ca = 1 + argb8888_alpha_get(color);
	/* use color alpha only */
	while (d < end)
	{
		uint32_t p = *m;
		switch (p)
		{
			case 0:
			break;

			case 0xffffffff:
			*d = argb8888_interp_256(ca, *s, *d);
			break;

			default:
			{
				uint16_t ma;

				ma = 1 + argb8888_lum_get(p);
				ma = (ca * ma) >> 8;
				*d = argb8888_interp_256(ma, *s, *d);
			}
			break;
		}
		m++;
		s++;
		d++;
	}

}

#endif
