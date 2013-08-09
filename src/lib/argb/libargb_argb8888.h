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
#ifndef LIBARGB_ARGB8888_H
#define LIBARGB_ARGB8888_H

#include "libargb_macros.h"
#include "libargb_types.h"

#if LIBARGB_MMX
#include "libargb_mmx.h"
#endif

#if LIBARGB_SSE2
#include "libargb_sse2.h"
#endif

#include "libargb_argb8888_core.h"
#include "libargb_argb8888_misc.h"
#include "libargb_argb8888_mul4_sym.h"
#include "libargb_argb8888_fill.h"
#include "libargb_argb8888_blend.h"

#endif
