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
#include "enesim_renderer_path_kiia_16_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_RENDERER_PATH_KIIA_MASK_TYPE uint16_t
#define ENESIM_RENDERER_PATH_KIIA_MASK_MAX 0xffff
#define ENESIM_RENDERER_PATH_KIIA_GET_ALPHA enesim_renderer_path_kiia_16_even_odd_get_alpha

#include "enesim_renderer_path_kiia_common.h"
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
ENESIM_RENDERER_PATH_KIIA_SPAN_SIMPLE(16, even_odd, color)
ENESIM_RENDERER_PATH_KIIA_SPAN_SIMPLE(16, even_odd, renderer)
ENESIM_RENDERER_PATH_KIIA_SPAN_FULL(16, even_odd, color, color)
ENESIM_RENDERER_PATH_KIIA_SPAN_FULL(16, even_odd, renderer, color)
ENESIM_RENDERER_PATH_KIIA_SPAN_FULL(16, even_odd, color, renderer)
ENESIM_RENDERER_PATH_KIIA_SPAN_FULL(16, even_odd, renderer, renderer)

ENESIM_RENDERER_PATH_KIIA_WORKER_SETUP(16, even_odd)
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
