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
#ifndef LIBARGB_TYPES_H
#define LIBARGB_TYPES_H

#include <stdint.h>

/* SIMD intrinsics */
#if LIBARGB_MMX
#include <mmintrin.h>
typedef __m64 mmx_t;
#endif

#if LIBARGB_SSE
#include <xmmintrin.h>
#endif

#if LIBARGB_SSE2
#include <emmintrin.h>
typedef __m128i sse2_t;
#endif

/* Debug definitions */
#if LIBARGB_SSE2
typedef union
{
	mmx_t mmx[2];
	sse2_t sse2;
} sse2_d;
#endif

#if LIBARGB_MMX
typedef union
{
	mmx_t mmx;
	uint32_t u32[2];
} mmx_d;
#endif

#endif /* LIBARGB_TYPES_H */
