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
#include "enesim_renderer_path_kiia_8_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_RENDERER_PATH_KIIA_MASK_TYPE int
#define ENESIM_RENDERER_PATH_KIIA_MASK_MAX ENESIM_RENDERER_PATH_KIIA_SAMPLES
#define ENESIM_RENDERER_PATH_KIIA_GET_ALPHA ENESIM_RENDERER_PATH_KIIA_NON_ZERO_GET_ALPHA

#include "enesim_renderer_path_kiia_common.h"
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
ENESIM_RENDERER_PATH_KIIA_SPAN_SIMPLE(8, non_zero, color)
ENESIM_RENDERER_PATH_KIIA_SPAN_SIMPLE(8, non_zero, renderer)
ENESIM_RENDERER_PATH_KIIA_SPAN_FULL(8, non_zero, color, color)
ENESIM_RENDERER_PATH_KIIA_SPAN_FULL(8, non_zero, renderer, color)
ENESIM_RENDERER_PATH_KIIA_SPAN_FULL(8, non_zero, color, renderer)
ENESIM_RENDERER_PATH_KIIA_SPAN_FULL(8, non_zero, renderer, renderer)

ENESIM_RENDERER_PATH_KIIA_WORKER_SETUP(8, non_zero)
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
