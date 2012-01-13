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
	Enesim_Renderer *path;
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
	thiz->final_fill(thiz->path, x, y, len, ddata);
}

static void _figure_generate_commands(Enesim_Renderer_Figure *thiz)
{
	Enesim_Figure *f;
	Enesim_Polygon *p;
	Eina_List *l1;

	f = thiz->figure;
	EINA_LIST_FOREACH(f->polygons, l1, p)
	{
		Enesim_Point *pt;
		Eina_List *l2;
		Eina_List *l3;

		l2 = p->points;

		if (!l2) continue;
		pt = eina_list_data_get(l2);
		l2 = eina_list_next(l2);

		enesim_renderer_path_move_to(thiz->path, pt->x, pt->y);
		EINA_LIST_FOREACH(l2, l3, pt)
		{
			enesim_renderer_path_line_to(thiz->path, pt->x, pt->y);
		}
		if (p->closed)
			enesim_renderer_path_close(thiz->path, EINA_TRUE);
	}
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
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Figure *thiz;
	Enesim_Renderer_Sw_Data *sdata;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	const Enesim_Renderer_Shape_State *css = sstates[ENESIM_STATE_CURRENT];

	thiz = _figure_get(r);
	if (!enesim_figure_polygon_count(thiz->figure))
	{
		/* TODO no polys do nothing, error? ok? */
		ENESIM_RENDERER_ERROR(r, error, "No points on the polygon, nothing to draw");
		return EINA_FALSE;
	}

	if (thiz->changed)
		_figure_generate_commands(thiz);

	/* pass all the properties to the path */
	enesim_renderer_shape_stroke_color_set(thiz->path, css->stroke.color);
	enesim_renderer_shape_stroke_renderer_set(thiz->path, css->stroke.r);
	enesim_renderer_shape_stroke_weight_set(thiz->path, css->stroke.weight);
	enesim_renderer_shape_fill_color_set(thiz->path, css->fill.color);
	enesim_renderer_shape_fill_renderer_set(thiz->path, css->fill.r);
	enesim_renderer_shape_draw_mode_set(thiz->path, css->draw_mode);
	/* base properties */
	enesim_renderer_origin_set(thiz->path, cs->ox, cs->oy);
	enesim_renderer_geometry_transformation_set(thiz->path, &cs->geometry_transformation);
	enesim_renderer_transformation_set(thiz->path, &cs->transformation);
	enesim_renderer_color_set(thiz->path, cs->color);
	enesim_renderer_rop_set(thiz->path, cs->rop);
	enesim_renderer_scale_set(thiz->path, cs->sx, cs->sy);

	if (!enesim_renderer_setup(thiz->path, s, error))
		return EINA_FALSE;

	*fill = _span;

	sdata = enesim_renderer_backend_data_get(thiz->path, ENESIM_BACKEND_SOFTWARE);
	thiz->final_fill = sdata->fill;

	return EINA_TRUE;
}

static void _state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Figure *thiz;

	thiz = _figure_get(r);
	enesim_renderer_cleanup(thiz->path, s);
	thiz->changed = EINA_FALSE;
}

static void _figure_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Renderer_Figure *thiz;

	thiz = _figure_get(r);
	enesim_renderer_flags(thiz->path, flags);
}

static Enesim_Renderer_Shape_Descriptor _figure_descriptor = {
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
	thiz->path = enesim_renderer_path_new();

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
