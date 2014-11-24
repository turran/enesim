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
/* define our own 32 bit sampling */
#define ENESIM_RENDERER_PATH_KIIA_SAMPLES 32
#define ENESIM_RENDERER_PATH_KIIA_INC 2048 /* in 16.16 1/32.0 */
#define ENESIM_RENDERER_PATH_KIIA_SHIFT 5
#define ENESIM_RENDERER_PATH_KIIA_MASK_TYPE uint32_t
#define ENESIM_RENDERER_PATH_KIIA_MASK_MAX 0xffffffff
#define ENESIM_RENDERER_PATH_KIIA_GET_ALPHA _kiia_32_get_alpha

static inline uint16_t _kiia_32_get_alpha(int cm)
{
	uint16_t coverage;

	/* use the hamming weight to know the number of bits set to 1 */
	cm = cm - ((cm >> 1) & 0x55555555);
	cm = (cm & 0x33333333) + ((cm >> 2) & 0x33333333);
	/* we use 21 instead of 24, because we need to rescale 32 -> 256 */
	coverage = (((cm + (cm >> 4)) & 0x0f0f0f0f) * 0x01010101) >> 21;

	return coverage;
}

#include "enesim_renderer_path_kiia_common.h"
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

void enesim_renderer_path_kiia_best_even_odd_simple_span(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Path_Kiia *thiz;
	Enesim_Renderer_Path_Kiia_Worker *w;

	thiz = ENESIM_RENDERER_PATH_KIIA(r);
	/* pick the worker at y coordinate */
	w = &thiz->workers[y % thiz->nworkers];
	_kiia_figure_even_odd_draw(r, thiz->current, w->mask, w->winding, x, y, len, ddata);
	/* update the latest y coordinate of the worker */
	w->y = y;
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

