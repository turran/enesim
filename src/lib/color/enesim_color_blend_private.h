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
#ifndef _ENESIM_COLOR_BLEND_PRIVATE_H
#define _ENESIM_COLOR_BLEND_PRIVATE_H

/*============================================================================*
 *                             Point operations                               *
 *============================================================================*/
static inline void enesim_color_blend_pt_none_color_none(uint32_t *d,
		uint32_t s EINA_UNUSED, uint32_t color, uint32_t m EINA_UNUSED)
{
	uint16_t a16 = 256 - argb8888_alpha_get(color);
	argb8888(d, a16, color);
}

static inline void enesim_color_blend_pt_none_color_argb8888(uint32_t *d,
		uint32_t s EINA_UNUSED, uint32_t color, uint32_t m)
{
	uint16_t ca = 256 - argb8888_alpha_get(color);
	uint16_t ma = argb8888_alpha_get(m);

	switch (ma)
	{
		case 0:
		break;

		case 255:
		argb8888(d, ca, color);
		break;

		default:
		{
			uint32_t mc;

			mc = argb8888_mul_sym(ma, color);
			ma = 256 - argb8888_alpha_get(mc);
			argb8888(d, ma, mc);
		}
		break;
	}
}

static inline void enesim_color_blend_pt_argb8888_none_none(uint32_t *d,
		uint32_t s, uint32_t color EINA_UNUSED, uint32_t m EINA_UNUSED)
{
	uint16_t a;

	a = 256 - argb8888_alpha_get(s);
	argb8888(d, a, s);
}
/*============================================================================*
 *                              Span operations                               *
 *============================================================================*/

/* no mask */
static inline void enesim_color_blend_sp_none_color_none(uint32_t *d,
		unsigned int len, uint32_t *s EINA_UNUSED,
		uint32_t color, uint32_t *m EINA_UNUSED)
{
	uint32_t *end = d + len;
	uint16_t a16 = 256 - argb8888_alpha_get(color);

	while (d < end)
	{
		argb8888(d, a16, color);
		d++;
	}
}

static inline void enesim_color_blend_sp_argb8888_none_none(uint32_t *d,
		unsigned int len, uint32_t *s,
		uint32_t color EINA_UNUSED, uint32_t *m EINA_UNUSED)
{
	uint32_t *end = d + len;

	while (d < end)
	{
		uint16_t a16 = 256 - argb8888_alpha_get(*s);

		switch (a16)
		{
			case 256:
			break;

			case 1:
			*d = *s;
			break;

			default:
			argb8888(d, a16, *s);
			break;
		}
		d++;
		s++;
	}
}

static inline void enesim_color_blend_sp_argb8888_color_none(uint32_t *d,
		unsigned int len, uint32_t *s,
		uint32_t color, uint32_t *m EINA_UNUSED)
{
	uint32_t *end = d + len;

	while (d < end)
	{
		uint32_t cs = argb8888_mul4_sym(color, *s);
		uint16_t a16 = 256 - argb8888_alpha_get(cs);

		argb8888(d, a16, cs);
		d++;
		s++;
	}
}

/* a8 mask */

static inline void enesim_color_blend_sp_none_color_a8_alpha(uint32_t *d, unsigned int len,
		uint32_t *s EINA_UNUSED, uint32_t color, uint8_t *m)
{
	uint16_t ca = 256 - argb8888_alpha_get(color);
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t ma = *m;

		switch (ma)
		{
			case 0:
			break;

			case 255:
			argb8888(d, ca, color);
			break;

			default:
			{
				uint32_t mc;

				mc = argb8888_mul_sym(ma, color);
				ma = 256 - argb8888_alpha_get(mc);
				argb8888(d, ma, mc);
			}
			break;
		}
		d++;
		m++;
	}
}

/* argb luminance-channel masking */
static inline void enesim_color_blend_sp_argb8888_none_argb8888_luminance(uint32_t *d, unsigned int len,
		uint32_t *s, uint32_t color EINA_UNUSED, uint32_t *m)
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
			{
				uint16_t sa;

				sa = 256 - argb8888_alpha_get(*s);
				if (sa < 256)
					argb8888(d, sa, *s);
			}
			break;

			default:
			{
				uint16_t sa;

				sa = 1 + argb8888_lum_get(p);
				p = argb8888_mul_256(sa, *s);
				sa = 256 - argb8888_alpha_get(p);
				if (sa < 256)
					argb8888(d, sa, p);
			}
			break;
		}
		m++;
		s++;
		d++;
	}
}

static inline void enesim_color_blend_sp_argb8888_color_argb8888_luminance(uint32_t *d, unsigned int len,
		uint32_t *s, uint32_t color, uint32_t *m)
{
	uint32_t *end = d + len;
	uint16_t ca = 1 + argb8888_alpha_get(color);
/*
   We use only the color aplha since using the luminace
   channel for masks, as per svg, when used in conjunction
   with an argb src, really only has the 'color' being opacity.

*/
	while (d < end)
	{
		uint32_t p = *m;

		switch (p)
		{
			case 0:
			break;

			case 0xffffffff:
			{
				if (p == *s)
				{
					uint16_t sa;

					p = argb8888_mul_256(ca, p);
					sa = 256 - argb8888_alpha_get(p);
					argb8888(d, sa, p);
				}
			}
			break;

			default:
			{
				if (*s)
				{
					uint16_t sa;

					sa = 1 + argb8888_lum_get(p);
					sa = (ca * sa) >> 8;
					p = argb8888_mul_256(sa, *s);
					sa = 256 - argb8888_alpha_get(p);
					argb8888(d, sa, p);
				}
			}
			break;
		}
		m++;
		s++;
		d++;
	}
}

/* argb alpha-channel masking */
static inline void enesim_color_blend_sp_none_color_argb8888_alpha(uint32_t *d, unsigned int len,
		uint32_t *s EINA_UNUSED, uint32_t color, uint32_t *m)
{
	uint16_t ca = 256 - argb8888_alpha_get(color);
	uint32_t *end = d + len;
	while (d < end)
	{
		uint16_t ma = 1 + argb8888_alpha_get(*m);

		switch (ma)
		{
			case 1:
			break;

			case 256:
			argb8888(d, ca, color);
			break;

			default:
			{
				uint32_t mc;

				mc = argb8888_mul_256(ma, color);
				ma = 256 - argb8888_alpha_get(mc);
				argb8888(d, ma, mc);
			}
			break;
		}
		d++;
		m++;
	}
}

static inline void enesim_color_blend_sp_argb8888_none_argb8888_alpha(uint32_t *d, unsigned int len,
		uint32_t *s, uint32_t color EINA_UNUSED, uint32_t *m)
{
	uint32_t *end = d + len;

	while (d < end)
	{
		uint16_t ma = 1 + argb8888_alpha_get(*m);

		switch (ma)
		{
			case 1:
			break;

			case 256:
			{
				uint16_t sa;

				sa = 256 - argb8888_alpha_get(*s);
				if (sa < 256)
					argb8888(d, sa, *s);
			}
			break;

			default:
			{
				uint32_t mc;

				mc = argb8888_mul_256(ma, *s);
				ma = 256 - argb8888_alpha_get(mc);
				if (ma < 256)
					argb8888(d, ma, mc);
			}
			break;
		}
		m++;
		s++;
		d++;
	}
}


static inline void enesim_color_blend_sp_argb8888_color_argb8888_alpha(uint32_t *d, unsigned int len,
		uint32_t *s, uint32_t color, uint32_t *m)
{
	uint32_t *end = d + len;
	uint16_t ca = 1 + argb8888_alpha_get(color);
/*
   We probably should just use the color's alpha only,
   since that's somewhat faster and it's all that svg
   considers when there's also an argb src
   (ie. the color is purely opacity then).
*/
	while (d < end)
	{
		uint16_t ma = 1 + argb8888_alpha_get(color);

		switch (ma)
		{
			case 1:
			break;

			case 256:
			{
				uint32_t mc;

				mc = argb8888_mul_256(ca, *s);
				ma = 256 - argb8888_alpha_get(mc);
				if (ma < 256)
					argb8888(d, ma, mc);
			}
			break;

			default:
			{
				uint32_t mc;

				ma = (ma * ca) >> 8;
				mc = argb8888_mul_256(ma, *s);
				ma = 256 - argb8888_alpha_get(mc);
				if (ma < 256)
					argb8888(d, ma, mc);
			}
			break;
		}
		m++;
		s++;
		d++;
	}
}
#endif
