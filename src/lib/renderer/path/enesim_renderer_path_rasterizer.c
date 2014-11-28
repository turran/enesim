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
#include "enesim_buffer_private.h"
#include "enesim_surface_private.h"
#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_path_abstract_private.h"
#include "enesim_rasterizer_private.h"

/**
 * TODO
 * - Use the threshold on the curve state
 */

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/* Some useful macros for debugging */
/* In case this is set, if the path has a stroke, only the stroke will be
 * rendered
 */
#define WIREFRAME 0
#define DUMP 0

#define ENESIM_RENDERER_PATH_ENESIM(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Path_Enesim,					\
		enesim_renderer_path_enesim_descriptor_get())

typedef struct _Enesim_Renderer_Path_Enesim
{
	Enesim_Renderer_Path_Abstract parent;
	/* properties */
	/* private */
	Enesim_Figure *fill_figure;
	Enesim_Figure *stroke_figure;
	Enesim_Renderer *bifigure;
} Enesim_Renderer_Path_Enesim;

typedef struct _Enesim_Renderer_Path_Enesim_Class
{
	Enesim_Renderer_Path_Abstract_Class parent;
} Enesim_Renderer_Path_Enesim_Class;

static void _enesim_renderer_path_rasterizer_span(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Path_Enesim *thiz;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	enesim_renderer_sw_draw(thiz->bifigure, x, y, len, ddata);
}

static void _enesim_renderer_path_rasterizer_generate_figures(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Enesim *thiz;
	Enesim_Renderer_Path_Abstract *pa;
	Enesim_Renderer_Shape_Draw_Mode dm;
	Enesim_Path_Generator *generator;
	Enesim_List *dashes;
	Enesim_Matrix transformation;
	Enesim_Renderer_Shape_Stroke_Join join;
	Enesim_Renderer_Shape_Stroke_Cap cap;
	Eina_List *dashes_l;
	Eina_Bool stroke_figure_used = EINA_FALSE;
	Eina_Bool stroke_scalable;
	double stroke_weight;
	double swx;
	double swy;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);

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

	join = enesim_renderer_shape_stroke_join_get(r);
	cap = enesim_renderer_shape_stroke_cap_get(r);
	stroke_weight = enesim_renderer_shape_stroke_weight_get(r);
	stroke_scalable = enesim_renderer_shape_stroke_scalable_get(r);
	enesim_renderer_transformation_get(r, &transformation);
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
			generator = enesim_path_generator_stroke_dashless_new();
		else
			generator = enesim_path_generator_dashed_new();
		stroke_figure_used = EINA_TRUE;
	}
	else
	{
		generator = enesim_path_generator_strokeless_new();
	}

	enesim_path_generator_figure_set(generator, thiz->fill_figure);
	enesim_path_generator_stroke_figure_set(generator, thiz->stroke_figure);
	enesim_path_generator_stroke_cap_set(generator, cap);
	enesim_path_generator_stroke_join_set(generator, join);
	enesim_path_generator_stroke_weight_set(generator, stroke_weight);
	enesim_path_generator_stroke_scalable_set(generator, stroke_scalable);
	enesim_path_generator_stroke_dash_set(generator, dashes_l);
	enesim_path_generator_scale_set(generator, 1, 1);
	enesim_path_generator_transformation_set(generator, &transformation);
	/* Now generate */
	pa = ENESIM_RENDERER_PATH_ABSTRACT(r);
	enesim_path_generator_generate(generator, pa->path->commands);
	enesim_list_unref(dashes);
	/* Remove the figure generators */
	enesim_path_generator_free(generator);

#if DUMP
	if (stroke_figure_used)
	{
		printf("stroke figure\n");
		enesim_figure_dump(thiz->stroke_figure);
	}
	printf("fill figure\n");
	enesim_figure_dump(thiz->fill_figure);
#endif

#if WIREFRAME
	if (stroke_figure_used)
		enesim_rasterizer_figure_set(thiz->bifigure, thiz->stroke_figure);
	else
		enesim_rasterizer_figure_set(thiz->bifigure, thiz->fill_figure);
#else
	/* set the fill figure on the bifigure as its under polys */
	enesim_rasterizer_figure_set(thiz->bifigure, thiz->fill_figure);
	/* set the stroke figure on the bifigure as its over polys */
	enesim_rasterizer_bifigure_over_figure_set(thiz->bifigure, stroke_figure_used ? thiz->stroke_figure : NULL);
#endif
	/* Finally mark as we have already generated the figure */
	enesim_renderer_path_abstract_generate(r);
}

/*----------------------------------------------------------------------------*
 *                          Path Abstract interface                           *
 *----------------------------------------------------------------------------*/
/*----------------------------------------------------------------------------*
 *                             Shape interface                                *
 *----------------------------------------------------------------------------*/
static void _enesim_renderer_path_rasterizer_shape_features_get(Enesim_Renderer *r EINA_UNUSED, Enesim_Renderer_Shape_Feature *features)
{
	*features = ENESIM_RENDERER_SHAPE_FEATURE_FILL_RENDERER | ENESIM_RENDERER_SHAPE_FEATURE_STROKE_RENDERER;
}

static Eina_Bool _enesim_renderer_path_rasterizer_has_changed(Enesim_Renderer *r EINA_UNUSED)
{
#if 0
	Enesim_Renderer_Path_Enesim *thiz;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	if (thiz->changed || thiz->path_changed || (thiz->path && thiz->path->changed) || (thiz->dashes_changed))
		return EINA_TRUE;
	else
#endif
	return EINA_FALSE;
}

static Eina_Bool _enesim_renderer_path_rasterizer_sw_setup(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, Enesim_Renderer_Sw_Fill *draw, Enesim_Log **l)
{
	Enesim_Renderer_Path_Enesim *thiz;
	Enesim_Renderer_Shape *bifigure_shape;
	Enesim_Renderer *m;
	const Enesim_Renderer_State *cs;
	const Enesim_Renderer_Shape_State *css;
	double swx, swy;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	cs = enesim_renderer_state_get(r);
	css = enesim_renderer_shape_state_get(r);

	/* generate the list of points/polygons */
	if (enesim_renderer_path_abstract_needs_generate(r))
	{
		_enesim_renderer_path_rasterizer_generate_figures(r);
	}
#if WIREFRAME
	enesim_renderer_shape_draw_mode_set(thiz->bifigure, ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE);
#else
	enesim_renderer_shape_draw_mode_set(thiz->bifigure, css->current.draw_mode);
#endif
	/* FIXME the logic on the bifigure depends on the stroke weight and the transformation
	 * matrix. We always pass an identity transformation matrix so the calculation of the
	 * real stroke (swx, swy) is always the passed in value. We better pass the max stroke
	 * weight to avoid such case
	 */
	enesim_renderer_shape_stroke_weight_setup(r, &swx, &swy);
	enesim_renderer_shape_stroke_weight_set(thiz->bifigure, swx > swy ? swx * 2: swy * 2);
	enesim_renderer_shape_stroke_color_set(thiz->bifigure, css->current.stroke.color);
	enesim_renderer_shape_stroke_renderer_set(thiz->bifigure, enesim_renderer_ref(css->current.stroke.r));
	enesim_renderer_shape_fill_color_set(thiz->bifigure, css->current.fill.color);
	enesim_renderer_shape_fill_renderer_set(thiz->bifigure, enesim_renderer_ref(css->current.fill.r));
	enesim_renderer_shape_fill_rule_set(thiz->bifigure, css->current.fill.rule);

	/* propagate the common renderer properties manually, because calling
	 * enesim_renderer_propagate() will also set the transformation which
	 * is something we dont want
	 */
	m = enesim_renderer_mask_get(r);
	enesim_renderer_color_set(thiz->bifigure, cs->current.color);
	enesim_renderer_origin_set(thiz->bifigure, cs->current.ox, cs->current.oy);
	enesim_renderer_mask_set(thiz->bifigure, m);
	/* pass the dashes */
	bifigure_shape = ENESIM_RENDERER_SHAPE(thiz->bifigure);
	if (bifigure_shape->state.dashes)
	{
		enesim_list_unref(bifigure_shape->state.dashes);
	}
	bifigure_shape->state.dashes = enesim_list_ref(css->dashes);

	/* finally do the setup */
	if (!enesim_renderer_setup(thiz->bifigure, s, rop, l))
	{
		return EINA_FALSE;
	}

	*draw = _enesim_renderer_path_rasterizer_span;

	return EINA_TRUE;
}

static void _enesim_renderer_path_rasterizer_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	enesim_renderer_path_abstract_cleanup(r);
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _enesim_renderer_path_rasterizer_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "enesim_path";
}

static void _enesim_renderer_path_rasterizer_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_BACKEND_SOFTWARE |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static void _enesim_renderer_path_rasterizer_sw_hints(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Rop rop EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	/* we always use another renderer for drawing so
	 * mark every hint
	 */
	*hints = ENESIM_RENDERER_SW_HINT_ROP | ENESIM_RENDERER_SW_HINT_MASK | ENESIM_RENDERER_SW_HINT_COLORIZE;
}

static void _enesim_renderer_path_rasterizer_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Path_Enesim *thiz;
	const Enesim_Renderer_State *cs;
	const Enesim_Renderer_Shape_State *css;
	double xmin;
	double ymin;
	double xmax;
	double ymax;
	double swx, swy;

	cs = enesim_renderer_state_get(r);
	css = enesim_renderer_shape_state_get(r);

	if (enesim_renderer_path_abstract_needs_generate(r))
	{
		_enesim_renderer_path_rasterizer_generate_figures(r);
	}

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	if (!thiz->fill_figure)
	{
		bounds->x = 0;
		bounds->y = 0;
		bounds->w = 0;
		bounds->h = 0;
		return;
	}

	enesim_renderer_shape_stroke_weight_setup(r, &swx, &swy);
	if (css->current.draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
	{
		if (swx > 1.0 || swy > 1.0)
		{
			if (!enesim_figure_bounds(thiz->stroke_figure, &xmin, &ymin, &xmax, &ymax))
				goto failed;
		}
		else
		{
			if (!enesim_figure_bounds(thiz->fill_figure, &xmin, &ymin, &xmax, &ymax))
				goto failed;
			/* add the stroke offset, even if the basic figure has its own bounds
			 * we need to define the correct one here
			 */
			xmin -= 0.5;
			ymin -= 0.5;
			xmax += 0.5;
			ymax += 0.5;
		}
	}
	else
	{
		if (!enesim_figure_bounds(thiz->fill_figure, &xmin, &ymin, &xmax, &ymax))
			goto failed;
	}

	bounds->x = xmin;
	bounds->w = xmax - xmin;
	bounds->y = ymin;
	bounds->h = ymax - ymin;

	/* translate by the origin */
	bounds->x += cs->current.ox;
	bounds->y += cs->current.oy;
	return;

failed:
	bounds->x = 0;
	bounds->y = 0;
	bounds->w = 0;
	bounds->h = 0;
}

/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_PATH_ABSTRACT_DESCRIPTOR,
		Enesim_Renderer_Path_Enesim, Enesim_Renderer_Path_Enesim_Class,
		enesim_renderer_path_enesim);

static void _enesim_renderer_path_enesim_class_init(void *k)
{
	Enesim_Renderer_Shape_Class *s_klass;
	Enesim_Renderer_Class *r_klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _enesim_renderer_path_rasterizer_name;
	r_klass->bounds_get = _enesim_renderer_path_rasterizer_bounds_get;
	r_klass->features_get = _enesim_renderer_path_rasterizer_features_get;
	r_klass->sw_hints_get = _enesim_renderer_path_rasterizer_sw_hints;

	s_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	s_klass->features_get = _enesim_renderer_path_rasterizer_shape_features_get;
	s_klass->sw_setup = _enesim_renderer_path_rasterizer_sw_setup;
	s_klass->sw_cleanup = _enesim_renderer_path_rasterizer_sw_cleanup;
	s_klass->has_changed = _enesim_renderer_path_rasterizer_has_changed;
}

static void _enesim_renderer_path_enesim_instance_init(void *o)
{
	Enesim_Renderer_Path_Enesim *thiz;
	Enesim_Renderer *r;

	thiz = ENESIM_RENDERER_PATH_ENESIM(o);

	r = enesim_rasterizer_bifigure_new();
	thiz->bifigure = r;

	/* FIXME for now */
	enesim_renderer_shape_stroke_join_set(ENESIM_RENDERER(o), ENESIM_RENDERER_SHAPE_STROKE_JOIN_ROUND);
}

static void _enesim_renderer_path_enesim_instance_deinit(void *o)
{
	Enesim_Renderer_Path_Enesim *thiz;

	/* TODO: not finished */
	thiz = ENESIM_RENDERER_PATH_ENESIM(o);
	if (thiz->bifigure)
		enesim_renderer_unref(thiz->bifigure);
	if (thiz->stroke_figure)
		enesim_figure_delete(thiz->stroke_figure);
	if (thiz->fill_figure)
		enesim_figure_delete(thiz->fill_figure);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_path_enesim_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_path_enesim);
	return r;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

