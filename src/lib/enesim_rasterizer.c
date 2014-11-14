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
#include "enesim_renderer_shape.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_list_private.h"
#include "enesim_renderer_private.h"
#include "enesim_vector_private.h"
#include "enesim_rasterizer_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
static int _tysort(const void *l, const void *r)
{
	Enesim_F16p16_Vector *lv = (Enesim_F16p16_Vector *)l;
	Enesim_F16p16_Vector *rv = (Enesim_F16p16_Vector *)r;

	if (lv->yy0 <= rv->yy0)
		return -1;
	return 1;
}

static Eina_Bool _rasterizer_vectors_generate(Enesim_Renderer *r)
{
	Enesim_Rasterizer *thiz;
	Enesim_Renderer_Shape_Draw_Mode draw_mode;
	Enesim_Polygon *p;
	Enesim_F16p16_Vector *vec;
	Eina_List *l1;
	int nvectors = 0;
	int n = 0;

	thiz = ENESIM_RASTERIZER(r);

	draw_mode = enesim_renderer_shape_draw_mode_get(r);
	/* allocate the maximum number of vectors possible */
	EINA_LIST_FOREACH(thiz->figure->polygons, l1, p)
		n += enesim_polygon_point_count(p);
	vec = malloc(n * sizeof(Enesim_F16p16_Vector));
	/* generate the vectors */
	n = 0;
	EINA_LIST_FOREACH(thiz->figure->polygons, l1, p)
	{
		Enesim_Point *fp, *lp, *pt, pp;
		Eina_List *points, *l2;
		double len;
		int sopen = !p->closed;
		int pclosed = p->closed;
		int npts;

		if (sopen && (draw_mode != ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE))
			sopen = EINA_FALSE;

		npts = enesim_polygon_point_count(p);
		/* check polygons integrity */
		if ((npts < 2) || ((npts < 3) && (draw_mode !=
				ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)))
		{
			continue;
		}
		nvectors += npts;
		fp = eina_list_data_get(p->points);
		lp = eina_list_data_get(eina_list_last(p->points));
		len = enesim_point_2d_distance(fp, lp);
		if (len < (1 / 256.0))
			pclosed = EINA_TRUE;

		/* start with the second point */
		pp = *fp;
		enesim_point_2d_round(&pp, 256.0);
		points = eina_list_next(p->points);
		EINA_LIST_FOREACH(points, l2, pt)
		{
			Enesim_Point pc;
			Enesim_F16p16_Vector *v = &vec[n];

			pc = *pt;
			enesim_point_2d_round(&pc, 256.0);
			if (enesim_f16p16_vector_setup(v, &pp, &pc, 1 / 256.0))
			{
				pp = pc;
				n++;
			}
		}
		/* add the last vertex */
		if (!sopen || pclosed)
		{
			Enesim_Point pc;
			Enesim_F16p16_Vector *v = &vec[n];

			pc = *fp;
			enesim_point_2d_round(&pc, 256.0);
			pp = *lp;
			enesim_point_2d_round(&pp, 256.0);
			enesim_f16p16_vector_setup(v, &pp, &pc, 1 / 256.0);
			n++;
		}
	}
	thiz->vectors = vec;
	thiz->nvectors = n;

	qsort(thiz->vectors, thiz->nvectors, sizeof(Enesim_F16p16_Vector), _tysort);
	thiz->changed = EINA_FALSE;
	return EINA_TRUE;
}

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
#if 0
static void _rasterizer_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	/* FIXME we should use the rasterizer implementation features */
	*features = ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_PROJECTIVE |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static void _rasterizer_sw_hints_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_SW_HINT_COLORIZE;
}

static void _rasterizer_shape_features_get(Enesim_Renderer *r EINA_UNUSED, Enesim_Renderer_Shape_Feature *features)
{
	*features = ENESIM_RENDERER_SHAPE_FEATURE_FILL_RENDERER | ENESIM_RENDERER_SHAPE_FEATURE_STROKE_RENDERER;
}
#endif

static void _rasterizer_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Rasterizer *thiz;
	Enesim_Rasterizer_Class *klass;

	thiz = ENESIM_RASTERIZER(r);
	thiz->changed = EINA_FALSE;

	klass = ENESIM_RASTERIZER_CLASS(r);
	if (klass->sw_cleanup)
		klass->sw_cleanup(r, s);
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_ABSTRACT_BOILERPLATE(ENESIM_RENDERER_SHAPE_DESCRIPTOR,
		Enesim_Rasterizer,
		Enesim_Rasterizer_Class,
		enesim_rasterizer);

static void _enesim_rasterizer_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->sw_cleanup = _rasterizer_sw_cleanup;
}

static void _enesim_rasterizer_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_rasterizer_instance_deinit(void *o EINA_UNUSED)
{
	Enesim_Rasterizer *thiz;

	thiz = ENESIM_RASTERIZER(o);
	if (thiz->vectors)
	{
		free(thiz->vectors);
		thiz->vectors = NULL;
	}
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Eina_Bool enesim_rasterizer_has_changed(Enesim_Renderer *r)
{
	Enesim_Rasterizer *thiz;

	thiz = ENESIM_RASTERIZER(r);
	return thiz->changed;
}

const Enesim_F16p16_Vector * enesim_rasterizer_figure_vectors_get(
		Enesim_Renderer *r, int *nvectors)
{
	Enesim_Rasterizer *thiz;

	thiz = ENESIM_RASTERIZER(r);
	if (!thiz->vectors_generated)
	{
		if (thiz->vectors)
		{
			free(thiz->vectors);
			thiz->vectors = NULL;
			thiz->nvectors = 0;
		}
		_rasterizer_vectors_generate(r);
		thiz->vectors_generated = EINA_TRUE;
	}

	*nvectors = thiz->nvectors;
	return thiz->vectors;
}

void enesim_rasterizer_figure_set(Enesim_Renderer *r, const Enesim_Figure *f)
{
	Enesim_Rasterizer *thiz;

	thiz = ENESIM_RASTERIZER(r);
	thiz->figure = f;
	thiz->changed = EINA_TRUE;
	thiz->vectors_generated = EINA_FALSE;
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

