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
#include "enesim_figure.h"
#include "enesim_path.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_figure.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_figure.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_list_private.h"
#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_shape_path_private.h"
#include "enesim_figure_private.h"
#include "enesim_vector_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_RENDERER_FIGURE(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Figure, enesim_renderer_figure_descriptor_get())

typedef struct _Enesim_Renderer_Figure
{
	Enesim_Renderer_Shape_Path parent;
	/* properties */
	Enesim_Figure *figure;
	int last_figure_change;
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
	/* only check if our figure has changed, there is no other property */
	if (thiz->last_figure_change != enesim_figure_changed(thiz->figure))
		return EINA_TRUE;
	return EINA_FALSE;
}

static void _figure_shape_features_get(Enesim_Renderer *r EINA_UNUSED,
		int *features)
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

	if (thiz->last_figure_change != enesim_figure_changed(thiz->figure))
	{
		_figure_generate_commands(thiz, path);
	}

	return EINA_TRUE;
}

static void _figure_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Figure *thiz;

	thiz = ENESIM_RENDERER_FIGURE(r);
	thiz->last_figure_change = enesim_figure_changed(thiz->figure);
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
		enesim_figure_unref(thiz->figure);
		thiz->figure = NULL;
	}
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a background renderer
 * @return The new renderer
 */
EAPI Enesim_Renderer * enesim_renderer_figure_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_figure);
	return r;
}

/**
 * Set the figure of a figure renderer
 * @ender_prop{inner_figure}
 * @param[in] r The figure renderer
 * @param[in] figure The figure to set @ender_transfer{full}
 */
EAPI void enesim_renderer_figure_inner_figure_set(Enesim_Renderer *r, Enesim_Figure *figure)
{
	Enesim_Renderer_Figure *thiz;

	thiz = ENESIM_RENDERER_FIGURE(r);
	if (!figure)
	{
		enesim_figure_clear(thiz->figure);
	}
	else
	{
		enesim_figure_unref(thiz->figure);
		thiz->figure = figure;
	}
}

/**
 * Get the figure of a figure renderer
 * @ender_prop{inner_figure}
 * @param[in] r The figure renderer
 * @return The figure of the figure renderer @ender_transfer{none}
 *
 * @note It is always faster to get the figure of a figure renderer and
 * add commands there than create a new figure using @ref enesim_figure_new,
 * set the commands and finally call @ref enesim_renderer_figure_inner_figure_set
 */
EAPI Enesim_Figure * enesim_renderer_figure_inner_figure_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Figure *thiz;

	thiz = ENESIM_RENDERER_FIGURE(r);
	return enesim_figure_ref(thiz->figure);
}
