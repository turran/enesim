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
#include <math.h>

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
#include "enesim_renderer_shape.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_color_private.h"
#include "enesim_color_fill_private.h"
#include "enesim_color_mul4_sym_private.h"
#include "enesim_list_private.h"
#include "enesim_vector_private.h"
#include "enesim_renderer_private.h"
#include "enesim_rasterizer_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_RASTERIZER_KIIA(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Rasterizer_Kiia,						\
		enesim_rasterizer_kiia_descriptor_get())

/* A worker is in charge of rasterize one span */
typedef struct _Enesim_Rasterizer_Kiia_Worker
{
	/* a span of length equal to the width of the bounds to store the sample mask */
	uint32_t *mask;
	/* a span of length equal to the width of the bounds * nsamples to
	 * store the sample winding
	 */
	uint32_t *winding;
	int cwinding;
	/* keep track of the current Y this worker is doing, to ease
	 * the x increment on the edges
	 */
	int y;
} Enesim_Rasterizer_Kiia_Worker;

/* Its own definition of an edge */
typedef struct _Enesim_Rasterizer_Kiia_Edge
{
	/* The top/bottom coordinates rounded to increment of 1/num samples */
	Eina_F16p16 yy0, yy1;
	/* The left/right coordinates in fixed point */
	Eina_F16p16 xx0, xx1;
	/* The x coordinate at yy0 */
	Eina_F16p16 mx;
	/* The increment on x when y increments 1/num samples */
	Eina_F16p16 slope;
	int sgn;
} Enesim_Rasterizer_Kiia_Edge;

typedef struct _Enesim_Rasterizer_Kiia
{
	Enesim_Rasterizer parent;
	/* private */
	Enesim_Color fcolor;
	Enesim_Renderer *fren;
	/* The coordinates of the figure */
	int lx;
	int rx;
	Eina_F16p16 llx;
	/* The number of samples (8, 16, 32) */
	int nsamples;
	/* the increment on the y direction (i.e 1/num samples) */
	Eina_F16p16 inc;
	/* the pattern to use */
	Eina_F16p16 *pattern;
	/* One worker per cpu */
	Enesim_Rasterizer_Kiia_Worker *workers;
	int nworkers;
	Enesim_Rasterizer_Kiia_Edge *edges;
	int nedges;
} Enesim_Rasterizer_Kiia;

typedef struct _Enesim_Rasterizer_Kiia_Class {
	Enesim_Rasterizer_Class parent;
} Enesim_Rasterizer_Kiia_Class;

static Eina_F16p16 _kiia_pattern8[8];
static Eina_F16p16 _kiia_pattern16[16];
static Eina_F16p16 _kiia_pattern32[32];

static int _kiia_edge_sort(const void *l, const void *r)
{
	Enesim_Rasterizer_Kiia_Edge *lv = (Enesim_Rasterizer_Kiia_Edge *)l;
	Enesim_Rasterizer_Kiia_Edge *rv = (Enesim_Rasterizer_Kiia_Edge *)r;

	if (lv->yy0 <= rv->yy0)
		return -1;
	return 1;
}

static Eina_Bool _kiia_edge_setup(Enesim_Rasterizer_Kiia_Edge *thiz,
		Enesim_Point *p0, Enesim_Point *p1)
{
	double x0, y0, x1, y1;
	double x01, y01;
	double start;
	double slope;
	double mx;
	int sgn;

	/* going down, swap the y */
	if (p0->y > p1->y)
	{
		x0 = p1->x;
		y0 = p1->y;
		x1 = p0->x;
		y1 = p0->y;
		sgn = -1;
	}
	else
	{
		x0 = p0->x;
		y0 = p0->y;
		x1 = p1->x;
		y1 = p1->y;
		sgn = 1;
	}
	/* get the slope */
	x01 = x1 - x0;
	y01 = y1 - y0;
	/* for horizontal edges we just skip */
	if (!y01) return EINA_FALSE;

	slope = x01 / y01;

	/* get the sampled Y inside the line */
	start = (((int)(y0 * 32.0) )/ 32.0);
	y1 = (((int)(y1 * 32.0)) / 32.0);
	mx = x0 + (slope * (start - y0));

	thiz->yy0 = eina_f16p16_double_from(start);
	thiz->yy1 = eina_f16p16_double_from(y1);
	thiz->xx0 = eina_f16p16_double_from(x0);
	thiz->xx1 = eina_f16p16_double_from(x1);
	thiz->slope = eina_f16p16_double_from(slope/32.0);
	thiz->mx = eina_f16p16_double_from(mx);
	thiz->sgn = sgn;

	return EINA_TRUE;
}

static Eina_Bool _kiia_edges_setup(Enesim_Renderer *r)
{
	Enesim_Rasterizer_Kiia *thiz;
	Enesim_Rasterizer *rr;
	Enesim_Polygon *p;
	const Enesim_Figure *f;
	Eina_List *l1;
	int n = 0;

	thiz = ENESIM_RASTERIZER_KIIA(r);
	rr = ENESIM_RASTERIZER(r);
	f = rr->figure;

	/* allocate the maximum number of vectors possible */
	EINA_LIST_FOREACH(f->polygons, l1, p)
		n += enesim_polygon_point_count(p);
	thiz->edges = malloc(n * sizeof(Enesim_Rasterizer_Kiia_Edge));

	/* create the edges */
	n = 0;
	EINA_LIST_FOREACH(f->polygons, l1, p)
	{
		Enesim_Point *fp, *pt, *pp;
		Eina_List *points, *l2;

		fp = eina_list_data_get(p->points);
		pp = fp;
		points = eina_list_next(p->points);
		EINA_LIST_FOREACH(points, l2, pt)
		{
			Enesim_Rasterizer_Kiia_Edge *e = &thiz->edges[n];

			if (_kiia_edge_setup(e, pp, pt))
				n++;
			pp = pt;
		}
		/* add the last point in case the polygon is closed */
		if (p->closed)
		{
			Enesim_Rasterizer_Kiia_Edge *e = &thiz->edges[n];
			if (_kiia_edge_setup(e, pp, fp))
				n++;
		}
	}
	thiz->nedges = n;
	qsort(thiz->edges, thiz->nedges, sizeof(Enesim_Rasterizer_Kiia_Edge), _kiia_edge_sort);
	return EINA_TRUE;
}

static inline void _kiia_even_odd_sample(Enesim_Rasterizer_Kiia_Worker *w,
		int x, int m, int nsamples EINA_UNUSED, int sample EINA_UNUSED,
		int winding EINA_UNUSED)
{
	w->mask[x] ^= m;
}

static inline void _kiia_non_zero_sample(Enesim_Rasterizer_Kiia_Worker *w,
		int x, int m, int nsamples, int sample, int winding)
{
	w->mask[x] |= m;
	w->winding[(x * nsamples) + sample] += winding;
}

static inline uint32_t _kiia_non_zero_get_mask(Enesim_Rasterizer_Kiia_Worker *w, int cm, int m, uint32_t *winding)
{
#if 0
	uint32_t ret = cm;
	uint32_t bit = 1;
	uint32_t values[32] = { 0 };
	int n;

	for (n = 0; n < 32; n++)
	{
		if (m & bit)
		{
			uint32_t t = values[n];

			values[n] += *winding;
			if ((t == 0) ^ (values[n] == 0))
				ret ^= bit;
		}
		*winding = 0;
		winding++;
		bit <<= 1;
	}
	//printf("had %08x has %08x values[1] %d\n", m, ret, values[1]);
	return ret;
#else
	uint32_t ret;
	uint32_t bit = 1;
	int ww = 0;
	int n;

	for (n = 0; n < 32; n++)
	{
		if (m & bit)
			ww += *winding;
		*winding = 0;
		winding++;
		bit <<= 1;
	}

	w->cwinding += ww;
	if (!w->cwinding)
	{
		ret = m;
	}
	else
	{
		ret = cm | m;
	}
	return ret;
#endif
}

static void _kiia_span(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Rasterizer_Kiia *thiz;
	Enesim_Rasterizer_Kiia_Worker *w;
	Eina_F16p16 yy0, yy1;
	Eina_F16p16 ll = INT_MAX, rr = -INT_MAX;
	Eina_F16p16 sinc;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	uint32_t cm = 0;
	int lx;
	int rx;
	int i;

	thiz = ENESIM_RASTERIZER_KIIA(r);
	w = &thiz->workers[y % thiz->nworkers];

	yy0 = eina_f16p16_int_from(y);
	yy1 = eina_f16p16_int_from(y + 1);
	sinc = thiz->inc;
	/* intersect with each edge */
	for (i = 0; i < thiz->nedges; i++)
	{
		Enesim_Rasterizer_Kiia_Edge *edge = &thiz->edges[i];
		Eina_F16p16 *pattern;
		Eina_F16p16 yyy0, yyy1;
		Eina_F16p16 cx;
		int sample;
		int m;

		/* up the span */
		if (yy0 >= edge->yy1)
			continue;
		/* down the span, just skip processing, the edges are ordered in y */
		if (yy1 < edge->yy0)
			break;

		/* make sure not overflow */
		yyy1 = yy1;
		if (yyy1 > edge->yy1)
			yyy1 = edge->yy1;

		yyy0 = yy0;
		if (yy0 <= edge->yy0)
		{
			yyy0 = edge->yy0;
			cx = edge->mx;
			sample = (eina_f16p16_fracc_get(yyy0) >> 11); /* 16.16 << nsamples */
			m = 1 << sample;
		}
		else
		{
			Eina_F16p16 inc;

			inc = eina_f16p16_mul(yyy0 - edge->yy0, eina_f16p16_int_from(32));
			cx = edge->mx + eina_f16p16_mul(inc, edge->slope);
			sample = 0;
			m = 1;
		}

		/* get the pattern to use */
		pattern = thiz->pattern;
		/* finally sample */
		for (; yyy0 < yyy1; yyy0 += sinc)
		{
			int mx;

			mx = eina_f16p16_int_to(cx - thiz->llx + *pattern);
			/* keep track of the start, end of intersections */
			if (mx < ll)
				ll = mx;
			if (mx > rr)
				rr = mx;
#if 0
			_kiia_even_odd_sample(w, mx, m, thiz->nsamples, sample, edge->sgn);
#else
			_kiia_non_zero_sample(w, mx, m, thiz->nsamples, sample, edge->sgn);
#endif
			cx += edge->slope;
			pattern++;
			sample++;
			m <<= 1;
		}
	}

	/* TODO memset [x ... left] [right ... x + len] */
	lx = x - thiz->lx;
	rx = lx + len;
	if (thiz->fren)
	{
		enesim_renderer_sw_draw(thiz->fren, x, y, len, dst);
	}
	/* advance the mask until we reach the requested x */
	/* also clear the mask in the process */
	for (i = 0; i < lx; i++)
	{
		cm ^= w->mask[i];
		w->mask[i] = 0;
	}
	/* iterate over the mask and fill */ 
	while (dst < end)
	{
		uint32_t p0;

#if 0
		cm ^= w->mask[i];
#else
		cm = _kiia_non_zero_get_mask(w, cm, w->mask[i], &w->winding[i * 32]);
#endif
		w->mask[i] = 0;
		if (cm == 0xffffffff)
		{
			if (thiz->fren)
			{
				if (thiz->fcolor != 0xffffffff)
				{
					p0 = *dst;
					p0 = enesim_color_mul4_sym(p0, thiz->fcolor);
				}
				else
				{
					goto next;
				}
			}
			else
			{
				p0 = thiz->fcolor;
			}
		}
		else if (cm == 0)
		{
			p0 = 0;
		}
		else
		{
			uint32_t count;
			uint16_t coverage;

			/* use the hamming weight to know the number of bits set to 1 */
			count = cm - ((cm >> 1) & 0x55555555);
			count = (count & 0x33333333) + ((count >> 2) & 0x33333333);
			/* we use 21 instead of 24, because we need to rescale 32 -> 256 */
			coverage = (((count + (count >> 4)) & 0x0f0f0f0f) * 0x01010101) >> 21;
			if (thiz->fren)
			{
				uint32_t q0 = *dst;

				if (thiz->fcolor != 0xffffffff)
					q0 = enesim_color_mul4_sym(thiz->fcolor, q0);
				p0 = enesim_color_mul_256(coverage, q0);
			}
			else
			{
				p0 = enesim_color_mul_256(coverage, thiz->fcolor);
			}
		}
		*dst = p0;
next:
		dst++;
		i++;
	}
	/* set to zero the rest of the bits of the mask */
	for (i = rx; i <= thiz->rx; i++)
	{
		w->mask[i] = 0;
	}
	/* update the latest y coordinate of the worker */
	w->y = y;
	w->cwinding = 0;
}
/*----------------------------------------------------------------------------*
 *                           Rasterizer interface                             *
 *----------------------------------------------------------------------------*/
static void _kiia_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Rasterizer_Kiia *thiz;
	int i;

	thiz = ENESIM_RASTERIZER_KIIA(r);
	enesim_renderer_unref(thiz->fren);
	/* cleanup the workers */
	for (i = 0; i < thiz->nworkers; i++)
	{
		free(thiz->workers[i].mask);
		free(thiz->workers[i].winding);
	}
}

/*----------------------------------------------------------------------------*
 *                    The Enesim's rasterizer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _kiia_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "kiia";
}

static Eina_Bool _kiia_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *draw, Enesim_Log **error)
{
	Enesim_Rasterizer_Kiia *thiz;
	Enesim_Rasterizer *rr;
	Enesim_Color color;
	double lx, rx, ty, by;
	int len;
	int i;
	int y;

	if (enesim_rasterizer_has_changed(r))
	{
		/* create the edges */
		if (!_kiia_edges_setup(r))
			return EINA_FALSE;
	}
	rr = ENESIM_RASTERIZER(r);
	if (!enesim_figure_bounds(rr->figure, &lx, &ty, &rx, &by))
		return EINA_FALSE;

	thiz = ENESIM_RASTERIZER_KIIA(r);
	thiz->fren = enesim_renderer_shape_fill_renderer_get(r);
	thiz->fcolor = enesim_renderer_shape_fill_color_get(r);
	color = enesim_renderer_color_get(r);
	if (color != ENESIM_COLOR_FULL)
		thiz->fcolor = enesim_color_mul4_sym(thiz->fcolor, color);
	/* TODO use the quality, 32 samples for now */
	thiz->nsamples = 32;
	thiz->inc = eina_f16p16_double_from(1/32.0);
	thiz->pattern = _kiia_pattern32;
	/* set the y coordinate with the topmost value */
	y = ceil(ty);
	/* the length of the mask buffer */
	len = ceil(rx - lx) + 1;
	thiz->rx = len;
	thiz->lx = floor(lx);
	thiz->llx = eina_f16p16_int_from(thiz->lx);
	/* setup the workers */
	for (i = 0; i < thiz->nworkers; i++)
	{
		thiz->workers[i].y = y;
		/* +1 because of the pattern offset */
		thiz->workers[i].mask = calloc(len + 1, sizeof(uint32_t));
		thiz->workers[i].winding = calloc((len + 1) * thiz->nsamples, sizeof(uint32_t));
	}
	*draw = _kiia_span;
	return EINA_TRUE;
}

static void _kiia_sw_hints(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Rop rop EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_SW_HINT_COLORIZE;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RASTERIZER_DESCRIPTOR,
		Enesim_Rasterizer_Kiia, Enesim_Rasterizer_Kiia_Class,
		enesim_rasterizer_kiia);

static void _enesim_rasterizer_kiia_class_init(void *k)
{
	Enesim_Renderer_Class *r_klass;
	Enesim_Rasterizer_Class *klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _kiia_name;
	r_klass->sw_setup = _kiia_sw_setup;
	r_klass->sw_hints_get = _kiia_sw_hints;

	klass = ENESIM_RASTERIZER_CLASS(k);
	klass->sw_cleanup = _kiia_sw_cleanup;

	/* create the sampling patterns */
	/* 8x8 sparse supersampling mask:
	 * [][][][][]##[][] 5
	 * ##[][][][][][][] 0
	 * [][][]##[][][][] 3
	 * [][][][][][]##[] 6
	 * []##[][][][][][] 1
	 * [][][][]##[][][] 4
	 * [][][][][][][]## 7
	 * [][]##[][][][][] 2
	 */
	_kiia_pattern8[0] = eina_f16p16_double_from(5.0/8.0);
	_kiia_pattern8[1] = eina_f16p16_double_from(0.0/8.0);
	_kiia_pattern8[2] = eina_f16p16_double_from(3.0/8.0);
	_kiia_pattern8[3] = eina_f16p16_double_from(6.0/8.0);
	_kiia_pattern8[4] = eina_f16p16_double_from(1.0/8.0);
	_kiia_pattern8[5] = eina_f16p16_double_from(4.0/8.0);
	_kiia_pattern8[6] = eina_f16p16_double_from(7.0/8.0);
	_kiia_pattern8[7] = eina_f16p16_double_from(2.0/8.0);

	/* 16x16 sparse supersampling mask:
	 * []##[][][][][][][][][][][][][][] 1
	 * [][][][][][][][]##[][][][][][][] 8
	 * [][][][]##[][][][][][][][][][][] 4
	 * [][][][][][][][][][][][][][][]## 15
	 * [][][][][][][][][][][]##[][][][] 11
	 * [][]##[][][][][][][][][][][][][] 2
	 * [][][][][][]##[][][][][][][][][] 6
	 * [][][][][][][][][][][][][][]##[] 14
	 * [][][][][][][][][][]##[][][][][] 10
	 * [][][]##[][][][][][][][][][][][] 3
	 * [][][][][][][]##[][][][][][][][] 7 
	 * [][][][][][][][][][][][]##[][][] 12
	 * ##[][][][][][][][][][][][][][][] 0
	 * [][][][][][][][][]##[][][][][][] 9
	 * [][][][][]##[][][][][][][][][][] 5
	 * [][][][][][][][][][][][][]##[][] 13
	 */
	_kiia_pattern16[0] = eina_f16p16_double_from(1.0/16.0);
	_kiia_pattern16[1] = eina_f16p16_double_from(8.0/16.0);
	_kiia_pattern16[2] = eina_f16p16_double_from(4.0/16.0);
	_kiia_pattern16[3] = eina_f16p16_double_from(15.0/16.0);
	_kiia_pattern16[4] = eina_f16p16_double_from(11.0/16.0);
	_kiia_pattern16[5] = eina_f16p16_double_from(2.0/16.0);
	_kiia_pattern16[6] = eina_f16p16_double_from(6.0/16.0);
	_kiia_pattern16[7] = eina_f16p16_double_from(14.0/16.0);
	_kiia_pattern16[8] = eina_f16p16_double_from(10.0/16.0);
	_kiia_pattern16[9] = eina_f16p16_double_from(3.0/16.0);
	_kiia_pattern16[10] = eina_f16p16_double_from(7.0/16.0);
	_kiia_pattern16[11] = eina_f16p16_double_from(12.0/16.0);
	_kiia_pattern16[12] = eina_f16p16_double_from(0.0/16.0);
	_kiia_pattern16[13] = eina_f16p16_double_from(9.0/16.0);
	_kiia_pattern16[14] = eina_f16p16_double_from(5.0/16.0);
	_kiia_pattern16[15] = eina_f16p16_double_from(13.0/16.0);

	/* 32x32 sparse supersampling mask
	 * [][][][][][][][][][][][][][][][][][][][][][][][][][][][]##[][][] 28
	 * [][][][][][][][][][][][][]##[][][][][][][][][][][][][][][][][][] 13
	 * [][][][][][]##[][][][][][][][][][][][][][][][][][][][][][][][][] 6
	 * [][][][][][][][][][][][][][][][][][][][][][][]##[][][][][][][][] 23
	 * ##[][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][] 0
	 * [][][][][][][][][][][][][][][][][]##[][][][][][][][][][][][][][] 17
	 * [][][][][][][][][][]##[][][][][][][][][][][][][][][][][][][][][] 10
	 * [][][][][][][][][][][][][][][][][][][][][][][][][][][]##[][][][] 27
	 * [][][][]##[][][][][][][][][][][][][][][][][][][][][][][][][][][] 4
	 * [][][][][][][][][][][][][][][][][][][][][]##[][][][][][][][][][] 21
	 * [][][][][][][][][][][][][][]##[][][][][][][][][][][][][][][][][] 14
	 * [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]## 31
	 * [][][][][][][][]##[][][][][][][][][][][][][][][][][][][][][][][] 8
	 * [][][][][][][][][][][][][][][][][][][][][][][][][]##[][][][][][] 25
	 * [][][][][][][][][][][][][][][][][][]##[][][][][][][][][][][][][] 18
	 * [][][]##[][][][][][][][][][][][][][][][][][][][][][][][][][][][] 3
	 * [][][][][][][][][][][][]##[][][][][][][][][][][][][][][][][][][] 12
	 * [][][][][][][][][][][][][][][][][][][][][][][][][][][][][]##[][] 29
	 * [][][][][][][][][][][][][][][][][][][][][][]##[][][][][][][][][] 22
	 * [][][][][][][]##[][][][][][][][][][][][][][][][][][][][][][][][] 7
	 * [][][][][][][][][][][][][][][][]##[][][][][][][][][][][][][][][] 16
	 * []##[][][][][][][][][][][][][][][][][][][][][][][][][][][][][][] 1
	 * [][][][][][][][][][][][][][][][][][][][][][][][][][]##[][][][][] 26
	 * [][][][][][][][][][][]##[][][][][][][][][][][][][][][][][][][][] 11
	 * [][][][][][][][][][][][][][][][][][][][]##[][][][][][][][][][][] 20
	 * [][][][][]##[][][][][][][][][][][][][][][][][][][][][][][][][][] 5
	 * [][][][][][][][][][][][][][][][][][][][][][][][][][][][][][]##[] 30
	 * [][][][][][][][][][][][][][][]##[][][][][][][][][][][][][][][][] 15
	 * [][][][][][][][][][][][][][][][][][][][][][][][]##[][][][][][][] 24
	 * [][][][][][][][][]##[][][][][][][][][][][][][][][][][][][][][][] 9
	 * [][]##[][][][][][][][][][][][][][][][][][][][][][][][][][][][][] 2
	 * [][][][][][][][][][][][][][][][][][][]##[][][][][][][][][][][][] 19
	 */
	_kiia_pattern32[0] = eina_f16p16_double_from(28.0/32.0);
	_kiia_pattern32[1] = eina_f16p16_double_from(13.0/32.0);
	_kiia_pattern32[2] = eina_f16p16_double_from(6.0/32.0);
	_kiia_pattern32[3] = eina_f16p16_double_from(23.0/32.0);
	_kiia_pattern32[4] = eina_f16p16_double_from(0.0/32.0);
	_kiia_pattern32[5] = eina_f16p16_double_from(17.0/32.0);
	_kiia_pattern32[6] = eina_f16p16_double_from(10.0/32.0);
	_kiia_pattern32[7] = eina_f16p16_double_from(27.0/32.0);
	_kiia_pattern32[8] = eina_f16p16_double_from(4.0/32.0);
	_kiia_pattern32[9] = eina_f16p16_double_from(21.0/32.0);
	_kiia_pattern32[10] = eina_f16p16_double_from(14.0/32.0);
	_kiia_pattern32[11] = eina_f16p16_double_from(31.0/32.0);
	_kiia_pattern32[12] = eina_f16p16_double_from(8.0/32.0);
	_kiia_pattern32[13] = eina_f16p16_double_from(25.0/32.0);
	_kiia_pattern32[14] = eina_f16p16_double_from(18.0/32.0);
	_kiia_pattern32[15] = eina_f16p16_double_from(3.0/32.0);
	_kiia_pattern32[16] = eina_f16p16_double_from(12.0/32.0);
	_kiia_pattern32[17] = eina_f16p16_double_from(29.0/32.0);
	_kiia_pattern32[18] = eina_f16p16_double_from(22.0/32.0);
	_kiia_pattern32[19] = eina_f16p16_double_from(7.0/32.0);
	_kiia_pattern32[20] = eina_f16p16_double_from(16.0/32.0);
	_kiia_pattern32[21] = eina_f16p16_double_from(1.0/32.0);
	_kiia_pattern32[22] = eina_f16p16_double_from(26.0/32.0);
	_kiia_pattern32[23] = eina_f16p16_double_from(11.0/32.0);
	_kiia_pattern32[24] = eina_f16p16_double_from(20.0/32.0);
	_kiia_pattern32[25] = eina_f16p16_double_from(5.0/32.0);
	_kiia_pattern32[26] = eina_f16p16_double_from(30.0/32.0);
	_kiia_pattern32[27] = eina_f16p16_double_from(15.0/32.0);
	_kiia_pattern32[28] = eina_f16p16_double_from(24.0/32.0);
	_kiia_pattern32[29] = eina_f16p16_double_from(9.0/32.0);
	_kiia_pattern32[30] = eina_f16p16_double_from(2.0/32.0);
	_kiia_pattern32[31] = eina_f16p16_double_from(19.0/32.0);
}

static void _enesim_rasterizer_kiia_instance_init(void *o)
{
	Enesim_Rasterizer_Kiia *thiz;

	thiz = ENESIM_RASTERIZER_KIIA(o);
	thiz->nworkers = enesim_renderer_sw_cpu_count();
	thiz->workers = calloc(thiz->nworkers, sizeof(Enesim_Rasterizer_Kiia_Worker));
}

static void _enesim_rasterizer_kiia_instance_deinit(void *o)
{
	Enesim_Rasterizer_Kiia *thiz;

	thiz = ENESIM_RASTERIZER_KIIA(o);
	free(thiz->workers);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_rasterizer_kiia_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_rasterizer_kiia);
	return r;
}
/** @endcond */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
