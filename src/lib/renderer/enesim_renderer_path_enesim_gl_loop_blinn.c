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
#include "enesim_renderer_path_enesim_private.h"

/* The idea is to tesselate the whole path without taking into account the
 * curves, just normalize theme to quadratic/cubic (i.e remove the arc).
 * once that's done we can use a shader and set the curve coefficients on the
 * vertex data, thus provide a simple in/out check to define the real curve
 * http://www.mdk.org.pl/2007/10/27/curvy-blues
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#if BUILD_OPENGL
#endif

