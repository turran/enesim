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
#include "enesim_format.h"

#include "enesim_compositor_private.h"
#include "enesim_color_private.h"
#include "enesim_color_fill_private.h"
#include "enesim_color_blend_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
/*----------------------------------------------------------------------------*
 *                           Fill point funcitons                            *
 *----------------------------------------------------------------------------*/
static void _argb8888_pt_none_color_none_fill(uint32_t *d,
		uint32_t s EINA_UNUSED, uint32_t color, uint32_t m EINA_UNUSED)
{
	enesim_color_fill_pt_none_color_none(d, color);
}

static void _argb8888_pt_none_color_argb8888_fill(uint32_t *d,
		uint32_t s EINA_UNUSED, uint32_t color, uint32_t m)
{
	enesim_color_fill_pt_none_color_argb8888(d, color, m);
}

static void _argb8888_pt_argb8888_none_argb8888_fill(uint32_t *d,
		uint32_t s, uint32_t color EINA_UNUSED, uint32_t m)
{
	enesim_color_fill_pt_argb8888_none_argb8888(d, s, m);
}

static void _argb8888_pt_argb8888_none_none_fill(uint32_t *d,
		uint32_t s, uint32_t color EINA_UNUSED, uint32_t m EINA_UNUSED)
{
	enesim_color_fill_pt_argb8888_none_none(d, s);
}

static void _argb8888_pt_argb8888_color_none_fill(uint32_t *d,
		uint32_t s, uint32_t color, uint32_t m EINA_UNUSED)
{
	enesim_color_fill_pt_argb8888_color_none(d, s, color);
}
/*----------------------------------------------------------------------------*
 *                            Fill span funcitons                            *
 *----------------------------------------------------------------------------*/
static void _argb8888_sp_none_color_none_fill(uint32_t *d, uint32_t len,
		uint32_t *s EINA_UNUSED, uint32_t color,
		uint32_t *m EINA_UNUSED)
{
	enesim_color_fill_sp_none_color_none(d, len, color);
}

static void _argb8888_sp_argb8888_none_none_fill(uint32_t *d, uint32_t len,
		uint32_t *s, uint32_t color EINA_UNUSED,
		uint32_t *m EINA_UNUSED)
{
	enesim_color_fill_sp_argb8888_none_none(d, len, s);
}

static void _argb8888_sp_argb8888_color_none_fill(uint32_t *d, uint32_t len,
		uint32_t *s, uint32_t color EINA_UNUSED,
		uint32_t *m EINA_UNUSED)
{
	enesim_color_fill_sp_argb8888_color_none(d, len, s, color);
}

static void _argb8888_sp_none_color_argb8888_alpha_fill(uint32_t *d, uint32_t len,
		uint32_t *s EINA_UNUSED, uint32_t color, uint32_t *m)
{
	enesim_color_fill_sp_none_color_argb8888_alpha(d, len, color, m);
}

static void _argb8888_sp_none_color_a8_alpha_fill(uint32_t *d, uint32_t len,
		uint32_t *s EINA_UNUSED, uint32_t color, uint8_t *m)
{
	enesim_color_fill_sp_none_color_a8_alpha(d, len, color, m);
}

static inline void _argb8888_sp_argb8888_none_argb8888_alpha_fill(uint32_t *d,
		uint32_t len, uint32_t *s, uint32_t color EINA_UNUSED,
		uint32_t *m)
{
	enesim_color_fill_sp_argb8888_none_argb8888_alpha(d, len, s, m);
}

static inline void _argb8888_sp_argb8888_none_argb8888_luminance_fill(uint32_t *d,
		uint32_t len, uint32_t *s, uint32_t color EINA_UNUSED,
		uint32_t *m)
{
	enesim_color_fill_sp_argb8888_none_argb8888_luminance(d, len, s, m);
}
/*----------------------------------------------------------------------------*
 *                           Blend point funcitons                            *
 *----------------------------------------------------------------------------*/
static void _argb8888_pt_none_color_none_blend(uint32_t *d,
		uint32_t s EINA_UNUSED, uint32_t color, uint32_t m EINA_UNUSED)
{
	enesim_color_blend_pt_none_color_none(d, color);
}

static void _argb8888_pt_none_color_argb8888_blend(uint32_t *d,
		uint32_t s EINA_UNUSED, uint32_t color, uint32_t m)
{
	enesim_color_blend_pt_none_color_argb8888_alpha(d, color, m);
}

static void _argb8888_pt_argb8888_none_none_blend(uint32_t *d,
		uint32_t s, uint32_t color EINA_UNUSED, uint32_t m EINA_UNUSED)
{
	enesim_color_blend_pt_argb8888_none_none(d, s);
}
/*----------------------------------------------------------------------------*
 *                            Blend span funcitons                            *
 *----------------------------------------------------------------------------*/
static void _argb8888_sp_none_color_none_blend(uint32_t *d,
		unsigned int len, uint32_t *s EINA_UNUSED,
		uint32_t color, uint32_t *m EINA_UNUSED)
{
#if BUILD_ORC
	argb8888_sp_none_color_none_blend_orc(d, color, len);
#else
	enesim_color_blend_sp_none_color_none(d, len, color);
#endif
}

static void _argb8888_sp_argb8888_none_none_blend(uint32_t *d,
		unsigned int len, uint32_t *s,
		uint32_t color EINA_UNUSED, uint32_t *m EINA_UNUSED)
{
#if BUILD_ORC
	argb8888_sp_argb8888_none_none_blend_orc(d, s, len);
#else
	enesim_color_blend_sp_argb8888_none_none(d, len, s);
#endif
}

static void _argb8888_sp_argb8888_color_none_blend(uint32_t *d,
		unsigned int len, uint32_t *s,
		uint32_t color, uint32_t *m EINA_UNUSED)
{
#if BUILD_ORC
	argb8888_sp_argb8888_color_none_blend_orc(d, s, color, len);
#else
	enesim_color_blend_sp_argb8888_color_none(d, len, s, color);
#endif
}

static void _argb8888_sp_none_color_argb8888_alpha_blend(uint32_t *d,
		unsigned int len, uint32_t *s EINA_UNUSED, uint32_t color,
		uint32_t *m)
{
#if BUILD_ORC
	argb8888_sp_none_color_argb8888_alpha_blend_orc(d, m, color, len);
#else
	enesim_color_blend_sp_none_color_argb8888_alpha(d, len, color, m);
#endif
}

static void _argb8888_sp_none_color_a8_alpha_blend(uint32_t *d,
		unsigned int len, uint32_t *s EINA_UNUSED, uint32_t color,
		uint8_t *m)
{
	enesim_color_blend_sp_none_color_a8_alpha(d, len, color, m);
}

static void _argb8888_sp_argb8888_none_argb8888_alpha_blend(uint32_t *d,
		unsigned int len, uint32_t *s, uint32_t color EINA_UNUSED,
		uint32_t *m)
{
	enesim_color_blend_sp_argb8888_none_argb8888_alpha(d, len, s, m);
}

static void _argb8888_sp_argb8888_none_argb8888_luminance_blend(uint32_t *d,
		unsigned int len, uint32_t *s, uint32_t color EINA_UNUSED,
		uint32_t *m)
{
	enesim_color_blend_sp_argb8888_none_argb8888_luminance(d, len, s, m);
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
			_argb8888_sp_none_color_argb8888_alpha_fill,
			ENESIM_ROP_FILL, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888, ENESIM_CHANNEL_ALPHA);
	enesim_compositor_span_mask_color_register(
			ENESIM_COMPOSITOR_SPAN(_argb8888_sp_none_color_a8_alpha_fill),
			ENESIM_ROP_FILL, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_A8, ENESIM_CHANNEL_ALPHA);
	enesim_compositor_span_mask_color_register(
			_argb8888_sp_none_color_argb8888_alpha_blend,
			ENESIM_ROP_BLEND, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888, ENESIM_CHANNEL_ALPHA);
	enesim_compositor_span_mask_color_register(
			ENESIM_COMPOSITOR_SPAN(_argb8888_sp_none_color_a8_alpha_blend),
			ENESIM_ROP_BLEND, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_A8, ENESIM_CHANNEL_ALPHA);
	/* pixel mask */
	enesim_compositor_span_pixel_mask_register(
			_argb8888_sp_argb8888_none_argb8888_alpha_fill,
			ENESIM_ROP_FILL, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888, ENESIM_FORMAT_ARGB8888,
			ENESIM_CHANNEL_ALPHA);
	enesim_compositor_span_pixel_mask_register(
			_argb8888_sp_argb8888_none_argb8888_luminance_fill,
			ENESIM_ROP_FILL, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888, ENESIM_FORMAT_ARGB8888,
			ENESIM_CHANNEL_LUMINANCE);
	enesim_compositor_span_pixel_mask_register(
			_argb8888_sp_argb8888_none_argb8888_alpha_blend,
			ENESIM_ROP_BLEND, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888, ENESIM_FORMAT_ARGB8888,
			ENESIM_CHANNEL_ALPHA);
	enesim_compositor_span_pixel_mask_register(
			_argb8888_sp_argb8888_none_argb8888_luminance_blend,
			ENESIM_ROP_BLEND, ENESIM_FORMAT_ARGB8888,
			ENESIM_FORMAT_ARGB8888, ENESIM_FORMAT_ARGB8888,
			ENESIM_CHANNEL_LUMINANCE);
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
/** @endcond */
