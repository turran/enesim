/* LIBARGB - ARGB helper functions
 * Copyright (C) 2010 Jorge Luis Zapata
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
#ifndef LIBARGB_H
#define LIBARGB_H

/* All the functions are defined as follows:
 * static inline void DST_TYPE_SRC_COLOR_MASK_OP, where:
 * TYPE is the type of operation: pt for point and sp for span
 * Where DST is the destination format
 * TYPE: Sp for span and Pt for point
 * SRC: Source pixel format, none for nothing
 * COLOR: color if it mutiplies by the color, none for nothing
 * MASK: Mask pixel format, none for nothing
 */

#ifndef __UNUSED__
#define __UNUSED__
#endif

#include "libargb_argb8888.h"
#include "libargb_argb8888_unpre.h"

#endif
