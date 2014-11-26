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
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_path.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_list_private.h"
#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_path_abstract_private.h"

/**
 * TODO
 * Ok this file needs a refactoring, basically it was the base class for every
 * path renderer on enesim.
 * For the generation of paths -> figures we have
 * rasterizer : used
 *   basic    : special case, stroke < 1
 *   kiia     : normal
 * cairo      : not-used
 * tesselator : used
 * nv         : not-used
 * loop-blinn : WIP but wont be used either
 *
 * - Make the generate function a class function pointer, this way we avoid
 *   the need to have changed/generated/has_changed on the implementations
 *   and whenever the renderer needs to do a setup, it just calls the implementation
 *   generate based on its own state
 * - The setup function must be split on two too, one for the real setup of
 *   fill/stroke renderers, etc, then do the generation and finally call the
 *   implementation setup
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_ABSTRACT_BOILERPLATE(ENESIM_RENDERER_SHAPE_DESCRIPTOR,
		Enesim_Renderer_Path_Abstract,
		Enesim_Renderer_Path_Abstract_Class,
		enesim_renderer_path_abstract);

static void _enesim_renderer_path_abstract_class_init(void *k EINA_UNUSED)
{
}

static void _enesim_renderer_path_abstract_instance_init(void *o EINA_UNUSED)
{
	Enesim_Renderer_Path_Abstract *thiz;

	thiz = ENESIM_RENDERER_PATH_ABSTRACT(o);
	/* create the different path implementations */
}

static void _enesim_renderer_path_abstract_instance_deinit(void *o)
{
	Enesim_Renderer_Path_Abstract *thiz;

	thiz = ENESIM_RENDERER_PATH_ABSTRACT(o);
	if (thiz->path)
		enesim_path_unref(thiz->path);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Eina_Bool enesim_renderer_path_abstract_needs_generate(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Abstract *thiz;
	Enesim_Renderer_Shape_Stroke_Join join;
	Enesim_Renderer_Shape_Stroke_Cap cap;
	Enesim_List *dashes;
	Enesim_Matrix cgm;
	double stroke_weight;

	thiz = ENESIM_RENDERER_PATH_ABSTRACT(r);
	/* in case some command has been changed be sure to cleanup
	 * the path and keep the changed flag of the path to true
	 */
	if (thiz->path && thiz->path->changed != thiz->last_path_change)
	{
		thiz->generated = EINA_FALSE;
	}

	/* check if the dashes have changed */
	dashes = enesim_renderer_shape_dashes_get(r);
	if (enesim_list_has_changed(dashes))
	{
		thiz->generated = EINA_FALSE;
		/* TODO use the same scheme as the path */
		enesim_list_clear_changed(dashes);
	}
	enesim_list_unref(dashes);

	/* is it generated already? */
	if (!thiz->generated)
		return EINA_TRUE;

	/* the stroke join is different */
	join = enesim_renderer_shape_stroke_join_get(r);
	if (thiz->last_join != join)
		return EINA_TRUE;

	/* the stroke cap is different */
	cap = enesim_renderer_shape_stroke_cap_get(r);
	if (thiz->last_cap != cap)
		return EINA_TRUE;

	stroke_weight = enesim_renderer_shape_stroke_weight_get(r);
	if (thiz->last_stroke_weight != stroke_weight)
		return EINA_TRUE;

	/* the geometry transformation is different */
	enesim_renderer_transformation_get(r, &cgm);
	if (!enesim_matrix_is_equal(&cgm, &thiz->last_matrix))
		return EINA_TRUE;

	return EINA_FALSE;
}

void enesim_renderer_path_abstract_generate(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Abstract *thiz;
	Enesim_Matrix transformation;
	Enesim_Renderer_Shape_Stroke_Join join;
	Enesim_Renderer_Shape_Stroke_Cap cap;
	Eina_Bool stroke_scalable;
	double stroke_weight;

	thiz = ENESIM_RENDERER_PATH_ABSTRACT(r);

	join = enesim_renderer_shape_stroke_join_get(r);
	cap = enesim_renderer_shape_stroke_cap_get(r);
	stroke_weight = enesim_renderer_shape_stroke_weight_get(r);
	stroke_scalable = enesim_renderer_shape_stroke_scalable_get(r);
	enesim_renderer_transformation_get(r, &transformation);

	thiz->generated = EINA_TRUE;
	thiz->last_path_change = thiz->path->changed;
	/* update the last values */
	thiz->last_join = join;
	thiz->last_cap = cap;
	thiz->last_matrix = transformation;
	thiz->last_stroke_weight = stroke_weight;
}

void enesim_renderer_path_abstract_path_set(Enesim_Renderer *r,
		Enesim_Path *path)
{
	Enesim_Renderer_Path_Abstract *thiz;
	Enesim_Renderer_Path_Abstract_Class *klass;

	thiz = ENESIM_RENDERER_PATH_ABSTRACT(r);
	if (thiz->path != path)
	{
		if (thiz->path)
			enesim_path_unref(thiz->path);
		thiz->path = enesim_path_ref(path);
		thiz->generated = EINA_FALSE;
		thiz->last_path_change = -1;
	}

	klass = ENESIM_RENDERER_PATH_ABSTRACT_CLASS_GET(r);
	if (klass->path_set)
		klass->path_set(r, path);
	else
		enesim_path_unref(path);
}

void enesim_renderer_path_abstract_cleanup(Enesim_Renderer *r EINA_UNUSED)
{
}

Eina_Bool enesim_renderer_path_abstract_is_available(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Abstract_Class *klass;

	klass = ENESIM_RENDERER_PATH_ABSTRACT_CLASS_GET(r);
	if (klass->is_available)
		return klass->is_available(r);
	else return EINA_TRUE;
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
