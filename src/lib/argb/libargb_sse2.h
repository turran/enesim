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
#ifndef LIBARGB_SSE2_H
#define LIBARGB_SSE2_H
/*============================================================================*
 *                               SSE2 Helper                                  *
 *============================================================================*/
#if LIBARGB_SSE2

#ifdef _MSC_VER 
#define UINT64_TO_MMX(dst_mmx, src_uint64) dst_mmx.m64_u64 = src_uint64
#else
#define UINT64_TO_MMX(dst_mmx, src_uint64) dst_mmx = (mmx_t)src_uint64;
#endif
/*
 * [aa aa aa aa]
 */
static inline sse2_t a2v_sse2(uint16_t a)
{
	sse2_t s;

	s = _mm_cvtsi32_si128(a);
	s = _mm_unpacklo_epi16(s, s);
	s = _mm_unpacklo_epi32(s, s);
	s = _mm_unpacklo_epi64(s, s);

	return s;
}

/*
 * [a1a1 a1a1 a2a2 a2a2]
 */
static inline sse2_t aa2v_sse2(uint64_t c)
{
	mmx_t m, r;
	sse2_t s;

	UINT64_TO_MMX(r, c);
	UINT64_TO_MMX(m, UINT64_C(0xff000000ff000000));
	r = _mm_and_si64(r, m);
	r = _mm_srli_pi32(r, 24);
	UINT64_TO_MMX(m, UINT64_C(0x0000010000000100));
	r = _mm_sub_pi16(m, r);
	r = _mm_packs_pi32(r, r);
	s = _mm_set_epi64 ((__m64)0LL, r);
	s = _mm_unpacklo_epi16(s, s);
	s = _mm_unpacklo_epi32(s, s);

	return s;
}

/*
 * [0a 0r 0g 0b 0a 0r 0g 0b]
 */
static inline sse2_t c2v_sse2(uint32_t c)
{
	sse2_t z, r;

	r = _mm_cvtsi32_si128(c);
	z = _mm_cvtsi32_si128(0);
	r = _mm_unpacklo_epi8(r, z);
	r = _mm_unpacklo_epi64(r, r);

	return r;
}

/*
 * [0a1 0r1 0g1 0b1 0a2 0r2 0g2 0b2]
 */
static inline sse2_t cc2v_sse2(uint64_t c)
{
	sse2_t r, z;
	mmx_t tmp;

	UINT64_TO_MMX(tmp, c);
	r = _mm_set_epi64((__m64)0LL, tmp);
	z = _mm_cvtsi32_si128(0);
	r = _mm_unpacklo_epi8(r, z);

	return r;
}
#endif

#endif
