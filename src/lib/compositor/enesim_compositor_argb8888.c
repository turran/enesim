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
#include "libargb.h"

#include "enesim_main.h"
#include "enesim_color.h"

#include "enesim_compositor_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/*----------------------------------------------------------------------------*
 *                           Fill point funcitons                            *
 *----------------------------------------------------------------------------*/
static void _argb8888_pt_none_color_none_fill(uint32_t *d, uint32_t s,
		uint32_t color, uint32_t m)
{
	argb8888_pt_none_color_none_fill(d, s, color, m);
}

static void _argb8888_pt_none_color_argb8888_fill(uint32_t *d,
		uint32_t s, uint32_t color, uint32_t m)
{
	argb8888_pt_none_color_argb8888_fill(d, s, color, m);
}

static void _argb8888_pt_argb8888_none_argb8888_fill(uint32_t *d,
		uint32_t s, uint32_t color, uint32_t m)
{
	argb8888_pt_argb8888_none_argb8888_fill(d, s, color, m);
}

static void _argb8888_pt_argb8888_none_none_fill(uint32_t *d,
		uint32_t s, uint32_t color, uint32_t m)
{
	argb8888_pt_argb8888_none_none_fill(d, s, color, m);
}

static void _argb8888_pt_argb8888_color_none_fill(uint32_t *d,
		uint32_t s, uint32_t color, uint32_t m)
{
	argb8888_pt_argb8888_color_none_fill(d, s, color, m);
}
/*----------------------------------------------------------------------------*
 *                            Fill span funcitons                            *
 *----------------------------------------------------------------------------*/
static void _argb8888_sp_none_color_none_fill(uint32_t *d, uint32_t len,
		uint32_t *s, uint32_t color, uint32_t *m)
{
	argb8888_sp_none_color_none_fill(d, len, s, color, m);
}

static void _argb8888_sp_argb8888_none_none_fill(uint32_t *d, uint32_t len,
		uint32_t *s, uint32_t color, uint32_t *m)
{
	argb8888_sp_argb8888_none_none_fill(d, len, s, color, m);
}

static void _argb8888_sp_argb8888_color_none_fill(uint32_t *d, uint32_t len,
		uint32_t *s, uint32_t color, uint32_t *m)
{
	argb8888_sp_argb8888_color_none_fill(d, len, s, color, m);
}

static void _argb8888_sp_none_color_argb8888_fill(uint32_t *d, uint32_t len,
		uint32_t *s, uint32_t color, uint32_t *m)
{
	argb8888_sp_none_color_argb8888_fill(d, len, s, color, m);
}

static void _argb8888_sp_none_color_a8_fill(uint32_t *d, uint32_t len,
		uint32_t *s, uint32_t color, uint8_t *m)
{
	argb8888_sp_none_color_a8_fill(d, len, s, color, m);
}

static inline void _argb8888_sp_argb8888_none_argb8888_fill(uint32_t *d,
		uint32_t len, uint32_t *s, uint32_t color,
		uint32_t *m)
{
	argb8888_sp_argb8888_none_argb8888_fill(d, len, s, color, m);
}
/*----------------------------------------------------------------------------*
 *                           Blend point funcitons                            *
 *----------------------------------------------------------------------------*/
static void _argb8888_pt_none_color_none_blend(uint32_t *d,
		uint32_t s, uint32_t color, uint32_t m)
{
	argb8888_pt_none_color_none_blend(d, s, color, m);
}

static void _argb8888_pt_none_color_argb8888_blend(uint32_t *d,
		uint32_t s, uint32_t color, uint32_t m)
{
	argb8888_pt_none_color_argb8888_blend(d, s, color, m);
}

static void _argb8888_pt_argb8888_none_none_blend(uint32_t *d,
		uint32_t s, uint32_t color, uint32_t m)
{
	argb8888_pt_argb8888_none_none_blend(d, s, color, m);
}
/*----------------------------------------------------------------------------*
 *                            Blend span funcitons                            *
 *----------------------------------------------------------------------------*/
static void _argb8888_sp_none_color_none_blend(uint32_t *d,
		unsigned int len, uint32_t *s,
		uint32_t color, uint32_t *m)
{
	argb8888_sp_none_color_none_blend(d, len, s, color, m);
}

static void _argb8888_sp_argb8888_none_none_blend(uint32_t *d,
		unsigned int len, uint32_t *s,
		uint32_t color, uint32_t *m)
{
	argb8888_sp_argb8888_none_none_blend(d, len, s, color, m);
}

static void _argb8888_sp_argb8888_color_none_blend(uint32_t *d,
		unsigned int len, uint32_t *s,
		uint32_t color, uint32_t *m)
{
	argb8888_sp_argb8888_color_none_blend(d, len, s, color, m);
}

static void _argb8888_sp_none_color_argb8888_blend(uint32_t *d, unsigned int len,
		uint32_t *s, uint32_t color,
		uint32_t *m)
{
	argb8888_sp_none_color_argb8888_blend(d, len, s, color, m);
}

static void _argb8888_sp_none_color_a8_blend(uint32_t *d, unsigned int len,
		uint32_t *s, uint32_t color,
		uint8_t *m)
{
	argb8888_sp_none_color_a8_blend(d, len, s, color, m);
}

static void _argb8888_sp_argb8888_none_argb8888_blend(uint32_t *d, unsigned int len,
		uint32_t *s, uint32_t color,
		uint32_t *m)
{
	argb8888_sp_argb8888_none_argb8888_blend(d, len, s, color, m);
}

static void _span_register(void)
{
	/* color */
	enesim_compositor_span_color_register(
			_argb8888_sp_none_color_none_fill, ENESIM_ROP_FILL,
			ENESIM_FORMAT_ARGB8888);
	enesim_compositor_span_color_register(
			_argb8888_sp_none_color_none_blend, ENESIM_ROP_BLEND,
			ENESIM_FORMAT_ARGB8888);
	/* pixel */
	enesim_compositor_span_pixel_register(_argb8888_sp_argb8888_none_none_fill,
			ENESIM_ROP_FILL, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888);
	enesim_compositor_span_pixel_register(_argb8888_sp_argb8888_none_none_blend,
			ENESIM_ROP_BLEND, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888);
	/* mask color */
	enesim_compositor_span_mask_color_register(
			_argb8888_sp_none_color_argb8888_fill,
			ENESIM_ROP_FILL, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888);
	enesim_compositor_span_mask_color_register(
			ENESIM_COMPOSITOR_SPAN(_argb8888_sp_none_color_a8_fill),
			ENESIM_ROP_FILL, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_A8);
	enesim_compositor_span_mask_color_register(
			_argb8888_sp_none_color_argb8888_blend,
			ENESIM_ROP_BLEND, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888);
	enesim_compositor_span_mask_color_register(
			ENESIM_COMPOSITOR_SPAN(_argb8888_sp_none_color_a8_blend),
			ENESIM_ROP_BLEND, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_A8);
	/* pixel mask */
	enesim_compositor_span_pixel_mask_register(
			_argb8888_sp_argb8888_none_argb8888_fill,
			ENESIM_ROP_FILL, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888, ENESIM_FORMAT_ARGB8888);
	enesim_compositor_span_pixel_mask_register(
			_argb8888_sp_argb8888_none_argb8888_blend,
			ENESIM_ROP_BLEND, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888, ENESIM_FORMAT_ARGB8888);
	/* pixel color */
	enesim_compositor_span_pixel_color_register(
			_argb8888_sp_argb8888_color_none_fill,
			ENESIM_ROP_FILL, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888);
	enesim_compositor_span_pixel_color_register(
			_argb8888_sp_argb8888_color_none_blend,
			ENESIM_ROP_BLEND, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888);
}

static void _point_register(void)
{
	/* color */
	enesim_compositor_pt_color_register(
			_argb8888_pt_none_color_none_fill,
			ENESIM_ROP_FILL,
			ENESIM_FORMAT_ARGB8888);
	enesim_compositor_pt_color_register(
			_argb8888_pt_none_color_none_blend,
			ENESIM_ROP_BLEND,
			ENESIM_FORMAT_ARGB8888);
	/* mask color */
	enesim_compositor_pt_mask_color_register(
			_argb8888_pt_none_color_argb8888_fill,
			ENESIM_ROP_FILL, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888);
	enesim_compositor_pt_mask_color_register(
			_argb8888_pt_none_color_argb8888_blend,
			ENESIM_ROP_BLEND, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888);
	/* pixel mask */
	enesim_compositor_pt_pixel_mask_register(
			_argb8888_pt_argb8888_none_argb8888_fill,
			ENESIM_ROP_FILL, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888, ENESIM_FORMAT_ARGB8888);
	/* pixel */
	enesim_compositor_pt_pixel_register(
			_argb8888_pt_argb8888_none_none_fill,
			ENESIM_ROP_FILL, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888);
	enesim_compositor_pt_pixel_register(
			_argb8888_pt_argb8888_none_none_blend,
			ENESIM_ROP_BLEND, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888);
	/* pixel color */
	enesim_compositor_pt_pixel_color_register(
			_argb8888_pt_argb8888_color_none_fill,
			ENESIM_ROP_FILL, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_compositor_argb8888_init(void)
{
	_span_register();
	_point_register();
}

void enesim_compositor_argb8888_shutdown(void)
{
}

