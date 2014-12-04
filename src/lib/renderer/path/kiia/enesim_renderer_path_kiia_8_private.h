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
#ifndef ENESIM_RENDERER_PATH_KIIA_8_PRIVATE_H
#define ENESIM_RENDERER_PATH_KIIA_8_PRIVATE_H

#include "enesim_private.h"
#include "enesim_renderer_path_kiia_private.h"

/* define our own 8 bit sampling */
#define ENESIM_RENDERER_PATH_KIIA_SAMPLES 8
#define ENESIM_RENDERER_PATH_KIIA_INC 8192 /* in 16.16 1/8.0 */
#define ENESIM_RENDERER_PATH_KIIA_SHIFT 3

static inline uint16_t enesim_renderer_path_kiia_8_non_zero_get_alpha(int cm)
{
	return cm << 5;
}

#endif
