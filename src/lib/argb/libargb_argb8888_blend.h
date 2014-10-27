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
#ifndef LIBARGB_ARGB8888_BLEND_H
#define LIBARGB_ARGB8888_BLEND_H

static inline void argb8888_blend(uint32_t *dplane0, uint16_t a, uint32_t splane0)
{
	*dplane0 = splane0 + argb8888_mul_256(a, *dplane0);
}

#if LIBARGB_MMX
static inline void blend_mmx(uint32_t *d, mmx_t alpha, mmx_t color)
{
	mmx_t r;

	r = c2v_mmx(*d);
	r = _mm_mullo_pi16(alpha, r);
	r = _mm_srli_pi16(r, 8);
	r = _mm_add_pi16(r, color);
	r  = _mm_packs_pu16(r, r);
	*d = _mm_cvtsi64_si32(r);
}
#endif

#if LIBARGB_SSE2
static inline void blend_sse2(uint64_t *d, sse2_t alpha, sse2_t color)
{
	sse2_t r;

	r = cc2v_sse2(*d);
	r = _mm_mullo_epi16(alpha, r);
	r = _mm_srli_epi16(r, 8);
	r = _mm_add_epi16(r, color);
	r = _mm_packus_epi16(r, r);
	*(mmx_t *)d = _mm_movepi64_pi64(r);
}
#endif
/*============================================================================*
 *                             Point operations                               *
 *============================================================================*/
static inline void argb8888_pt_none_color_none_blend(uint32_t *d,
		uint32_t s EINA_UNUSED, uint32_t color, uint32_t m EINA_UNUSED)
{
	uint16_t a16 = 256 - argb8888_alpha_get(color);
#if LIBARGB_MMX
	mmx_t r0 = a2v_mmx(a16);
	mmx_t r1 = c2v_mmx(color);
#endif

#if LIBARGB_MMX
	blend_mmx(d, r0, r1);
	_mm_empty();
#else
	argb8888_blend(d, a16, color);
#endif
}

static inline void argb8888_pt_none_color_argb8888_blend(uint32_t *d,
		uint32_t s EINA_UNUSED, uint32_t color, uint32_t m)
{
	uint16_t ca = 256 - argb8888_alpha_get(color);
	uint16_t ma = argb8888_alpha_get(m);

	switch (ma)
	{
		case 0:
		break;

		case 255:
		argb8888_blend(d, ca, color);
		break;

		default:
		{
			uint32_t mc;

			mc = argb8888_mul_sym(ma, color);
			ma = 256 - argb8888_alpha_get(mc);
			argb8888_blend(d, ma, mc);
		}
		break;
	}
}

static inline void argb8888_pt_argb8888_none_none_blend(uint32_t *d,
		uint32_t s, uint32_t color EINA_UNUSED, uint32_t m EINA_UNUSED)
{
	uint16_t a;

	a = 256 - argb8888_alpha_get(s);
	argb8888_blend(d, a, s);
}
/*============================================================================*
 *                              Span operations                               *
 *============================================================================*/

/* no mask */
static inline void argb8888_sp_none_color_none_blend(uint32_t *d,
		unsigned int len, uint32_t *s EINA_UNUSED,
		uint32_t color, uint32_t *m EINA_UNUSED)
{
	uint32_t *end = d + len;
	uint16_t a16 = 256 - argb8888_alpha_get(color);
#if LIBARGB_MMX
	mmx_t r0 = a2v_mmx(a16);
	mmx_t r1 = c2v_mmx(color);
#endif
	while (d < end)
	{
#if LIBARGB_MMX
		blend_mmx(d, r0, r1);
#else
		argb8888_blend(d, a16, color);
#endif
		d++;
	}
#if LIBARGB_MMX
	_mm_empty();
#endif
}

#if 0
static inline void argb8888_sp_argb8888_none_none_blend(uint32_t *d,
		unsigned int len, uint32_t *s,
		uint32_t color EINA_UNUSED, uint32_t *m EINA_UNUSED)
{
	uint32_t *end = d + len;
#if LIBARGB_MMX
	mmx_t r0, r1;
#endif

	while (d < end)
	{
		uint16_t a16 = 256 - ((*s) >> 24);
#if LIBARGB_MMX
		r0 = a2v_mmx(a16);
		r1 = c2v_mmx(*s);

		blend_mmx(d, r0, r1);
#else
		argb8888_blend(d, a16, *s);
#endif
		d++;
		s++;
	}
#if LIBARGB_MMX
	_mm_empty();
#endif
}
#endif

static inline void argb8888_sp_argb8888_none_none_blend(uint32_t *d,
		unsigned int len, uint32_t *s,
		uint32_t color EINA_UNUSED, uint32_t *m EINA_UNUSED)
{
	uint32_t *end = d + len;
#if LIBARGB_MMX
	mmx_t r0, r1;
#endif

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
#if LIBARGB_MMX
			r0 = a2v_mmx(a16);
			r1 = c2v_mmx(*s);

			blend_mmx(d, r0, r1);
#else
			argb8888_blend(d, a16, *s);
#endif
			break;
		}
		d++;
		s++;
	}
#if LIBARGB_MMX
	_mm_empty();
#endif
}

static inline void argb8888_sp_argb8888_color_none_blend(uint32_t *d,
		unsigned int len, uint32_t *s,
		uint32_t color, uint32_t *m EINA_UNUSED)
{
	uint32_t *end = d + len;
#if LIBARGB_MMX
	mmx_t r0, r1;
#endif

	while (d < end)
	{
		uint32_t cs = argb8888_mul4_sym(color, *s);
		uint16_t a16 = 256 - argb8888_alpha_get(cs);

#if LIBARGB_MMX
		r0 = a2v_mmx(a16);
		r1 = c2v_mmx(cs);

		blend_mmx(d, r0, r1);
#else
		argb8888_blend(d, a16, cs);
#endif
		d++;
		s++;
	}
#if LIBARGB_MMX
	_mm_empty();
#endif
}

/* a8 mask */

static inline void argb8888_sp_none_color_a8_alpha_blend(uint32_t *d, unsigned int len,
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
			argb8888_blend(d, ca, color);
			break;

			default:
			{
				uint32_t mc;

				mc = argb8888_mul_sym(ma, color);
				ma = 256 - argb8888_alpha_get(mc);
				argb8888_blend(d, ma, mc);
			}
			break;
		}
		d++;
		m++;
	}
}

/* argb luminance-channel masking */
static inline void argb8888_sp_argb8888_none_argb8888_luminance_blend(uint32_t *d, unsigned int len,
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
					argb8888_blend(d, sa, *s);
			}
			break;

			default:
			{
				uint16_t sa;

				sa = 1 + argb8888_lum_get(p);
				p = argb8888_mul_256(sa, *s);
				sa = 256 - argb8888_alpha_get(p);
				if (sa < 256)
					argb8888_blend(d, sa, p);
			}
			break;
		}
		m++;
		s++;
		d++;
	}
}

static inline void argb8888_sp_argb8888_color_argb8888_luminance_blend(uint32_t *d, unsigned int len,
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
					argb8888_blend(d, sa, p);
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
					argb8888_blend(d, sa, p);
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
static inline void argb8888_sp_none_color_argb8888_alpha_blend(uint32_t *d, unsigned int len,
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
			argb8888_blend(d, ca, color);
			break;

			default:
			{
				uint32_t mc;

				mc = argb8888_mul_256(ma, color);
				ma = 256 - argb8888_alpha_get(mc);
				argb8888_blend(d, ma, mc);
			}
			break;
		}
		d++;
		m++;
	}
}

static inline void argb8888_sp_argb8888_none_argb8888_alpha_blend(uint32_t *d, unsigned int len,
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
					argb8888_blend(d, sa, *s);
			}
			break;

			default:
			{
				uint32_t mc;

				mc = argb8888_mul_256(ma, *s);
				ma = 256 - argb8888_alpha_get(mc);
				if (ma < 256)
					argb8888_blend(d, ma, mc);
			}
			break;
		}
		m++;
		s++;
		d++;
	}
}


static inline void argb8888_sp_argb8888_color_argb8888_alpha_blend(uint32_t *d, unsigned int len,
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
					argb8888_blend(d, ma, mc);
			}
			break;

			default:
			{
				uint32_t mc;

				ma = (ma * ca) >> 8;
				mc = argb8888_mul_256(ma, *s);
				ma = 256 - argb8888_alpha_get(mc);
				if (ma < 256)
					argb8888_blend(d, ma, mc);
			}
			break;
		}
		m++;
		s++;
		d++;
	}
}

#if 0
static void argb8888_sp_color_blend_sse2(uint32_t *d,
		unsigned int len, uint32_t *s,
		uint32_t color, uint32_t *m)
{
	int r = (len % 2);
	int l = len - r;

	uint32_t *dtmp = d->plane0;
	uint32_t *end = d->plane0 + l;
	uint8_t a;

#if 1
	sse2_t r0, r1;

	a = color->plane0 >> 24;
	r0 = a2v_sse2(256 - a);
	r1 = c2v_sse2(color->plane0);

	while (dtmp < end)
	{
		blend_sse2((uint64_t *)dtmp, r0, r1);
		dtmp += 2;
	}
#else
	argb8888_sp_color_blend_mmx(d, l, s, color, m);
#endif
#if 0
	if (r)
	{
		mmx_t m0, m1;

		m0 = _mm_movepi64_pi64(r0);
		m1 = _mm_movepi64_pi64(r1);
		blend_mmx(++dtmp, m0, m1);
	}
#endif
	_mm_empty();
}

static void argb8888_sp_argb8888_blend_argb8888_sse2(uint32_t *d,
		unsigned int len, uint32_t *s,
		uint32_t color, uint32_t *m)
{
	int r = (len % 2);
	int l = len - r;
	sse2_t r0, r1;
	uint32_t *stmp = s->plane0;
	uint32_t *dtmp = d->plane0;
	uint32_t *end = d->plane0 + l;

	while (dtmp < end)
	{
		r0 = aa2v_sse2(*(uint64_t *)stmp);
		r1 = cc2v_sse2(*(uint64_t *)stmp);

		blend_sse2((uint64_t *)dtmp, r0, r1);
		dtmp += 2;
		stmp += 2;
	}
#if 0
	if (r)
	{
		mmx_t m0, m1;
		uint8_t a;

		stmp++;
		dtmp++;

		a = (*stmp) >> 24;
		m0 = a2v_mmx(256 - a);
		m1 = c2v_mmx(*stmp);
		blend_mmx(dtmp, m0, m1);
	}
#endif
	_mm_empty();
}
#endif
#endif
