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
#ifndef ENESIM_RENDERER_SW_PRIVATE_H_
#define ENESIM_RENDERER_SW_PRIVATE_H_

#include "enesim_thread_private.h"
#include "enesim_barrier_private.h"

/**
 * The fill function that every software based renderer should implement
 * @param r The renderer to draw
 * @param x The x coordinate of the span to fill. This is the coordinate the
 * destination buffer is at
 * @param y The y coordinate of the span to fill. This is the coordinate the
 * destination buffer is at
 * @param len The length of the span to fill
 * @param dst The destination buffer to draw at
 */
typedef void (*Enesim_Renderer_Sw_Fill)(Enesim_Renderer *r,
		int x, int y, int len, void *dst);
typedef struct _Enesim_Renderer_Sw_Data Enesim_Renderer_Sw_Data;

#if BUILD_THREAD
typedef struct _Enesim_Renderer_Thread_Operation
{
	/* common attributes */
	Enesim_Renderer *renderer;
	uint8_t * dst;
	size_t stride;
	Eina_Rectangle area;
} Enesim_Renderer_Thread_Operation;

typedef struct _Enesim_Renderer_Thread
{
	int cpuidx;
	Enesim_Thread tid;
	Eina_Bool done;
	Enesim_Renderer_Sw_Data *sw_data;
} Enesim_Renderer_Thread;
#endif

typedef enum _Enesim_Renderer_Sw_Hint
{
	ENESIM_RENDERER_SW_HINT_COLORIZE 		= (1 << 0), /* Can draw directly using the color property */
	ENESIM_RENDERER_SW_HINT_ROP 		= (1 << 1), /* Can draw directly using the raster operation */
	ENESIM_RENDERER_SW_HINT_MASK 		= (1 << 2), /* Can draw directly using the mask renderer */
} Enesim_Renderer_Sw_Hint;

struct _Enesim_Renderer_Sw_Data
{
#if BUILD_THREAD
	/* the threaded information */
	Enesim_Renderer_Thread *threads;
	Enesim_Renderer_Thread_Operation op;
	Enesim_Barrier start;
	Enesim_Barrier end;
#endif
	/* TODO for later we might need a pointer to the function that calls
	 *  the fill only or both, to avoid the if
	 */
	Enesim_Renderer_Sw_Fill fill;
	Enesim_Compositor_Span span;
	Eina_Bool use_mask;
};

void enesim_renderer_sw_hints_get(Enesim_Renderer *r, Enesim_Rop rop, Enesim_Renderer_Sw_Hint *hints);
void enesim_renderer_sw_draw(Enesim_Renderer *r, int x, int y, int len, uint32_t *data);
void enesim_renderer_sw_init(void);
void enesim_renderer_sw_shutdown(void);
void enesim_renderer_sw_draw_area(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, Eina_Rectangle *area, int x, int y);
void enesim_renderer_sw_free(Enesim_Renderer *r);

Eina_Bool enesim_renderer_sw_setup(Enesim_Renderer *r, Enesim_Surface *s, Enesim_Rop rop, Enesim_Log **error);
void enesim_renderer_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s);
unsigned int enesim_renderer_sw_cpu_count(void);

#endif
