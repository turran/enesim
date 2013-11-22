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

#include "enesim_renderer_path_enesim_private.h"

/* Some useful macros for debugging */
/* In case this is set, if the path has a stroke, only the stroke will be
 * rendered
 */
#define WIREFRAME 0
#define DUMP 0
/**
 * TODO
 * - Use the threshold on the curve state
 */
/*
 * Some formulas found on the research process:
 * l1 = Ax + By + C
 * l2 || l1 => l2 = Ax + By + C'
 * C' = C + d * hypot(A, B);
 */

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static void _path_span(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Path_Enesim *thiz;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	enesim_renderer_sw_draw(thiz->bifigure, x, y, len, ddata);
}

static Eina_Bool _path_needs_generate(Enesim_Renderer_Path_Enesim *thiz,
		const Enesim_Matrix *cgm,
		double stroke_width,
		Enesim_Renderer_Shape_Stroke_Join join,
		Enesim_Renderer_Shape_Stroke_Cap cap,
		Enesim_List *dashes)
{
	Eina_Bool ret = EINA_FALSE;

	/* in case some command has been changed be sure to cleanup
	 * the path and keep the changed flag of the path to true
	 */
	if (thiz->path && thiz->path->changed)
	{
		thiz->path_changed = EINA_TRUE;
		thiz->path->changed = EINA_FALSE;
		thiz->generated = EINA_FALSE;
	}

	if (enesim_list_has_changed(dashes))
	{
		thiz->dashes_changed = EINA_TRUE;
		thiz->generated = EINA_FALSE;
		enesim_list_clear_changed(dashes);
	}

	/* a new path has been set or some command has been added */
	if ((thiz->changed || thiz->path_changed || thiz->dashes_changed) && !thiz->generated)
		ret = EINA_TRUE;
	/* the stroke join is different */
	else if (thiz->last_join != join)
		ret = EINA_TRUE;
	/* the stroke cap is different */
	else if (thiz->last_cap != cap)
		ret = EINA_TRUE;
	else if (thiz->last_stroke_weight != stroke_width)
		ret = EINA_TRUE;
	/* the geometry transformation is different */
	else if (!enesim_matrix_is_equal(cgm, &thiz->last_matrix))
		ret = EINA_TRUE;

	return ret;
}

static void _path_generate_figures(Enesim_Renderer_Path_Enesim *thiz,
		Enesim_Renderer_Shape_Draw_Mode dm,
		double sw,
		const Enesim_Matrix *transformation,
		double sx,
		double sy,
		Enesim_Renderer_Shape_Stroke_Join join,
		Enesim_Renderer_Shape_Stroke_Cap cap,
		Eina_List *dashes)
{
	Enesim_Path_Generator *generator;
	Enesim_Figure *stroke_figure = NULL;

	if (thiz->fill_figure)
		enesim_figure_clear(thiz->fill_figure);
	else
		thiz->fill_figure = enesim_figure_new();

	if (thiz->stroke_figure)
		enesim_figure_clear(thiz->stroke_figure);
	else
		thiz->stroke_figure = enesim_figure_new();

	/* decide what generator to use */
	/* for a stroke smaller than 1px we will use the basic
	 * rasterizer directly, so we dont need to generate the
	 * stroke path
	 */
	if ((dm & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE) && (dashes || sw > 1.0))
	{
		if (!dashes)
			generator = thiz->stroke_path;
		else
			generator = thiz->dashed_path;
		stroke_figure = thiz->stroke_figure;
	}
	else
	{
		generator = thiz->strokeless_path;

	}
	enesim_path_generator_figure_set(generator, thiz->fill_figure);
	enesim_path_generator_stroke_figure_set(generator, thiz->stroke_figure);
	enesim_path_generator_stroke_cap_set(generator, cap);
	enesim_path_generator_stroke_join_set(generator, join);
	enesim_path_generator_stroke_weight_set(generator, sw);
	enesim_path_generator_stroke_dash_set(generator, dashes);
	enesim_path_generator_scale_set(generator, sx, sy);
	enesim_path_generator_transformation_set(generator, transformation);
	if (thiz->path)
	{
		enesim_path_generator_generate(generator, thiz->path->commands);
	}

	/* set the fill figure on the bifigure as its under polys */
	enesim_rasterizer_figure_set(thiz->bifigure, thiz->fill_figure);
#if WIREFRAME
	enesim_rasterizer_figure_set(thiz->bifigure, stroke_figure);
#else
	/* set the stroke figure on the bifigure as its under polys */
	enesim_rasterizer_bifigure_over_figure_set(thiz->bifigure, stroke_figure);
#endif

#if DUMP
	if (stroke_figure)
	{
		printf("stroke figure\n");
		enesim_figure_dump(stroke_figure);
	}
	printf("fill figure\n");
	enesim_figure_dump(thiz->fill_figure);
#endif

	thiz->generated = EINA_TRUE;
	/* update the last values */
	thiz->last_join = join;
	thiz->last_cap = cap;
	thiz->last_matrix = *transformation;
	thiz->last_stroke_weight = sw;
#if BUILD_OPENGL
	thiz->gl.fill.needs_tesselate = EINA_TRUE;
	thiz->gl.stroke.needs_tesselate = EINA_TRUE;
#endif
}


static void _path_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Path_Enesim *thiz;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	enesim_renderer_cleanup(thiz->bifigure, s);
	thiz->changed = EINA_FALSE;
	thiz->path_changed = EINA_FALSE;
}

/*----------------------------------------------------------------------------*
 *                          Path Abstract interface                           *
 *----------------------------------------------------------------------------*/
static void _path_path_set(Enesim_Renderer *r, Enesim_Path *path)
{
	Enesim_Renderer_Path_Enesim *thiz;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	if (thiz->path != path)
	{
		if (thiz->path) enesim_path_unref(thiz->path);
		thiz->path = path;
		thiz->changed = EINA_TRUE;
		thiz->generated = EINA_FALSE;
	}
	else
	{
		enesim_path_unref(path);
	}
}
/*----------------------------------------------------------------------------*
 *                             Shape interface                                *
 *----------------------------------------------------------------------------*/
static void _path_shape_features_get(Enesim_Renderer *r EINA_UNUSED, Enesim_Renderer_Shape_Feature *features)
{
	*features = ENESIM_RENDERER_SHAPE_FEATURE_FILL_RENDERER | ENESIM_RENDERER_SHAPE_FEATURE_STROKE_RENDERER;
}

static Eina_Bool _path_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Enesim *thiz;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	if (thiz->changed || thiz->path_changed || (thiz->path && thiz->path->changed) || (thiz->dashes_changed))
		return EINA_TRUE;
	else
		return EINA_FALSE;
}

static Eina_Bool _path_sw_setup(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, Enesim_Renderer_Sw_Fill *draw, Enesim_Log **l)
{
	Enesim_Renderer_Path_Enesim *thiz;
	Enesim_Renderer_Shape *bifigure_shape;
	const Enesim_Renderer_State *cs;
	const Enesim_Renderer_Shape_State *css;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	cs = enesim_renderer_state_get(r);
	css = enesim_renderer_shape_state_get(r);

	/* generate the list of points/polygons */
	if (_path_needs_generate(thiz, &cs->current.transformation,
			css->current.stroke.weight,
			css->current.stroke.join, css->current.stroke.cap,
			css->dashes))
	{
		_path_generate_figures(thiz, css->current.draw_mode, css->current.stroke.weight,
				&cs->current.transformation, 1, 1,
				css->current.stroke.join, css->current.stroke.cap, css->dashes->l);
	}

#if WIREFRAME
	enesim_renderer_shape_draw_mode_set(thiz->bifigure, ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE);
#else
	enesim_renderer_shape_draw_mode_set(thiz->bifigure, css->current.draw_mode);
#endif
	enesim_renderer_shape_stroke_weight_set(thiz->bifigure, css->current.stroke.weight);
	enesim_renderer_shape_stroke_color_set(thiz->bifigure, css->current.stroke.color);
	enesim_renderer_shape_stroke_renderer_set(thiz->bifigure, enesim_renderer_ref(css->current.stroke.r));
	enesim_renderer_shape_fill_color_set(thiz->bifigure, css->current.fill.color);
	enesim_renderer_shape_fill_renderer_set(thiz->bifigure, enesim_renderer_ref(css->current.fill.r));
	enesim_renderer_shape_fill_rule_set(thiz->bifigure, css->current.fill.rule);

	enesim_renderer_color_set(thiz->bifigure, cs->current.color);
	enesim_renderer_origin_set(thiz->bifigure, cs->current.ox, cs->current.oy);
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

	*draw = _path_span;

	return EINA_TRUE;
}

static void _path_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	_path_state_cleanup(r, s);
}

#if BUILD_OPENGL
static Eina_Bool _path_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Path_Enesim *thiz;
	const Enesim_Renderer_State *cs;
	const Enesim_Renderer_Shape_State *css;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	cs = enesim_renderer_state_get(r);
	css = enesim_renderer_shape_state_get(r);

	/* generate the figures */
	if (_path_needs_generate(thiz, &cs->current.transformation,
			css->current.stroke.weight,
			css->current.stroke.join, css->current.stroke.cap,
			css->dashes))
	{
		_path_generate_figures(thiz, css->current.draw_mode, css->current.stroke.weight,
				&cs->current.transformation, 1, 1,
				css->current.stroke.join, css->current.stroke.cap, css->dashes->l);
	}

	/* call the implementation setup */
	if (!enesim_renderer_path_enesim_gl_tesselator_setup(r, draw, l))
		return EINA_FALSE;

	return EINA_TRUE;
}

static void _path_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	_path_state_cleanup(r, s);
}
#endif


/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _path_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "enesim_path";
}

static void _path_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static void _path_sw_hints(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Rop rop EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	/* we always use another renderer for drawing so
	 * mark every hint
	 */
	*hints = ENESIM_RENDERER_HINT_ROP | ENESIM_RENDERER_HINT_MASK | ENESIM_RENDERER_HINT_COLORIZE;
}

static void _path_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Path_Enesim *thiz;
	const Enesim_Renderer_State *cs;
	const Enesim_Renderer_Shape_State *css;
	double xmin;
	double ymin;
	double xmax;
	double ymax;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	cs = enesim_renderer_state_get(r);
	css = enesim_renderer_shape_state_get(r);

	if (_path_needs_generate(thiz, &cs->current.transformation,
			css->current.stroke.weight,
			css->current.stroke.join, css->current.stroke.cap,
			css->dashes))
	{
		_path_generate_figures(thiz, css->current.draw_mode, css->current.stroke.weight,
				&cs->current.transformation, 1, 1,
				css->current.stroke.join, css->current.stroke.cap, css->dashes->l);
	}

	if (!thiz->fill_figure)
	{
		bounds->x = 0;
		bounds->y = 0;
		bounds->w = 0;
		bounds->h = 0;
		return;
	}

	if (css->current.draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
	{
		if (css->current.stroke.weight > 1.0)
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

#if BUILD_OPENGL
static Eina_Bool _path_opengl_initialize(Enesim_Renderer *r EINA_UNUSED,
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***programs)
{
	return enesim_renderer_path_enesim_gl_tesselator_initialize(num_programs, programs);
}
#endif
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_PATH_ABSTRACT_DESCRIPTOR,
		Enesim_Renderer_Path_Enesim, Enesim_Renderer_Path_Enesim_Class,
		enesim_renderer_path_enesim);

static void _enesim_renderer_path_enesim_class_init(void *k)
{
	Enesim_Renderer_Path_Abstract_Class *klass;
	Enesim_Renderer_Shape_Class *s_klass;
	Enesim_Renderer_Class *r_klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _path_name;
	r_klass->bounds_get = _path_bounds_get;
	r_klass->features_get = _path_features_get;
	r_klass->sw_hints_get = _path_sw_hints;
#if BUILD_OPENGL
	r_klass->opengl_initialize = _path_opengl_initialize;
#endif

	s_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	s_klass->features_get = _path_shape_features_get;
	s_klass->sw_setup = _path_sw_setup;
	s_klass->sw_cleanup = _path_sw_cleanup;
	s_klass->has_changed = _path_has_changed;
#if BUILD_OPENGL
	s_klass->opengl_setup = _path_opengl_setup;
	s_klass->opengl_cleanup = _path_opengl_cleanup;
#endif

	klass = ENESIM_RENDERER_PATH_ABSTRACT_CLASS(k);
	klass->path_set = _path_path_set;
}

static void _enesim_renderer_path_enesim_instance_init(void *o)
{
	Enesim_Renderer_Path_Enesim *thiz;
	Enesim_Renderer *r;

	thiz = ENESIM_RENDERER_PATH_ENESIM(o);

	r = enesim_rasterizer_bifigure_new();
	thiz->bifigure = r;

	/* create the different path implementations */
	thiz->stroke_path = enesim_path_generator_stroke_dashless_new();
	thiz->strokeless_path = enesim_path_generator_strokeless_new();
	thiz->dashed_path = enesim_path_generator_dashed_new();
	/* FIXME for now */
	enesim_renderer_shape_stroke_join_set(ENESIM_RENDERER(o), ENESIM_RENDERER_SHAPE_STROKE_JOIN_ROUND);
}

static void _enesim_renderer_path_enesim_instance_deinit(void *o)
{
	Enesim_Renderer_Path_Enesim *thiz;

	/* TODO: not finished */
	thiz = ENESIM_RENDERER_PATH_ENESIM(o);
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
	if (thiz->bifigure)
		enesim_renderer_unref(thiz->bifigure);
	if (thiz->path)
		enesim_path_unref(thiz->path);
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

