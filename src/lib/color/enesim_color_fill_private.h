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
#ifndef _ENESIM_COLOR_FILL_PRIVATE_H
#define _ENESIM_COLOR_FILL_PRIVATE_H

/*============================================================================*
 *                             Point operations                               *
 *============================================================================*/
static inline void enesim_color_fill_pt_none_color_none(uint32_t *d, uint32_t s EINA_UNUSED,
		uint32_t color, uint32_t m EINA_UNUSED)
{
	*d = color;
}

static inline void enesim_color_fill_pt_none_color_argb8888(uint32_t *d,
		uint32_t s EINA_UNUSED, uint32_t color, uint32_t m)
{
	uint16_t a;

	a = enesim_color_alpha_get(m);
	switch (a)
	{
		case 0:
		break;

		case 255:
		*d = color;
		break;

		default:
		*d = enesim_color_interp_256(a + 1, color, *d);
		break;
	}
}

static inline void enesim_color_fill_pt_argb8888_none_argb8888(uint32_t *d,
		uint32_t s, uint32_t color EINA_UNUSED, uint32_t m)
{
	uint16_t a;

	a = enesim_color_alpha_get(m);
	switch (a)
	{
		case 0:
		break;

		case 255:
		*d = s;
		break;

		default:
		*d = enesim_color_interp_256(a + 1, s, *d);
		break;
	}
}

static inline void enesim_color_fill_pt_argb8888_none_none(uint32_t *d,
		uint32_t s, uint32_t color EINA_UNUSED, uint32_t m EINA_UNUSED)
{
	*d = s;
}

static inline void enesim_color_fill_pt_argb8888_color_none(uint32_t *d,
		uint32_t s, uint32_t color, uint32_t m EINA_UNUSED)
{
	*d = enesim_color_mul4_sym(color, s);
}

/*============================================================================*
 *                              Span operations                               *
 *============================================================================*/

/** "fill" functions - do a straight copy **/
/** dst = src*color*ma **/
/** no mask or src is equivalent to them being 1 everywhere **/
static inline void enesim_color_fill_sp_none_color_none(uint32_t *d, uint32_t len,
		uint32_t *s EINA_UNUSED, uint32_t color, uint32_t *m EINA_UNUSED)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		*d = color;
		d++;
	}
}
static inline void enesim_color_fill_sp_argb8888_none_none(uint32_t *d, uint32_t len,
		uint32_t *s, uint32_t color EINA_UNUSED, uint32_t *m EINA_UNUSED)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		*d = *s;
		d++;
		s++;
	}
}

static inline void enesim_color_fill_sp_argb8888_color_none(uint32_t *d, uint32_t len,
		uint32_t *s, uint32_t color, uint32_t *m EINA_UNUSED)
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
			*d = enesim_color_mul4_sym(color, c);
			break;
		}
		d++;
		s++;
	}
}

static inline void enesim_color_fill_sp_argb8888_color_argb8888_alpha(uint32_t *d,
		uint32_t len, uint32_t *s, uint32_t color, uint32_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t a;

		a = enesim_color_alpha_get(*m);
		switch (a)
		{
			case 0:
			*d = 0;
			break;

			case 255:
			*d = enesim_color_mul4_sym(color, *s);
			break;

			default:
			{
				uint32_t _tmp_color = enesim_color_mul4_sym(color, *s);

				*d = enesim_color_mul_sym(a, _tmp_color);
			}
			break;
		}
		m++;
		s++;
		d++;
	}

}

static inline void enesim_color_fill_sp_argb8888_none_argb8888_alpha(uint32_t *d,
		uint32_t len, uint32_t *s, uint32_t color EINA_UNUSED, uint32_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t a;

		a = enesim_color_alpha_get(*m);
		switch (a)
		{
			case 0:
			*d = 0;
			break;

			case 255:
			*d = *s;
			break;

			default:
			*d = enesim_color_mul_sym(a, *s);
			break;
		}
		m++;
		s++;
		d++;
	}

}

static inline void enesim_color_fill_sp_none_color_argb8888_alpha(uint32_t *d, uint32_t len,
		uint32_t *s EINA_UNUSED, uint32_t color, uint32_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t a;

		a = enesim_color_alpha_get(*m);
		switch (a)
		{
			case 0:
			*d = 0;
			break;

			case 255:
			*d = color;
			break;

			default:
			*d = enesim_color_mul_sym(a, color);
			break;
		}
		d++;
		m++;
	}
}

static inline void enesim_color_fill_sp_none_none_argb8888_alpha(uint32_t *d, uint32_t len,
		uint32_t *s EINA_UNUSED, uint32_t color EINA_UNUSED, uint32_t *m)
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

static inline void enesim_color_fill_sp_none_color_a8_alpha(uint32_t *d, uint32_t len,
		uint32_t *s EINA_UNUSED, uint32_t color, uint8_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t a = *m;
		switch (a)
		{
			case 0:
			*d = 0;
			break;

			case 255:
			*d = color;
			break;

			default:
			*d = enesim_color_mul_256(a + 1, color);
			break;
		}
		d++;
		m++;
	}
}

static inline void enesim_color_fill_sp_argb8888_color_argb8888_luminance(uint32_t *d,
		uint32_t len, uint32_t *s, uint32_t color, uint32_t *m)
{
	uint32_t *end = d + len;
	uint16_t ca = 1 + enesim_color_alpha_get(color);
	while (d < end)
	{
		uint32_t p = *m;

		switch (p)
		{
			case 0:
			*d = 0;
			break;

			case 0xffffffff:
			*d = enesim_color_mul_256(ca, *s);
			break;

			default:
			{
				uint16_t ma;

				ma = 1 + enesim_color_lum_get(p);
				ma = (ca * ma) >> 8;
				*d = enesim_color_mul_256(ma, *s);
			}
			break;
		}
		m++;
		s++;
		d++;
	}
}

static inline void enesim_color_fill_sp_argb8888_none_argb8888_luminance(uint32_t *d,
		uint32_t len, uint32_t *s, uint32_t color EINA_UNUSED, uint32_t *m)
{
	uint32_t *end = d + len;
	while (d < end)
	{
		uint32_t p = *m;
		switch (p)
		{
			case 0:
			*d = 0;
			break;

			case 0xffffffff:
			*d = *s;
			break;

			default:
			{
				uint16_t ma;

				ma = 1 + enesim_color_lum_get(p);
				*d = enesim_color_mul_256(ma, *s);
			}
			break;
		}
		m++;
		s++;
		d++;
	}

}

#endif
