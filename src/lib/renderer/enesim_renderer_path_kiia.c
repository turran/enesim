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

/* Add support for the different fill rules
 * Add support for the different qualities
 * Modify it to handle the current_figure
 * Make it generate both figures
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
static Eina_F16p16 _kiia_pattern8[8];
static Eina_F16p16 _kiia_pattern16[16];
static Eina_F16p16 _kiia_pattern32[32];

static Enesim_Renderer_Sw_Fill _fill_simple[3][ENESIM_RENDERER_SHAPE_FILL_RULES][2];
static Enesim_Renderer_Sw_Fill _fill_full[3][ENESIM_RENDERER_SHAPE_FILL_RULES][2][2];

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
	if (fabs(y01) < DBL_EPSILON)
		return EINA_FALSE;
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

	//printf("edges %f %f %f %f %f (%f)\n", x0, start, x1, y1, slope, y01);

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

static Eina_Bool _kiia_generate(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Kiia *thiz;
	Enesim_Quality q;

	thiz = ENESIM_RENDERER_PATH_KIIA(r);
	if (!enesim_renderer_path_abstract_needs_generate(r))
		return EINA_TRUE;

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
	if (!_kiia_figures_generate(r))
		return EINA_FALSE;
	if (!_kiia_edges_generate(r))
		return EINA_FALSE;
	/* Finally mark as we have already generated the figure */
	enesim_renderer_path_abstract_generate(r);

	return EINA_TRUE;
}
/*----------------------------------------------------------------------------*
 *                               Path abstract                                *
 *----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*
 *                             Shape interface                                *
 *----------------------------------------------------------------------------*/
static void _kiia_shape_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Shape_Feature *features)
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
		Enesim_Renderer_Feature *features)
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
	Enesim_Color color;
	double lx, rx, ty, by;
	int len;
	int i;
	int y;

	thiz = ENESIM_RENDERER_PATH_KIIA(r);
	/* Convert the path to a figure */
	if (!_kiia_generate(r))
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
	fr = enesim_renderer_shape_fill_rule_get(r);
	dm = enesim_renderer_shape_draw_mode_get(r);
	if (dm == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE_FILL)
	{
		Eina_Bool has_fill_renderer = EINA_FALSE;
		Eina_Bool has_stroke_renderer = EINA_FALSE;
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
		if (thiz->fill.ren)
			has_fill_renderer = EINA_TRUE;
		if (thiz->stroke.ren)
			has_stroke_renderer = EINA_TRUE;
		*draw = _fill_full[ENESIM_QUALITY_BEST][fr][has_fill_renderer][has_stroke_renderer];
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
		if (!enesim_figure_bounds(thiz->current->figure, &lx, &ty, &rx, &by))
			return EINA_FALSE;
		*draw = _fill_simple[ENESIM_QUALITY_BEST][fr][has_renderer];
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
	/* snap the coordinates lx, rx, ty and by */
#if 0
	lx = (((int)(lx * thiz->nsamples) + 1) / thiz->nsamples) - 1;
	ty = (((int)(ty * thiz->nsamples)) / thiz->nsamples);
	rx = (((int)(rx * thiz->nsamples)) / thiz->nsamples);
	by = (((int)(by * thiz->nsamples)) / thiz->nsamples);
#endif
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
		thiz->workers[i].mask = calloc(len + 10, sizeof(uint32_t));
		thiz->workers[i].winding = calloc((len + 10), sizeof(int));
		thiz->workers[i].omask = calloc(len + 10, sizeof(uint32_t));
		thiz->workers[i].owinding = calloc((len + 10), sizeof(int));
	}
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
		free(thiz->workers[i].winding);
		free(thiz->workers[i].owinding);
	}
}

static void _kiia_sw_hints(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Rop rop EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_SW_HINT_COLORIZE;
}

static void _kiia_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Shape_Draw_Mode dm;
	double xmin = DBL_MAX;
	double ymin = DBL_MAX;
	double xmax = -DBL_MAX;
	double ymax = -DBL_MAX;

	if (!_kiia_generate(r))
		goto failed;
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
	bounds->w = (xmax - xmin) + 1;
	bounds->h = (ymax - ymin) + 1;
	return;
failed:
	bounds->x = 0;
	bounds->y = 0;
	bounds->w = 0;
	bounds->h = 0;
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
	r_klass->bounds_get = _kiia_bounds_get;

	s_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	s_klass->sw_setup = _kiia_sw_setup;
	s_klass->sw_cleanup = _kiia_sw_cleanup;
	s_klass->features_get = _kiia_shape_features_get;

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

	/* setup our function pointers */
	_fill_simple[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][0] = 
			enesim_renderer_path_kiia_32_even_odd_color_simple;
	_fill_simple[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][1] = 
			enesim_renderer_path_kiia_32_even_odd_renderer_simple;
	_fill_simple[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][0] = 
			enesim_renderer_path_kiia_32_non_zero_color_simple;
	_fill_simple[ENESIM_QUALITY_BEST][ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][1] = 
			enesim_renderer_path_kiia_32_non_zero_renderer_simple;

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
		enesim_figure_delete(thiz->stroke.figure);
	if (thiz->fill.figure)
		enesim_figure_delete(thiz->fill.figure);
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
