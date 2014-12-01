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
#include "enesim_private.h"
#include "enesim_renderer_path_kiia_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
/* define our own 8 bit sampling */
#define ENESIM_RENDERER_PATH_KIIA_SAMPLES 8
#define ENESIM_RENDERER_PATH_KIIA_INC 8192 /* in 16.16 1/8.0 */
#define ENESIM_RENDERER_PATH_KIIA_SHIFT 3
#define ENESIM_RENDERER_PATH_KIIA_MASK_TYPE uint8_t
#define ENESIM_RENDERER_PATH_KIIA_MASK_MAX 0xff
#define ENESIM_RENDERER_PATH_KIIA_GET_ALPHA _kiia_8_get_alpha

static uint8_t _kiia_8_mask_values[256] = {
	0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4, 
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
	1, 2, 2, 3, 2, 3, 3, 4, 2, 3, 3, 4, 3, 4, 4, 5, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
	2, 3, 3, 4, 3, 4, 4, 5, 3, 4, 4, 5, 4, 5, 5, 6, 
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
	3, 4, 4, 5, 4, 5, 5, 6, 4, 5, 5, 6, 5, 6, 6, 7, 
	4, 5, 5, 6, 5, 6, 6, 7, 5, 6, 6, 7, 6, 7, 7, 8
};

static inline uint16_t _kiia_8_get_alpha(int cm)
{
	uint16_t coverage;

	coverage = _kiia_8_mask_values[cm] << 5;
	return coverage;
}

#include "enesim_renderer_path_kiia_common.h"
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
ENESIM_RENDERER_PATH_KIIA_SPAN_SIMPLE(8, even_odd, color)
ENESIM_RENDERER_PATH_KIIA_SPAN_SIMPLE(8, even_odd, renderer)
ENESIM_RENDERER_PATH_KIIA_SPAN_SIMPLE(8, non_zero, color)
ENESIM_RENDERER_PATH_KIIA_SPAN_SIMPLE(8, non_zero, renderer)
ENESIM_RENDERER_PATH_KIIA_SPAN_FULL(8, even_odd, color, color)
ENESIM_RENDERER_PATH_KIIA_SPAN_FULL(8, even_odd, renderer, color)
ENESIM_RENDERER_PATH_KIIA_SPAN_FULL(8, even_odd, color, renderer)
ENESIM_RENDERER_PATH_KIIA_SPAN_FULL(8, even_odd, renderer, renderer)
ENESIM_RENDERER_PATH_KIIA_SPAN_FULL(8, non_zero, color, color)
ENESIM_RENDERER_PATH_KIIA_SPAN_FULL(8, non_zero, renderer, color)
ENESIM_RENDERER_PATH_KIIA_SPAN_FULL(8, non_zero, color, renderer)
ENESIM_RENDERER_PATH_KIIA_SPAN_FULL(8, non_zero, renderer, renderer)

ENESIM_RENDERER_PATH_KIIA_WORKER_SETUP(8)
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

