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
#ifndef LIBARGB_MACROS_H
#define LIBARGB_MACROS_H

/* Check that the library user has defined all the macros we need */
#ifndef LIBARGB_MMX
#error "LIBARGB_MMX not defined"
#endif

#ifndef LIBARGB_SSE2
#error "LIBARGB_SSE2 not defined"
#endif

#ifndef LIBARGB_DEBUG
#error "LIBARGB_DEBUG not defined"
#endif

#endif /* LIBARGB_MACROS_H */
