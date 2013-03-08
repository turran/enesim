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
#include "enesim_private.h"

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_path.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_path.h"
#include "enesim_renderer_figure.h"

#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_shape_path_private.h"
#include "enesim_vector_private.h"
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
	Eina_Bool changed :1;
	Eina_Bool generated :1;
} Enesim_Renderer_Figure;

static inline Enesim_Renderer_Figure * _figure_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Figure *thiz;

	thiz = enesim_renderer_shape_path_data_get(r);
	ENESIM_RENDERER_FIGURE_MAGIC_CHECK(thiz);

	return thiz;
}

static void _figure_generate_commands(Enesim_Renderer_Figure *thiz,
		Enesim_Renderer *path)
{
	Enesim_Figure *f;
	Enesim_Polygon *p;
	Eina_List *l1;

	f = thiz->figure;
	enesim_renderer_path_command_clear(path);
	EINA_LIST_FOREACH(f->polygons, l1, p)
	{
		Enesim_Point *pt;
		Eina_List *l2;
		Eina_List *l3;

		l2 = p->points;

		if (!l2) continue;
		pt = eina_list_data_get(l2);
		l2 = eina_list_next(l2);

		enesim_renderer_path_move_to(path, pt->x, pt->y);
		EINA_LIST_FOREACH(l2, l3, pt)
		{
			enesim_renderer_path_line_to(path, pt->x, pt->y);
		}
		if (p->closed)
			enesim_renderer_path_close(path, EINA_TRUE);
	}
}

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _figure_base_name_get(Enesim_Renderer *r EINA_UNUSED)
{
	return "figure";
}

static Eina_Bool _figure_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Figure *thiz;

	thiz = _figure_get(r);
	return thiz->changed;
}

static void _figure_shape_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Shape_Feature *features)
{
	*features = ENESIM_SHAPE_FLAG_FILL_RENDERER | ENESIM_SHAPE_FLAG_STROKE_RENDERER;
}

static Eina_Bool _figure_setup(Enesim_Renderer *r, Enesim_Renderer *path,
		Enesim_Log **l)
{
	Enesim_Renderer_Figure *thiz;

	thiz = _figure_get(r);
	if (!enesim_figure_polygon_count(thiz->figure))
	{
		/* TODO no polys do nothing, l? ok? */
		ENESIM_RENDERER_LOG(r, l, "No points on the polygon, nothing to draw");
		return EINA_FALSE;
	}

	if (thiz->changed && !thiz->generated)
	{
		_figure_generate_commands(thiz, path);
		thiz->generated = EINA_TRUE;
	}

	return EINA_TRUE;
}

static void _figure_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Figure *thiz;

	thiz = _figure_get(r);
	thiz->changed = EINA_FALSE;
}

static void _figure_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Figure *thiz;

	thiz = _figure_get(r);
	if (thiz->figure)
	{
		enesim_figure_delete(thiz->figure);
		thiz->figure = NULL;
	}
	free(thiz);
}

static Enesim_Renderer_Shape_Path_Descriptor _figure_descriptor = {
	/* .base_name_get = 		*/ _figure_base_name_get,
	/* .free = 			*/ _figure_free,
	/* .has_changed = 		*/ _figure_has_changed,
	/* .shape_features_get =	*/ _figure_shape_features_get,
	/* .bounds_get = 			*/ NULL,
	/* .setup = 			*/ _figure_setup,
	/* .cleanup = 			*/ _figure_cleanup,
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

	r = enesim_renderer_shape_path_new(&_figure_descriptor, thiz);
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
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
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

	thiz = _figure_get(r);

	p = thiz->last_polygon;
	if (!p) return;
	enesim_polygon_point_append_from_coords(p, x, y);
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
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
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
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
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_figure_polygon_set(Enesim_Renderer *r, Eina_List *list)
{
	Enesim_Renderer_Figure_Polygon *polygon;
	Eina_List *l1;

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
