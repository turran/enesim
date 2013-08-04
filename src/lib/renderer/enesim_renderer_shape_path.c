/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2010 Jorge Luis Zapata
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
/* We need to start removing all the extra logic in the renderer and move it
 * here for simple usage
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
#include "enesim_renderer_path.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_shape_path_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_renderer_shape

static Eina_Bool _shape_path_propagate(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape_Path *thiz;
	Enesim_Renderer_Shape_Path_Class *klass;
	Enesim_Renderer *fill, *stroke;
	const Enesim_Renderer_State *rstate;
	const Enesim_Renderer_Shape_State *sstate;

	rstate = enesim_renderer_state_get(r);
	sstate = enesim_renderer_shape_state_get(r);

	thiz = ENESIM_RENDERER_SHAPE_PATH(r);
	klass = ENESIM_RENDERER_SHAPE_PATH_CLASS_GET(r);

	/* common properties */
	enesim_renderer_color_set(thiz->r_path, rstate->current.color);
	enesim_renderer_origin_set(thiz->r_path, rstate->current.ox, rstate->current.oy);
	enesim_renderer_transformation_set(thiz->r_path, &rstate->current.transformation);

	/* shape properties */
	fill = sstate->current.fill.r;
	if (fill) fill = enesim_renderer_ref(fill);
	enesim_renderer_shape_fill_renderer_set(thiz->r_path, fill);
	enesim_renderer_shape_fill_color_set(thiz->r_path, sstate->current.fill.color);
	enesim_renderer_shape_fill_rule_set(thiz->r_path, sstate->current.fill.rule);

	stroke = sstate->current.stroke.r;
	if (stroke) stroke = enesim_renderer_ref(stroke);
	enesim_renderer_shape_stroke_renderer_set(thiz->r_path, stroke);
	enesim_renderer_shape_stroke_weight_set(thiz->r_path, sstate->current.stroke.weight);
	enesim_renderer_shape_stroke_color_set(thiz->r_path, sstate->current.stroke.color);
	enesim_renderer_shape_stroke_cap_set(thiz->r_path, sstate->current.stroke.cap);
	enesim_renderer_shape_stroke_join_set(thiz->r_path, sstate->current.stroke.join);
	/* FIXME in fact the path renderer does not support the stroke location */
	enesim_renderer_shape_stroke_location_set(thiz->r_path, sstate->current.stroke.location);

	if (sstate->stroke_dashes_changed)
	{
		Enesim_Renderer_Shape_Stroke_Dash *d;
		const Eina_List *ll;

		enesim_renderer_shape_stroke_dash_clear(thiz->r_path);
		EINA_LIST_FOREACH (sstate->stroke_dashes, ll, d)
		{
			enesim_renderer_shape_stroke_dash_add(thiz->r_path, d);
		}
	}

	enesim_renderer_shape_draw_mode_set(thiz->r_path, sstate->current.draw_mode);
	/* now let the implementation override whatever it wants to */
	if (klass->setup)
		if (!klass->setup(r, thiz->path))
			return EINA_FALSE;
	return EINA_TRUE;
}

static Eina_Bool _shape_path_setup(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Log **l)
{
	Enesim_Renderer_Shape_Path *thiz;
	Enesim_Renderer_Shape_Path_Class *klass;

	thiz = ENESIM_RENDERER_SHAPE_PATH(r);
	klass = ENESIM_RENDERER_SHAPE_PATH_CLASS_GET(r);

	if (!_shape_path_propagate(r))
		return EINA_FALSE;

	if (!enesim_renderer_setup(thiz->r_path, s, l))
	{
		if (klass->cleanup)
			klass->cleanup(r);
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

static void _shape_path_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Shape_Path *thiz;
	Enesim_Renderer_Shape_Path_Class *klass;

	thiz = ENESIM_RENDERER_SHAPE_PATH(r);
	klass = ENESIM_RENDERER_SHAPE_PATH_CLASS_GET(r);
	enesim_renderer_shape_state_commit(r);
	enesim_renderer_cleanup(thiz->r_path, s);
	if (klass->cleanup)
		klass->cleanup(r);
}
/*----------------------------------------------------------------------------*
 *                               Span functions                               *
 *----------------------------------------------------------------------------*/
/* Use the internal path for drawing */
static void _shape_path_path_span(Enesim_Renderer *r,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = ENESIM_RENDERER_SHAPE_PATH(r);
	enesim_renderer_sw_draw(thiz->r_path, x, y, len, ddata);
}

#if BUILD_OPENGL
static void _shape_path_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		const Eina_Rectangle *area, int w, int h)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = ENESIM_RENDERER_SHAPE_PATH(r);
	enesim_renderer_opengl_draw(thiz->r_path, s, area, w, h);
}
#endif
/*----------------------------------------------------------------------------*
 *                             Shape interface                                *
 *----------------------------------------------------------------------------*/
static Eina_Bool _shape_path_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape_Path *thiz;
	Enesim_Renderer_Shape_Path_Class *klass;

	thiz = ENESIM_RENDERER_SHAPE_PATH(r);
	klass = ENESIM_RENDERER_SHAPE_PATH_CLASS_GET(r);
	if (enesim_renderer_has_changed(thiz->r_path))
		return EINA_TRUE;
	if (klass->has_changed)
		return klass->has_changed(r);
	return EINA_FALSE;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static Eina_Bool _shape_path_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *draw, Enesim_Log **l)
{
	if (!_shape_path_setup(r, s, l))
		return EINA_FALSE;
	*draw = _shape_path_path_span;
	return EINA_TRUE;
}

static void _shape_path_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	_shape_path_cleanup(r, s);
}

static void _shape_path_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_AFFINE |
		ENESIM_RENDERER_FEATURE_ARGB8888;
}

static void _shape_path_sw_hints(Enesim_Renderer *r,
		Enesim_Renderer_Sw_Hint *hints)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = ENESIM_RENDERER_SHAPE_PATH(r);
	enesim_renderer_sw_hints_get(thiz->r_path, hints);
}

#if BUILD_OPENGL
static Eina_Bool _shape_path_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l)
{
	if (!_shape_path_setup(r, s, l))
		return EINA_FALSE;

	*draw = _shape_path_opengl_draw;
	return EINA_TRUE;
}

static void _shape_path_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	_shape_path_cleanup(r, s);
}
#endif
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_ABSTRACT_BOILERPLATE(ENESIM_RENDERER_SHAPE_DESCRIPTOR,
		Enesim_Renderer_Shape_Path,
		Enesim_Renderer_Shape_Path_Class,
		enesim_renderer_shape_path);

static void _enesim_renderer_shape_path_class_init(void *k)
{
	Enesim_Renderer_Shape_Class *shape_klass;
	Enesim_Renderer_Class *klass;

	shape_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	shape_klass->has_changed = _shape_path_has_changed;
	shape_klass->features_get = enesim_renderer_shape_path_shape_features_get_default;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->bounds_get = enesim_renderer_shape_path_bounds_get_default;
	klass->features_get = _shape_path_features_get;
	klass->sw_hints_get = _shape_path_sw_hints;
	/* override the renderer setup or we will do the setup of the
	 * fill/stroke renderers twice
	 */
	klass->sw_setup = _shape_path_sw_setup;
	klass->sw_cleanup = _shape_path_sw_cleanup;
#if BUILD_OPENGL
	klass->opengl_setup = _shape_path_opengl_setup;
	klass->opengl_cleanup = _shape_path_opengl_cleanup;
#endif
}

static void _enesim_renderer_shape_path_instance_init(void *o)
{
	Enesim_Renderer_Shape_Path *thiz;
	Enesim_Renderer *r;
	Enesim_Path *path;

	thiz = ENESIM_RENDERER_SHAPE_PATH(o);
	r = enesim_renderer_path_new();
	enesim_renderer_path_path_get(r, &thiz->path);
	thiz->r_path = r;
}

static void _enesim_renderer_shape_path_instance_deinit(void *o)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = ENESIM_RENDERER_SHAPE_PATH(o);
	enesim_renderer_unref(thiz->r_path);
	enesim_path_unref(thiz->path);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_renderer_shape_path_shape_features_get_default(Enesim_Renderer *r,
		Enesim_Renderer_Shape_Feature *features)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = ENESIM_RENDERER_SHAPE_PATH(r);
	enesim_renderer_shape_features_get(thiz->r_path, features);
}

void enesim_renderer_shape_path_bounds_get_default(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Shape_Path *thiz;
	Enesim_Renderer_Shape_Path_Class *klass;

	thiz = ENESIM_RENDERER_SHAPE_PATH(r);
	_shape_path_propagate(r);
	enesim_renderer_bounds(thiz->r_path, bounds);
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

