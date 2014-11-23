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
#include "enesim_path.h"
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
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_path_abstract_private.h"
/* Add support for the different fill rules
 * Add support for the different qualities
 * Modify it to handle the current_figure
 * Make it generate both figures
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_RENDERER_PATH_KIIA(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Path_Kiia,					\
		enesim_renderer_path_kiia_descriptor_get())

/* A worker is in charge of rasterize one span */
typedef struct _Enesim_Renderer_Path_Kiia_Worker
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
} Enesim_Renderer_Path_Kiia_Worker;

/* Its own definition of an edge */
typedef struct _Enesim_Renderer_Path_Kiia_Edge
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
} Enesim_Renderer_Path_Kiia_Edge;

typedef struct _Enesim_Renderer_Path_Kiia_Figure
{
	Enesim_Figure *figure;
	Enesim_Renderer_Path_Kiia_Edge *edges;
	int nedges;
	Enesim_Renderer *ren;
	Enesim_Color color;
} Enesim_Renderer_Path_Kiia_Figure;

typedef struct _Enesim_Renderer_Path_Kiia
{
	Enesim_Renderer_Path_Abstract parent;
	/* private */
	/* To generate the figures */
	Enesim_Path_Generator *stroke_path;
	Enesim_Path_Generator *strokeless_path;
	Enesim_Path_Generator *dashed_path;

	/* The figures themselves */
	Enesim_Renderer_Path_Kiia_Figure fill;
	Enesim_Renderer_Path_Kiia_Figure stroke;
	Enesim_Renderer_Path_Kiia_Figure *current;

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
	Enesim_Renderer_Path_Kiia_Worker *workers;
	int nworkers;
} Enesim_Renderer_Path_Kiia;

typedef struct _Enesim_Renderer_Path_Kiia_Class {
	Enesim_Renderer_Path_Abstract_Class parent;
} Enesim_Renderer_Path_Kiia_Class;

static Eina_F16p16 _kiia_pattern8[8];
static Eina_F16p16 _kiia_pattern16[16];
static Eina_F16p16 _kiia_pattern32[32];

static int _kiia_edge_sort(const void *l, const void *r)
{
	Enesim_Renderer_Path_Kiia_Edge *lv = (Enesim_Renderer_Path_Kiia_Edge *)l;
	Enesim_Renderer_Path_Kiia_Edge *rv = (Enesim_Renderer_Path_Kiia_Edge *)r;

	if (lv->yy0 <= rv->yy0)
		return -1;
	return 1;
}

static Eina_Bool _kiia_edge_setup(Enesim_Renderer_Path_Kiia_Edge *thiz,
		Enesim_Point *p0, Enesim_Point *p1, double nsamples)
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
	start = (((int)(y0 * nsamples) )/ nsamples);
	y1 = (((int)(y1 * nsamples)) / nsamples);
	mx = x0 + (slope * (start - y0));

	thiz->yy0 = eina_f16p16_double_from(start);
	thiz->yy1 = eina_f16p16_double_from(y1);
	thiz->xx0 = eina_f16p16_double_from(x0);
	thiz->xx1 = eina_f16p16_double_from(x1);
	thiz->slope = eina_f16p16_double_from(slope/nsamples);
	thiz->mx = eina_f16p16_double_from(mx);
	thiz->sgn = sgn;

	return EINA_TRUE;
}

static Enesim_Renderer_Path_Kiia_Edge * _kiia_edges_setup(Enesim_Figure *f,
		int nsamples, int *nedges)
{
	Enesim_Renderer_Path_Kiia_Edge *edges;
	Enesim_Polygon *p;
	Eina_List *l1;
	int n = 0;

	/* allocate the maximum number of possible edges */
	EINA_LIST_FOREACH(f->polygons, l1, p)
		n += enesim_polygon_point_count(p);
	edges = malloc(n * sizeof(Enesim_Renderer_Path_Kiia_Edge));

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
			Enesim_Renderer_Path_Kiia_Edge *e = &edges[n];

			if (_kiia_edge_setup(e, pp, pt, nsamples))
				n++;
			pp = pt;
		}
		/* add the last point in case the polygon is closed */
		if (p->closed)
		{
			Enesim_Renderer_Path_Kiia_Edge *e = &edges[n];
			if (_kiia_edge_setup(e, pp, fp, nsamples))
				n++;
		}
	}
	qsort(edges, n, sizeof(Enesim_Renderer_Path_Kiia_Edge), _kiia_edge_sort);
	*nedges = n;
	return edges;
}

static Eina_Bool _kiia_figures_generate(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Kiia *thiz;
	Enesim_Renderer_Path_Abstract *pa;
	Enesim_Renderer_Shape_Draw_Mode dm;
	Enesim_Matrix transformation;
	Enesim_Renderer_Shape_Stroke_Join join;
	Enesim_Renderer_Shape_Stroke_Cap cap;
	Enesim_Path_Generator *generator;
	Enesim_List *dashes;
	Eina_List *dashes_l;
	Eina_Bool stroke_scalable;
	double stroke_weight;

	thiz = ENESIM_RENDERER_PATH_KIIA(r);

	/* TODO check that we actually need to generate */
	if (thiz->fill.figure)
		enesim_figure_clear(thiz->fill.figure);
	else
		thiz->fill.figure = enesim_figure_new();

	if (thiz->stroke.figure)
		enesim_figure_clear(thiz->stroke.figure);
	else
		thiz->stroke.figure = enesim_figure_new();


	dm = enesim_renderer_shape_draw_mode_get(r);
	dashes = enesim_renderer_shape_dashes_get(r);
	dashes_l = dashes->l;

	/* decide what generator to use */
	/* for a stroke smaller than 1px we will use the basic
	 * rasterizer directly, so we dont need to generate the
	 * stroke path
	 */
	if (dm & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE) 
	{
		if (!dashes_l)
			generator = thiz->stroke_path;
		else
			generator = thiz->dashed_path;
	}
	else
	{
		generator = thiz->strokeless_path;
	}
	enesim_list_unref(dashes);

	join = enesim_renderer_shape_stroke_join_get(r);
	cap = enesim_renderer_shape_stroke_cap_get(r);
	stroke_weight = enesim_renderer_shape_stroke_weight_get(r);
	stroke_scalable = enesim_renderer_shape_stroke_scalable_get(r);
	enesim_renderer_transformation_get(r, &transformation);

	enesim_path_generator_figure_set(generator, thiz->fill.figure);
	enesim_path_generator_stroke_figure_set(generator, thiz->stroke.figure);
	enesim_path_generator_stroke_cap_set(generator, cap);
	enesim_path_generator_stroke_join_set(generator, join);
	enesim_path_generator_stroke_weight_set(generator, stroke_weight);
	enesim_path_generator_stroke_scalable_set(generator, stroke_scalable);
	enesim_path_generator_stroke_dash_set(generator, dashes_l);
	enesim_path_generator_scale_set(generator, 1, 1);
	enesim_path_generator_transformation_set(generator, &transformation);

	/* Finall generate the figures */
	pa = ENESIM_RENDERER_PATH_ABSTRACT(r);
	enesim_path_generator_generate(generator, pa->path->commands);
	return EINA_TRUE;
}

static Eina_Bool _kiia_edges_generate(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Kiia *thiz;
	Enesim_Renderer_Shape_Draw_Mode dm;

	thiz = ENESIM_RENDERER_PATH_KIIA(r);
	if (thiz->fill.edges)
	{
		free(thiz->fill.edges);
		thiz->fill.edges = NULL;
	}

	if (thiz->stroke.edges)
	{
		free(thiz->stroke.edges);
		thiz->stroke.edges = NULL;
	}

	dm = enesim_renderer_shape_draw_mode_get(r);
	if (dm & ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL)
	{
		thiz->fill.edges = _kiia_edges_setup(thiz->fill.figure,
				thiz->nsamples, &thiz->fill.nedges);
		if (!thiz->fill.edges)
			return EINA_FALSE;
	}
	if (dm & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
	{
		thiz->stroke.edges = _kiia_edges_setup(thiz->stroke.figure,
				thiz->nsamples, &thiz->stroke.nedges);
		if (!thiz->stroke.edges)
			return EINA_FALSE;
	}
	return EINA_TRUE;
}

static inline void _kiia_even_odd_sample(Enesim_Renderer_Path_Kiia_Worker *w,
		int x, int m, int winding EINA_UNUSED)
{
	w->mask[x] ^= m;
}

static inline uint32_t _kiia_even_odd_get_mask(Enesim_Renderer_Path_Kiia_Worker *w, int i, int cm)
{
	cm ^= w->mask[i];
	w->mask[i] = 0;
	return cm;
}

static inline void _kiia_non_zero_sample(Enesim_Renderer_Path_Kiia_Worker *w,
		int x, int m, int winding)
{
	w->mask[x] ^= m;
	w->winding[x] += winding;
}

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

static inline uint16_t _kiia_16_get_alpha(int cm)
{
	uint16_t coverage;

	/* use the hamming weight to know the number of bits set to 1 */
	cm = cm - ((cm >> 1) & 0x55555555);
	cm = (cm & 0x33333333) + ((cm >> 2) & 0x33333333);
	/* we use 20 instead of 24, because we need to rescale 16 -> 256 */
	coverage = (((cm + (cm >> 4)) & 0x0f0f0f0f) * 0x01010101) >> 20;

	return coverage;
}

static inline uint32_t _kiia_non_zero_get_mask(Enesim_Renderer_Path_Kiia_Worker *w, int i, int cm)
{
	uint32_t ret;
	int m;
	int winding;

	m = w->mask[i];
	winding = w->winding[i];
	w->cwinding += winding;
	/* TODO Use the number of samples */
	if (abs(w->cwinding) < 32)
	{
		if (w->cwinding == 0)
			ret = 0;
		else
			ret = cm ^ m;
	}
	else
	{
		ret = 0xffffffff;
	}
	/* reset the winding */
	w->winding[i] = 0;
	return ret;
}

static inline void _kiia_figure_draw(Enesim_Renderer *r,
		Enesim_Renderer_Path_Kiia_Figure *f, int x, int y, int len,
		void *ddata)
{
	Enesim_Renderer_Path_Kiia *thiz;
	Enesim_Renderer_Path_Kiia_Worker *w;
	Eina_F16p16 yy0, yy1;
	Eina_F16p16 sinc;
	uint32_t *dst = ddata;
	uint32_t *end, *rend = dst + len;
	uint32_t cm = 0;
	int lx, mlx = INT_MAX;
	int rx, mrx = -INT_MAX;
	int i;

	thiz = ENESIM_RENDERER_PATH_KIIA(r);
	w = &thiz->workers[y % thiz->nworkers];

	yy0 = eina_f16p16_int_from(y);
	yy1 = eina_f16p16_int_from(y + 1);
	sinc = thiz->inc;
	/* intersect with each edge */
	for (i = 0; i < f->nedges; i++)
	{
		Enesim_Renderer_Path_Kiia_Edge *edge = &f->edges[i];
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
			//sample = (eina_f16p16_fracc_get(yyy0) >> 12); /* 16.16 << nsamples */
			m = 1 << sample;
		}
		else
		{
			Eina_F16p16 inc;

			/* TODO use the correct sample */
			inc = eina_f16p16_mul(yyy0 - edge->yy0, eina_f16p16_int_from(32));
			//inc = eina_f16p16_mul(yyy0 - edge->yy0, eina_f16p16_int_from(16));
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
			if (mx < mlx)
				mlx = mx;
			if (mx > mrx)
				mrx = mx;
#if 0
			_kiia_even_odd_sample(w, mx, m, edge->sgn);
#else
			_kiia_non_zero_sample(w, mx, m, edge->sgn);
#endif
			cx += edge->slope;
			pattern++;
			m <<= 1;
		}
	}

	/* does not intersect with anything */
	if (mlx == INT_MAX)
	{
		memset(dst, 0, len * sizeof(uint32_t));
		return;
	}

	/* clip on the left side [x.. left] */
	lx = x - thiz->lx;
	if (lx < mlx)
	{
		int adv;

		adv = mlx - lx;
		memset(dst, 0, adv * sizeof(uint32_t));
		len -= adv;
		dst += adv;
		lx = mlx;
		i = mlx;
	}
	/* advance the mask until we reach the requested x */
	/* also clear the mask in the process */
	else
	{
		for (i = mlx; i < lx; i++)
		{
#if 0
			cm ^= w->mask[i];
#else
			cm = _kiia_non_zero_get_mask(w, i, cm);
#endif
		}
	}
	/* clip on the right side [right ... x + len] */
	rx = lx + len;
	/* given that we always jump to the next pixel because of the offset
	 * pattern, start counting on the next pixel */
	mrx += 1;
	if (rx > mrx)
	{
		int adv;

		adv = rx - mrx;
		len -= adv;
		rx = mrx;
	}

	/* time to draw */
 	end = dst + len;
	if (f->ren)
	{
		enesim_renderer_sw_draw(f->ren, x, y, len, dst);
	}
	/* iterate over the mask and fill */ 
	while (dst < end)
	{
		uint32_t p0;

#if 0
		cm ^= w->mask[i];
#else
		cm = _kiia_non_zero_get_mask(w, i, cm);
#endif
		w->mask[i] = 0;
		if (cm == 0xffffffff)
		{
			if (f->ren)
			{
				if (f->color != 0xffffffff)
				{
					p0 = *dst;
					p0 = enesim_color_mul4_sym(p0, f->color);
				}
				else
				{
					goto next;
				}
			}
			else
			{
				p0 = f->color;
			}
		}
		else if (cm == 0)
		{
			p0 = 0;
		}
		else
		{
			uint16_t coverage;

			coverage = _kiia_32_get_alpha(cm);
			if (f->ren)
			{
				uint32_t q0 = *dst;

				if (f->color != 0xffffffff)
					q0 = enesim_color_mul4_sym(f->color, q0);
				p0 = enesim_color_mul_256(coverage, q0);
			}
			else
			{
				p0 = enesim_color_mul_256(coverage, f->color);
			}
		}
		*dst = p0;
next:
		dst++;
		i++;
	}
	/* finally memset on dst at the end to keep the correct order on the
	 * dst access
	 */
	if (dst < rend)
	{
		memset(dst, 0, (rend - dst) * sizeof(uint32_t));
	}
	/* set to zero the rest of the bits of the mask */
	else
	{
		for (i = rx; i < mrx; i++)
		{
			w->mask[i] = 0;
		}
	}
	/* update the latest y coordinate of the worker */
	w->y = y;
	w->cwinding = 0;

}

static void _kiia_span(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Path_Kiia *thiz;

	thiz = ENESIM_RENDERER_PATH_KIIA(r);
	_kiia_figure_draw(r, thiz->current, x, y, len, ddata);
}
/*----------------------------------------------------------------------------*
 *                               Path abstract                                *
 *----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _kiia_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "kiia";
}

static void _kiia_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_BACKEND_SOFTWARE |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static Eina_Bool _kiia_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *draw, Enesim_Log **error EINA_UNUSED)
{
	Enesim_Renderer_Path_Kiia *thiz;
	Enesim_Renderer_Shape_Draw_Mode dm;
	Enesim_Color color;
	double lx, rx, ty, by;
	int len;
	int i;
	int y;

	thiz = ENESIM_RENDERER_PATH_KIIA(r);
	/* TODO use the quality, 32 samples for now */
	thiz->nsamples = 32;
	/* Convert the path to a figure */
	/* TODO later all of this will be on the generate interface */
	if (!_kiia_figures_generate(r))
		return EINA_FALSE;
	if (!_kiia_edges_generate(r))
		return EINA_FALSE;

	/* setup the fill properties */
	thiz->fill.ren = enesim_renderer_shape_fill_renderer_get(r);
	thiz->fill.color = enesim_renderer_shape_fill_color_get(r);
	/* setup the stroke properties */
	thiz->stroke.ren = enesim_renderer_shape_stroke_renderer_get(r);
	thiz->stroke.color = enesim_renderer_shape_stroke_color_get(r);
	/* simplify the calcs */
	color = enesim_renderer_color_get(r);
	if (color != ENESIM_COLOR_FULL)
	{
		thiz->stroke.color = enesim_color_mul4_sym(thiz->stroke.color, color);
		thiz->fill.color = enesim_color_mul4_sym(thiz->fill.color, color);
	}
	dm = enesim_renderer_shape_draw_mode_get(r);
	if (dm == ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL)
	{
		thiz->current = &thiz->fill;
	}
	else if (dm == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
	{
		thiz->current = &thiz->stroke;
	}
	else
	{
		/* TODO handle both at once */
		return EINA_FALSE;
	}

	switch (thiz->nsamples)
	{
		case 8:
		thiz->pattern = _kiia_pattern8;
		break;

		case 16:
		thiz->pattern = _kiia_pattern16;
		break;

		case 32:
		thiz->pattern = _kiia_pattern32;
		break;
	}
	thiz->inc = eina_f16p16_double_from(1/(double)thiz->nsamples);
	/* TODO snap the coordinates */
	if (!enesim_figure_bounds(thiz->current->figure, &lx, &ty, &rx, &by))
		return EINA_FALSE;
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
		thiz->workers[i].winding = calloc((len + 1), sizeof(uint32_t));
	}
	*draw = _kiia_span;
	return EINA_TRUE;
}

static void _kiia_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Path_Kiia *thiz;
	int i;

	thiz = ENESIM_RENDERER_PATH_KIIA(r);
	enesim_renderer_unref(thiz->fill.ren);
	enesim_renderer_unref(thiz->stroke.ren);
	/* cleanup the workers */
	for (i = 0; i < thiz->nworkers; i++)
	{
		free(thiz->workers[i].mask);
		free(thiz->workers[i].winding);
	}
}

static void _kiia_sw_hints(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Rop rop EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_SW_HINT_COLORIZE;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_PATH_ABSTRACT_DESCRIPTOR,
		Enesim_Renderer_Path_Kiia, Enesim_Renderer_Path_Kiia_Class,
		enesim_renderer_path_kiia);

static void _enesim_renderer_path_kiia_class_init(void *k)
{
	Enesim_Renderer_Class *r_klass;
	Enesim_Renderer_Shape_Class *s_klass;
	Enesim_Renderer_Path_Abstract_Class *klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _kiia_name;
	r_klass->features_get = _kiia_features_get;
	r_klass->sw_hints_get = _kiia_sw_hints;

	s_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	s_klass->sw_setup = _kiia_sw_setup;
	s_klass->sw_cleanup = _kiia_sw_cleanup;

	klass = ENESIM_RENDERER_PATH_ABSTRACT_CLASS(k);

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

static void _enesim_renderer_path_kiia_instance_init(void *o)
{
	Enesim_Renderer_Path_Kiia *thiz;

	thiz = ENESIM_RENDERER_PATH_KIIA(o);
	thiz->nworkers = enesim_renderer_sw_cpu_count();
	thiz->workers = calloc(thiz->nworkers, sizeof(Enesim_Renderer_Path_Kiia_Worker));

	/* create the different path implementations */
	thiz->stroke_path = enesim_path_generator_stroke_dashless_new();
	thiz->strokeless_path = enesim_path_generator_strokeless_new();
	thiz->dashed_path = enesim_path_generator_dashed_new();
}

static void _enesim_renderer_path_kiia_instance_deinit(void *o)
{
	Enesim_Renderer_Path_Kiia *thiz;

	thiz = ENESIM_RENDERER_PATH_KIIA(o);
	/* Remove the figures */
	if (thiz->stroke.figure)
		enesim_figure_delete(thiz->stroke.figure);
	if (thiz->fill.figure)
		enesim_figure_delete(thiz->fill.figure);
	/* Remove the figure generators */
	if (thiz->dashed_path)
		enesim_path_generator_free(thiz->dashed_path);
	if (thiz->strokeless_path)
		enesim_path_generator_free(thiz->strokeless_path);
	if (thiz->stroke_path)
		enesim_path_generator_free(thiz->stroke_path);
	/* Remove the workers */
	free(thiz->workers);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_path_kiia_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_path_kiia);
	return r;
}
/** @endcond */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
