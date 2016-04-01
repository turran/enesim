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
#include "enesim_path.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_figure.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_path.h"


#include "enesim_path_private.h"
#include "enesim_curve_private.h"
#include "enesim_figure_private.h"
#include "enesim_vector_private.h"
#include "enesim_path_generator_private.h"
#include "enesim_path_normalizer_private.h"

/* To round the input coordinates to pixel values (integers) */
#define PIXEL_ALIGN 0

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
typedef struct _Enesim_Path_Edge
{
	double x0;
	double y0;
	double x1;
	double y1;
} Enesim_Path_Edge;

typedef struct _Enesim_Path_Generator_Dashed
{
	Enesim_Path_Generator *p; /* ourselves */
	Enesim_Path_Generator *fill;
	Enesim_Path_Generator *stroke;
	/* our own private data */
	Eina_Bool started;
	Eina_Bool first;
	Enesim_Point first_point;
	Enesim_Point prev_point;
	const Eina_List *current;
	double dist;
} Enesim_Path_Generator_Dashed;

typedef struct _Enesim_Path_Generator_Full
{
	/* keep the original path */
	Enesim_Path_Generator *p;
	/* our own private data */
	Enesim_Polygon *original_polygon;
	Enesim_Polygon *offset_polygon;
	Enesim_Polygon *inset_polygon;
	Enesim_Point first;
	Enesim_Point last;
	Enesim_Point p0, p1, p2;
	Enesim_Point n01, n12;
	double rx;
	double ry;
	int count;
} Enesim_Path_Generator_Full;

typedef struct _Enesim_Path_Generator_Stroke
{
	/* keep the original path */
	Enesim_Path_Generator *p;
	/* our own private data */
	Enesim_Polygon *offset_polygon;
	Enesim_Polygon *inset_polygon;
	Enesim_Path_Edge first;
	Enesim_Point p0, p1, p2;
	Enesim_Point n01, n12;
	Enesim_Matrix sw_m;
	double rx;
	double ry;
	int count;
} Enesim_Path_Generator_Stroke;

typedef struct _Enesim_Path_Generator_Stroke_Dashless
{
	Enesim_Path_Generator *p; /* ourselves */
	Enesim_Path_Generator *fill;
	Enesim_Path_Generator *stroke;
} Enesim_Path_Generator_Stroke_Dashless;

typedef struct _Enesim_Path_Generator_Strokeless
{
	Enesim_Path_Generator *p;
} Enesim_Path_Generator_Strokeless;
/*----------------------------------------------------------------------------*
 *                         Descriptor interface                               *
 *----------------------------------------------------------------------------*/
static void _path_vertex_add(Enesim_Path_Generator *thiz, double x, double y)
{
	thiz->descriptor->vertex_add(x, y, thiz->data);
}

static void _path_polygon_add(Enesim_Path_Generator *thiz)
{
	thiz->descriptor->polygon_add(thiz->data);
}

static void _path_polygon_close(Enesim_Path_Generator *thiz, Eina_Bool close)
{
	thiz->descriptor->polygon_close(close, thiz->data);
}

static void _path_done(Enesim_Path_Generator *thiz)
{
	if (thiz->descriptor->path_done)
		thiz->descriptor->path_done(thiz->data);
}

static void _path_begin(Enesim_Path_Generator *thiz)
{
	if (thiz->descriptor->path_begin)
		thiz->descriptor->path_begin(thiz->data);
}
/*----------------------------------------------------------------------------*
 *                                 Commands                                   *
 *----------------------------------------------------------------------------*/
#if 0
static void _path_move_to(Enesim_Path_Generator *thiz,
		double x, double y)
{
	/* we have to reset the curve state too */
	thiz->st.last_x = x;
	thiz->st.last_y = y;
	thiz->st.last_ctrl_x = x;
	thiz->st.last_ctrl_y = y;
	/* now call the interface */
	_path_polygon_add(thiz);
	_path_vertex_add(thiz, x, y);
}
#endif
/*----------------------------------------------------------------------------*
 *                              Without stroke                                *
 *----------------------------------------------------------------------------*/

static void _strokeless_path_free(void *data)
{
	free(data);
}

static void _strokeless_path_vertex_add(double x, double y, void *data)
{
	Enesim_Path_Generator_Strokeless *thiz = data;
	Enesim_Path_Generator *path = thiz->p;

	enesim_figure_polygon_vertex_add(path->figure, x, y);
}

static void _strokeless_path_polygon_add(void *data)
{
	Enesim_Path_Generator_Strokeless *thiz = data;
	Enesim_Path_Generator *path = thiz->p;
	Enesim_Polygon *p;

	p = enesim_polygon_new();
	enesim_polygon_threshold_set(p, 1/256.0); // FIXME make 1/256.0 a constant */
	enesim_figure_polygon_append(path->figure, p);
}

static void _strokeless_path_polygon_close(Eina_Bool close, void *data)
{
	Enesim_Path_Generator_Strokeless *thiz = data;
	Enesim_Path_Generator *path = thiz->p;

	if (close)
		enesim_figure_polygon_close(path->figure);
}

/* the strokeless path generator */
static Enesim_Path_Descriptor _strokeless_descriptor = {
	/* .free		= */ _strokeless_path_free,
	/* .vertex_add		= */ _strokeless_path_vertex_add,
	/* .polygon_add 	= */ _strokeless_path_polygon_add,
	/* .polygon_close 	= */ _strokeless_path_polygon_close,
	/* .path_begin 		= */ NULL,
	/* .path_done 		= */ NULL,
};
/*----------------------------------------------------------------------------*
 *                                With stroke                                 *
 *----------------------------------------------------------------------------*/
static Eina_Bool _edge_intersect(Enesim_Path_Edge *e1, Enesim_Path_Edge *e2,
		double *x, double *y, double threshold)
{
	double a1, b1, c1;
	double a2, b2, c2;
	double determinant;

	/* compute the normal form */
	a1 = e1->y1 - e1->y0;
	b1 = e1->x0 - e1->x1;
	c1 = (a1 * e1->x1) + (b1 * e1->y1);

	a2 = e2->y1 - e2->y0;
	b2 = e2->x0 - e2->x1;
	c2 = (a2 * e2->x1) + (b2 * e2->y1);

	determinant = (a1*b2) - (a2*b1);
	/* lines are parallel */
	if (fabs(determinant) < threshold)
		return EINA_FALSE;

	*x = ((b2 * c1) - (b1 * c2))/determinant;
	*y = ((a1 * c2) - (a2 * c1))/determinant;
	return EINA_TRUE;
}

static void _edge_join(Enesim_Path_Edge *e1,
		Enesim_Path_Edge *e2,
		Enesim_Renderer_Shape_Stroke_Join join,
		double threshold,
		Enesim_Curve_Vertex_Add vertex_add,
		void *data)
{
	Enesim_Curve_State st;
	double ix, iy;

	switch (join)
	{
		case ENESIM_RENDERER_SHAPE_STROKE_JOIN_MITER:
		if (_edge_intersect(e1, e2, &ix, &iy, threshold))
		{
			/* TODO we need a miter length variable too */
			vertex_add(ix, iy, data);
		}
		else
		{
			vertex_add(e2->x0, e2->y0, data);
		}
		break;

		/* join theme with an arc */
		case ENESIM_RENDERER_SHAPE_STROKE_JOIN_ROUND:
		if (_edge_intersect(e1, e2, &ix, &iy, threshold))
		{
			st.vertex_add = vertex_add;
			st.last_x = e1->x1;
			st.last_y = e1->y1;
			st.last_ctrl_x = e1->x1;
			st.last_ctrl_y = e1->y1;
			st.data = data;
			enesim_curve_quadratic_to(&st, ix, iy, e2->x0, e2->y0);
		}
		else
		{
			vertex_add(e2->x0, e2->y0, data);
		}
		break;

		case ENESIM_RENDERER_SHAPE_STROKE_JOIN_BEVEL:
		/* just join theme with a line */
		vertex_add(e2->x0, e2->y0, data);
		break;

		default:
		break;
	}
}

static inline Eina_Bool _do_normal(Enesim_Point *n, Enesim_Point *p0, Enesim_Point *p1, double threshold)
{
	double dx;
	double dy;
	double f;

	dx = p1->x - p0->x;
	dy = p1->y - p0->y;

	if (fabs(dx) < threshold && fabs(dy) < threshold)
		return EINA_FALSE;
	f = 1.0 / hypot(dx, dy);
	n->x = dy * f;
	n->y = -dx * f;

	return EINA_TRUE;
}

static void _stroke_curve_append(double x, double y, void *data)
{
	Enesim_Polygon *p = data;

	enesim_polygon_point_append_from_coords(p, x, y);
}

static void _stroke_curve_prepend(double x, double y, void *data)
{
	Enesim_Polygon *p = data;

	enesim_polygon_point_prepend_from_coords(p, x, y);
}

static void _stroke_path_merge(Enesim_Path_Generator_Stroke *thiz)
{
	Enesim_Polygon *to_merge;
	Enesim_Point *off, *ofl;
	Enesim_Point *inf, *inl;

	/* FIXME is not complete yet */
	/* TODO use the stroke cap to close the offset and the inset */
	if (thiz->p->cap != ENESIM_RENDERER_SHAPE_STROKE_CAP_BUTT)
	{
		Eina_List *l;

		inf = eina_list_data_get(thiz->inset_polygon->points);
		l = eina_list_last(thiz->inset_polygon->points);
		inl = eina_list_data_get(l);

		off = eina_list_data_get(thiz->offset_polygon->points);
		l = eina_list_last(thiz->offset_polygon->points);
		ofl = eina_list_data_get(l);
		/* do an arc from last offet to first inset */
		if (thiz->p->cap == ENESIM_RENDERER_SHAPE_STROKE_CAP_ROUND)
		{
			Enesim_Curve_State st;

			st.vertex_add = _stroke_curve_prepend;
			st.data = thiz->offset_polygon;
			st.last_x = off->x;
			st.last_y = off->y;
			st.last_ctrl_x = off->x;
			st.last_ctrl_y = off->y;
			/* FIXME what about the sweep and the large? */
			enesim_curve_arc_to(&st, thiz->rx, thiz->ry, 0, EINA_TRUE, EINA_FALSE, inl->x, inl->y);

			st.vertex_add = _stroke_curve_append;
			st.data = thiz->offset_polygon;
			st.last_x = ofl->x;
			st.last_y = ofl->y;
			st.last_ctrl_x = ofl->x;
			st.last_ctrl_y = ofl->y;
			enesim_curve_arc_to(&st, thiz->rx, thiz->ry, 0, EINA_FALSE, EINA_TRUE, inf->x, inf->y);
		}
		/* square case extend the last offset r length and the first inset r length, join them */
		else
		{
			/* TODO */
		}
	}
	to_merge = thiz->inset_polygon;
	enesim_figure_polygon_remove(thiz->p->stroke_figure, to_merge);
	enesim_polygon_merge(thiz->offset_polygon, thiz->inset_polygon);
	thiz->inset_polygon = NULL;
}

static void _stroke_path_vertex_process(double x, double y, Enesim_Path_Generator_Stroke *thiz)
{
	Enesim_Polygon *inset = thiz->inset_polygon;
	Enesim_Polygon *offset = thiz->offset_polygon;
	Enesim_Point o0, o1;
	Enesim_Point i0, i1;
	Enesim_Path_Edge e1, e2;
	double ox;
	double oy;
	double c1;

	/* just store the first point */
	if (thiz->count < 2)
	{
		switch (thiz->count)
		{
			case 0:
			thiz->first.x0 = thiz->p0.x = x;
			thiz->first.y0 = thiz->p0.y = y;
			thiz->count++;
			return;

			case 1:
			thiz->first.x1 = thiz->p1.x = x;
			thiz->first.y1 = thiz->p1.y = y;
			/* FIXME use the threshold set by the quality prop or something like that */
			if (!_do_normal(&thiz->n01, &thiz->p0, &thiz->p1, 1/256.0))
				return;

			if (thiz->p->sw_scalable)
			{
				enesim_matrix_point_transform(&thiz->sw_m, thiz->n01.x, thiz->n01.y, &ox, &oy);
			}
			else
			{
				ox = thiz->n01.x * thiz->p->sw;
				oy = thiz->n01.y * thiz->p->sw;
			}

			o0.x = thiz->p0.x + ox;
			o0.y = thiz->p0.y + oy;
			enesim_polygon_point_append_from_coords(offset, o0.x, o0.y);

			o1.x = thiz->p1.x + ox;
			o1.y = thiz->p1.y + oy;
			enesim_polygon_point_append_from_coords(offset, o1.x, o1.y);

			i0.x = thiz->p0.x - ox;
			i0.y = thiz->p0.y - oy;
			enesim_polygon_point_prepend_from_coords(inset, i0.x, i0.y);

			i1.x = thiz->p1.x - ox;
			i1.y = thiz->p1.y - oy;
			enesim_polygon_point_prepend_from_coords(inset, i1.x, i1.y);

			thiz->count++;
			return;

			default:
			break;
		}
	}

	/* get the normals of the new edge */
	thiz->p2.x = x;
	thiz->p2.y = y;
	/* FIXME use the threshold set by the quality prop or something like that */
	if (!_do_normal(&thiz->n12, &thiz->p1, &thiz->p2, 1/256.0))
		return;
	if (thiz->p->sw_scalable)
	{
		enesim_matrix_point_transform(&thiz->sw_m, thiz->n12.x, thiz->n12.y, &ox, &oy);
	}
	else
	{
		ox = thiz->n12.x * thiz->p->sw;
		oy = thiz->n12.y * thiz->p->sw;
	}

	o0.x = thiz->p1.x + ox;
	o0.y = thiz->p1.y + oy;

	i0.x = thiz->p1.x - ox;
	i0.y = thiz->p1.y - oy;

	o1.x = thiz->p2.x + ox;
	o1.y = thiz->p2.y + oy;

	i1.x = thiz->p2.x - ox;
	i1.y = thiz->p2.y - oy;

	c1 = ((thiz->p2.x - thiz->p1.x) * thiz->n01.x) + ((thiz->p2.y - thiz->p1.y) * thiz->n01.y);
	/* add the vertices of the new edge */
	/* check if the previous edge and this one to see the concave/convex thing */
	/* dot product
	 * = 1 pointing same direction
	 * > 0 concave
	 * = 0 orthogonal
	 * < 0 convex
	 * = -1 pointing opposite direction
	 */
	/* right side */
	if (c1 >= 0)
	{
		Enesim_Point *p;
		Eina_List *l2;

		enesim_polygon_point_append_from_coords(offset, o0.x, o0.y);
		/* join the inset */
		p = eina_list_data_get(inset->points);
		e1.x1 = p->x;
		e1.y1 = p->y;
		l2 = eina_list_next(inset->points);
		p = eina_list_data_get(l2);
		e1.x0 = p->x;
		e1.y0 = p->y;

		e2.x0 = i0.x;
		e2.y0 = i0.y;
		e2.x1 = i1.x;
		e2.y1 = i1.y;

		_edge_join(&e1, &e2, thiz->p->join, 1/256.0, _stroke_curve_prepend, inset);
	}
	/* left side */
	else
	{
		Enesim_Point *p;
		Eina_List *l2;

		enesim_polygon_point_prepend_from_coords(inset, i0.x, i0.y);
		/* join the offset */
		l2 = eina_list_last(offset->points);
		p = eina_list_data_get(l2);
		e1.x1 = p->x;
		e1.y1 = p->y;
		l2 = eina_list_prev(l2);
		p = eina_list_data_get(l2);
		e1.x0 = p->x;
		e1.y0 = p->y;

		e2.x0 = o0.x;
		e2.y0 = o0.y;
		e2.x1 = o1.x;
		e2.y1 = o1.y;
		_edge_join(&e1, &e2, thiz->p->join, 1/256.0, _stroke_curve_append, offset);
	}

	enesim_polygon_point_append_from_coords(offset, o1.x, o1.y);
	enesim_polygon_point_prepend_from_coords(inset, i1.x, i1.y);

	thiz->p0 = thiz->p1;
	thiz->p1 = thiz->p2;
	thiz->n01 = thiz->n12;
	thiz->count++;
}

static void _stroke_path_free(void *data)
{
	free(data);
}

static void _stroke_path_vertex_add(double x, double y, void *data)
{
	_stroke_path_vertex_process(x, y, data);
}

static void _stroke_path_polygon_add(void *data)
{
	Enesim_Path_Generator_Stroke *thiz = data;
	Enesim_Path_Generator *path = thiz->p;
	Enesim_Polygon *p;

	/* just reset */
	thiz->count = 0;

	/* we are adding a new polygon, check that we need to merge the previous ones */
	if (thiz->offset_polygon && thiz->inset_polygon)
	{
		_stroke_path_merge(thiz);
	}

	p = enesim_polygon_new();
	enesim_polygon_threshold_set(p, 1/256.0);
	enesim_figure_polygon_append(path->stroke_figure, p);
	thiz->offset_polygon = p;

	p = enesim_polygon_new();
	enesim_polygon_threshold_set(p, 1/256.0);
	enesim_figure_polygon_append(path->stroke_figure, p);
	thiz->inset_polygon = p;
}

static void _stroke_path_begin(void *data)
{
	Enesim_Path_Generator_Stroke *thiz = data;
	Enesim_Path_Generator *path = thiz->p;

	/* initialize our state */
	thiz->count = 0;
	thiz->inset_polygon = NULL;
	thiz->offset_polygon = NULL;


	/* In case we do scale the stroke weight bsaed on the transformation
	 * matrix we need to calculate a new normal for the normal generated
	 * on the transformed shape's vertices.
	 */
	if (thiz->p->sw_scalable)
	{
		Enesim_Matrix s, m, t;
		double xx, xy, yx, yy;
		double sx, sy;

		sx = hypot(path->gm->xx, path->gm->yx);
		sy = hypot(path->gm->xy, path->gm->yy);

		thiz->rx = path->sw / 2 * sx;
		thiz->ry = path->sw / 2 * sy;

		xx = path->gm->xx / sx;
		yx = path->gm->yx / sx;

		xy = path->gm->xy / sy;
		yy = path->gm->yy / sy;
		/* set a normalized matrix */
		enesim_matrix_values_set(&m, xx, xy, 0, yx, yy, 0, 0, 0, 1);
		enesim_matrix_scale(&s, thiz->rx, thiz->ry);
		enesim_matrix_transpose(&m, &t);
		enesim_matrix_compose(&m, &s, &m);
		enesim_matrix_compose(&m, &t, &thiz->sw_m);
	}
	else
	{
		thiz->rx = path->sw / 2;
		thiz->ry = path->sw / 2;
	}
}

static void _stroke_path_done(void *data)
{
        Enesim_Path_Generator_Stroke *thiz = data;

	if (thiz->offset_polygon)
		enesim_polygon_close(thiz->offset_polygon, EINA_TRUE);
	if (thiz->inset_polygon)
		enesim_polygon_close(thiz->inset_polygon, EINA_TRUE);
}

static void _stroke_path_polygon_close(Eina_Bool close, void *data)
{
        Enesim_Path_Generator_Stroke *thiz = data;

	if (close)
	{
		/* also close the figure itself with the first edge */
		_stroke_path_vertex_process(thiz->first.x0, thiz->first.y0, thiz);
		_stroke_path_vertex_process(thiz->first.x1, thiz->first.y1, thiz);
		if (thiz->offset_polygon)
			enesim_polygon_close(thiz->offset_polygon, EINA_TRUE);
		if (thiz->inset_polygon)
			enesim_polygon_close(thiz->inset_polygon, EINA_TRUE);

		/* reset the inset/offset */
		thiz->inset_polygon = NULL;
		thiz->offset_polygon = NULL;
	}
}

static Enesim_Path_Descriptor _stroke_descriptor = {
	/* .free		= */ _stroke_path_free,
	/* .vertex_add		= */ _stroke_path_vertex_add,
	/* .polygon_add 	= */ _stroke_path_polygon_add,
	/* .polygon_close 	= */ _stroke_path_polygon_close,
	/* .path_begin 		= */ _stroke_path_begin,
	/* .path_done 		= */ _stroke_path_done,
};

/*----------------------------------------------------------------------------*
 *                          Stroke and dashless                               *
 *----------------------------------------------------------------------------*/

static void _stroke_dashless_path_free(void *data)
{
	Enesim_Path_Generator_Stroke_Dashless *thiz = data;

	enesim_path_generator_free(thiz->fill);
	enesim_path_generator_free(thiz->stroke);
	free(thiz);
}

static void _stroke_dashless_path_vertex_add(double x, double y, void *data)
{
	Enesim_Path_Generator_Stroke_Dashless *thiz = data;

	_path_vertex_add(thiz->fill, x, y);
	_path_vertex_add(thiz->stroke, x, y);
}

static void _stroke_dashless_path_polygon_add(void *data)
{
	Enesim_Path_Generator_Stroke_Dashless *thiz = data;

	_path_polygon_add(thiz->fill);
	_path_polygon_add(thiz->stroke);
}

static void _stroke_dashless_path_polygon_close(Eina_Bool close, void *data)
{
	Enesim_Path_Generator_Stroke_Dashless *thiz = data;

	_path_polygon_close(thiz->fill, close);
	_path_polygon_close(thiz->stroke, close);
}

static void _stroke_dashless_path_begin(void *data)
{
	Enesim_Path_Generator_Stroke_Dashless *thiz = data;
	Enesim_Path_Generator *path = thiz->p;
	Enesim_Path_Generator *paths[2] = { thiz->fill, thiz->stroke };
	int i;

	/* set the properties on both paths */
	for (i = 0; i < 2; i++)
	{
		Enesim_Path_Generator *p = paths[i];

		enesim_path_generator_figure_set(p, path->figure);
		enesim_path_generator_stroke_figure_set(p, path->stroke_figure);
		enesim_path_generator_stroke_cap_set(p, path->cap);
		enesim_path_generator_stroke_join_set(p, path->join);
		enesim_path_generator_stroke_weight_set(p, path->sw);
		enesim_path_generator_stroke_scalable_set(p, path->sw_scalable);
		enesim_path_generator_stroke_dash_set(p, path->dashes);
		enesim_path_generator_scale_set(p, path->scale_x, path->scale_y);
		enesim_path_generator_transformation_set(p, path->gm);
	}

	_path_begin(thiz->fill);
	_path_begin(thiz->stroke);
}

static void _stroke_dashless_path_done(void *data)
{
	Enesim_Path_Generator_Stroke_Dashless *thiz = data;
	Enesim_Polygon *last;

	/* check if the last polygon is closed and if so
	 * close it and also merge the stroke path
	 */
	last = eina_list_data_get(
			eina_list_last(thiz->fill->figure->polygons));
	if (last && !last->closed)
	{
		enesim_figure_polygon_close(thiz->fill->figure);
		_stroke_path_merge(thiz->stroke->data);
	}
	_path_done(thiz->fill);
	_path_done(thiz->stroke);
}

static Enesim_Path_Descriptor _stroke_dashless_descriptor = {
	/* .free		= */ _stroke_dashless_path_free,
	/* .vertex_add		= */ _stroke_dashless_path_vertex_add,
	/* .polygon_add 	= */ _stroke_dashless_path_polygon_add,
	/* .polygon_close 	= */ _stroke_dashless_path_polygon_close,
	/* .path_begin 		= */ _stroke_dashless_path_begin,
	/* .path_done 		= */ _stroke_dashless_path_done,
};
/*----------------------------------------------------------------------------*
 *                              Dashed stroke                                 *
 *----------------------------------------------------------------------------*/
static void _dashed_point_at(Enesim_Point *pc, double d, Enesim_Point *p0,
		Enesim_Point *p1)
{
	Enesim_Point tmp;
	Enesim_Point diff;

	diff.x = p1->x - p0->x;
	diff.y = p1->y - p0->y;

	tmp.x = p0->x + (d * (diff.x));
	tmp.y = p0->y + (d * (diff.y));

	pc->x = tmp.x;
	pc->y = tmp.y;
}

static void _dashed_path_free(void *data)
{
	Enesim_Path_Generator_Dashed *d = data;

	if (d->stroke)
		enesim_path_generator_free(d->stroke);
	if (d->fill)
		enesim_path_generator_free(d->fill);
	free(d);
}

static void _dashed_path_vertex_add(double x, double y, void *data)
{
	Enesim_Path_Generator_Dashed *thiz = data;
	Enesim_Point p;
	double d;

	/* generate the fill polygon */
	_path_vertex_add(thiz->fill, x, y);

	/* check if we can add a new vertex */
	if (thiz->first)
	{
 		thiz->first = EINA_FALSE;
		thiz->first_point.x = x;
		thiz->first_point.y = y;
		/* TODO later we need to handle the stroke dash offset */
		//_path_vertex_add(thiz->stroke, x, y);
		thiz->prev_point.x = x;
		thiz->prev_point.y = y;
		return;
	}

	/* TODO calculate the vector description of this new line */
	/* divide the edge by the current dash, etc */
	p.x = x;
	p.y = y;
	p.z = 0;
	//printf("new vertex %g %g -> %g %g\n", x, y, thiz->prev_point.x, thiz->prev_point.y);
	d = enesim_point_2d_distance(&thiz->prev_point, &p);

	while (d)
	{
		Enesim_Renderer_Shape_Stroke_Dash *dash;

		dash = eina_list_data_get(thiz->current);
		/* we are on the stroke zone */
		if (thiz->dist < dash->length)
		{
			/* add a polygon in case we need to */
			if (!thiz->dist)
			{
				//printf("> polygon add\n");
				thiz->started = EINA_TRUE;
				_path_begin(thiz->stroke);
				_path_polygon_add(thiz->stroke);
				//printf("  adding point at %g %g\n", thiz->prev_point.x, thiz->prev_point.y);
				_path_vertex_add(thiz->stroke, thiz->prev_point.x, thiz->prev_point.y);
			}

			/* the stroke should be cut */
			if (thiz->dist + d > dash->length)
			{
				double offset = dash->length - thiz->dist;
				double nd = offset / d;

				/* add a new vertex at prev + (length - dist) */
				//printf("cut at %g %g from %g %g (%g)\n", thiz->prev_point.x, thiz->prev_point.y, x, y, nd);
				_dashed_point_at(&thiz->prev_point, nd, &thiz->prev_point, &p);
				//printf("  adding point at %g %g\n", thiz->prev_point.x, thiz->prev_point.y);
				_path_vertex_add(thiz->stroke, thiz->prev_point.x, thiz->prev_point.y);
				/* and finish this path */
				_stroke_path_merge(thiz->stroke->data);
				_path_done(thiz->stroke);
				thiz->started = EINA_FALSE;
				//printf("< polygon close\n");

				thiz->dist += offset;
				d -= offset;
			}
			/* the stroke should pass this vertex */
			else
			{
				/* add a new vertex at prev + d */
				//printf("  adding point at %g %g\n", x, y);
				_path_vertex_add(thiz->stroke, x, y);
				thiz->prev_point.x = x;
				thiz->prev_point.y = y;
				/* update the prev point */
				thiz->dist += d;
				d = 0;
			}
		}
		/* we are on the gap zone */
		else
		{
			double end = dash->length + dash->gap;
			/* we pass the end, use the next dash */
			if (thiz->dist + d > end)
			{
				double offset = end - thiz->dist;
				double nd = offset / d;

				/* update the prev point */
				_dashed_point_at(&thiz->prev_point, nd, &thiz->prev_point, &p);
				//printf("next dash\n");
				/* go to next gap */
				thiz->current = eina_list_next(thiz->current);
				if (!thiz->current)
					thiz->current = thiz->p->dashes;
				d -= offset;
				thiz->dist = 0;
			}
			/* still more gap to go */
			else
			{
				//printf("more gap\n");
				thiz->dist += d;
				d = 0;
			}
		}
	}

	thiz->prev_point.x = x;
	thiz->prev_point.y = y;
}

static void _dashed_path_polygon_add(void *data)
{
	Enesim_Path_Generator_Dashed *thiz = data;
	Enesim_Path_Generator *path = thiz->p;

	_path_polygon_add(thiz->fill);

	/* reset our own state */
 	thiz->first = EINA_TRUE;
	thiz->current = path->dashes;
	thiz->dist = 0;
}

static void _dashed_path_polygon_close(Eina_Bool close, void *data)
{
        Enesim_Path_Generator_Dashed *thiz = data;
	_path_polygon_close(thiz->fill, close);
}

static void _dashed_path_begin(void *data)
{
	Enesim_Path_Generator_Dashed *thiz = data;
	Enesim_Path_Generator *path = thiz->p;
	Enesim_Path_Generator *paths[2] = { thiz->fill, thiz->stroke };
	int i;

	/* set the properties on both paths */
	for (i = 0; i < 2; i++)
	{
		Enesim_Path_Generator *p = paths[i];

		enesim_path_generator_figure_set(p, path->figure);
		enesim_path_generator_stroke_figure_set(p, path->stroke_figure);
		enesim_path_generator_stroke_cap_set(p, path->cap);
		enesim_path_generator_stroke_join_set(p, path->join);
		enesim_path_generator_stroke_weight_set(p, path->sw);
		enesim_path_generator_stroke_scalable_set(p, path->sw_scalable);
		enesim_path_generator_stroke_dash_set(p, path->dashes);
		enesim_path_generator_scale_set(p, path->scale_x, path->scale_y);
		enesim_path_generator_transformation_set(p, path->gm);
	}

	_path_begin(thiz->fill);

	/* initialize our state */
 	thiz->first = EINA_TRUE;
	thiz->current = path->dashes;
	thiz->dist = 0;
}

static void _dashed_path_done(void *data)
{
        Enesim_Path_Generator_Dashed *thiz = data;

	_path_done(thiz->fill);
	if (thiz->started)
	{
		_stroke_path_merge(thiz->stroke->data);
		_path_done(thiz->stroke);
	}
}

static Enesim_Path_Descriptor _dashed_descriptor = {
	/* .free		= */ _dashed_path_free,
	/* .vertex_add		= */ _dashed_path_vertex_add,
	/* .polygon_add 	= */ _dashed_path_polygon_add,
	/* .polygon_close 	= */ _dashed_path_polygon_close,
	/* .path_begin 		= */ _dashed_path_begin,
	/* .path_done 		= */ _dashed_path_done,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Path_Generator * enesim_path_generator_new(Enesim_Path_Descriptor *descriptor,
		void *data)
{
	Enesim_Path_Generator *thiz;

	thiz = calloc(1, sizeof(Enesim_Path_Generator));
	thiz->descriptor = descriptor;
	thiz->data = data;
	return thiz;
}

void enesim_path_generator_free(Enesim_Path_Generator *thiz)
{
	/* TODO call the interface to free the path implementation
	 * data
	 */
	if (thiz->descriptor->free)
		thiz->descriptor->free(thiz->data);
	free(thiz);
}

void enesim_path_generator_figure_set(Enesim_Path_Generator *thiz, Enesim_Figure *figure)
{
	thiz->figure = figure;
}

void enesim_path_generator_transformation_set(Enesim_Path_Generator *thiz, const Enesim_Matrix *matrix)
{
	thiz->gm = matrix;
}

void enesim_path_generator_scale_set(Enesim_Path_Generator *thiz, double scale_x, double scale_y)
{
	thiz->scale_x = scale_x;
	thiz->scale_y = scale_y;
}

void enesim_path_generator_stroke_figure_set(Enesim_Path_Generator *thiz, Enesim_Figure *stroke)
{
	thiz->stroke_figure = stroke;
}

void enesim_path_generator_stroke_cap_set(Enesim_Path_Generator *thiz, Enesim_Renderer_Shape_Stroke_Cap cap)
{
	thiz->cap = cap;
}

void enesim_path_generator_stroke_join_set(Enesim_Path_Generator *thiz, Enesim_Renderer_Shape_Stroke_Join join)
{
	thiz->join = join;
}

void enesim_path_generator_stroke_weight_set(Enesim_Path_Generator *thiz, double sw)
{
	thiz->sw = sw;
}

void enesim_path_generator_stroke_scalable_set(Enesim_Path_Generator *thiz, Eina_Bool scalable)
{
	thiz->sw_scalable = scalable;
}

void enesim_path_generator_stroke_dash_set(Enesim_Path_Generator *thiz, const Eina_List *dashes)
{
	thiz->dashes = dashes;
}

void * enesim_path_generator_data_get(Enesim_Path_Generator *thiz)
{
	return thiz->data;
}

Enesim_Path_Generator * enesim_path_generator_strokeless_new(void)
{
	Enesim_Path_Generator_Strokeless *s;
	Enesim_Path_Generator *thiz;

	s = calloc(1, sizeof(Enesim_Path_Generator_Strokeless));
	thiz = enesim_path_generator_new(&_strokeless_descriptor, s);
	s->p = thiz;

	return thiz;
}

Enesim_Path_Generator * enesim_path_generator_stroke_new(void)
{
	Enesim_Path_Generator_Stroke *s;
	Enesim_Path_Generator *thiz;

	s = calloc(1, sizeof(Enesim_Path_Generator_Stroke));
	thiz = enesim_path_generator_new(&_stroke_descriptor, s);
	s->p = thiz;

	return thiz;
}

Enesim_Path_Generator * enesim_path_generator_stroke_dashless_new(void)
{
#if 1
	Enesim_Path_Generator_Stroke_Dashless *s;
	Enesim_Path_Generator *thiz;

	s = calloc(1, sizeof(Enesim_Path_Generator_Stroke_Dashless));
	thiz = enesim_path_generator_new(&_stroke_dashless_descriptor, s);
	s->p = thiz;
	s->fill = enesim_path_generator_strokeless_new();
	s->stroke = enesim_path_generator_stroke_new();
#else
	Enesim_Path_Generator_Full *s;
	Enesim_Path_Generator *thiz;

	s = calloc(1, sizeof(Enesim_Path_Generator_Full));
	thiz = enesim_path_generator_new(&_full_descriptor, s);
	s->p = thiz;
#endif

	return thiz;
}

Enesim_Path_Generator * enesim_path_generator_dashed_new(void)
{
	Enesim_Path_Generator_Dashed *d;
	Enesim_Path_Generator *thiz;

	d = calloc(1, sizeof(Enesim_Path_Generator_Dashed));
	thiz = enesim_path_generator_new(&_dashed_descriptor, d);
	d->p = thiz;
	d->fill = enesim_path_generator_strokeless_new();
	d->stroke = enesim_path_generator_stroke_new();

	return thiz;
}

#if 1
void enesim_path_generator_generate(Enesim_Path_Generator *thiz, Eina_List *commands)
{
	Enesim_Path_Normalizer *normalizer;
	Enesim_Path_Normalizer_Figure_Descriptor descriptor;
	Enesim_Path_Command *cmd;
	Enesim_Path_Command_Line_To line_to;
	Enesim_Path_Command_Move_To move_to;
	Enesim_Path_Command_Cubic_To cubic_to;
	Enesim_Path_Command_Scubic_To scubic_to;
	Enesim_Path_Command_Quadratic_To quadratic_to;
	Enesim_Path_Command_Squadratic_To squadratic_to;
	Enesim_Path_Command_Arc_To arc_to;
	Enesim_Path_Command_Close close;
	const Enesim_Matrix *gm;
	Eina_List *l;
	double scale_x;
	double scale_y;

	/* set the needed variables */
	scale_x = thiz->scale_x;
	scale_y = thiz->scale_y;
	gm = thiz->gm;

	/* set the normalizer functions based on the generator functions */
	descriptor.vertex_add = thiz->descriptor->vertex_add;
	descriptor.polygon_add = thiz->descriptor->polygon_add;
	descriptor.polygon_close = thiz->descriptor->polygon_close;

	normalizer = enesim_path_normalizer_figure_new(&descriptor, thiz->data);
	_path_begin(thiz);

	EINA_LIST_FOREACH(commands, l, cmd)
	{
		double x, y;
		double rx;
		double ry;
		double ctrl_x0;
		double ctrl_y0;
		double ctrl_x1;
		double ctrl_y1;
		double ca, sa;
		/* send the new vertex to the figure renderer */
		switch (cmd->type)
		{
			case ENESIM_PATH_COMMAND_TYPE_MOVE_TO:
			x = scale_x * cmd->data.move_to.x;
			y = scale_y * cmd->data.move_to.y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
#if PIXEL_ALIGN
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
#endif
			enesim_path_command_move_to_values_from(&move_to, x, y);
			enesim_path_normalizer_move_to(normalizer, &move_to);
			break;

			case ENESIM_PATH_COMMAND_TYPE_LINE_TO:
			x = scale_x * cmd->data.line_to.x;
			y = scale_y * cmd->data.line_to.y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
#if PIXEL_ALIGN
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
#endif
			enesim_path_command_line_to_values_from(&line_to, x, y);
			enesim_path_normalizer_line_to(normalizer, &line_to);
			break;

			case ENESIM_PATH_COMMAND_TYPE_QUADRATIC_TO:
			x = scale_x * cmd->data.quadratic_to.x;
			y = scale_y * cmd->data.quadratic_to.y;
			ctrl_x0 = scale_x * cmd->data.quadratic_to.ctrl_x;
			ctrl_y0 = scale_y * cmd->data.quadratic_to.ctrl_y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			enesim_matrix_point_transform(gm, ctrl_x0, ctrl_y0, &ctrl_x0, &ctrl_y0);
#if PIXEL_ALIGN
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
#endif
			enesim_path_command_quadratic_to_values_from(&quadratic_to, x, y, ctrl_x0, ctrl_y0);
			enesim_path_normalizer_quadratic_to(normalizer, &quadratic_to);
			break;

			case ENESIM_PATH_COMMAND_TYPE_SQUADRATIC_TO:
			x = scale_x * cmd->data.squadratic_to.x;
			y = scale_y * cmd->data.squadratic_to.y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			//x = ((int) (2*x + 0.5)) / 2.0;
			//y = ((int) (2*y + 0.5)) / 2.0;
			enesim_path_command_squadratic_to_values_from(&squadratic_to, x, y);
			enesim_path_normalizer_squadratic_to(normalizer, &squadratic_to);
			break;

			case ENESIM_PATH_COMMAND_TYPE_CUBIC_TO:
			x = scale_x * cmd->data.cubic_to.x;
			y = scale_y * cmd->data.cubic_to.y;
			ctrl_x0 = scale_x * cmd->data.cubic_to.ctrl_x0;
			ctrl_y0 = scale_y * cmd->data.cubic_to.ctrl_y0;
			ctrl_x1 = scale_x * cmd->data.cubic_to.ctrl_x1;
			ctrl_y1 = scale_y * cmd->data.cubic_to.ctrl_y1;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			enesim_matrix_point_transform(gm, ctrl_x0, ctrl_y0, &ctrl_x0, &ctrl_y0);
			enesim_matrix_point_transform(gm, ctrl_x1, ctrl_y1, &ctrl_x1, &ctrl_y1);
#if PIXEL_ALIGN
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
#endif
			enesim_path_command_cubic_to_values_from(&cubic_to, x, y, ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1);
			enesim_path_normalizer_cubic_to(normalizer, &cubic_to);
			break;

			case ENESIM_PATH_COMMAND_TYPE_SCUBIC_TO:
			x = scale_x * cmd->data.scubic_to.x;
			y = scale_y * cmd->data.scubic_to.y;
			ctrl_x0 = scale_x * cmd->data.scubic_to.ctrl_x;
			ctrl_y0 = scale_y * cmd->data.scubic_to.ctrl_y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			enesim_matrix_point_transform(gm, ctrl_x0, ctrl_y0, &ctrl_x0, &ctrl_y0);
#if PIXEL_ALIGN
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
#endif
			enesim_path_command_scubic_to_values_from(&scubic_to, x, y, ctrl_x0, ctrl_y0);
			enesim_path_normalizer_scubic_to(normalizer, &scubic_to);
			break;

			case ENESIM_PATH_COMMAND_TYPE_ARC_TO:
			x = scale_x * cmd->data.arc_to.x;
			y = scale_y * cmd->data.arc_to.y;
			rx = scale_x * cmd->data.arc_to.rx;
			ry = scale_y * cmd->data.arc_to.ry;
			ca = cos(cmd->data.arc_to.angle * M_PI / 180.0);
			sa = sin(cmd->data.arc_to.angle * M_PI / 180.0);

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			rx = rx * hypot((ca * gm->xx) + (sa * gm->xy), (ca * gm->yx) + (sa * gm->yy));
			ry = ry * hypot((ca * gm->xy) - (sa * gm->xx), (ca * gm->yy) - (sa * gm->yx));
			ca = atan2((ca * gm->yx) + (sa * gm->yy), (ca * gm->xx) + (sa * gm->xy));
#if PIXEL_ALIGN
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
#endif
			enesim_path_command_arc_to_values_from(&arc_to, rx, ry, ca * 180.0 / M_PI,
					x, y, cmd->data.arc_to.large,
					cmd->data.arc_to.sweep);
			enesim_path_normalizer_arc_to(normalizer, &arc_to);
			break;

			case ENESIM_PATH_COMMAND_TYPE_CLOSE:
			close.closed = cmd->data.close.closed;
			enesim_path_normalizer_close(normalizer, &close);
			break;

			default:
			break;
		}
	}
	/* in case we delay the creation of the vertices this triggers that */
	_path_done(thiz);
	enesim_path_normalizer_free(normalizer);
}
#else
void enesim_path_generator_generate(Enesim_Path_Generator *thiz, Eina_List *commands)
{
	Eina_List *l;
	Enesim_Path_Command *cmd;
	const Enesim_Matrix *gm;
	double scale_x;
	double scale_y;

	/* reset our curve state */
	thiz->st.vertex_add = thiz->descriptor->vertex_add;
	thiz->st.last_x = 0;
	thiz->st.last_y = 0;
	thiz->st.last_ctrl_x = 0;
	thiz->st.last_ctrl_y = 0;
	thiz->st.data = thiz->data;

	/* set the needed variables */
	scale_x = thiz->scale_x;
	scale_y = thiz->scale_y;
	gm = thiz->gm;

	_path_begin(thiz);

	EINA_LIST_FOREACH(commands, l, cmd)
	{
		double x, y;
		double rx;
		double ry;
		double ctrl_x0;
		double ctrl_y0;
		double ctrl_x1;
		double ctrl_y1;
		double ca, sa;
		/* send the new vertex to the figure renderer */
		switch (cmd->type)
		{
			case ENESIM_PATH_COMMAND_TYPE_MOVE_TO:
			x = scale_x * cmd->data.move_to.x;
			y = scale_y * cmd->data.move_to.y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			_path_move_to(thiz, x, y);
			break;

			case ENESIM_PATH_COMMAND_TYPE_LINE_TO:
			x = scale_x * cmd->data.line_to.x;
			y = scale_y * cmd->data.line_to.y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			enesim_curve_line_to(&thiz->st, x, y);
			break;

			case ENESIM_PATH_COMMAND_TYPE_QUADRATIC_TO:
			x = scale_x * cmd->data.quadratic_to.x;
			y = scale_y * cmd->data.quadratic_to.y;
			ctrl_x0 = scale_x * cmd->data.quadratic_to.ctrl_x;
			ctrl_y0 = scale_y * cmd->data.quadratic_to.ctrl_y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			enesim_matrix_point_transform(gm, ctrl_x0, ctrl_y0, &ctrl_x0, &ctrl_y0);
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			enesim_curve_quadratic_to(&thiz->st, ctrl_x0, ctrl_y0, x, y);
			break;

			case ENESIM_PATH_COMMAND_TYPE_SQUADRATIC_TO:
			x = scale_x * cmd->data.squadratic_to.x;
			y = scale_y * cmd->data.squadratic_to.y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			enesim_curve_squadratic_to(&thiz->st, x, y);
			break;

			case ENESIM_PATH_COMMAND_TYPE_CUBIC_TO:
			x = scale_x * cmd->data.cubic_to.x;
			y = scale_y * cmd->data.cubic_to.y;
			ctrl_x0 = scale_x * cmd->data.cubic_to.ctrl_x0;
			ctrl_y0 = scale_y * cmd->data.cubic_to.ctrl_y0;
			ctrl_x1 = scale_x * cmd->data.cubic_to.ctrl_x1;
			ctrl_y1 = scale_y * cmd->data.cubic_to.ctrl_y1;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			enesim_matrix_point_transform(gm, ctrl_x0, ctrl_y0, &ctrl_x0, &ctrl_y0);
			enesim_matrix_point_transform(gm, ctrl_x1, ctrl_y1, &ctrl_x1, &ctrl_y1);
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			enesim_curve_cubic_to(&thiz->st,
					ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1,
					x, y);
			break;

			case ENESIM_PATH_COMMAND_TYPE_SCUBIC_TO:
			x = scale_x * cmd->data.scubic_to.x;
			y = scale_y * cmd->data.scubic_to.y;
			ctrl_x0 = scale_x * cmd->data.scubic_to.ctrl_x;
			ctrl_y0 = scale_y * cmd->data.scubic_to.ctrl_y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			enesim_matrix_point_transform(gm, ctrl_x0, ctrl_y0, &ctrl_x0, &ctrl_y0);
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			enesim_curve_scubic_to(&thiz->st, ctrl_x0, ctrl_y0,
					x, y);
			break;

			case ENESIM_PATH_COMMAND_TYPE_ARC_TO:
			x = scale_x * cmd->data.arc_to.x;
			y = scale_y * cmd->data.arc_to.y;
			rx = scale_x * cmd->data.arc_to.rx;
			ry = scale_y * cmd->data.arc_to.ry;
			ca = cos(cmd->data.arc_to.angle * M_PI / 180.0);
			sa = sin(cmd->data.arc_to.angle * M_PI / 180.0);

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			rx = rx * hypot((ca * gm->xx) + (sa * gm->xy), (ca * gm->yx) + (sa * gm->yy));
			ry = ry * hypot((ca * gm->xy) - (sa * gm->xx), (ca * gm->yy) - (sa * gm->yx));
			ca = atan2((ca * gm->yx) + (sa * gm->yy), (ca * gm->xx) + (sa * gm->xy));

			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			enesim_curve_arc_to(&thiz->st,
					rx, ry,
					ca * 180.0 / M_PI,
					cmd->data.arc_to.large,
					cmd->data.arc_to.sweep,
					x, y);
			break;

			case ENESIM_PATH_COMMAND_TYPE_CLOSE:
			_path_polygon_close(thiz, cmd->data.close.close);
			break;

			default:
			break;
		}
	}
	/* in case we delay the creation of the vertices this triggers that */
	_path_done(thiz);
}
#endif
/** @endcond */
