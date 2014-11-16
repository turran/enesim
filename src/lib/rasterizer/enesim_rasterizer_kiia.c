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
	/* a span of length equal to the width of the bounds */
	uint32_t *mask;
	/* keep track of the current Y this worker is doing, to ease
	 * the x increment on the edges
	 */
	int y;
} Enesim_Rasterizer_Kiia_Worker;

/* Its own definition of an edge */
typedef struct _Enesim_Rasterizer_Kiia_Edge
{
	Eina_F16p16 xx0, yy0, xx1, yy1;
	Eina_F16p16 mx;
	Eina_F16p16 slope;
	int sgn;
} Enesim_Rasterizer_Kiia_Edge;

typedef struct _Enesim_Rasterizer_Kiia
{
	Enesim_Rasterizer parent;
	/* private */
	/* One worker per cpu */
	Enesim_Rasterizer_Kiia_Worker *workers;
	int nworkers;
	Enesim_Rasterizer_Kiia_Edge *edges;
	int nedges;
} Enesim_Rasterizer_Kiia;

typedef struct _Enesim_Rasterizer_Kiia_Class {
	Enesim_Rasterizer_Class parent;
} Enesim_Rasterizer_Kiia_Class;

static Eina_Bool _kiia_edge_setup(Enesim_Rasterizer_Kiia_Edge *thiz,
		Enesim_Point *p0, Enesim_Point *p1)
{
	double x0, y0, x1, y1;
	double x01, y01;
	double slope;
	double mx;

	x0 = p0->x;
	y0 = p0->y;
	x1 = p1->x;
	y1 = p1->y;

	printf("y = %g %g\n", y0, ((int)(y0 * 4.0))/4.0);

	thiz->xx0 = eina_f16p16_double_from(x0);
	thiz->yy0 = eina_f16p16_double_from(y0);
	thiz->xx1 = eina_f16p16_double_from(x1);
	thiz->yy1 = eina_f16p16_double_from(y1);
	/* get the slope */
	x01 = x1 - x0;
	y01 = y1 - y0;
	slope = x01 / y01;
	thiz->slope = eina_f16p16_double_from(slope);
	/* get the sign */
	if ((thiz->yy0 == thiz->yy1) || (thiz->xx0 == thiz->xx1))
		thiz->sgn = 0;
	else
	{
		thiz->sgn = 1;
		if (thiz->yy1 > thiz->yy0)
		{
			if (thiz->xx1 < thiz->xx0)
				thiz->sgn = -1;
		}
		else
		{
			 if (thiz->xx1 > thiz->xx0)
				thiz->sgn = -1;
		}
	}

	if (thiz->xx0 > thiz->xx1)
	{
		Eina_F16p16 xx0 = thiz->xx0;
		thiz->xx0 = thiz->xx1;
		thiz->xx1 = xx0;
	}

	if (thiz->yy0 > thiz->yy1)
	{
		Eina_F16p16 yy0 = thiz->yy0;
		thiz->yy0 = thiz->yy1;
		thiz->yy1 = yy0;
	}
	return EINA_TRUE;
}

static Eina_Bool _kiia_edges_setup(Enesim_Renderer *r)
{
	Enesim_Rasterizer_Kiia *thiz;
	Enesim_Rasterizer *rr;
	Enesim_Polygon *p;
	const Enesim_Figure *f;
	Eina_List *l1;
	int n;

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
		Enesim_Point *fp, *lp, *pt, *pp;
		Eina_List *points, *l2;

		fp = eina_list_data_get(p->points);
		pp = fp;
		points = eina_list_next(p->points);
		EINA_LIST_FOREACH(points, l2, pt)
		{
			Enesim_Rasterizer_Kiia_Edge *e = &thiz->edges[n];

			_kiia_edge_setup(e, pp, pt);
			pp = pt;
			n++;
		}
	}
	return EINA_TRUE;
}

static void _kiia_span(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Rasterizer_Kiia *thiz;
	Enesim_Rasterizer_Kiia_Worker *w;
	uint32_t *dst = ddata;
	uint32_t *buf;

	thiz = ENESIM_RASTERIZER_KIIA(r);
	w = &thiz->workers[y % thiz->nworkers];

	//printf("y = %d thread %d old y = %d\n", y, y % thiz->nworkers, w->y);
	/* iterate over the subpixel y and set the mask value */
	/* iterate over the mask and fill */ 
	/* update the latest y coordinate of the worker */
	w->y = y;
}
/*----------------------------------------------------------------------------*
 *                           Rasterizer interface                             *
 *----------------------------------------------------------------------------*/
static void _kiia_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Rasterizer_Kiia *thiz;
	int i;

	thiz = ENESIM_RASTERIZER_KIIA(r);
	/* cleanup the workers */
	for (i = 0; i < thiz->nworkers; i++)
	{
		free(thiz->workers[i].mask);
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
	double lx, rx, ty, by;
	int len;
	int i;
	int y;

	/* create the edges */
	if (!_kiia_edges_setup(r))
		return EINA_FALSE;

	rr = ENESIM_RASTERIZER(r);
	if (!enesim_figure_bounds(rr->figure, &lx, &ty, &rx, &by))
		return EINA_FALSE;

	thiz = ENESIM_RASTERIZER_KIIA(r);

	/* set the y coordinate with the topmost value */
	y = ceil(ty);
	/* the length of the mask buffer */
	len = ceil(rx - lx) + 1;
	/* setup the workers */
	for (i = 0; i < thiz->nworkers; i++)
	{
		thiz->workers[i].y = y;
		thiz->workers[i].mask = malloc(sizeof(uint32_t) * len * 32);
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
