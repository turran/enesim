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
#include "libargb.h"

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_path.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_path.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#ifdef BUILD_OPENGL
#include "Enesim_OpenGL.h"
#endif

#include "enesim_list_private.h"
#include "enesim_path_private.h"
#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_path_abstract_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_LOG_DEFAULT enesim_log_renderer

#define ENESIM_RENDERER_PATH(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Path,					\
		enesim_renderer_path_descriptor_get())

typedef struct _Enesim_Renderer_Path
{
	Enesim_Renderer_Shape parent;
	/* properties */
	Enesim_Path *path;
	/* private */
	Eina_List *abstracts;
	Enesim_Renderer *current;
	int last_path_change;
} Enesim_Renderer_Path;

typedef struct _Enesim_Renderer_Path_Class {
	Enesim_Renderer_Shape_Class parent;
} Enesim_Renderer_Path_Class;

static Eina_Bool _enesim_renderer_path_is_valid(Enesim_Renderer *r, Enesim_Surface *s)
{
	if (!enesim_renderer_path_abstract_is_available(r))
		return EINA_FALSE;
	if (!enesim_renderer_is_supported(r, s))
		return EINA_FALSE;
	return EINA_TRUE;
}

static void _enesim_renderer_path_propagate(Enesim_Renderer *r, Enesim_Renderer *abstract)
{
	Enesim_Renderer_Path *thiz;

	thiz = ENESIM_RENDERER_PATH(r);
	/* propagate all the shape properties */
	enesim_renderer_shape_propagate(r, abstract);
	/* propagate all the renderer properties */
	enesim_renderer_propagate(r, abstract);
	/* set the commands */
	enesim_renderer_path_abstract_path_set(abstract, enesim_path_ref(thiz->path));
}

static void _path_span(Enesim_Renderer *r, int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Path *thiz;

	thiz = ENESIM_RENDERER_PATH(r);
	enesim_renderer_sw_draw(thiz->current, x, y, len, ddata);
}

#if BUILD_OPENGL
static void _path_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, const Eina_Rectangle *area, int x, int y)
{
	Enesim_Renderer_Path *thiz;

	thiz = ENESIM_RENDERER_PATH(r);
	enesim_renderer_opengl_draw(thiz->current, s, rop, area, x, y);
}
#endif

static Eina_Bool _path_setup(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, Enesim_Log **log)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Renderer *abstract;
	Eina_List *l;

	thiz = ENESIM_RENDERER_PATH(r);
	thiz->current = NULL;
	EINA_LIST_FOREACH (thiz->abstracts, l, abstract)
	{
		if (!abstract) continue;
		if (!_enesim_renderer_path_is_valid(abstract, s))
		{
			INF("Abstract '%s' is not valid", enesim_renderer_name_get(abstract));
			continue;
		}
		_enesim_renderer_path_propagate(r, abstract);
		if (!enesim_renderer_setup(abstract, s, rop, log))
		{
			INF("Abstract '%s' failed on the setup", enesim_renderer_name_get(abstract));
			enesim_renderer_cleanup(abstract, s);
			continue;
		}
		thiz->current = abstract;
		break;
	}
	if (!thiz->current)
	{
		ENESIM_RENDERER_LOG(r, log, "No path implementation found");
		return EINA_FALSE;
	}

	return EINA_TRUE;
}

static void _path_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Path *thiz;

	thiz = ENESIM_RENDERER_PATH(r);
	if (!thiz->current)
	{
		WRN("Doing a cleanup without a setup");
		return;
	}
	enesim_renderer_shape_state_commit(r);
	enesim_renderer_cleanup(thiz->current, s);
	thiz->current = NULL;
	/* reset the change count */
	thiz->last_path_change = thiz->path->changed;
}
/*----------------------------------------------------------------------------*
 *                             Shape interface                                *
 *----------------------------------------------------------------------------*/
static void _path_shape_features_get(Enesim_Renderer *r,
		Enesim_Renderer_Shape_Feature *features)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Renderer *abstract;
	Eina_List *l;

	thiz = ENESIM_RENDERER_PATH(r);
	EINA_LIST_FOREACH (thiz->abstracts, l, abstract)
	{
		if (!abstract) continue;
		*features |= enesim_renderer_shape_features_get(abstract);
	}
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _path_name_get(Enesim_Renderer *r EINA_UNUSED)
{
	return "path";
}

static void _path_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Renderer *abstract;
	Eina_List *l;

	thiz = ENESIM_RENDERER_PATH(r);
	if (thiz->current)
	{
		enesim_renderer_bounds_get(thiz->current, bounds);
	}
	else
	{
		EINA_LIST_FOREACH (thiz->abstracts, l, abstract)
		{
			if (!abstract) continue;
			if (!_enesim_renderer_path_is_valid(abstract, NULL))
				continue;
			_enesim_renderer_path_propagate(r, abstract);
			enesim_renderer_bounds_get(abstract, bounds);
			break;
		}
	}
}

static void _path_features_get(Enesim_Renderer *r,
		Enesim_Renderer_Feature *features)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Renderer *abstract;
	Eina_List *l;

	thiz = ENESIM_RENDERER_PATH(r);
	EINA_LIST_FOREACH (thiz->abstracts, l, abstract)
	{
		if (!abstract) continue;
		*features |= enesim_renderer_features_get(abstract);
	}
}

static Eina_Bool _path_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;

	thiz = ENESIM_RENDERER_PATH(r);
	if (enesim_renderer_shape_state_has_changed(r))
		return EINA_TRUE;
	/* only check if our path has changed, there is no other property */
	if (thiz->last_path_change != thiz->path->changed)
		return EINA_TRUE;
	return EINA_FALSE;
}

static void _path_sw_hints(Enesim_Renderer *r EINA_UNUSED, Enesim_Rop rop
		EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	/* we always use the implementation renderer for drawing
	 * so mark every hint
	 */
	*hints = ENESIM_RENDERER_SW_HINT_ROP | ENESIM_RENDERER_SW_HINT_MASK |
			ENESIM_RENDERER_SW_HINT_COLORIZE;
}


static Eina_Bool _path_sw_setup(Enesim_Renderer *r ,Enesim_Surface *s,
		Enesim_Rop rop, Enesim_Renderer_Sw_Fill *draw, Enesim_Log **l)
{
	if (!_path_setup(r, s, rop, l))
		return EINA_FALSE;

	*draw = _path_span;
	return EINA_TRUE;
}

static void _path_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	_path_cleanup(r, s);
}

#if BUILD_OPENGL
static Eina_Bool _path_opengl_setup(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l)
{
	if (!_path_setup(r, s, rop, l))
		return EINA_FALSE;

	*draw = _path_opengl_draw;
	return EINA_TRUE;
}

static void _path_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	_path_cleanup(r, s);
}
#endif
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_SHAPE_DESCRIPTOR,
		Enesim_Renderer_Path, Enesim_Renderer_Path_Class,
		enesim_renderer_path);

static void _enesim_renderer_path_class_init(void *k)
{
	Enesim_Renderer_Class *klass;
	Enesim_Renderer_Shape_Class *shape_klass;

	shape_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	shape_klass->features_get = _path_shape_features_get;
	shape_klass->has_changed = _path_has_changed;

	/* we need to override the default implementation on every renderer
	 * virtual function. That is because we need to propagate the properties
	 * before calling the function on the real renderer and because the
	 * dash list and the path have the changed flag cleared every time
	 * we need to calc the bounds/draw/etc. We override the setup/cleanup
	 * too because if the renderers are set we will setup/cleanup such
	 * renderers twice
	 */
	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _path_name_get;
	klass->bounds_get = _path_bounds_get;
	klass->features_get = _path_features_get;

	klass->sw_hints_get = _path_sw_hints;
	klass->sw_setup = _path_sw_setup;
	klass->sw_cleanup = _path_sw_cleanup;
#if BUILD_OPENGL
	klass->opengl_setup = _path_opengl_setup;
	klass->opengl_cleanup = _path_opengl_cleanup;
#endif
}

static void _enesim_renderer_path_instance_init(void *o)
{
	Enesim_Renderer_Path *thiz;

	thiz = ENESIM_RENDERER_PATH(o);
	thiz->path = enesim_path_new();
	/* create the abstracts */
	thiz->abstracts = eina_list_append(thiz->abstracts, enesim_renderer_path_enesim_new());
#if BUILD_OPENGL
	thiz->abstracts = eina_list_append(thiz->abstracts, enesim_renderer_path_nv_new());
	thiz->abstracts = eina_list_append(thiz->abstracts, enesim_renderer_path_tesselator_new());
#endif
#if BUILD_CAIRO
	thiz->abstracts = eina_list_append(thiz->abstracts, enesim_renderer_path_cairo_new());
#endif
}

static void _enesim_renderer_path_instance_deinit(void *o)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Renderer *abstract;

	thiz = ENESIM_RENDERER_PATH(o);
	EINA_LIST_FREE(thiz->abstracts, abstract)
		enesim_renderer_unref(abstract);
	enesim_path_unref(thiz->path);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Create a new path renderer
 * @return The newly created path renderer
 */
EAPI Enesim_Renderer * enesim_renderer_path_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_path);
	return r;
}

/**
 * Set the path of a path renderer
 * @param[in] r The path renderer
 * @param[in] path The path to set [transfer full]
 */
EAPI void enesim_renderer_path_path_set(Enesim_Renderer *r, Enesim_Path *path)
{
	Enesim_Renderer_Path *thiz;

	thiz = ENESIM_RENDERER_PATH(r);
	if (!path)
	{
		enesim_path_command_clear(thiz->path);
	}
	else
	{
		enesim_path_command_set(thiz->path, path->commands);
		enesim_path_unref(path);
	}
}

/**
 * Get the path of a path renderer
 * @param[in] r The path renderer
 * @return The path of the path renderer [transfer none]
 *
 * @note It is always faster to get the path of a path renderer and
 * add commands there than create a new path using @ref enesim_path_new,
 * set the commands and finally call @ref enesim_renderer_path_path_set
 */
EAPI Enesim_Path * enesim_renderer_path_path_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;

	thiz = ENESIM_RENDERER_PATH(r);
	return enesim_path_ref(thiz->path);
}
