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

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_convolve.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_color_private.h"
#include "enesim_renderer_private.h"
#include "enesim_draw_cache_private.h"
#include "enesim_coord_private.h"
/*
 * A matrix convolution filter.
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */

#define ENESIM_RENDERER_CONVOLVE(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Convolve,					\
		enesim_renderer_convolve_descriptor_get())



typedef struct _Enesim_Renderer_Convolve
{
	Enesim_Renderer parent;
	/* properties */
	Enesim_Surface *src;
	Enesim_Renderer *src_r;
	Enesim_Convolve_Channel channel;
	int nrows, ncolumns;
	double *mvalues;
	double divisor;
	int cx, cy;

	/* The state variables */
	int *imvalues;
	int icx, icy;

	Enesim_Color color;
	Enesim_Renderer *mask;
	Eina_Bool do_mask;

	/* private */
	uint32_t *ssrc;
	size_t sstride;
	Enesim_Draw_Cache *cache;
	Eina_Bool changed;
} Enesim_Renderer_Convolve;

typedef struct _Enesim_Renderer_Convolve_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Convolve_Class;

static Eina_Bool _convolve_state_setup(Enesim_Renderer_Convolve *thiz,
		Enesim_Renderer *r, Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Log **l)
{
	if (!thiz->src && !thiz->src_r)
	{
		ENESIM_RENDERER_LOG(r, l, "No surface or renderer set");
		return EINA_FALSE;
	}

	/* Set the renderer */
	if (thiz->src_r)
	{
		Enesim_Renderer *old_r;

		if (!enesim_renderer_setup(thiz->src_r, s, rop, l))
			return EINA_FALSE;
		enesim_draw_cache_renderer_get(thiz->cache, &old_r);
		if (old_r != thiz->src_r)
			enesim_draw_cache_renderer_set(thiz->cache, enesim_renderer_ref(thiz->src_r));
		if (old_r)
			enesim_renderer_unref(old_r);
	}
	else if (thiz->src)
	{
		/* lock the surface for read only */
		enesim_surface_lock(thiz->src, EINA_FALSE);
	}
	return EINA_TRUE;
}

static void _convolve_state_cleanup(Enesim_Renderer_Convolve *thiz,
		Enesim_Renderer *r EINA_UNUSED, Enesim_Surface *s)
{
	if (thiz->src_r)
	{
		enesim_renderer_cleanup(thiz->src_r, s);
	}
	else if (thiz->src)
	{
		enesim_surface_unlock(thiz->src);
	}
	thiz->changed = EINA_FALSE;
}

static Enesim_Renderer_Sw_Fill _spans[ENESIM_CONVOLVE_CHANNELS];
/*----------------------------------------------------------------------------*
 *                        The Software fill variants                          *
 *----------------------------------------------------------------------------*/
static void _argb8888_span_identity(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Convolve *thiz;
	Enesim_Color color;
//	double ox, oy;
	uint32_t *d = ddata;
	uint32_t *e = d + len;
	uint32_t *src, *pbuf;
	size_t sstride;
	int sw, sh;
	int ix, ix0, iy, iy0;
	int *mvals;
	int mw, mh;

	thiz = ENESIM_RENDERER_CONVOLVE(r);
	color = thiz->color;
	if (color == 0xffffffff)
		color = 0;

	if (thiz->do_mask)
		enesim_renderer_sw_draw(thiz->mask, x, y, len, d);

	/* setup the parameters */

	if (thiz->src_r)
	{
		Eina_Rectangle bounds;
		Eina_Rectangle area;
		Enesim_Buffer_Sw_Data sw_data;

		enesim_renderer_destination_bounds_get(thiz->src_r, &bounds, 0, 0);
		sw = bounds.w;
		sh = bounds.h;
		eina_rectangle_coords_from(&area, x, y, len + thiz->ncolumns, thiz->nrows);
		enesim_draw_cache_map_sw(thiz->cache, &area, &sw_data);
		src = sw_data.argb8888.plane0;
		sstride = sw_data.argb8888.plane0_stride;
	}
	else
	{
		enesim_surface_size_get(thiz->src, &sw, &sh);
		src = thiz->ssrc;
		sstride = thiz->sstride;
	}

	if ((y < 0) || (y >= sh) || (x >= sw) || (x + len < 0))
	{
		memset(d, 0, len * sizeof(unsigned int));
		return;
	}
	if (x < 0)
	{
		memset(d, 0, -x * sizeof(unsigned int));
		len += x;
		d -= x;
		x = 0;
	}
	if ((x + len) > sw)
	{
		memset(d + sw - x, 0, (x + len - sw) * sizeof(unsigned int));
		e -= (x + len - sw);
	}

	mh = thiz->nrows;
	mw = thiz->ncolumns;
	ix = ix0 = x - thiz->icx;
	iy = iy0 = y - thiz->icy;

	src = enesim_color_at(src, sstride, ix, iy);
	mvals = thiz->imvalues;
	while (d < e)
	{
		unsigned int p0 = 0;
		int ma = 255;

		if (thiz->do_mask)
			ma = (*d) >> 24;
		if (ma)
		{
			int a0 = 0, r0 = 0, g0 = 0, b0 = 0;
			int *bmvals = mvals;
			int tmh = mh;

			pbuf = src + (ix - ix0);
			iy = iy0;

			while (tmh--)
			{
				unsigned int *p = pbuf;
				int *pmvals = bmvals;
				int px = ix, tmw = mw;

				while (tmw--)
				{
					unsigned int p3 = 0;

					if (((unsigned)px < sw) & ((unsigned)iy < sh))
						p3 = *p;
					if (p3)
					{
						int a = *pmvals;

						a0 += ((p3 >> 24) * a);
						r0 += (((p3 & 0xff0000) >> 16) * a);
						g0 += (((p3 & 0xff00) >> 8) * a);
						b0 += ((p3 & 0xff) * a);
					}
					p++;  px++;  pmvals++;
				}
				pbuf += sw;  iy++;
				bmvals += mw;
			}
			a0 = (a0 + 0xffff) >> 16;  r0 = (r0 + 0xffff) >> 16;
			g0 = (g0 + 0xffff) >> 16;  b0 = (b0 + 0xffff) >> 16;

			a0 = (a0 | (-(a0 >> 8))) & (~(a0 >> 9));
			r0 = (r0 | (-(r0 >> 8))) & (~(r0 >> 9));
			g0 = (g0 | (-(g0 >> 8))) & (~(g0 >> 9));
			b0 = (b0 | (-(b0 >> 8))) & (~(b0 >> 9));

			p0 = (a0<<24) | (r0<<16) | (g0<<8) | b0;

			if (color)
				p0 = enesim_color_mul4_sym(p0, color);
			if (ma < 255)
				p0 = enesim_color_mul_sym(ma, p0);
		}
		*d++ = p0;  ix++;
	}
}

static void _xrgb8888_span_identity(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Convolve *thiz;
	Enesim_Color color;
//	double ox, oy;
	uint32_t *d = ddata;
	uint32_t *e = d + len;
	uint32_t *src, *pbuf;
	size_t sstride;
	int sw, sh;
	int ix, ix0, iy, iy0;
	int *mvals;
	int mw, mh;

	thiz = ENESIM_RENDERER_CONVOLVE(r);
	if (thiz->src_r)
	{
		Eina_Rectangle bounds;
		Eina_Rectangle area;
		Enesim_Buffer_Sw_Data sw_data;

		enesim_renderer_destination_bounds_get(thiz->src_r, &bounds, 0, 0);
		sw = bounds.w;
		sh = bounds.h;
		eina_rectangle_coords_from(&area, x, y, len + thiz->ncolumns, thiz->nrows);
		enesim_draw_cache_map_sw(thiz->cache, &area, &sw_data);
		src = sw_data.argb8888.plane0;
		sstride = sw_data.argb8888.plane0_stride;
	}
	else
	{
		enesim_surface_size_get(thiz->src, &sw, &sh);
		src = thiz->ssrc;
		sstride = thiz->sstride;
	}

	if ((y < 0) || (y >= sh) || (x >= sw) || (x + len < 0))
	{
		memset(d, 0, len * sizeof(unsigned int));
		return;
	}
	if (x < 0)
	{
		memset(d, 0, -x * sizeof(unsigned int));
		len += x;
		d -= x;
		x = 0;
	}
	if ((x + len) > sw)
	{
		memset(d + sw - x, 0, (x + len - sw) * sizeof(unsigned int));
		e -= (x + len - sw);
	}

	color = thiz->color;
	if (color == 0xffffffff)
		color = 0;

	if (thiz->do_mask)
		enesim_renderer_sw_draw(thiz->mask, x, y, len, d);

	mh = thiz->nrows;
	mw = thiz->ncolumns;
	ix = ix0 = x - thiz->icx;
	iy = iy0 = y - thiz->icy;

	src = enesim_color_at(src, sstride, ix, iy);
	mvals = thiz->imvalues;
	while (d < e)
	{
		unsigned int p0 = 0;
		int ma = 255;

		if (thiz->do_mask)
			ma = (*d) >> 24;
		if (ma)
		{
			int r0 = 0, g0 = 0, b0 = 0;
			int *bmvals = mvals;
			int tmh = mh;

			pbuf = src + (ix - ix0);
			iy = iy0;

			while (tmh--)
			{
				unsigned int *p = pbuf;
				int *pmvals = bmvals;
				int px = ix, tmw = mw;

				while (tmw--)
				{
					unsigned int p3 = 0;

					if (((unsigned)px < sw) & ((unsigned)iy < sh))
						p3 = *p;
					if (p3)
					{
						int a = *pmvals;

						r0 += (((p3 & 0xff0000) >> 16) * a);
						g0 += (((p3 & 0xff00) >> 8) * a);
						b0 += ((p3 & 0xff) * a);
					}
					p++;  px++;  pmvals++;
				}
				pbuf += sw;  iy++;
				bmvals += mw;
			}
			r0 = (r0 + 0xffff) >> 16;
			g0 = (g0 + 0xffff) >> 16;
			b0 = (b0 + 0xffff) >> 16;

			r0 = (r0 | (-(r0 >> 8))) & (~(r0 >> 9));
			g0 = (g0 | (-(g0 >> 8))) & (~(g0 >> 9));
			b0 = (b0 | (-(b0 >> 8))) & (~(b0 >> 9));

			p0 = 0xff000000 | (r0<<16) | (g0<<8) | b0;

			if (color)
				p0 = enesim_color_mul4_sym(p0, color);
			if (ma < 255)
				p0 = enesim_color_mul_sym(ma, p0);
		}
		*d++ = p0;  ix++;
	}
}

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _convolve_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "convolve";
}

static Eina_Bool _convolve_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop, 
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l)
{
	Enesim_Renderer_Convolve *thiz;
	double d;
	int len;
	double *mval;
	int *imval;

	thiz = ENESIM_RENDERER_CONVOLVE(r);
	if (!thiz->mvalues)
		return EINA_FALSE;
	if (!_convolve_state_setup(thiz, r, s, rop, l))
		return EINA_FALSE;
	if (thiz->src_r)
		enesim_draw_cache_setup_sw(thiz->cache, ENESIM_FORMAT_ARGB8888, NULL);
	else
	{
		if (!enesim_surface_map(thiz->src, (void **)&thiz->ssrc, &thiz->sstride))
		{
			_convolve_state_cleanup(thiz, r, s);
			return EINA_FALSE;
		}
	}

	len = thiz->nrows * thiz->ncolumns;
	if (!thiz->imvalues)
	{
		thiz->imvalues = malloc(len * sizeof(int));
		if (!thiz->imvalues)
			return EINA_FALSE;
	}

	if (thiz->divisor)
		d = thiz->divisor;
	else
	{
		d = 0;
		mval = thiz->mvalues;
		while (len--)
		{
			d += *mval;
			mval++;
		}
	}
	/* create our integer 16.16 matrix */
	/* and reverse the entries for our purposes */
	if (fabs(d) < 1/65536.0)
		d = 1;
	len = thiz->nrows * thiz->ncolumns;
	mval = thiz->mvalues;
	imval = thiz->imvalues + len - 1;
	while (len--)
	{
		*imval = 65536 * ((*mval) / d);
		imval--;  mval++;
	}

	if ((thiz->cx < 0) || (thiz->cy < 0))
	{
		thiz->icy = thiz->nrows / 2;
		thiz->icx = thiz->ncolumns / 2;
	}
	else
	{
		thiz->icx = thiz->cx;
		thiz->icy = thiz->cy;
		if (thiz->icy > (thiz->nrows - 1))
			thiz->icy = thiz->nrows - 1;
		if (thiz->icx > (thiz->ncolumns - 1))
			thiz->icx = thiz->ncolumns - 1;
	}

	thiz->color = enesim_renderer_color_get(r);
	thiz->mask = enesim_renderer_mask_get(r);
	thiz->do_mask = 0;
	if (thiz->mask)
	{
		Enesim_Channel mchannel;

		mchannel = enesim_renderer_mask_channel_get(r);
		if (mchannel == ENESIM_CHANNEL_ALPHA)
			thiz->do_mask = 1;
	}

	*fill = _spans[thiz->channel];
	if (!*fill) return EINA_FALSE;

	return EINA_TRUE;
}


static void _convolve_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Convolve *thiz;

	thiz = ENESIM_RENDERER_CONVOLVE(r);
	if (!thiz->src_r)
		enesim_surface_unmap(thiz->src, thiz->ssrc, EINA_FALSE);
	if (thiz->mask)
		enesim_renderer_unref(thiz->mask);
	thiz->mask = NULL;
	_convolve_state_cleanup(thiz, r, s);
}

static void _convolve_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_ARGB8888 | ENESIM_RENDERER_FEATURE_TRANSLATE;
}

static void _convolve_sw_hints_get(Enesim_Renderer *r,
		Enesim_Rop rop EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	Enesim_Channel mchannel;

	*hints = ENESIM_RENDERER_SW_HINT_COLORIZE;

	mchannel = enesim_renderer_mask_channel_get(r);
	if (mchannel == ENESIM_CHANNEL_ALPHA)
		*hints |= ENESIM_RENDERER_SW_HINT_MASK;
}

static Eina_Bool _convolve_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Convolve *thiz;

	thiz = ENESIM_RENDERER_CONVOLVE(r);
	if (!thiz->changed) return EINA_FALSE;
	return EINA_TRUE;
}

static void _convolve_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Convolve *thiz;
	double ox, oy;

	thiz = ENESIM_RENDERER_CONVOLVE(r);
	/* in case we have a renderer, use it */
	if (thiz->src_r)
	{
		Eina_Rectangle bounds;
		enesim_renderer_destination_bounds_get(thiz->src_r, &bounds, 0, 0);
		enesim_rectangle_coords_from(rect, 0, 0, bounds.w, bounds.h);
	}
	/* otherwise use the surface */
	else if (thiz->src)
	{
		int sw, sh;

		enesim_surface_size_get(thiz->src, &sw, &sh);
		enesim_rectangle_coords_from(rect, 0, 0, sw, sh);
	}
	else
	{
		enesim_rectangle_coords_from(rect, 0, 0, 0, 0);
		return;
	}
	/* translate */
//	enesim_renderer_origin_get(r, &ox, &oy);
//	rect->x += ox;
//	rect->y += oy;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Convolve, Enesim_Renderer_Convolve_Class,
		enesim_renderer_convolve);

static void _enesim_renderer_convolve_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _convolve_name;
	klass->bounds_get = _convolve_bounds_get;
	klass->features_get = _convolve_features_get;
	klass->has_changed =  _convolve_has_changed;
	klass->sw_hints_get = _convolve_sw_hints_get;
	klass->sw_setup = _convolve_sw_setup;
	klass->sw_cleanup = _convolve_sw_cleanup;
	/* initialize the static information */
	_spans[ENESIM_CONVOLVE_CHANNEL_ARGB]
		= _argb8888_span_identity;
	_spans[ENESIM_CONVOLVE_CHANNEL_XRGB]
		= _xrgb8888_span_identity;
}

static void _enesim_renderer_convolve_instance_init(void *o)
{
	Enesim_Renderer_Convolve *thiz = ENESIM_RENDERER_CONVOLVE(o);

	thiz->cache = enesim_draw_cache_new();
	/* initial properties */
	thiz->cx = thiz->cy = -1;
	thiz->divisor = 0;
	thiz->channel = ENESIM_CONVOLVE_CHANNEL_ARGB;
}

static void _enesim_renderer_convolve_instance_deinit(void *o EINA_UNUSED)
{
	Enesim_Renderer_Convolve *thiz = ENESIM_RENDERER_CONVOLVE(o);

	if (thiz->src)
	{
		enesim_surface_unref(thiz->src);
		thiz->src = NULL;
	}

	if (thiz->src_r)
	{
		enesim_renderer_unref(thiz->src_r);
		thiz->src_r = NULL;
	}

	if (thiz->cache)
	{
		enesim_draw_cache_free(thiz->cache);
		thiz->cache = NULL;
	}
	if (thiz->mvalues)
		free(thiz->mvalues);
	if (thiz->imvalues)
		free(thiz->imvalues);
	thiz->mvalues = NULL;
	thiz->imvalues = NULL;

}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Creates a new convolve filter renderer
 * @return The renderer
 */
EAPI Enesim_Renderer * enesim_renderer_convolve_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_convolve);
	return r;
}

/**
 * @brief Sets the channel to use in the source data
 * @ender_prop{channel}
 * @param[in] r The convolve filter renderer
 * @param[in] channel The channel to use
 */
EAPI void enesim_renderer_convolve_channel_set(Enesim_Renderer *r,
	Enesim_Convolve_Channel channel)
{
	Enesim_Renderer_Convolve *thiz;

	thiz = ENESIM_RENDERER_CONVOLVE(r);

	if (thiz->channel == channel)
		return;
	thiz->channel = channel;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the channel used in the source data
 * @ender_prop{channel}
 * @param[in] r The convolve filter renderer
 * @return the channel used
 */
EAPI Enesim_Convolve_Channel enesim_renderer_convolve_channel_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Convolve *thiz;

	thiz = ENESIM_RENDERER_CONVOLVE(r);

	return thiz->channel;
}

/**
 * @brief Sets the source surface to use as the source data
 * @ender_prop{source_surface}
 * @param[in] r The convolve filter renderer
 * @param[in] src The surface to use @ender_transfer{full}
 */
EAPI void enesim_renderer_convolve_source_surface_set(Enesim_Renderer *r, Enesim_Surface *src)
{
	Enesim_Renderer_Convolve *thiz;

	thiz = ENESIM_RENDERER_CONVOLVE(r);
	if (thiz->src)
	{
		enesim_surface_unref(thiz->src);
		thiz->src = NULL;
	}
	thiz->src = src;
	thiz->changed = EINA_TRUE;
}

/**
 * @ender_prop{source_surface}
 * @brief Gets the source surface used as the source data
 * @param[in] r The convolve filter renderer
 * @return The surface to convolve @ender_transfer{none}
 */
EAPI Enesim_Surface * enesim_renderer_convolve_source_surface_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Convolve *thiz;

	thiz = ENESIM_RENDERER_CONVOLVE(r);
	return enesim_surface_ref(thiz->src);
}

/**
 * @brief Sets the source renderer to use as the source data
 * @ender_prop{source_renderer}
 * @param[in] r The convolve filter renderer
 * @param[in] sr The renderer to use @ender_transfer{full}
 */
EAPI void enesim_renderer_convolve_source_renderer_set(Enesim_Renderer *r, Enesim_Renderer *sr)
{
	Enesim_Renderer_Convolve *thiz;

	thiz = ENESIM_RENDERER_CONVOLVE(r);
	if (thiz->src_r)
	{
		enesim_renderer_unref(thiz->src_r);
		thiz->src_r = NULL;
	}
	thiz->src_r = sr;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the source renderer used as the source data
 * @ender_prop{source_renderer}
 * @param[in] r The convolve filter renderer
 * @return The renderer to convolve @ender_transfer{none}
 */
EAPI Enesim_Renderer * enesim_renderer_convolve_source_renderer_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Convolve *thiz;

	thiz = ENESIM_RENDERER_CONVOLVE(r);
	return enesim_renderer_ref(thiz->src_r);
}

/**
 * @brief Sets the convolve matrix size. Rows x Columns must be <= 1024
 * @param[in] r The convolve filter renderer
 * @param[in] nrows The convolve matrix's number of rows
 * @param[in] ncolumns The convolve matrix's number of columns
 */
EAPI void enesim_renderer_convolve_matrix_size_set(Enesim_Renderer *r, int nrows, int ncolumns)
{
	Enesim_Renderer_Convolve *thiz;
	int valid = 1;

	thiz = ENESIM_RENDERER_CONVOLVE(r);
	if ((nrows <= 0) || (ncolumns <= 0))
		valid = 0;
	if ((nrows > 1024) || (ncolumns > 1024))
		valid = 0;
	if ((nrows * ncolumns) > 1024)
		valid = 0;
	if (!valid)
	{
		if (thiz->mvalues)
			free(thiz->mvalues);
		if (thiz->imvalues)
			free(thiz->imvalues);
		thiz->mvalues = NULL;
		thiz->imvalues = NULL;
		thiz->nrows = thiz->ncolumns = 0;
		return;
	}
	if ((thiz->nrows != nrows) || (thiz->ncolumns != ncolumns))
	{
		if (thiz->mvalues)
			free(thiz->mvalues);
		if (thiz->imvalues)
			free(thiz->imvalues);
		thiz->mvalues = NULL;
		thiz->imvalues = NULL;
	}
	if (!thiz->mvalues)
		thiz->mvalues = malloc(nrows * ncolumns * sizeof(double));
	thiz->nrows = nrows;
	thiz->ncolumns = ncolumns;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the convolve matrix size used
 * @param[in] r The convolve filter renderer
 * @param[in] *nrows The convolve matrix's number of rows used
 * @param[in] *ncolumns The convolve matrix's number of columns used
 */
EAPI void enesim_renderer_convolve_matrix_size_get(Enesim_Renderer *r, int *nrows, int *ncolumns)
{
	Enesim_Renderer_Convolve *thiz;

	thiz = ENESIM_RENDERER_CONVOLVE(r);
	if (nrows) *nrows = thiz->nrows;
	if (ncolumns) *ncolumns = thiz->ncolumns;
}

/**
 * @brief Sets the convolve matrix value to use.
 * @param[in] r The convolve filter renderer
 * @param[in] val The value of the convolve matrix to use at this i,j position
 * @param[in] i The row index of the convolve matrix
 * @param[in] j The column index of theconvolve matrix
 */
EAPI void enesim_renderer_convolve_matrix_value_set(Enesim_Renderer *r, double val, int i, int j)
{
	Enesim_Renderer_Convolve *thiz;

	thiz = ENESIM_RENDERER_CONVOLVE(r);

	if (!thiz->mvalues) return;
	if ((i < 0) || (i >= thiz->nrows)) return;
	if ((j < 0) || (j >= thiz->ncolumns)) return;
	*(thiz->mvalues + (i * (thiz->ncolumns)) + j) = val;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the convolve matrix value used.
 * @param[in] r The convolve filter renderer
 * @param[in] i The row index of the convolve matrix
 * @param[in] j The column index of the convolve matrix
 * @return The value of the convolve matrix at this i,j position
 */
EAPI double enesim_renderer_convolve_matrix_value_get(Enesim_Renderer *r, int i, int j)
{
	Enesim_Renderer_Convolve *thiz;

	thiz = ENESIM_RENDERER_CONVOLVE(r);

	if (!thiz->mvalues) return 0;
	if ((i < 0) || (i >= thiz->nrows)) return 0;
	if ((j < 0) || (j >= thiz->ncolumns)) return 0;
	return *(thiz->mvalues + (i * (thiz->ncolumns)) + j);
}

/**
 * @brief Sets the convolve center in the x direction
 * @ender_prop{center_x}
 * @param[in] r The convolve filter renderer
 * @param[in] x The convolve center in the x direction
 */
EAPI void enesim_renderer_convolve_center_x_set(Enesim_Renderer *r, int x)
{
	Enesim_Renderer_Convolve *thiz;

	thiz = ENESIM_RENDERER_CONVOLVE(r);

	thiz->cx = x;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the convolve center used in the x direction
 * @ender_prop{center_x}
 * @param[in] r The convolve filter renderer
 * @return the convolve x direction center used
 */
EAPI int enesim_renderer_convolve_center_x_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Convolve *thiz;

	thiz = ENESIM_RENDERER_CONVOLVE(r);

	return thiz->cx;
}

/**
 * @brief Sets the convolve center in the y direction
 * @ender_prop{center_y}
 * @param[in] r The convolve filter renderer
 * @param[in] y The convolve center in the y direction
 */
EAPI void enesim_renderer_convolve_center_y_set(Enesim_Renderer *r, int y)
{
	Enesim_Renderer_Convolve *thiz;

	thiz = ENESIM_RENDERER_CONVOLVE(r);

	thiz->cy = y;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the convolve center used in the y direction
 * @ender_prop{center_y}
 * @param[in] r The convolve filter renderer
 * @return the convolve y direction center used
 */
EAPI int enesim_renderer_convolve_center_y_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Convolve *thiz;

	thiz = ENESIM_RENDERER_CONVOLVE(r);

	return thiz->cy;
}

/**
 * @brief Sets the convolve divisor
 * @ender_prop{divisor}
 * @param[in] r The convolve filter renderer
 * @param[in] d The convolve divisor
 */
EAPI void enesim_renderer_convolve_divisor_set(Enesim_Renderer *r, double d)
{
	Enesim_Renderer_Convolve *thiz;

	thiz = ENESIM_RENDERER_CONVOLVE(r);

	thiz->divisor = d;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the convolve divisor used
 * @ender_prop{divisor}
 * @param[in] r The convolve filter renderer
 * @return the convolve divisor used
 */
EAPI double enesim_renderer_convolve_divisor_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Convolve *thiz;

	thiz = ENESIM_RENDERER_CONVOLVE(r);

	return thiz->divisor;
}

