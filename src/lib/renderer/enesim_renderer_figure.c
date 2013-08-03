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
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_figure.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_shape_path_private.h"
#include "enesim_vector_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_FIGURE(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Figure, enesim_renderer_figure_descriptor_get())

typedef struct _Enesim_Renderer_Figure
{
	Enesim_Renderer_Shape_Path parent;
	Enesim_Figure *figure;
	Enesim_Polygon *last_polygon;
	Eina_Bool changed :1;
	Eina_Bool generated :1;
} Enesim_Renderer_Figure;

typedef struct _Enesim_Renderer_Figure_Class {
	Enesim_Renderer_Shape_Path_Class parent;
} Enesim_Renderer_Figure_Class;

static void _figure_generate_commands(Enesim_Renderer_Figure *thiz,
		Enesim_Path *path)
{
	Enesim_Figure *f;
	Enesim_Polygon *p;
	Eina_List *l1;

	f = thiz->figure;
	enesim_path_command_clear(path);
	EINA_LIST_FOREACH(f->polygons, l1, p)
	{
		Enesim_Point *pt;
		Eina_List *l2;
		Eina_List *l3;

		l2 = p->points;

		if (!l2) continue;
		pt = eina_list_data_get(l2);
		l2 = eina_list_next(l2);

		enesim_path_move_to(path, pt->x, pt->y);
		EINA_LIST_FOREACH(l2, l3, pt)
		{
			enesim_path_line_to(path, pt->x, pt->y);
		}
		if (p->closed)
			enesim_path_close(path);
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

	thiz = ENESIM_RENDERER_FIGURE(r);
	return thiz->changed;
}

static void _figure_shape_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Shape_Feature *features)
{
	*features = ENESIM_RENDERER_SHAPE_FEATURE_FILL_RENDERER | ENESIM_RENDERER_SHAPE_FEATURE_STROKE_RENDERER;
}

static Eina_Bool _figure_setup(Enesim_Renderer *r, Enesim_Path *path)
{
	Enesim_Renderer_Figure *thiz;

	thiz = ENESIM_RENDERER_FIGURE(r);
	if (!enesim_figure_polygon_count(thiz->figure))
	{
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

	thiz = ENESIM_RENDERER_FIGURE(r);
	thiz->changed = EINA_FALSE;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_SHAPE_PATH_DESCRIPTOR,
		Enesim_Renderer_Figure, Enesim_Renderer_Figure_Class,
		enesim_renderer_figure);

static void _enesim_renderer_figure_class_init(void *k)
{
	Enesim_Renderer_Class *r_klass;
	Enesim_Renderer_Shape_Class *s_klass;
	Enesim_Renderer_Shape_Path_Class *klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _figure_base_name_get;

	s_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	s_klass->features_get = _figure_shape_features_get;

	klass = ENESIM_RENDERER_SHAPE_PATH_CLASS(k);
	klass->has_changed = _figure_has_changed;
	klass->setup = _figure_setup;
	klass->cleanup = _figure_cleanup;
}

static void _enesim_renderer_figure_instance_init(void *o)
{
	Enesim_Renderer_Figure *thiz;

	thiz = ENESIM_RENDERER_FIGURE(o);
	/* to maintain compatibility */
	enesim_renderer_shape_stroke_location_set(ENESIM_RENDERER(o),
			ENESIM_RENDERER_SHAPE_STROKE_LOCATION_INSIDE);
	thiz->figure = enesim_figure_new();
}

static void _enesim_renderer_figure_instance_deinit(void *o EINA_UNUSED)
{
	Enesim_Renderer_Figure *thiz;

	thiz = ENESIM_RENDERER_FIGURE(o);
	if (thiz->figure)
	{
		enesim_figure_delete(thiz->figure);
		thiz->figure = NULL;
	}
}
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

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_figure);
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

	thiz = ENESIM_RENDERER_FIGURE(r);

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

	thiz = ENESIM_RENDERER_FIGURE(r);

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

	thiz = ENESIM_RENDERER_FIGURE(r);

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

	thiz = ENESIM_RENDERER_FIGURE(r);
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
