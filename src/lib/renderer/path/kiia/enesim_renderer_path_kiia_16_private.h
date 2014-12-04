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
#ifndef ENESIM_RENDERER_PATH_KIIA_16_PRIVATE_H
#define ENESIM_RENDERER_PATH_KIIA_16_PRIVATE_H

#include "enesim_private.h"
#include "enesim_renderer_path_kiia_private.h"

/* define our own 16 bit sampling */
#define ENESIM_RENDERER_PATH_KIIA_SAMPLES 16
#define ENESIM_RENDERER_PATH_KIIA_INC 4096 /* in 16.16 1/16.0 */
#define ENESIM_RENDERER_PATH_KIIA_SHIFT 4
#define ENESIM_RENDERER_PATH_KIIA_NON_ZERO_GET_ALPHA(cm) ((cm) << 4)

static inline uint16_t enesim_renderer_path_kiia_16_even_odd_get_alpha(int cm)
{
	uint16_t coverage;

	/* use the hamming weight to know the number of bits set to 1 */
	cm = cm - ((cm >> 1) & 0x55555555);
	cm = (cm & 0x33333333) + ((cm >> 2) & 0x33333333);
	/* we use 21 instead of 24, because we need to rescale 32 -> 256 */
	coverage = (((cm + (cm >> 4)) & 0x0f0f0f0f) * 0x01010101) >> 20;

	return coverage;
}

#endif
