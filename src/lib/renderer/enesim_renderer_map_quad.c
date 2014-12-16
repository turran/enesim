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
#include "enesim_quad.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_color_private.h"
#include "enesim_renderer_private.h"
#include "enesim_draw_cache_private.h"
#include "enesim_coord_private.h"
/*
 * A colors and/or texture mapped quad renderer.
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */

#define ENESIM_RENDERER_MAP_QUAD(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Map_Quad,					\
		enesim_renderer_map_quad_descriptor_get())

typedef struct _Enesim_Renderer_Map_Quad
{
	Enesim_Renderer parent;
	/* properties */
	Enesim_Surface *src;
	Enesim_Argb vcolors[4];
	double vx[4], vy[4];

	/* The state variables */
	/* private */
	double qx[4], qy[4];
	int sgn[4];

	uint32_t *ssrc;
	size_t sstride;
	int sw, sh;

	uint32_t *src_c;
	size_t stride_c;
	int sw_c, sh_c;

	Enesim_Matrix it, ct;
	int  lx, rx, ty, by;

	Enesim_Color rcolor;

	Eina_Bool changed;
} Enesim_Renderer_Map_Quad;

typedef struct _Enesim_Renderer_Map_Quad_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Map_Quad_Class;

static Eina_Bool _map_quad_state_setup(Enesim_Renderer_Map_Quad *thiz)
{
	if (thiz->src)
	{
		/* lock the surface for read only */
		enesim_surface_lock(thiz->src, EINA_FALSE);
	}
	return EINA_TRUE;
}

static void _map_quad_state_cleanup(Enesim_Renderer_Map_Quad *thiz)
{
	if (thiz->src)
	{
		enesim_surface_unlock(thiz->src);
	}
	thiz->changed = EINA_FALSE;
}

static inline int _map_quad_get_endpoints(Enesim_Renderer_Map_Quad *thiz,
		int y, int *lx, int *rx)
{
	int active = 0;
	int i = 0;

	*rx = thiz->lx;
	*lx = thiz->rx;

	while (i < 4)
	{
		int j = i+1;

		if (i == 3)  j = 0;

		if (((thiz->qy[i] < (y + 1)) && (thiz->qy[j] >= y)) ||
			((thiz->qy[j] < (y + 1)) && (thiz->qy[i] >= y)))
		{
			double lxt, rxt;

			active++;

			if (thiz->sgn[i])
			{
				double d2 = (thiz->qx[j] - thiz->qx[i]) / (thiz->qy[j] - thiz->qy[i]);

				/* get the intersection to the edges */
				if (thiz->sgn[i] > 0)
				{
					lxt = (d2 * (y - thiz->qy[i])) + thiz->qx[i];
					rxt = (d2 * (y + 1 - thiz->qy[i])) + thiz->qx[i];
				}
				else
				{
					lxt = (d2 * (y + 1 - thiz->qy[i])) + thiz->qx[i];
					rxt = (d2 * (y - thiz->qy[i])) + thiz->qx[i];
				}
				/* make sure it is not out of x-bounds */
				if (lxt < MIN(thiz->qx[i], thiz->qx[j]))
					lxt = MIN(thiz->qx[i], thiz->qx[j]);
				if (rxt > MAX(thiz->qx[i], thiz->qx[j]))
					rxt = MAX(thiz->qx[i], thiz->qx[j]);
			}
			else
			{
				lxt = MIN(thiz->qx[i], thiz->qx[j]);
				rxt = MAX(thiz->qx[i], thiz->qx[j]);
			}
			/* get the min/max */
			if (*lx > lxt)
				*lx = lxt;
			if (*rx < rxt)
				*rx = rxt;
		}
		i++;
	}

	return active;
}
/*----------------------------------------------------------------------------*
 *                        The Software fill variants                          *
 *----------------------------------------------------------------------------*/
/* fast */
static void
_span_color_fast(Enesim_Renderer *r, int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Map_Quad *thiz = ENESIM_RENDERER_MAP_QUAD(r);
	uint32_t *d = ddata, *e = d + len;
	int lx = thiz->rx, rx = thiz->lx;
	int active = 0;
	uint32_t *src = thiz->src_c;
	int sw = thiz->sw_c, sh = thiz->sh_c;
	int sxx, syy, dxx, dyy;
	double sxf = 0, syf = 0;
	Enesim_Color color = thiz->rcolor;

	if ( (y < thiz->ty) || (y >= thiz->by) || 
		(x >= thiz->rx) || ((x + len) <= thiz->lx) || !src)
	{
get_out:
		while (d < e)
			*d++ = 0;
		return;
	}

	if (color == 0xffffffff)
		color = 0;

	/* get left and right points on the quad on this scanline */
	active = _map_quad_get_endpoints(thiz, y, &lx, &rx);

	if (!active || ((rx - lx) < 1) || (rx < x) || (lx >= (x + len)))
		goto get_out;

	/* get corresponding points and increments on src space */
	enesim_matrix_point_transform(&(thiz->ct), lx, y, &sxf, &syf);
	sxx = sxf * 65536;  syy = syf * 65536;
	enesim_matrix_point_transform(&(thiz->ct), rx, y, &sxf, &syf);
	dxx = ((sxf * 65536) - sxx) / (double)(rx - lx);
	dyy = ((syf * 65536) - syy) / (double)(rx - lx);

	rx -= x;
	if (len > rx)
	{
		len -= rx + 1;
		memset(d + rx + 1, 0, sizeof(uint32_t) * len);
		e -= len;
	}

	lx -= x;
	if (lx > 0)
	{
		memset(d, 0, sizeof(uint32_t) * lx);
		d += lx;
	}
	else
	{
		sxx -= lx * dxx;
		syy -= lx * dyy;
	}

	while (d < e)
	{
		uint32_t p0 = 0;

		p0 = enesim_coord_sample_fast(src, thiz->stride_c, sw, sh, sxx, syy);
		if (p0 && ((p0>>24) < 255))
		{
			int a = 1 + (p0>>24);

			p0 = enesim_color_mul_256(a, p0 | 0xff000000);
		}
		if (color && p0)
			p0 = enesim_color_mul4_sym(p0, color);
		*d++ = p0;
		sxx += dxx; syy += dyy;
	}
}

static void
_span_src_fast(Enesim_Renderer *r, int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Map_Quad *thiz = ENESIM_RENDERER_MAP_QUAD(r);
	uint32_t *d = ddata, *e = d + len, *src = thiz->ssrc;
	int lx = thiz->rx, rx = thiz->lx;
	int active = 0;
	int sw = thiz->sw, sh = thiz->sh;
	int sxx, syy, dxx, dyy;
	double sxf = 0, syf = 0;
	Enesim_Color color = thiz->rcolor;

	if ( (y < thiz->ty) || (y >= thiz->by) || 
		(x >= thiz->rx) || ((x + len) <= thiz->lx) || !src)
	{
get_out:
		while (d < e)
		   *d++ = 0;
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	/* get left and right points on the quad on this scanline */
	active = _map_quad_get_endpoints(thiz, y, &lx, &rx);

	if (!active || ((rx - lx) < 1) || (rx < x) || (lx >= (x + len)))
		goto get_out;

	/* get corresponding points and increments on src space */
	enesim_matrix_point_transform(&(thiz->it), lx, y, &sxf, &syf);
	sxx = sxf * 65536;  syy = syf * 65536;
	enesim_matrix_point_transform(&(thiz->it), rx, y, &sxf, &syf);
	dxx = ((sxf * 65536) - sxx) / (double)(rx - lx);
	dyy = ((syf * 65536) - syy) / (double)(rx - lx);

	rx -= x;
	if (len > rx)
	{
		len -= rx + 1;
		memset(d + rx + 1, 0, sizeof(uint32_t) * len);
		e -= len;
	}

	lx -= x;
	if (lx > 0)
	{
		memset(d, 0, sizeof(uint32_t) * lx);
		d += lx;
	}
	else
	{
		sxx -= lx * dxx;
		syy -= lx * dyy;
	}

	while (d < e)
	{
		uint32_t p0 = 0;

		p0 = enesim_coord_sample_fast(src, thiz->sstride, sw, sh, sxx, syy);
		if (color && p0)
			p0 = enesim_color_mul4_sym(p0, color);
		*d++ = p0;
		sxx += dxx; syy += dyy;
	}
}

static void
_span_src_color_fast(Enesim_Renderer *r, int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Map_Quad *thiz = ENESIM_RENDERER_MAP_QUAD(r);
	uint32_t *d = ddata, *e = d + len, *src = thiz->ssrc;
	uint32_t *src_c = thiz->src_c;
	int lx = thiz->rx, rx = thiz->lx;
	int active = 0;
	int sw = thiz->sw, sh = thiz->sh;
	int sw_c = thiz->sw_c, sh_c = thiz->sh_c;
	int sxx, syy, dxx, dyy;
	int sxx_c, syy_c, dxx_c, dyy_c;
	double sxf = 0, syf = 0;
	Enesim_Color color = thiz->rcolor;

	if ( (y < thiz->ty) || (y >= thiz->by) || 
		(x >= thiz->rx) || ((x + len) <= thiz->lx) || !src || !src_c)
	{
get_out:
		while (d < e)
		   *d++ = 0;
		return;
	}

	if (color == 0xffffffff)
		color = 0;
	/* get left and right points on the quad on this scanline */
	active = _map_quad_get_endpoints(thiz, y, &lx, &rx);

	if (!active || ((rx - lx) < 1) || (rx < x) || (lx >= (x + len)))
		goto get_out;

	/* get corresponding points and increments on src space */
	enesim_matrix_point_transform(&(thiz->it), lx, y, &sxf, &syf);
	sxx = sxf * 65536;  syy = syf * 65536;
	enesim_matrix_point_transform(&(thiz->it), rx, y, &sxf, &syf);
	dxx = ((sxf * 65536) - sxx) / (double)(rx - lx);
	dyy = ((syf * 65536) - syy) / (double)(rx - lx);

	enesim_matrix_point_transform(&(thiz->ct), lx, y, &sxf, &syf);
	sxx_c = sxf * 65536;  syy_c = syf * 65536;
	enesim_matrix_point_transform(&(thiz->ct), rx, y, &sxf, &syf);
	dxx_c = ((sxf * 65536) - sxx_c) / (double)(rx - lx);
	dyy_c = ((syf * 65536) - syy_c) / (double)(rx - lx);

	rx -= x;
	if (len > rx)
	{
		len -= rx + 1;
		memset(d + rx + 1, 0, sizeof(uint32_t) * len);
		e -= len;
	}

	lx -= x;
	if (lx > 0)
	{
		memset(d, 0, sizeof(uint32_t) * lx);
		d += lx;
	}
	else
	{
		sxx -= lx * dxx;
		syy -= lx * dyy;
		sxx_c -= lx * dxx_c;
		syy_c -= lx * dyy_c;
	}

	while (d < e)
	{
		uint32_t p0 = 0, q0 = 0;

		p0 = enesim_coord_sample_fast(src, thiz->sstride, sw, sh, sxx, syy);
		q0 = enesim_coord_sample_fast(src_c, thiz->stride_c, sw_c, sh_c, sxx_c, syy_c);
		if (q0 && ((q0>>24) < 255))
		{
			int a = 1 + (q0>>24);

			q0 = enesim_color_mul_256(a, q0 | 0xff000000);
		}
		if (p0 | q0)
			p0 = enesim_color_mul4_sym(p0, q0);
		if (color && p0)
			p0 = enesim_color_mul4_sym(p0, color);
		*d++ = p0;
		sxx += dxx; syy += dyy;
		sxx_c += dxx_c; syy_c += dyy_c;
	}
}

/* good */
static void
_span_color_good(Enesim_Renderer *r, int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Map_Quad *thiz = ENESIM_RENDERER_MAP_QUAD(r);
	uint32_t *d = ddata, *e = d + len;
	int lx = thiz->rx, rx = thiz->lx;
	int active = 0;
	uint32_t *src = thiz->src_c;
	int sw = thiz->sw_c, sh = thiz->sh_c;
	int sxx, syy, dxx, dyy;
	double sxf = 0, syf = 0;
	Enesim_Color color = thiz->rcolor;

	if ( (y < thiz->ty) || (y >= thiz->by) || 
		(x >= thiz->rx) || ((x + len) <= thiz->lx) || !src)
	{
get_out:
		while (d < e)
			*d++ = 0;
		return;
	}

	if (color == 0xffffffff)
		color = 0;

	/* get left and right points on the quad on this scanline */
	active = _map_quad_get_endpoints(thiz, y, &lx, &rx);

	if (!active || ((rx - lx) < 1) || (rx < x) || (lx >= (x + len)))
		goto get_out;

	/* get corresponding points and increments on src space */
	enesim_matrix_point_transform(&(thiz->ct), lx, y, &sxf, &syf);
	sxx = sxf * 65536;  syy = syf * 65536;
	enesim_matrix_point_transform(&(thiz->ct), rx, y, &sxf, &syf);
	dxx = ((sxf * 65536) - sxx) / (double)(rx - lx);
	dyy = ((syf * 65536) - syy) / (double)(rx - lx);

	rx -= x;
	if (len > rx)
	{
		len -= rx + 1;
		memset(d + rx + 1, 0, sizeof(uint32_t) * len);
		e -= len;
	}

	lx -= x;
	if (lx > 0)
	{
		memset(d, 0, sizeof(uint32_t) * lx);
		d += lx;
	}
	else
	{
		sxx -= lx * dxx;
		syy -= lx * dyy;
	}

	while (d < e)
	{
		uint32_t p0 = 0;

		p0 = enesim_coord_sample_good_restrict(src, thiz->stride_c, sw, sh, sxx, syy);
		if (p0 && ((p0>>24) < 255))
		{
			int a = 1 + (p0>>24);

			p0 = enesim_color_mul_256(a, p0 | 0xff000000);
		}
		if (color && p0)
			p0 = enesim_color_mul4_sym(p0, color);
		*d++ = p0;
		sxx += dxx; syy += dyy;
	}
}


static void
_span_src_good(Enesim_Renderer *r, int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Map_Quad *thiz = ENESIM_RENDERER_MAP_QUAD(r);
	uint32_t *d = ddata, *e = d + len, *src = thiz->ssrc;
	int lx = thiz->rx, rx = thiz->lx;
	int active = 0;
	int sw = thiz->sw, sh = thiz->sh;
	int sxx, syy, dxx, dyy;
	double sxf = 0, syf = 0;
	Enesim_Color color = thiz->rcolor;

	if ( (y < thiz->ty) || (y >= thiz->by) || 
		(x >= thiz->rx) || ((x + len) <= thiz->lx) || !src)
	{
get_out:
		while (d < e)
		   *d++ = 0;
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	/* get left and right points on the quad on this scanline */
	active = _map_quad_get_endpoints(thiz, y, &lx, &rx);

	if (!active || ((rx - lx) < 1) || (rx < x) || (lx >= (x + len)))
		goto get_out;

	/* get corresponding points and increments on src space */
	enesim_matrix_point_transform(&(thiz->it), lx, y, &sxf, &syf);
	sxx = sxf * 65536;  syy = syf * 65536;
	enesim_matrix_point_transform(&(thiz->it), rx, y, &sxf, &syf);
	dxx = ((sxf * 65536) - sxx) / (double)(rx - lx);
	dyy = ((syf * 65536) - syy) / (double)(rx - lx);

	rx -= x;
	if (len > rx)
	{
		len -= rx + 1;
		memset(d + rx + 1, 0, sizeof(uint32_t) * len);
		e -= len;
	}

	lx -= x;
	if (lx > 0)
	{
		memset(d, 0, sizeof(uint32_t) * lx);
		d += lx;
	}
	else
	{
		sxx -= lx * dxx;
		syy -= lx * dyy;
	}

	while (d < e)
	{
		uint32_t p0 = 0;

		p0 = enesim_coord_sample_good_restrict(src, thiz->sstride, sw, sh, sxx, syy);
		if (color && p0)
			p0 = enesim_color_mul4_sym(p0, color);
		*d++ = p0;
		sxx += dxx; syy += dyy;
	}
}


static void
_span_src_color_good(Enesim_Renderer *r, int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Map_Quad *thiz = ENESIM_RENDERER_MAP_QUAD(r);
	uint32_t *d = ddata, *e = d + len, *src = thiz->ssrc;
	uint32_t *src_c = thiz->src_c;
	int lx = thiz->rx, rx = thiz->lx;
	int active = 0;
	int sw = thiz->sw, sh = thiz->sh;
	int sw_c = thiz->sw_c, sh_c = thiz->sh_c;
	int sxx, syy, dxx, dyy;
	int sxx_c, syy_c, dxx_c, dyy_c;
	double sxf = 0, syf = 0;
	Enesim_Color color = thiz->rcolor;

	if ( (y < thiz->ty) || (y >= thiz->by) || 
		(x >= thiz->rx) || ((x + len) <= thiz->lx) || !src || !src_c)
	{
get_out:
		while (d < e)
		   *d++ = 0;
		return;
	}

	if (color == 0xffffffff)
		color = 0;
	/* get left and right points on the quad on this scanline */
	active = _map_quad_get_endpoints(thiz, y, &lx, &rx);

	if (!active || ((rx - lx) < 1) || (rx < x) || (lx >= (x + len)))
		goto get_out;

	/* get corresponding points and increments on src space */
	enesim_matrix_point_transform(&(thiz->it), lx, y, &sxf, &syf);
	sxx = sxf * 65536;  syy = syf * 65536;
	enesim_matrix_point_transform(&(thiz->it), rx, y, &sxf, &syf);
	dxx = ((sxf * 65536) - sxx) / (double)(rx - lx);
	dyy = ((syf * 65536) - syy) / (double)(rx - lx);

	enesim_matrix_point_transform(&(thiz->ct), lx, y, &sxf, &syf);
	sxx_c = sxf * 65536;  syy_c = syf * 65536;
	enesim_matrix_point_transform(&(thiz->ct), rx, y, &sxf, &syf);
	dxx_c = ((sxf * 65536) - sxx_c) / (double)(rx - lx);
	dyy_c = ((syf * 65536) - syy_c) / (double)(rx - lx);

	rx -= x;
	if (len > rx)
	{
		len -= rx + 1;
		memset(d + rx + 1, 0, sizeof(uint32_t) * len);
		e -= len;
	}

	lx -= x;
	if (lx > 0)
	{
		memset(d, 0, sizeof(uint32_t) * lx);
		d += lx;
	}
	else
	{
		sxx -= lx * dxx;
		syy -= lx * dyy;
		sxx_c -= lx * dxx_c;
		syy_c -= lx * dyy_c;
	}

	while (d < e)
	{
		uint32_t p0 = 0, q0 = 0;

		p0 = enesim_coord_sample_good_restrict(src, thiz->sstride, sw, sh, sxx, syy);
		q0 = enesim_coord_sample_good_restrict(src_c, thiz->stride_c, sw_c, sh_c, sxx_c, syy_c);
		if (q0 && ((q0>>24) < 255))
		{
			int a = 1 + (q0>>24);

			q0 = enesim_color_mul_256(a, q0 | 0xff000000);
		}
		if (p0 | q0)
			p0 = enesim_color_mul4_sym(p0, q0);
		if (color && p0)
			p0 = enesim_color_mul4_sym(p0, color);
		*d++ = p0;
		sxx += dxx; syy += dyy;
		sxx_c += dxx_c; syy_c += dyy_c;
	}
}


/* best */
static void
_span_color_best(Enesim_Renderer *r, int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Map_Quad *thiz = ENESIM_RENDERER_MAP_QUAD(r);
	uint32_t *d = ddata, *e = d + len;
	int lx = thiz->rx, rx = thiz->lx;
	int active = 0;
	uint32_t *src = thiz->src_c;
	int sw = thiz->sw_c, sh = thiz->sh_c;
	int xx, yy, sxx, syy;
	int axx, ayx;
	float azx;
	float zf;
	Enesim_Color color = thiz->rcolor;

	if ( (y < thiz->ty) || (y >= thiz->by) || 
		(x >= thiz->rx) || ((x + len) <= thiz->lx) || !src)
	{
get_out:
		while (d < e)
			*d++ = 0;
		return;
	}

	if (color == 0xffffffff)
		color = 0;

	/* get left and right points on the quad on this scanline */
	active = _map_quad_get_endpoints(thiz, y, &lx, &rx);

	if (!active || ((rx - lx) < 1) || (rx < x) || (lx >= (x + len)))
		goto get_out;

	axx = (thiz->ct.xx) * 65536;
	ayx = (thiz->ct.yx) * 65536;
	azx = thiz->ct.zx;
	xx = (axx * lx) + ((thiz->ct.xy * 65536) * y) + (thiz->ct.xz * 65536);
	yy = (ayx * lx) + ((thiz->ct.yy * 65536) * y) + (thiz->ct.yz * 65536);
	zf = (azx * lx) + (thiz->ct.zy * y) + thiz->ct.zz;

	rx -= x;
	if (len > rx)
	{
		len -= rx + 1;
		memset(d + rx + 1, 0, sizeof(uint32_t) * len);
		e -= len;
	}

	lx -= x;
	if (lx > 0)
	{
		memset(d, 0, sizeof(uint32_t) * lx);
		d += lx;
	}
	else
	{
		xx -= lx * axx;
		yy -= lx * ayx;
		zf -= lx * azx;
	}

	while (d < e)
	{
		uint32_t p0 = 0;

		sxx = xx / zf;
		syy = yy / zf;

		p0 = enesim_coord_sample_good_restrict(src, thiz->stride_c, sw, sh, sxx, syy);
		if (p0 && ((p0>>24) < 255))
		{
			int a = 1 + (p0>>24);

			p0 = enesim_color_mul_256(a, p0 | 0xff000000);
		}
		if (color && p0)
			p0 = enesim_color_mul4_sym(p0, color);
		*d++ = p0;
		xx += axx; yy += ayx; zf += azx;
	}
}

static void
_span_src_best(Enesim_Renderer *r, int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Map_Quad *thiz = ENESIM_RENDERER_MAP_QUAD(r);
	uint32_t *d = ddata, *e = d + len, *src = thiz->ssrc;
	int lx = thiz->rx, rx = thiz->lx;
	int active = 0;
	int sw = thiz->sw, sh = thiz->sh;
	int xx, yy, sxx, syy;
	int axx, ayx;
	float azx;
	float zf;
	Enesim_Color color = thiz->rcolor;

	if ( (y < thiz->ty) || (y >= thiz->by) || 
		(x >= thiz->rx) || ((x + len) <= thiz->lx) || !src)
	{
get_out:
		while (d < e)
		   *d++ = 0;
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	/* get left and right points on the quad on this scanline */
	active = _map_quad_get_endpoints(thiz, y, &lx, &rx);

	if (!active || ((rx - lx) < 1) || (rx < x) || (lx >= (x + len)))
		goto get_out;

	axx = (thiz->it.xx) * 65536;
	ayx = (thiz->it.yx) * 65536;
	azx = thiz->it.zx;
	xx = (axx * lx) + ((thiz->it.xy * 65536) * y) + (thiz->it.xz * 65536);
	yy = (ayx * lx) + ((thiz->it.yy * 65536) * y) + (thiz->it.yz * 65536);
	zf = (azx * lx) + (thiz->it.zy * y) + thiz->it.zz;

	rx -= x;
	if (len > rx)
	{
		len -= rx + 1;
		memset(d + rx + 1, 0, sizeof(uint32_t) * len);
		e -= len;
	}

	lx -= x;
	if (lx > 0)
	{
		memset(d, 0, sizeof(uint32_t) * lx);
		d += lx;
	}
	else
	{
		xx -= lx * axx;
		yy -= lx * ayx;
		zf -= lx * azx;
	}

	while (d < e)
	{
		uint32_t p0 = 0;

		sxx = xx / zf;
		syy = yy / zf;

		p0 = enesim_coord_sample_good_restrict(src, thiz->sstride, sw, sh, sxx, syy);
		if (color && p0)
			p0 = enesim_color_mul4_sym(p0, color);
		*d++ = p0;
		xx += axx; yy += ayx; zf += azx;
	}
}

static void
_span_src_color_best(Enesim_Renderer *r, int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Map_Quad *thiz = ENESIM_RENDERER_MAP_QUAD(r);
	uint32_t *d = ddata, *e = d + len, *src = thiz->ssrc;
	uint32_t *src_c = thiz->src_c;
	int lx = thiz->rx, rx = thiz->lx;
	int active = 0;
	int sw = thiz->sw, sh = thiz->sh;
	int sw_c = thiz->sw_c, sh_c = thiz->sh_c;
	int xx, yy, sxx, syy;
	int axx, ayx;
	float azx;
	float zf;
	int xx_c, yy_c;
	int axx_c, ayx_c;
	float azx_c;
	float zf_c;
	Enesim_Color color = thiz->rcolor;

	if ( (y < thiz->ty) || (y >= thiz->by) || 
		(x >= thiz->rx) || ((x + len) <= thiz->lx) || !src || !src_c)
	{
get_out:
		while (d < e)
		   *d++ = 0;
		return;
	}

	if (color == 0xffffffff)
		color = 0;
	/* get left and right points on the quad on this scanline */
	active = _map_quad_get_endpoints(thiz, y, &lx, &rx);

	if (!active || ((rx - lx) < 1) || (rx < x) || (lx >= (x + len)))
		goto get_out;

	axx = (thiz->it.xx) * 65536;
	ayx = (thiz->it.yx) * 65536;
	azx = thiz->it.zx;
	xx = (axx * lx) + ((thiz->it.xy * 65536) * y) + (thiz->it.xz * 65536);
	yy = (ayx * lx) + ((thiz->it.yy * 65536) * y) + (thiz->it.yz * 65536);
	zf = (azx * lx) + (thiz->it.zy * y) + thiz->it.zz;

	axx_c = (thiz->ct.xx) * 65536;
	ayx_c = (thiz->ct.yx) * 65536;
	azx_c = thiz->ct.zx;
	xx_c = (axx_c * lx) + ((thiz->ct.xy * 65536) * y) + (thiz->ct.xz * 65536);
	yy_c = (ayx_c * lx) + ((thiz->ct.yy * 65536) * y) + (thiz->ct.yz * 65536);
	zf_c = (azx_c * lx) + (thiz->ct.zy * y) + thiz->ct.zz;

	rx -= x;
	if (len > rx)
	{
		len -= rx + 1;
		memset(d + rx + 1, 0, sizeof(uint32_t) * len);
		e -= len;
	}

	lx -= x;
	if (lx > 0)
	{
		memset(d, 0, sizeof(uint32_t) * lx);
		d += lx;
	}
	else
	{
		xx -= lx * axx;
		yy -= lx * ayx;
		zf -= lx * azx;
		xx_c -= lx * axx_c;
		yy_c -= lx * ayx_c;
		zf_c -= lx * azx_c;
	}

	while (d < e)
	{
		uint32_t p0 = 0, q0 = 0;

		sxx = xx / zf;
		syy = yy / zf;

		p0 = enesim_coord_sample_good_restrict(src, thiz->sstride, sw, sh, sxx, syy);

		sxx = xx_c / zf_c;
		syy = yy_c / zf_c;

		q0 = enesim_coord_sample_good_restrict(src_c, thiz->stride_c, sw_c, sh_c, sxx, syy);
		if (q0 && ((q0>>24) < 255))
		{
			int a = 1 + (q0>>24);

			q0 = enesim_color_mul_256(a, q0 | 0xff000000);
		}
		if (p0 | q0)
			p0 = enesim_color_mul4_sym(p0, q0);
		if (color && p0)
			p0 = enesim_color_mul4_sym(p0, color);
		*d++ = p0;
		xx += axx; yy += ayx; zf += azx;
		xx_c += axx_c; yy_c += ayx_c; zf_c += azx_c;
	}
}



/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _map_quad_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "map_quad";
}

static Eina_Bool _map_quad_sw_vertices_setup(Enesim_Renderer *r)
{
	Enesim_Renderer_Map_Quad *thiz;
	int i;
	double lx, rx, ty, by;
	double fx, fy;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	/* assign initial working vertices */
	enesim_renderer_origin_get(r, &fx, &fy);
	i = 0;
	while(i < 4)
	{
		thiz->qx[i] = thiz->vx[i] - fx;
		thiz->qy[i] = thiz->vy[i] - fy;
		i++;
	}
	/* if there's a transform */
	if (enesim_renderer_transformation_type_get(r) != ENESIM_MATRIX_TYPE_IDENTITY)
	{
		Enesim_Matrix t;

		enesim_renderer_transformation_get(r, &t);
		i = 0;
		while(i < 4)
		{
			enesim_matrix_point_transform(&t, thiz->qx[i], thiz->qy[i], &fx, &fy);
			thiz->qx[i] = fx;
			thiz->qy[i] = fy;
			i++;
		}
	}

	/* get bounds */
	ty = by = thiz->qy[0];
	i = 1;
	while (i < 4)
	{
		if (thiz->qy[i] < ty)
			ty = thiz->qy[i];
		if (thiz->qy[i] > by)
			by = thiz->qy[i];
		i++;
	}
	lx = rx = thiz->qx[0];
	i = 1;
	while (i < 4)
	{
		if (thiz->qx[i] < lx)
			lx = thiz->qx[i];
		if (thiz->qx[i] > rx)
			rx = thiz->qx[i];
		i++;
	}
	if ((rx - lx) < 2) return EINA_FALSE;
	if ((by - ty) < 2) return EINA_FALSE;

	/* normalize and align working vertices */
	i = 0;
	fx = (rx - lx - 1) / (rx - lx);
	fy = (by - ty - 1) / (by - ty);
	while(i < 4)
	{
		thiz->qx[i] = ((int)(256 * (lx + ((thiz->qx[i] - lx) * fx)))) / 256.0;
		thiz->qy[i] = ((int)(256 * (ty + ((thiz->qy[i] - ty) * fy)))) / 256.0;
		i++;
	}

	thiz->lx = lx;  thiz->rx = rx - (1 / 256.0);
	thiz->ty = ty;  thiz->by = by - (1 / 256.0);

	/* get signs of vertex edges */
	i = 0;
	while (i < 4)
	{
		int j = i + 1;

		if (i == 3) j = 0;

		fx = thiz->qx[j] - thiz->qx[i];
		fy = thiz->qy[j] - thiz->qy[i];

		thiz->sgn[i] = 0;
		if ((fy != 0) && (fx != 0))
		{
			thiz->sgn[i] = 1;
			if (fy > 0)
			{
				if (fx < 0)
					thiz->sgn[i] = -1;
			}
			else
			{
				if (fx > 0)
					thiz->sgn[i] = -1;
			}
		}
		i++;
	}
	return EINA_TRUE;
}

static Eina_Bool _map_quad_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Map_Quad *thiz;
	Enesim_Quality quality;
	Enesim_Quad sq, dq;
	Eina_Bool do_cols = 0;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	if (!_map_quad_state_setup(thiz))
		return EINA_FALSE;
	if (thiz->src)
	{
		if (!enesim_surface_map(thiz->src, (void **)&thiz->ssrc, &thiz->sstride))
		{
			_map_quad_state_cleanup(thiz);
			return EINA_FALSE;
		}
		enesim_surface_size_get(thiz->src, &(thiz->sw), &(thiz->sh));
		if ((thiz->sw < 2) || (thiz->sh < 2))
			return EINA_FALSE;
	}

	/* setup internal vertices and get bounds */
	if (!_map_quad_sw_vertices_setup(r))
		return EINA_FALSE;

	/* setup quad transforms */
	dq.x0 = thiz->qx[0];  dq.y0 = thiz->qy[0];
	dq.x1 = thiz->qx[1];  dq.y1 = thiz->qy[1];
	dq.x2 = thiz->qx[2];  dq.y2 = thiz->qy[2];
	dq.x3 = thiz->qx[3];  dq.y3 = thiz->qy[3];

	if (thiz->src)
	{
		sq.x0 = sq.y0 = 0;
		sq.x1 = thiz->sw - 1; sq.y1 = 0;
		sq.x2 = thiz->sw - 1; sq.y2 = thiz->sh - 1;
		sq.x3 = 0; sq.y3 = thiz->sh - 1;

		if (!enesim_matrix_quad_quad_map(&(thiz->it), &dq, &sq))
			return EINA_FALSE;

		if ( (thiz->vcolors[0] != 0xffffffff) | (thiz->vcolors[1] != 0xffffffff) | 
			(thiz->vcolors[2] != 0xffffffff) | (thiz->vcolors[3] != 0xffffffff) )
		{
			thiz->sw_c = thiz->sh_c = 3;
			if (!thiz->src_c)
			{
				thiz->stride_c = thiz->sw_c * sizeof(uint32_t);
				thiz->src_c = malloc(thiz->sh_c * thiz->stride_c);
			}
			if (thiz->src_c)
			{
				sq.x0 = sq.y0 = 0;
				sq.x1 = 2; sq.y1 = 0;
				sq.x2 = 2; sq.y2 = 2;
				sq.x3 = 0; sq.y3 = 2;
				if (!enesim_matrix_quad_quad_map(&(thiz->ct), &dq, &sq))
					return EINA_FALSE;
				thiz->src_c[0] = thiz->vcolors[0];  thiz->src_c[2] = thiz->vcolors[1];
				thiz->src_c[6] = thiz->vcolors[3];  thiz->src_c[8] = thiz->vcolors[2];
				thiz->src_c[1] = enesim_color_interp_256(128, thiz->src_c[0], thiz->src_c[2]);
				thiz->src_c[7] = enesim_color_interp_256(128, thiz->src_c[6], thiz->src_c[8]);
				thiz->src_c[3] = enesim_color_interp_256(128, thiz->src_c[0], thiz->src_c[6]);
				thiz->src_c[5] = enesim_color_interp_256(128, thiz->src_c[2], thiz->src_c[8]);
				thiz->src_c[4] = enesim_color_interp_256(128, thiz->src_c[3], thiz->src_c[5]);
				do_cols = 1;
			}
		}
		else
		{
			if (thiz->src_c)
			{
				free(thiz->src_c);
				thiz->src_c = NULL;
			}
		}
	}
	else
	{
		thiz->sw_c = thiz->sh_c = 3;
		if (!thiz->src_c)
		{
			thiz->stride_c = thiz->sw_c * sizeof(uint32_t);
			thiz->src_c = malloc(thiz->sh_c * thiz->stride_c);
		}
		if (thiz->src_c)
		{
			sq.x0 = sq.y0 = 0;
			sq.x1 = 2; sq.y1 = 0;
			sq.x2 = 2; sq.y2 = 2;
			sq.x3 = 0; sq.y3 = 2;
			if (!enesim_matrix_quad_quad_map(&(thiz->ct), &dq, &sq))
				return EINA_FALSE;
			thiz->src_c[0] = thiz->vcolors[0];  thiz->src_c[2] = thiz->vcolors[1];
			thiz->src_c[6] = thiz->vcolors[3];  thiz->src_c[8] = thiz->vcolors[2];
			thiz->src_c[1] = enesim_color_interp_256(128, thiz->src_c[0], thiz->src_c[2]);
			thiz->src_c[7] = enesim_color_interp_256(128, thiz->src_c[6], thiz->src_c[8]);
			thiz->src_c[3] = enesim_color_interp_256(128, thiz->src_c[0], thiz->src_c[6]);
			thiz->src_c[5] = enesim_color_interp_256(128, thiz->src_c[2], thiz->src_c[8]);
			thiz->src_c[4] = enesim_color_interp_256(128, thiz->src_c[3], thiz->src_c[5]);
		}
	}

	thiz->rcolor = enesim_renderer_color_get(r);
	/* assign span funcs */
	/* assign span funcs */
	quality = enesim_renderer_quality_get(r);

	if (!thiz->src)
	{
		if (enesim_matrix_type_get(&(thiz->ct)) == ENESIM_MATRIX_TYPE_AFFINE)
			quality = ENESIM_QUALITY_GOOD;

		if (quality == ENESIM_QUALITY_BEST)
			*fill = _span_color_best;
		else if (quality == ENESIM_QUALITY_GOOD)
			*fill = _span_color_good;
		else
			*fill = _span_color_fast;
	}
	else
	{
		if (enesim_matrix_type_get(&(thiz->it)) == ENESIM_MATRIX_TYPE_AFFINE)
		{
			if (quality == ENESIM_QUALITY_BEST)
			{
				if (!do_cols)
					quality = ENESIM_QUALITY_GOOD;
				else if (enesim_matrix_type_get(&(thiz->ct)) == ENESIM_MATRIX_TYPE_AFFINE)
					quality = ENESIM_QUALITY_GOOD;
			}
		}

		if (quality == ENESIM_QUALITY_BEST)
		{
			*fill = _span_src_best;
			if (do_cols)
				*fill = _span_src_color_best;
		}
		else if (quality == ENESIM_QUALITY_GOOD)
		{
			*fill = _span_src_good;
			if (do_cols)
				*fill = _span_src_color_good;
		}
		else
		{
			*fill = _span_src_fast;
			if (do_cols)
				*fill = _span_src_color_fast;
		}
	}
	if (!*fill)
		return EINA_FALSE;

	return EINA_TRUE;
}


static void _map_quad_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Map_Quad *thiz;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	_map_quad_state_cleanup(thiz);
}

static void _map_quad_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_ARGB8888 | ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_PROJECTIVE;
}

static void _map_quad_sw_hints_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Rop rop EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_SW_HINT_COLORIZE;
}

static Eina_Bool _map_quad_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Map_Quad *thiz;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	return thiz->changed;
}

static void _map_quad_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Map_Quad *thiz;

	if (!_map_quad_sw_vertices_setup(r))
	{
		rect->x = rect->y = rect->w = rect->h = 0;
		return;
	}
	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	rect->x = thiz->lx;
	rect->y = thiz->ty;
	rect->w = thiz->rx - rect->x;
	rect->h = thiz->by - rect->y;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Map_Quad, Enesim_Renderer_Map_Quad_Class,
		enesim_renderer_map_quad);

static void _enesim_renderer_map_quad_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _map_quad_name;
	klass->bounds_get = _map_quad_bounds_get;
	klass->features_get = _map_quad_features_get;
	klass->has_changed =  _map_quad_has_changed;
	klass->sw_hints_get = _map_quad_sw_hints_get;
	klass->sw_setup = _map_quad_sw_setup;
	klass->sw_cleanup = _map_quad_sw_cleanup;
}

static void _enesim_renderer_map_quad_instance_init(void *o)
{
	Enesim_Renderer_Map_Quad *thiz = ENESIM_RENDERER_MAP_QUAD(o);

	/* initial properties */
	thiz->vx[0] = thiz->vy[0] = 0;
	thiz->vx[1] = 2; thiz->vy[1] = 0;
	thiz->vx[2] = 2; thiz->vy[2] = 2;
	thiz->vx[3] = 0; thiz->vy[3] = 2;

	thiz->vcolors[0] = thiz->vcolors[1] = thiz->vcolors[2] = thiz->vcolors[3] = 0xffffffff;
}

static void _enesim_renderer_map_quad_instance_deinit(void *o EINA_UNUSED)
{
	Enesim_Renderer_Map_Quad *thiz = ENESIM_RENDERER_MAP_QUAD(o);

	if (thiz->src)
	{
		enesim_surface_unref(thiz->src);
		thiz->src = NULL;
	}
	if (thiz->src_c)
	{
		free(thiz->src);
		thiz->src_c = NULL;
	}
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Creates a new map_quad renderer
 * @return The renderer
 */
EAPI Enesim_Renderer * enesim_renderer_map_quad_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_map_quad);
	return r;
}

/**
 * @brief Sets a vertex's coordinates
 * @ender_prop{vertex}
 * @param[in] r The map quad renderer
 * @param[in] index The vertex index
 * @param[in] x The x coord of the vertex
 * @param[in] y The y coord of the vertex
 */
EAPI void enesim_renderer_map_quad_vertex_position_set(Enesim_Renderer *r,
		int index, double x, double y)
{
	Enesim_Renderer_Map_Quad *thiz;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	if (!thiz) return;

	if (index < 0) index = 0;
	if (index > 3) index = 3;
	thiz->vx[index] = x;
	thiz->vy[index] = y;
	thiz->changed = EINA_TRUE;
}


/**
 * @brief Gets a vertex's coordinates
 * @ender_prop{vertex}
 * @param[in] r The map quad renderer
 * @param[in] index The vertex index
 * @param[in] x The x coord of the vertex to get
 * @param[in] y The y coord of the vertex to get
 */
EAPI void enesim_renderer_map_quad_vertex_position_get(Enesim_Renderer *r,
		int index, double *x, double *y)
{
	Enesim_Renderer_Map_Quad *thiz;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	if (!thiz) return;

	if (index < 0) index = 0;
	if (index > 3) index = 3;
	if (x) *x = thiz->vx[index];
	if (y) *y = thiz->vy[index];
}

/**
 * @brief Sets the source surface to use as the source data
 * @ender_prop{source_surface}
 * @param[in] r The map quad renderer
 * @param[in] src The surface to map @ender_transfer{full}
 */
EAPI void enesim_renderer_map_quad_source_surface_set(Enesim_Renderer *r, Enesim_Surface *src)
{
	Enesim_Renderer_Map_Quad *thiz;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	if (!thiz) return;
	if (thiz->src)
	{
		enesim_surface_unref(thiz->src);
		thiz->src = NULL;
	}
	thiz->src = src;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the source surface used as the source data
 * @ender_prop{source_surface}
 * @param[in] r The map quad renderer
 * @return The surface to map @ender_transfer{none}
 */
EAPI Enesim_Surface * enesim_renderer_map_quad_source_surface_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Map_Quad *thiz;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	if (!thiz) return NULL;
	return enesim_surface_ref(thiz->src);
}

/**
 * @brief Sets the non-premul color for a vertex
 * @ender_prop{vertex_color}
 * @param[in] r The map quad renderer
 * @param[in] index The vertex index
 * @param[in] color The argb color
 */
EAPI void enesim_renderer_map_quad_vertex_color_set(Enesim_Renderer *r,
		int index, Enesim_Argb color)
{
	Enesim_Renderer_Map_Quad *thiz;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	if (!thiz) return;

	if (index < 0) index = 0;
	if (index > 3) index = 3;
	thiz->vcolors[index] = color;

	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the color of a vertex
 * @ender_prop{vertex_color}
 * @param[in] r The map quad renderer
 * @param[in] index The vertex index
 * @return the color of the vertex
 */
EAPI Enesim_Argb enesim_renderer_map_quad_vertex_color_get(Enesim_Renderer *r, int index)
{
	Enesim_Renderer_Map_Quad *thiz;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	if (!thiz) return 0;

	if (index < 0) index = 0;
	if (index > 3) index = 3;
	return thiz->vcolors[index];
}

