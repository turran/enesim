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
#define DUMP 0
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
	thiz->stroke_path = enesim_path_generator_stroke_dashless_new();
	thiz->strokeless_path = enesim_path_generator_strokeless_new();
	thiz->dashed_path = enesim_path_generator_dashed_new();
}

static void _enesim_renderer_path_abstract_instance_deinit(void *o)
{
	Enesim_Renderer_Path_Abstract *thiz;

	thiz = ENESIM_RENDERER_PATH_ABSTRACT(o);
	if (thiz->stroke_figure)
		enesim_figure_delete(thiz->stroke_figure);
	if (thiz->fill_figure)
		enesim_figure_delete(thiz->fill_figure);
	if (thiz->dashed_path)
		enesim_path_generator_free(thiz->dashed_path);
	if (thiz->strokeless_path)
		enesim_path_generator_free(thiz->strokeless_path);
	if (thiz->stroke_path)
		enesim_path_generator_free(thiz->stroke_path);
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
	Enesim_Renderer_Shape_Draw_Mode dm;
	Enesim_Matrix transformation;
	Enesim_Renderer_Shape_Stroke_Join join;
	Enesim_Renderer_Shape_Stroke_Cap cap;
	Enesim_Path_Generator *generator;
	Enesim_List *dashes;
	Eina_List *dashes_l;
	Eina_Bool stroke_scalable;
	double stroke_weight;
	double swx;
	double swy;

	thiz = ENESIM_RENDERER_PATH_ABSTRACT(r);
	if (thiz->fill_figure)
		enesim_figure_clear(thiz->fill_figure);
	else
		thiz->fill_figure = enesim_figure_new();

	if (thiz->stroke_figure)
		enesim_figure_clear(thiz->stroke_figure);
	else
		thiz->stroke_figure = enesim_figure_new();


	dm = enesim_renderer_shape_draw_mode_get(r);
	dashes = enesim_renderer_shape_dashes_get(r);
	dashes_l = dashes->l;
	enesim_list_unref(dashes);
	enesim_renderer_shape_stroke_weight_setup(r, &swx, &swy);

	/* decide what generator to use */
	/* for a stroke smaller than 1px we will use the basic
	 * rasterizer directly, so we dont need to generate the
	 * stroke path
	 */
	if ((dm & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE) &&
			(dashes_l || swx > 1.0 || swy > 1.0))
	{
		if (!dashes_l)
			generator = thiz->stroke_path;
		else
			generator = thiz->dashed_path;
		thiz->stroke_figure_used = EINA_TRUE;
	}
	else
	{
		generator = thiz->strokeless_path;
		thiz->stroke_figure_used = EINA_FALSE;

	}

	join = enesim_renderer_shape_stroke_join_get(r);
	cap = enesim_renderer_shape_stroke_cap_get(r);
	stroke_weight = enesim_renderer_shape_stroke_weight_get(r);
	stroke_scalable = enesim_renderer_shape_stroke_scalable_get(r);
	enesim_renderer_transformation_get(r, &transformation);

	enesim_path_generator_figure_set(generator, thiz->fill_figure);
	enesim_path_generator_stroke_figure_set(generator, thiz->stroke_figure);
	enesim_path_generator_stroke_cap_set(generator, cap);
	enesim_path_generator_stroke_join_set(generator, join);
	enesim_path_generator_stroke_weight_set(generator, stroke_weight);
	enesim_path_generator_stroke_scalable_set(generator, stroke_scalable);
	enesim_path_generator_stroke_dash_set(generator, dashes_l);
	enesim_path_generator_scale_set(generator, 1, 1);
	enesim_path_generator_transformation_set(generator, &transformation);
	if (thiz->path)
	{
		enesim_path_generator_generate(generator, thiz->path->commands);
	}

#if DUMP
	if (thiz->stroke_figure_used)
	{
		printf("stroke figure\n");
		enesim_figure_dump(thiz->stroke_figure);
	}
	printf("fill figure\n");
	enesim_figure_dump(thiz->fill_figure);
#endif

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
