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
#include <float.h>

#include "enesim_renderer_path_kiia_private.h"
#include "enesim_surface_private.h"

#ifdef BUILD_OPENCL
#include "Enesim_OpenCL.h"
#endif

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
static Eina_F16p16 _kiia_pattern8[8];
static Eina_F16p16 _kiia_pattern16[16];
static Eina_F16p16 _kiia_pattern32[32];

static Enesim_Renderer_Sw_Fill _fill_simple[3][ENESIM_RENDERER_SHAPE_FILL_RULES][2];
static Enesim_Renderer_Sw_Fill _fill_full[3][ENESIM_RENDERER_SHAPE_FILL_RULES][2][2];
static Enesim_Renderer_Path_Kiia_Worker_Setup _worker_setup[3][ENESIM_RENDERER_SHAPE_FILL_RULES];
static Eina_F16p16 *_patterns[3];


/*----------------------------------------------------------------------------*
 *                              Edge helpers                                  *
 *----------------------------------------------------------------------------*/
typedef int (*Enesim_Renderer_Path_Kiia_Edge_Cmp)(const void *edge1,
		const void *edge2);
typedef void (*Enesim_Renderer_Path_Kiia_Edge_Store)(
		Enesim_Renderer_Path_Kiia_Edge *thiz, void *edges, int at);

static void _kiia_edge_to_sw(Enesim_Renderer_Path_Kiia_Edge *thiz,
		Enesim_Renderer_Path_Kiia_Edge_Sw *sw)
{
	sw->yy0 = eina_f16p16_double_from(thiz->y0);
	sw->yy1 = eina_f16p16_double_from(thiz->y1);
	sw->xx0 = eina_f16p16_double_from(thiz->x0);
	sw->xx1 = eina_f16p16_double_from(thiz->x1);
	sw->slope = eina_f16p16_double_from(thiz->slope);
	sw->mx = eina_f16p16_double_from(thiz->mx);
	sw->sgn = thiz->sgn;
}

static void _kiia_edge_store(Enesim_Renderer_Path_Kiia_Edge *thiz, void *data,
		int at)
{
	Enesim_Renderer_Path_Kiia_Edge_Sw *edges = data;
	Enesim_Renderer_Path_Kiia_Edge_Sw *sw;

	sw = &edges[at];
	_kiia_edge_to_sw(thiz, sw);
}

static int _kiia_edge_cmp(const void *l, const void *r)
{
	Enesim_Renderer_Path_Kiia_Edge_Sw *lv = (Enesim_Renderer_Path_Kiia_Edge_Sw *)l;
	Enesim_Renderer_Path_Kiia_Edge_Sw *rv = (Enesim_Renderer_Path_Kiia_Edge_Sw *)r;

	if (lv->yy0 <= rv->yy0)
		return -1;
	return 1;
}

#if BUILD_OPENCL
static void _kiia_edge_cl_store(Enesim_Renderer_Path_Kiia_Edge *thiz,
		void *data, int at)
{
	cl_float *edges = data;
	cl_float *edge = &edges[at * 7]; // 7 members per edge
	edge[0] = thiz->x0;
	edge[1] = thiz->y0;
	edge[2] = thiz->x1;
	edge[3] = thiz->y1;
	edge[4] = thiz->slope;
	edge[5] = thiz->mx;
	edge[6] = thiz->sgn;
}

static int _kiia_edge_cl_cmp(const void *l, const void *r)
{
	const cl_float *lv = l;
	const cl_float *rv = r;

	/* compare y0 */
	if (lv[1] <= rv[1])
		return -1;
	return 1;
}
#endif

/* On the first edge we will modify the first and the second point
 * with the snapped y's coordinates
 */
static Eina_Bool _kiia_edge_first_setup(Enesim_Renderer_Path_Kiia_Edge *thiz,
		Enesim_Point *p0, Enesim_Point *p1, double nsamples)
{
	double x0, y0, x1, y1;
	double x01, y01;
	double slope;
	double mx;
	int sgn;

	//printf("first edge 0: %g %g %g %g\n", p0->x, p0->y, p1->x, p1->y);
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
	/* get the sampled Y inside the line */
	y0 = (ceil(y0 * nsamples) / nsamples);
	y1 = (floor(y1 * nsamples) / nsamples);
	if (y0 == y1)
		return EINA_FALSE;

	/* get the slope */
	x01 = x1 - x0;
	y01 = y1 - y0;

	slope = x01 / y01;
	mx = x0;

	thiz->x0 = x0;
	thiz->y0 = y0;
	thiz->x1 = x1;
	thiz->y1 = y1;
	thiz->slope = slope/nsamples;
	thiz->mx = mx;
	thiz->sgn = sgn;

	if (sgn < 0)
	{
		p0->y = y1;
		p1->y = y0;
	}
	else
	{
		p0->y = y0;
		p1->y = y1;
	}

	//printf("first edge 1: %g %g %g %g (%g)\n", p0->x, p0->y, p1->x, p1->y, slope);

	return EINA_TRUE;
}

/* Only modify the second point, the first is already snapped
 */
static Eina_Bool _kiia_edge_setup(Enesim_Renderer_Path_Kiia_Edge *thiz,
		Enesim_Point *p0, Enesim_Point *p1, double nsamples)
{
	double x0, y0, x1, y1;
	double x01, y01;
	double slope;
	double mx;
	int sgn;

	//printf("edges 0: %g %g %g %g\n", p0->x, p0->y, p1->x, p1->y);
	/* going down, swap the y */
	if (p0->y > p1->y)
	{
		x0 = p1->x;
		y0 = p1->y;
		x1 = p0->x;
		y1 = p0->y;
		/* get the sampled Y inside the line */
		y0 = (ceil(y0 * nsamples) / nsamples);
		sgn = -1;
		/* set the p1 y */
		p1->y = y0;
	}
	else
	{
		x0 = p0->x;
		y0 = p0->y;
		x1 = p1->x;
		y1 = p1->y;
		/* get the sampled Y inside the line */
		y1 = (floor(y1 * nsamples) / nsamples);
		sgn = 1;
		/* set the p1 y */
		p1->y = y1;
	}
	if (y0 == y1)
		return EINA_FALSE;

	/* get the slope */
	x01 = x1 - x0;
	y01 = y1 - y0;

	slope = x01 / y01;
	mx = x0;

	thiz->x0 = x0;
	thiz->y0 = y0;
	thiz->x1 = x1;
	thiz->y1 = y1;
	thiz->slope = slope/nsamples;
	thiz->mx = mx;
	thiz->sgn = sgn;
	//printf("edges 1: %g %g %g %g (%g)\n", p0->x, p0->y, p1->x, p1->y, slope);

	return EINA_TRUE;
}

static Enesim_Renderer_Path_Kiia_Edge_Sw * _kiia_edges_setup(Enesim_Figure *f,
		int nsamples, int *nedges, size_t edge_size,
		Enesim_Renderer_Path_Kiia_Edge_Store store,
		Enesim_Renderer_Path_Kiia_Edge_Cmp cmp)
{
	Enesim_Polygon *p;
	Eina_List *l1;
	void *edges;
	int n = 0;

	/* allocate the maximum number of possible edges */
	EINA_LIST_FOREACH(f->polygons, l1, p)
	{
		n += enesim_polygon_point_count(p);
		if (p->closed && n)
			n++;
		/* one more for the sanity edge, the one that will make
		 * the figure closed
		 */
		n++;
	}
	/* Generic store of edges, or either the fp for sw backend
	 * or the cl one
	 */
	edges = malloc(n * edge_size);

	/* create the edges */
	n = 0;
	EINA_LIST_FOREACH(f->polygons, l1, p)
	{
		Enesim_Point *pt;
		Enesim_Point lp;
		Enesim_Point fp;
		Enesim_Point pp;
		Eina_List *points, *l2;
		Eina_Bool found = EINA_FALSE;

		pt = eina_list_data_get(p->points);
		if (!pt)
			continue;

		fp = lp = pp = *pt;
		points = eina_list_next(p->points);
		/* find the first edge */
		EINA_LIST_FOREACH(points, l2, pt)
		{
			Enesim_Renderer_Path_Kiia_Edge e;
			Enesim_Point cp;

			/* make a copy so we can modify the point */
			cp = *pt;
			if (_kiia_edge_first_setup(&e, &pp, &cp, nsamples))
			{
				store(&e, edges, n);
				/* ok, we found the first edge, update the real
				 * first/last point y */
				fp.y = pp.y;
				lp.y = pp.y;
				/* swap */
				pp = cp;
				n++;
				found = EINA_TRUE;
				break;
			}
			else
			{
				/* just swap */
				pp = cp;
			}
		}
		/* no points left */
		if (!l2)
		{
			if (found)
				n--;
			continue;
		}

		/* iterate over the other edges */
		points = eina_list_next(l2);
		if (!points)
		{
			if (found)
				n--;
			continue;
		}
		EINA_LIST_FOREACH(points, l2, pt)
		{
			Enesim_Renderer_Path_Kiia_Edge e;
			Enesim_Point cp;

			/* make a copy so we can modify the point */
			cp = *pt;
			if (_kiia_edge_setup(&e, &pp, &cp, nsamples))
			{
				store(&e, edges, n);
				n++;
			}
			pp = cp;
		}
		/* add the last point in case the polygon is closed */
		if (p->closed)
		{
			Enesim_Renderer_Path_Kiia_Edge e;
			if (_kiia_edge_setup(&e, &pp, &lp, nsamples))
			{
				store(&e, edges, n);
				n++;
			}
			/* sanity edge */
			{
				if (_kiia_edge_setup(&e, &lp, &fp, nsamples))
				{
					store(&e, edges, n);
					n++;
				}
			}
		}
		else
		{
			Enesim_Renderer_Path_Kiia_Edge e;
			/* sanity edge */
			/* TODO for a multi polygon figure, the generator is generating
			 * a figure with the all open but the last that is closed
			 */
			{
				if (_kiia_edge_setup(&e, &pp, &fp, nsamples))
				{
					store(&e, edges, n);
					n++;
				}
			}
		}
	}

	if (n)
		qsort(edges, n, edge_size, cmp);
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
			generator = enesim_path_generator_stroke_dashless_new();
		else
			generator = enesim_path_generator_dashed_new();
	}
	else
	{
		generator = enesim_path_generator_strokeless_new();
	}

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

	/* Now generate */
	pa = ENESIM_RENDERER_PATH_ABSTRACT(r);
	enesim_path_generator_generate(generator, pa->path->commands);
	enesim_list_unref(dashes);
	/* Remove the figure generators */
	enesim_path_generator_free(generator);

	/* The figure has been generated, not the edges */
	thiz->edges_generated = EINA_FALSE;

	return EINA_TRUE;
}

static Eina_Bool _kiia_edges_generate(Enesim_Renderer *r,
		Enesim_Backend backend)
{
	Enesim_Renderer_Path_Kiia *thiz;
	Enesim_Renderer_Path_Kiia_Edge_Store store;
	Enesim_Renderer_Path_Kiia_Edge_Cmp cmp;
	Enesim_Renderer_Shape_Draw_Mode dm;
	Enesim_Quality q;
	size_t edge_size;

	thiz = ENESIM_RENDERER_PATH_KIIA(r);
	q = enesim_renderer_quality_get(r);
	switch (q)
	{
		case ENESIM_QUALITY_FAST:
		thiz->nsamples = 8;
		break;

		case ENESIM_QUALITY_GOOD:
		thiz->nsamples = 16;
		break;

		case ENESIM_QUALITY_BEST:
		thiz->nsamples = 32;
		break;
	}

	switch (backend)
	{
		case ENESIM_BACKEND_SOFTWARE:
		edge_size = sizeof(Enesim_Renderer_Path_Kiia_Edge_Sw);
		store = _kiia_edge_store;
		cmp = _kiia_edge_cmp;
		break;

#if BUILD_OPENCL
		case ENESIM_BACKEND_OPENCL:
		edge_size = sizeof(cl_float) * 7;
		store = _kiia_edge_cl_store;
		cmp = _kiia_edge_cl_cmp;
		break;
#endif
		default:
		return EINA_FALSE;
	}

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
				thiz->nsamples, &thiz->fill.nedges,
				edge_size, store, cmp);
		if (!thiz->fill.edges)
			return EINA_FALSE;
	}
	if (dm & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
	{
		thiz->stroke.edges = _kiia_edges_setup(thiz->stroke.figure,
				thiz->nsamples, &thiz->stroke.nedges,
				edge_size, store, cmp);
		if (!thiz->stroke.edges)
			return EINA_FALSE;
	}
	thiz->edges_generated = EINA_TRUE;
	thiz->edges_generated_backend = backend;

	return EINA_TRUE;
}

static Eina_Bool _kiia_generate(Enesim_Renderer *r, Enesim_Backend backend)
{
	Enesim_Renderer_Path_Kiia *thiz;
	Eina_Bool needs_generate;

	thiz = ENESIM_RENDERER_PATH_KIIA(r);
	needs_generate = enesim_renderer_path_abstract_needs_generate(r);
	if (!needs_generate && thiz->edges_generated &&
			thiz->edges_generated_backend == backend)
		return EINA_TRUE;

	printf("inside generate %d %d\n", needs_generate, thiz->edges_generated);
	if (!_kiia_figures_generate(r))
		return EINA_FALSE;

	if (!_kiia_edges_generate(r, backend))
		return EINA_FALSE;

	/* Finally mark as we have already generated the figure */
	enesim_renderer_path_abstract_generate(r);
	return EINA_TRUE;
}

static Eina_Bool _kiia_setup(Enesim_Renderer *r, Enesim_Backend backend,
		double *rty, double *rby, double *rlx, double *rrx)
{
	Enesim_Renderer_Path_Kiia *thiz;
	Enesim_Renderer_Shape_Draw_Mode dm;
	Enesim_Color color;
	double lx, rx, ty, by;

	thiz = ENESIM_RENDERER_PATH_KIIA(r);
	/* Generate the edges */
	if (!_kiia_generate(r, backend))
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
	if (dm == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE_FILL)
	{
		double olx, oty, orx, oby;

		if (!enesim_figure_bounds(thiz->fill.figure, &lx, &ty, &rx, &by))
			return EINA_FALSE;
		if (!enesim_figure_bounds(thiz->stroke.figure, &olx, &oty, &orx, &oby))
			return EINA_FALSE;
		if (olx < lx)
			lx = olx;
		if (orx > rx)
			rx = orx;
		if (oty < ty)
			ty = oty;
		if (oby > by)
			by = oby;
	}
	else
	{
		if (dm == ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL)
			thiz->current = &thiz->fill;
		else
			thiz->current = &thiz->stroke;
		if (!enesim_figure_bounds(thiz->current->figure, &lx, &ty, &rx, &by))
			return EINA_FALSE;
	}

	/* snap the coordinates lx, rx, ty and by */
	*rty = (((int)(ty * thiz->nsamples)) / thiz->nsamples);
	*rby = (((int)(by * thiz->nsamples)) / thiz->nsamples);
	*rrx = rx;
	*rlx = lx;

	return EINA_TRUE;
}

/*----------------------------------------------------------------------------*
 *                               Path abstract                                *
 *----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*
 *                             Shape interface                                *
 *----------------------------------------------------------------------------*/
static void _kiia_shape_features_get(Enesim_Renderer *r EINA_UNUSED,
		int *features)
{
	*features = ENESIM_RENDERER_SHAPE_FEATURE_FILL_RENDERER |
			ENESIM_RENDERER_SHAPE_FEATURE_STROKE_RENDERER;
}

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _kiia_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "kiia";
}

static void _kiia_features_get(Enesim_Renderer *r EINA_UNUSED,
		int *features)
{
	*features =  ENESIM_RENDERER_FEATURE_QUALITY |
			ENESIM_RENDERER_FEATURE_BACKEND_SOFTWARE |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static Eina_Bool _kiia_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *draw, Enesim_Log **error EINA_UNUSED)
{
	Enesim_Renderer_Path_Kiia *thiz;
	Enesim_Renderer_Shape_Draw_Mode dm;
	Enesim_Renderer_Shape_Fill_Rule fr;
	Enesim_Quality quality;
	double lx, rx, ty, by;
	int len;
	int y;

	if (!_kiia_setup(r, ENESIM_BACKEND_SOFTWARE, &ty, &by, &lx, &rx))
		return EINA_FALSE;

	thiz = ENESIM_RENDERER_PATH_KIIA(r);
	fr = enesim_renderer_shape_fill_rule_get(r);
	dm = enesim_renderer_shape_draw_mode_get(r);
	quality = enesim_renderer_quality_get(r);
	if (dm == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE_FILL)
	{
		Eina_Bool has_fill_renderer = EINA_FALSE;
		Eina_Bool has_stroke_renderer = EINA_FALSE;

		if (thiz->fill.ren)
			has_fill_renderer = EINA_TRUE;
		if (thiz->stroke.ren)
			has_stroke_renderer = EINA_TRUE;
		*draw = _fill_full[quality][fr][has_fill_renderer][has_stroke_renderer];
	}
	else
	{
		Eina_Bool has_renderer = EINA_FALSE;

		if (dm == ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL)
			thiz->current = &thiz->fill;
		else
		{
			thiz->current = &thiz->stroke;
			/* the stroke must always be non-zero */
			fr = ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO;
		}
		if (thiz->current->ren)
			has_renderer = EINA_TRUE;
		*draw = _fill_simple[quality][fr][has_renderer];
	}

	thiz->inc = eina_f16p16_double_from(1/(double)thiz->nsamples);
	/* set the y coordinate with the topmost value */
	y = ceil(ty);
	/* the length of the mask buffer */
	len = ceil(rx - lx) + 1;
	thiz->rx = len;
	thiz->lx = floor(lx);
	thiz->llx = eina_f16p16_int_from(thiz->lx);
	/* set the patterns */
	thiz->pattern = _patterns[quality];
	/* setup the worker */
	_worker_setup[quality][fr](r, y, len);

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
		free(thiz->workers[i].omask);
	}
}

static void _kiia_sw_hints(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Rop rop EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_SW_HINT_COLORIZE;
}

#if BUILD_OPENCL
static Eina_Bool _kiia_opencl_kernel_get(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		const char **program_name, const char **program_source,
		size_t *program_length)
{
	*program_name = "path_kiia";
	*program_source =
	"#include \"enesim_opencl.h\"\n" 
	#include "enesim_renderer_path_kiia.cl"
	*program_length = strlen(*program_source);

	return EINA_TRUE;
}

static Eina_Bool _kiia_opencl_kernel_setup(Enesim_Renderer *r,
		Enesim_Surface *s, int argc,
		Enesim_Renderer_OpenCL_Kernel_Mode *mode)
{
	Enesim_Renderer_Path_Kiia *thiz;
	Enesim_Renderer_OpenCL_Data *rdata;
	Enesim_Buffer_OpenCL_Data *sdata;
	cl_mem cl_fill_edges = NULL, cl_stroke_edges = NULL;
	cl_int cl_nedges = 0;
	double lx, rx, ty, by;

	thiz = ENESIM_RENDERER_PATH_KIIA(r);
	/* The algorithm requires a Enesim_Renderer_Path_Kiia_Edge_Sw structure to be used
	 * we can share a struct, but because of the alignment it is not safe to do so
	 * We better use a buffer of floats for edges, i.e x0, y0, x1, y1, mx, slope
	 * and sign all sequentially stored as cl_floats
	 */
	/* TODO override the draw method to also draw the fill and/or stroke renderer */
	if (!_kiia_setup(r, ENESIM_BACKEND_OPENCL, &ty, &by, &lx, &rx))
		return EINA_FALSE;

	sdata = enesim_surface_backend_data_get(s);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENCL);
	if (thiz->fill.figure)
	{
		cl_fill_edges = clCreateBuffer(sdata->context,
				CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
				sizeof(cl_float) * 7 * thiz->fill.nedges,
				thiz->fill.edges, NULL);
		cl_nedges = thiz->fill.nedges;
	}
	clSetKernelArg(rdata->kernel, argc++, sizeof(cl_mem), (void *)&cl_fill_edges);
	clSetKernelArg(rdata->kernel, argc++, sizeof(cl_int), (void *)&cl_nedges);
	clSetKernelArg(rdata->kernel, argc++, sizeof(cl_uchar4), &thiz->fill.color);

	cl_nedges = 0;
	if (thiz->stroke.figure)
	{
		cl_stroke_edges = clCreateBuffer(sdata->context,
				CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR,
				sizeof(cl_float) * 7 * thiz->stroke.nedges,
				thiz->stroke.edges, NULL);
	}
	clSetKernelArg(rdata->kernel, argc++, sizeof(cl_mem), (void *)&cl_stroke_edges);
	clSetKernelArg(rdata->kernel, argc++, sizeof(cl_int), (void *)&cl_nedges);
	clSetKernelArg(rdata->kernel, argc++, sizeof(cl_uchar4), &thiz->stroke.color);
	*mode = ENESIM_RENDERER_OPENCL_KERNEL_MODE_HSPAN;
	return EINA_TRUE;
}

static void _kiia_opencl_kernel_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
}
#endif

static Eina_Bool _kiia_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds, Enesim_Log **log EINA_UNUSED)
{
	Enesim_Renderer_Shape_Draw_Mode dm;
	double xmin = DBL_MAX;
	double ymin = DBL_MAX;
	double xmax = -DBL_MAX;
	double ymax = -DBL_MAX;

	/* Only generate the figures, not the edges */
	if (enesim_renderer_path_abstract_needs_generate(r))
	{
		printf("needs generate inside bounds\n");
		if (!_kiia_figures_generate(r))
			goto failed;
		enesim_renderer_path_abstract_generate(r);
	}

	dm = enesim_renderer_shape_draw_mode_get(r);
	/* check the type of draw mode */
	if (dm & ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL)
	{
		Enesim_Renderer_Path_Kiia *thiz;
		double lx, rx, ty, by;

		thiz = ENESIM_RENDERER_PATH_KIIA(r);
		if (!enesim_figure_bounds(thiz->fill.figure, &lx, &ty, &rx, &by))
			goto failed;
		if (lx < xmin)
			xmin = lx;
		if (rx > xmax)
			xmax = rx;
		if (ty < ymin)
			ymin = ty;
		if (by > ymax)
			ymax = by;
	}
	if (dm & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
	{
		Enesim_Renderer_Path_Kiia *thiz;
		double lx, rx, ty, by;

		thiz = ENESIM_RENDERER_PATH_KIIA(r);
		if (!enesim_figure_bounds(thiz->stroke.figure, &lx, &ty, &rx, &by))
			goto failed;
		if (lx < xmin)
			xmin = lx;
		if (rx > xmax)
			xmax = rx;
		if (ty < ymin)
			ymin = ty;
		if (by > ymax)
			ymax = by;
	}
	/* snap the bounds */
	bounds->x = xmin;
	bounds->y = ymin;
	bounds->w = (xmax - xmin);
	bounds->h = (ymax - ymin);
	return EINA_TRUE;

failed:
	bounds->x = 0;
	bounds->y = 0;
	bounds->w = 0;
	bounds->h = 0;
	return EINA_FALSE;
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

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _kiia_name;
	r_klass->features_get = _kiia_features_get;
	r_klass->sw_hints_get = _kiia_sw_hints;
	r_klass->bounds_get = _kiia_bounds_get;
#ifdef BUILD_OPENCL
	r_klass->opencl_kernel_get = _kiia_opencl_kernel_get;
	r_klass->opencl_kernel_setup = _kiia_opencl_kernel_setup;
	r_klass->opencl_kernel_cleanup = _kiia_opencl_kernel_cleanup;
#endif

	s_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	s_klass->sw_setup = _kiia_sw_setup;
	s_klass->sw_cleanup = _kiia_sw_cleanup;
	s_klass->features_get = _kiia_shape_features_get;

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

	/* set the patterns */
	_patterns[ENESIM_QUALITY_FAST] = _kiia_pattern8;
	_patterns[ENESIM_QUALITY_GOOD] = _kiia_pattern16;
	_patterns[ENESIM_QUALITY_BEST] = _kiia_pattern32;

	/* set the workers setup */
	_worker_setup[ENESIM_QUALITY_FAST][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO] =
			enesim_renderer_path_kiia_8_non_zero_worker_setup;
	_worker_setup[ENESIM_QUALITY_GOOD][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO] =
			enesim_renderer_path_kiia_16_non_zero_worker_setup;
	_worker_setup[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO] =
			enesim_renderer_path_kiia_32_non_zero_worker_setup;

	_worker_setup[ENESIM_QUALITY_FAST][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD] =
			enesim_renderer_path_kiia_8_even_odd_worker_setup;
	_worker_setup[ENESIM_QUALITY_GOOD][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD] =
			enesim_renderer_path_kiia_16_even_odd_worker_setup;
	_worker_setup[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD] =
			enesim_renderer_path_kiia_32_even_odd_worker_setup;

	/* setup our function pointers */
	_fill_simple[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][0] = 
			enesim_renderer_path_kiia_32_even_odd_color_simple;
	_fill_simple[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][1] = 
			enesim_renderer_path_kiia_32_even_odd_renderer_simple;
	_fill_simple[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][0] = 
			enesim_renderer_path_kiia_32_non_zero_color_simple;
	_fill_simple[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][1] = 
			enesim_renderer_path_kiia_32_non_zero_renderer_simple;

	_fill_simple[ENESIM_QUALITY_GOOD][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][0] = 
			enesim_renderer_path_kiia_16_even_odd_color_simple;
	_fill_simple[ENESIM_QUALITY_GOOD][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][1] = 
			enesim_renderer_path_kiia_16_even_odd_renderer_simple;
	_fill_simple[ENESIM_QUALITY_GOOD][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][0] = 
			enesim_renderer_path_kiia_16_non_zero_color_simple;
	_fill_simple[ENESIM_QUALITY_GOOD][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][1] = 
			enesim_renderer_path_kiia_16_non_zero_renderer_simple;

	_fill_simple[ENESIM_QUALITY_FAST][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][0] = 
			enesim_renderer_path_kiia_8_even_odd_color_simple;
	_fill_simple[ENESIM_QUALITY_FAST][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][1] = 
			enesim_renderer_path_kiia_8_even_odd_renderer_simple;
	_fill_simple[ENESIM_QUALITY_FAST][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][0] = 
			enesim_renderer_path_kiia_8_non_zero_color_simple;
	_fill_simple[ENESIM_QUALITY_FAST][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][1] = 
			enesim_renderer_path_kiia_8_non_zero_renderer_simple;

	/* the full variants */
	_fill_full[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][0][0] = 
			enesim_renderer_path_kiia_32_even_odd_color_color_full;
	_fill_full[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][0][0] = 
			enesim_renderer_path_kiia_32_non_zero_color_color_full;
	_fill_full[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][1][0] = 
			enesim_renderer_path_kiia_32_even_odd_renderer_color_full;
	_fill_full[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][1][0] = 
			enesim_renderer_path_kiia_32_non_zero_renderer_color_full;
	_fill_full[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][0][1] = 
			enesim_renderer_path_kiia_32_even_odd_color_renderer_full;
	_fill_full[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][0][1] = 
			enesim_renderer_path_kiia_32_non_zero_color_renderer_full;

	_fill_full[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][1][1] = 
			enesim_renderer_path_kiia_32_non_zero_renderer_renderer_full;
	_fill_full[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][1][1] = 
			enesim_renderer_path_kiia_32_even_odd_renderer_renderer_full;

	_fill_full[ENESIM_QUALITY_GOOD][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][0][0] = 
			enesim_renderer_path_kiia_16_even_odd_color_color_full;
	_fill_full[ENESIM_QUALITY_GOOD][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][0][0] = 
			enesim_renderer_path_kiia_16_non_zero_color_color_full;
	_fill_full[ENESIM_QUALITY_GOOD][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][1][0] = 
			enesim_renderer_path_kiia_16_even_odd_renderer_color_full;
	_fill_full[ENESIM_QUALITY_GOOD][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][1][0] = 
			enesim_renderer_path_kiia_16_non_zero_renderer_color_full;
	_fill_full[ENESIM_QUALITY_GOOD][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][0][1] = 
			enesim_renderer_path_kiia_16_even_odd_color_renderer_full;
	_fill_full[ENESIM_QUALITY_GOOD][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][0][1] = 
			enesim_renderer_path_kiia_16_non_zero_color_renderer_full;

	_fill_full[ENESIM_QUALITY_GOOD][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][1][1] = 
			enesim_renderer_path_kiia_16_non_zero_renderer_renderer_full;
	_fill_full[ENESIM_QUALITY_GOOD][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][1][1] = 
			enesim_renderer_path_kiia_16_even_odd_renderer_renderer_full;

	_fill_full[ENESIM_QUALITY_FAST][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][0][0] = 
			enesim_renderer_path_kiia_8_even_odd_color_color_full;
	_fill_full[ENESIM_QUALITY_FAST][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][0][0] = 
			enesim_renderer_path_kiia_8_non_zero_color_color_full;
	_fill_full[ENESIM_QUALITY_FAST][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][1][0] = 
			enesim_renderer_path_kiia_8_even_odd_renderer_color_full;
	_fill_full[ENESIM_QUALITY_FAST][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][1][0] = 
			enesim_renderer_path_kiia_8_non_zero_renderer_color_full;
	_fill_full[ENESIM_QUALITY_FAST][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][0][1] = 
			enesim_renderer_path_kiia_8_even_odd_color_renderer_full;
	_fill_full[ENESIM_QUALITY_FAST][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][0][1] = 
			enesim_renderer_path_kiia_8_non_zero_color_renderer_full;

	_fill_full[ENESIM_QUALITY_FAST][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][1][1] = 
			enesim_renderer_path_kiia_8_non_zero_renderer_renderer_full;
	_fill_full[ENESIM_QUALITY_FAST][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][1][1] = 
			enesim_renderer_path_kiia_8_even_odd_renderer_renderer_full;
}

static void _enesim_renderer_path_kiia_instance_init(void *o)
{
	Enesim_Renderer_Path_Kiia *thiz;

	thiz = ENESIM_RENDERER_PATH_KIIA(o);
	thiz->nworkers = enesim_renderer_sw_cpu_count();
	thiz->workers = calloc(thiz->nworkers, sizeof(Enesim_Renderer_Path_Kiia_Worker));
}

static void _enesim_renderer_path_kiia_instance_deinit(void *o)
{
	Enesim_Renderer_Path_Kiia *thiz;

	thiz = ENESIM_RENDERER_PATH_KIIA(o);
	/* Remove the figures */
	if (thiz->stroke.figure)
	{
		enesim_figure_unref(thiz->stroke.figure);
		thiz->stroke.figure = NULL;
	}
	if (thiz->fill.figure)
	{
		enesim_figure_unref(thiz->fill.figure);
		thiz->fill.figure = NULL;
	}
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
 *                                   API                                      *
 *============================================================================*/
