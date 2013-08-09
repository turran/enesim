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
#ifndef LIBARGB_MMX_H
#define LIBARGB_MMX_H
/*============================================================================*
 *                               MMX Helper                                   *
 *============================================================================*/
#if LIBARGB_MMX
/*
 * [a a a a]
 */
static inline mmx_t a2v_mmx(uint16_t a)
{
	mmx_t r;

	r = _mm_cvtsi32_si64(a);
	r = _mm_unpacklo_pi16(r, r);
	r = _mm_unpacklo_pi32(r, r);

	return r;
}
/*
 * [0a 0r 0g 0b]
 */
static inline mmx_t c2v_mmx(uint32_t c)
{
	mmx_t z, r;

	r = _mm_cvtsi32_si64(c);
	z = _mm_cvtsi32_si64(0);
	r = _mm_unpacklo_pi8(r, z);

	return r;
}
#endif
#endif /* LIBARGB_MMX_H */
