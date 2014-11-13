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

#include <float.h>

#include "enesim_vector_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
static inline void _polygon_update_bounds(Enesim_Polygon *ep, Enesim_Point *p)
{
	if (p->x > ep->xmax) ep->xmax = p->x;
	if (p->y > ep->ymax) ep->ymax = p->y;
	if (p->x < ep->xmin) ep->xmin = p->x;
	if (p->y < ep->ymin) ep->ymin = p->y;
}

static Eina_Bool _points_equal(Enesim_Point *p0, Enesim_Point *p1, double threshold)
{
	Eina_Bool ret = EINA_FALSE;
	double x01;
	double y01;

	x01 = fabs(p0->x - p1->x);
	y01 = fabs(p0->y - p1->y);
	if (x01 < threshold && y01 < threshold)
		ret = EINA_TRUE;
	return ret;
}

static void _polygon_point_append(Enesim_Polygon *thiz, Enesim_Point *p)
{
	thiz->points = eina_list_append(thiz->points, p);
	_polygon_update_bounds(thiz, p);
}

static void _polygon_point_prepend(Enesim_Polygon *thiz, Enesim_Point *p)
{
	thiz->points = eina_list_prepend(thiz->points, p);
	_polygon_update_bounds(thiz, p);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Point * enesim_point_new(void)
{
	Enesim_Point *thiz;

	thiz = calloc(1, sizeof(Enesim_Point));
	return thiz;
}

Enesim_Point * enesim_point_new_from_coords(double x, double y, double z)
{
	Enesim_Point *thiz;

	thiz = enesim_point_new();
	enesim_point_coords_set(thiz, x, y, z);

	return thiz;
}

Enesim_Polygon * enesim_polygon_new(void)
{
	Enesim_Polygon *p;

	p = calloc(1, sizeof(Enesim_Polygon));
	p->xmax = p->ymax = -DBL_MAX;
	p->xmin = p->ymin = DBL_MAX;
	p->threshold = DBL_EPSILON;
	return p;
}

void enesim_polygon_threshold_set(Enesim_Polygon *p, double threshold)
{
	p->threshold = threshold;
}

void enesim_polygon_point_append_from_coords(Enesim_Polygon *thiz, double x, double y)
{
	Enesim_Point *p;
	Eina_List *l;

	l = eina_list_last(thiz->points);
	if (l)
	{
		Enesim_Point tmp;

		p = eina_list_data_get(l);
		tmp.x = x;
		tmp.y = y;
		tmp.z = 0;
		if (_points_equal(&tmp, p, thiz->threshold))
			return;
	}
	p = enesim_point_new_from_coords(x, y, 0);
	_polygon_point_append(thiz, p);
}

void enesim_polygon_point_prepend_from_coords(Enesim_Polygon *thiz, double x, double y)
{
	Enesim_Point *p;

	if (thiz->points)
	{
		Enesim_Point tmp;

		p = eina_list_data_get(thiz->points);
		tmp.x = x;
		tmp.y = y;
		tmp.z = 0;
		if (_points_equal(&tmp, p, thiz->threshold))
			return;
	}
	p = enesim_point_new_from_coords(x, y, 0);
	_polygon_point_prepend(thiz, p);
}

int enesim_polygon_point_count(Enesim_Polygon *thiz)
{
	return eina_list_count(thiz->points);
}

void enesim_polygon_clear(Enesim_Polygon *thiz)
{
	Enesim_Point *pt;
	EINA_LIST_FREE(thiz->points, pt)
	{
		free(pt);
	}
}

void enesim_polygon_delete(Enesim_Polygon *thiz)
{
	enesim_polygon_clear(thiz);
	free(thiz);
}

void enesim_polygon_dump(Enesim_Polygon *thiz)
{
	Enesim_Point *pt;
	Eina_List *l2;

	printf("New %s polygon\n", thiz->closed ? "closed": "opened");
	EINA_LIST_FOREACH(thiz->points, l2, pt)
	{
		printf("%g %g\n", pt->x, pt->y);
	}
}

void enesim_polygon_merge(Enesim_Polygon *thiz, Enesim_Polygon *to_merge)
{
	Enesim_Point *last, *first;
	Eina_List *l;

	if (!thiz->points) return;
	if (!to_merge->points) return;

	/* check that the last point at thiz is not equal to the first point to merge */
	l = eina_list_last(thiz->points);
	last = eina_list_data_get(l);
	first = eina_list_data_get(to_merge->points);
	if (_points_equal(first, last, thiz->threshold))
	{
		to_merge->points = eina_list_remove(to_merge->points, first);
		free(first);
	}
	thiz->points = eina_list_merge(thiz->points, to_merge->points);
	/* update the bounds */
	if (to_merge->xmax > thiz->xmax) thiz->xmax = to_merge->xmax;
	if (to_merge->ymax > thiz->ymax) thiz->ymax = to_merge->ymax;
	if (to_merge->xmin < thiz->xmin) thiz->xmin = to_merge->xmin;
	if (to_merge->ymin < thiz->ymin) thiz->ymin = to_merge->ymin;

	/* finally remove the to_merge polygon */
	free(to_merge);
}

void enesim_polygon_close(Enesim_Polygon *thiz, Eina_Bool close)
{
	thiz->closed = close;
}

Eina_Bool enesim_polygon_bounds(const Enesim_Polygon *thiz, double *xmin, double *ymin, double *xmax, double *ymax)
{
	if (!thiz->points) return EINA_FALSE;
	*xmin = thiz->xmin;
	*ymin = thiz->ymin;
	*ymax = thiz->ymax;
	*xmax = thiz->xmax;
	return EINA_TRUE;
}

#if 0
Enesim_F16p16_Vector * enesim_polygon_to_vectors(Enesim_Figure *f, int *nvectos,
	double tolerance)
{
	Enesim_F16p16_Vector *ret;
	Enesim_Polygon *p;
	Eina_List *l;
	int n = 0;

	/* allocate the maximum number of vectors possible */
	EINA_LIST_FOREACH(thiz->polygons, l, p)
		n += enesim_polygon_point_count(p);
	ret = malloc(n, sizeof(Enesim_F16p16_Vector));
	/* iterate over the polygons/points and create the vectors */
	EINA_LIST_FOREACH(thiz->polygons, l, p)
	{
		Enesim_Point *fp, *lp;
		Eina_List *llp;

		fp = eina_list_data_get(p->points);
		llp = p->points->next;

		while (llp)
		{
			Enesim_Point *pt = eina_list_data_get(llp);
			llp = llp->next;
		}
	}
#if 0
EINA_LIST_FOREACH(thiz->figure->polygons, l1, p)
{

	Eina_List *l2;
	int n = 0;
	int nverts = enesim_polygon_point_count(p);
	int sopen = !p->closed;
	int pclosed = 0;

	if (sopen && (draw_mode != ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE))
		sopen = 0;

	first_point = eina_list_data_get(p->points);
	last_point = eina_list_data_get(eina_list_last(p->points));

	{
		double x0, x1, y0, y1;
		double x01, y01;
		double len;

		x0 = ((int) (first_point->x * 256)) / 256.0;
		x1 = ((int) (last_point->x * 256)) / 256.0;
		y0 = ((int) (first_point->y * 256)) / 256.0;
		y1 = ((int) (last_point->y * 256)) / 256.0;
		//printf("%g %g -> %g %g\n", x0, y0, x1, y1);
		x01 = x1 - x0;
		y01 = y1 - y0;
		len = hypot(x01, y01);
		//printf("len = %g\n", len);
		if (len < (1 / 256.0))
		{
			//printf("last == first\n");
			nverts--;
			pclosed = 1;
		}
	}

	if (sopen && !pclosed)
		nverts--;

	pt = first_point;
	l2 = eina_list_next(p->points);
	while (n < nverts)
	{
		Enesim_Point *npt;
		double x0, y0, x1, y1;
		double x01, y01;
		double len;

		npt = eina_list_data_get(l2);
		if ((n == (enesim_polygon_point_count(p) - 1)) && !sopen)
			npt = first_point;
#if 0
		x0 = sx * (pt->x - lx) + lx;
		y0 = sy * (pt->y - ty) + ty;
		x1 = sx * (npt->x - lx) + lx;
		y1 = sy * (npt->y - ty) + ty;
		x0 = ((int) (x0 * 256)) / 256.0;
		x1 = ((int) (x1 * 256)) / 256.0;
		y0 = ((int) (y0 * 256)) / 256.0;
		y1 = ((int) (y1 * 256)) / 256.0;
#endif
		x0 = pt->x;
		y0 = pt->y;
		x1 = npt->x;
		y1 = npt->y;
		x01 = x1 - x0;
		y01 = y1 - y0;
		len = hypot(x01, y01);
#if 0
		if (len < (1 / 512.0))
		{
			/* FIXME what to do here?
			* skip this point, pick the next? */
			ENESIM_RENDERER_LOG(r, error, "Length %g < %g for points %gx%g %gx%g", len, 1/512.0, x0, y0, x1, y1);
			return EINA_FALSE;
		}
#endif
		//len *= 1 + (1 / 32.0);
		vec->a = -(y01 * 65536) / len;
		vec->b = (x01 * 65536) / len;
		vec->c = (65536 * ((y1 * x0) - (x1 * y0)))
				/ len;
		vec->xx0 = x0 * 65536;
		vec->yy0 = y0 * 65536;
		vec->xx1 = x1 * 65536;
		vec->yy1 = y1 * 65536;

		if ((vec->yy0 == vec->yy1) || (vec->xx0 == vec->xx1))
			vec->sgn = 0;
		else
		{
			vec->sgn = 1;
			if (vec->yy1 > vec->yy0)
			{
				if (vec->xx1 < vec->xx0)
					vec->sgn = -1;
			}
			else
			{
				if (vec->xx1 > vec->xx0)
					vec->sgn = -1;
			}
		}

		if (vec->xx0 > vec->xx1)
		{
			int xx0 = vec->xx0;
			vec->xx0 = vec->xx1;
			vec->xx1 = xx0;
		}

		if (vec->yy0 > vec->yy1)
		{
			int yy0 = vec->yy0;
			vec->yy0 = vec->yy1;
			vec->yy1 = yy0;
		}

		pt = npt;
		vec++;
		n++;
		l2 = eina_list_next(l2);

	}
#endif
	*nvectors = n;
	return ret;
}
#endif

Enesim_Figure * enesim_figure_new(void)
{
	Enesim_Figure *thiz;

	thiz = calloc(1, sizeof(Enesim_Figure));
	return thiz;
}

void enesim_figure_delete(Enesim_Figure *thiz)
{
	enesim_figure_clear(thiz);
	free(thiz);
}

Eina_Bool enesim_figure_bounds(const Enesim_Figure *thiz, double *xmin, double *ymin, double *xmax, double *ymax)
{
	Enesim_Polygon *p;
	Eina_Bool valid = EINA_FALSE;
	Eina_List *l;
	double fxmax;
	double fxmin;
	double fymax;
	double fymin;

	if (!thiz->polygons) return EINA_FALSE;

	fxmax = fymax = -DBL_MAX;
	fxmin = fymin = DBL_MAX;
	EINA_LIST_FOREACH(thiz->polygons, l, p)
	{
		double pxmin;
		double pxmax;
		double pymin;
		double pymax;

		if (!enesim_polygon_bounds(p, &pxmin, &pymin, &pxmax, &pymax))
			continue;

		if (pxmax > fxmax) fxmax = pxmax;
		if (pymax > fymax) fymax = pymax;
		if (pxmin < fxmin) fxmin = pxmin;
		if (pymin < fymin) fymin = pymin;
		valid = EINA_TRUE;
	}
	if (!valid) return EINA_FALSE;

	*xmax = fxmax;
	*xmin = fxmin;
	*ymax = fymax;
	*ymin = fymin;
	return EINA_TRUE;
}

void enesim_figure_polygon_append(Enesim_Figure *thiz, Enesim_Polygon *p)
{
	thiz->polygons = eina_list_append(thiz->polygons, p);
}

void enesim_figure_polygon_remove(Enesim_Figure *thiz, Enesim_Polygon *p)
{
	thiz->polygons = eina_list_remove(thiz->polygons, p);
}

int enesim_figure_polygon_count(Enesim_Figure *thiz)
{
	return eina_list_count(thiz->polygons);
}

void enesim_figure_clear(Enesim_Figure *thiz)
{
	Enesim_Polygon *p;
	EINA_LIST_FREE(thiz->polygons, p)
	{
		enesim_polygon_delete(p);
	}
}

void enesim_figure_dump(Enesim_Figure *f)
{
	Enesim_Polygon *p;
	Eina_List *l1;

	EINA_LIST_FOREACH(f->polygons, l1, p)
	{
		enesim_polygon_dump(p);
	}
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
