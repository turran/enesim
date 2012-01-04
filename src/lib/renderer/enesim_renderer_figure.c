/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2011 Jorge Luis Zapata
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
#include "Enesim.h"
#include "enesim_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_FIGURE_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_FIGURE_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_FIGURE_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Figure
{
	EINA_MAGIC
	Enesim_Figure *figure;
	Enesim_Polygon *last_polygon;
	Enesim_Point *last_point;
	Enesim_Renderer *bifigure;
	Enesim_Renderer_Sw_Fill final_fill;
	Eina_Bool changed :1;
} Enesim_Renderer_Figure;

static inline Enesim_Renderer_Figure * _figure_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Figure *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	ENESIM_RENDERER_FIGURE_MAGIC_CHECK(thiz);

	return thiz;
}

static void _span(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Figure *thiz;

	thiz = _figure_get(r);
	thiz->final_fill(thiz->bifigure, x, y, len, ddata);
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _figure_name(Enesim_Renderer *r)
{
	return "figure";
}

static void _free(Enesim_Renderer *r)
{
	Enesim_Renderer_Figure *thiz;

	thiz = _figure_get(r);
	enesim_figure_delete(thiz->figure);
}

static Eina_Bool _state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Figure *thiz;
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Renderer *sr;
	Enesim_Renderer *fr;
	Enesim_Renderer_Sw_Data *sdata;
	double sw;

	thiz = _figure_get(r);
	if (!enesim_figure_polygon_count(thiz->figure))
	{
		/* TODO no polys do nothing, error? ok? */
		ENESIM_RENDERER_ERROR(r, error, "No points on the polygon, nothing to draw");
		return EINA_FALSE;
	}
	if (!enesim_renderer_shape_setup(r, state, s, error))
	{
		return EINA_FALSE;
	}

	enesim_renderer_shape_stroke_weight_get(r, &sw);
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);
	/* pick up the correct renderer */
	if ((draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE) && sw >= 1.0)
	{
		/* FIXME not yet */
		/* TODO generate the stroke */
	}
	else
	{
		/* FIXME set the correct draw mode for this renderer */
		enesim_rasterizer_figure_set(thiz->bifigure, thiz->figure);
	}

	enesim_renderer_shape_stroke_renderer_get(r, &sr);
	enesim_renderer_shape_stroke_renderer_set(thiz->bifigure, sr);
	enesim_renderer_shape_fill_renderer_get(r, &fr);
	enesim_renderer_shape_fill_renderer_set(thiz->bifigure, fr);

	if (!enesim_renderer_setup(thiz->bifigure, s, error))
		return EINA_FALSE;

	*fill = _span;

	sdata = enesim_renderer_backend_data_get(thiz->bifigure, ENESIM_BACKEND_SOFTWARE);
	thiz->final_fill = sdata->fill;

	return EINA_TRUE;
}

static void _state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	enesim_renderer_shape_cleanup(r, s);
}

static void _figure_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_PROJECTIVE |
			ENESIM_RENDERER_FLAG_ARGB8888 |
			ENESIM_SHAPE_FLAG_FILL_RENDERER;
}

static Enesim_Renderer_Descriptor _figure_descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _figure_name,
	/* .free = 			*/ _free,
	/* .boundings = 		*/ NULL,
	/* .destination_transform = 	*/ NULL,
	/* .flags = 			*/ _figure_flags,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ NULL,
	/* .sw_setup = 			*/ _state_setup,
	/* .sw_cleanup = 		*/ _state_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
	/* .opengl_setup =          	*/ NULL,
	/* .opengl_shader_setup = 	*/ NULL,
	/* .opengl_cleanup =        	*/ NULL
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Renderer * enesim_renderer_figure_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Figure *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Figure));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_FIGURE_MAGIC);
	thiz->figure = enesim_figure_new();
	thiz->bifigure = enesim_rasterizer_bifigure_new();

	r = enesim_renderer_shape_new(&_figure_descriptor, thiz);
	return r;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_figure_polygon_add(Enesim_Renderer *r)
{
	Enesim_Renderer_Figure *thiz;
	Enesim_Polygon *p;

	thiz = _figure_get(r);

	p = enesim_polygon_new();
	enesim_figure_polygon_append(thiz->figure, p);

	thiz->last_polygon = p;
	thiz->changed = 1;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_figure_polygon_vertex_add(Enesim_Renderer *r,
		double x, double y)
{
	Enesim_Renderer_Figure *thiz;
	Enesim_Polygon *p;
	Enesim_Point *pt;

	thiz = _figure_get(r);

	p = thiz->last_polygon;
	if (!p) return;

	pt = thiz->last_point;
	if (pt)
	{
		
		if ((fabs(pt->x - x) < (1 / 256.0)) &&
				(fabs(pt->y - y) < (1 / 256.0)))
			return;
	}

	pt = enesim_point_new_from_coords(x, y);
	enesim_polygon_point_append(p, pt);
	thiz->last_point = pt;
	thiz->changed = 1;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_figure_polygon_close(Enesim_Renderer *r, Eina_Bool close)
{
	Enesim_Renderer_Figure *thiz;
	Enesim_Polygon *p;

	thiz = _figure_get(r);

	p = thiz->last_polygon;
	if (!p) return;

	enesim_polygon_close(p, close);
	thiz->changed = 1;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_figure_clear(Enesim_Renderer *r)
{
	Enesim_Renderer_Figure *thiz;

	thiz = _figure_get(r);
	enesim_figure_clear(thiz->figure);
	thiz->changed = 1;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_figure_polygon_set(Enesim_Renderer *r, Eina_List *list)
{
	Enesim_Renderer_Figure_Polygon *polygon;
	Eina_List *l1;
	Enesim_Renderer_Figure *thiz;

	thiz = _figure_get(r);
	enesim_renderer_figure_clear(r);
	EINA_LIST_FOREACH(list, l1, polygon)
	{
		Eina_List *l2;
		Enesim_Renderer_Figure_Vertex *vertex;

		enesim_renderer_figure_polygon_add(r);
		EINA_LIST_FOREACH(polygon->vertices, l2, vertex)
		{
			enesim_renderer_figure_polygon_vertex_add(r, vertex->x, vertex->y);
		}
	}
}
