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
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_color_private.h"
#include "enesim_list_private.h"
#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
/* TODO
 * - whenever we need the damage area of a shape, we need to check if
 *   the renderer has a fill renderer, if so, we should call the damage
 *   on the fill renderer in case the shape hasnt changed
 * - when the bounds are requested, if we are using a fill renderer
 *   and our draw mode is fill we should intersect our bounds
 *   with the one of the fill renderer
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_LOG_DEFAULT enesim_log_renderer_shape

typedef struct _Enesim_Renderer_Shape_Damage_Data
{
	Eina_Rectangle *bounds;
	Enesim_Renderer_Damage real_cb;
	void *real_data;
} Enesim_Renderer_Shape_Damage_Data;

/* called from the optimized case of the damages to just clip the damages
 * to our own bounds
 */
static Eina_Bool _shape_damage_cb(Enesim_Renderer *r,
		const Eina_Rectangle *area, Eina_Bool past, void *data)
{
	Enesim_Renderer_Shape_Damage_Data *ddata = data;
	Eina_Rectangle new_area = *area;

	/* here we just intersect the damages with our bounds */
	if (eina_rectangle_intersection(&new_area, ddata->bounds))
		ddata->real_cb(r, &new_area, past, ddata->real_data);
	return EINA_TRUE;
}
/*----------------------------------------------------------------------------*
 *                     Internal state related functions                       *
 *----------------------------------------------------------------------------*/
static Eina_Bool _state_changed_basic(Enesim_Renderer_Shape_State *thiz,
		Enesim_Renderer_Shape_Feature features)
{
	Eina_Bool support_dashes = features & ENESIM_RENDERER_SHAPE_FEATURE_STROKE_DASH;
	if (!thiz->changed && (support_dashes && !enesim_list_has_changed(thiz->dashes)))
		return EINA_FALSE;
	/* optional properties */
	/* the stroke */
	/* color */
	if (thiz->current.stroke.color != thiz->past.stroke.color)
	{
		DBG("Stroke color changed");
		return EINA_TRUE;
	}
	/* weight */
	if (thiz->current.stroke.weight != thiz->past.stroke.weight)
	{
		DBG("Stroke weight changed");
		return EINA_TRUE;
	}
	/* location */
	if (features & ENESIM_RENDERER_SHAPE_FEATURE_STROKE_LOCATION)
	{
		if (thiz->current.stroke.location != thiz->past.stroke.location)
		{
			DBG("Stroke location changed");
			return EINA_TRUE;
		}
	}
	/* join */
	if (thiz->current.stroke.join != thiz->past.stroke.join)
	{
		DBG("Stroke join changed");
		return EINA_TRUE;
	}
	/* cap */
	if (thiz->current.stroke.cap != thiz->past.stroke.cap)
	{
		DBG("Stroke cap changed");
		return EINA_TRUE;
	}
	/* scalable */
	if (thiz->current.stroke.scalable != thiz->past.stroke.scalable)
	{
		DBG("Stroke scalable changed");
		return EINA_TRUE;
	}
	/* dashes */
	if (support_dashes)
	{
		/* we wont compare the stroke dashes, it has changed, then
		 * modify it directly
		 */
		if (enesim_list_has_changed(thiz->dashes))
		{
			DBG("Stroke dashes changed");
			return EINA_TRUE;
		}
	}

	/* fill */
	/* color */
	if (thiz->current.fill.color != thiz->past.fill.color)
	{
		DBG("Fill color changed");
		return EINA_TRUE;
	}
	/* renderer */
	if (thiz->current.fill.r != thiz->past.fill.r)
	{
		DBG("Fill renderer changed");
		return EINA_TRUE;
	}
	/* fill rule */
	if (thiz->current.fill.rule != thiz->past.fill.rule)
	{
		DBG("Fill rule changed");
		return EINA_TRUE;
	}
	/* draw mode */
	if (thiz->current.draw_mode != thiz->past.draw_mode)
	{
		DBG("Draw mode changed %d %d", thiz->current.draw_mode,
				thiz->past.draw_mode);
		return EINA_TRUE;
	}

	return EINA_FALSE;
}

static Eina_Bool _state_changed(Enesim_Renderer_Shape_State *thiz,
		Enesim_Renderer_Shape_Feature features)
{
	if (_state_changed_basic(thiz, features))
		return EINA_TRUE;

	/* renderer */
	if (features & ENESIM_RENDERER_SHAPE_FEATURE_STROKE_RENDERER)
	{
		if (thiz->current.stroke.r != thiz->past.stroke.r)
			return EINA_TRUE;
		if (thiz->current.stroke.r &&
			(thiz->current.draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE))
		{
			if (enesim_renderer_has_changed(thiz->current.stroke.r))
			{
				DBG("The stroke renderer %s has changed",
						enesim_renderer_name_get(thiz->current.stroke.r));
				return EINA_TRUE;
			}
		}
	}
	/* renderer */
	if (features & ENESIM_RENDERER_SHAPE_FEATURE_FILL_RENDERER)
	{
		if (thiz->current.fill.r != thiz->past.fill.r)
			return EINA_TRUE;
		/* we should first check if the fill renderer has changed */
		if (thiz->current.fill.r &&
				(thiz->current.draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL))
		{
			if (enesim_renderer_has_changed(thiz->current.fill.r))
			{
				DBG("The fill renderer %s has changed",
						enesim_renderer_name_get(thiz->current.fill.r));
				return EINA_TRUE;
			}
		}
	}
	return EINA_FALSE;
}

static void _state_clear(Enesim_Renderer_Shape_State *thiz)
{
	if (thiz->current.fill.r)
	{
		enesim_renderer_unref(thiz->current.fill.r);
		thiz->current.fill.r = NULL;
	}
	if (thiz->past.fill.r)
	{
		enesim_renderer_unref(thiz->past.fill.r);
		thiz->past.fill.r = NULL;
	}
	if (thiz->current.stroke.r)
	{
		enesim_renderer_unref(thiz->current.stroke.r);
		thiz->current.stroke.r = NULL;
	}
	if (thiz->past.stroke.r)
	{
		enesim_renderer_unref(thiz->past.stroke.r);
		thiz->past.stroke.r = NULL;
	}
	enesim_list_unref(thiz->dashes);
}

static void _state_init(Enesim_Renderer_Shape_State *thiz)
{
	/* set default properties */
	thiz->current.fill.color = thiz->past.fill.color = 0xffffffff;
	thiz->current.stroke.color = thiz->past.stroke.color = 0xffffffff;
	thiz->current.stroke.location = ENESIM_RENDERER_SHAPE_STROKE_LOCATION_CENTER;
	thiz->current.stroke.scalable = EINA_TRUE;
	thiz->current.draw_mode = ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL;
	thiz->dashes = enesim_list_new(free);
}

static void _state_commit(Enesim_Renderer_Shape_State *thiz)
{
	/* swap the states */
	thiz->past.draw_mode = thiz->current.draw_mode;
	/* the stroke */
	thiz->past.stroke.color = thiz->current.stroke.color;
	thiz->past.stroke.weight = thiz->current.stroke.weight;
	thiz->past.stroke.location = thiz->current.stroke.location;
	thiz->past.stroke.join = thiz->current.stroke.join;
	thiz->past.stroke.cap = thiz->current.stroke.cap;
	thiz->past.stroke.scalable = thiz->current.stroke.scalable;
	/* the fill */
	thiz->past.fill.color = thiz->current.fill.color;
	thiz->past.fill.rule = thiz->current.fill.rule;
	if (thiz->past.fill.r)
	{
		enesim_renderer_unref(thiz->past.fill.r);
		thiz->past.fill.r = NULL;
	}
	if (thiz->current.fill.r)
	{
		thiz->past.fill.r = enesim_renderer_ref(thiz->current.fill.r);
	}

	if (thiz->past.stroke.r)
	{
		enesim_renderer_unref(thiz->past.stroke.r);
		thiz->past.stroke.r = NULL;
	}
	if (thiz->current.stroke.r)
	{
		thiz->past.stroke.r = enesim_renderer_ref(thiz->current.stroke.r);
	}

	/* unmark the changes */
	thiz->changed = EINA_FALSE;
	enesim_list_clear_changed(thiz->dashes);
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static void _shape_cleanup(Enesim_Renderer *r, int features, Enesim_Surface *s)
{
	Enesim_Renderer_Shape *thiz;
	Enesim_Renderer_Shape_State *state;

	thiz = ENESIM_RENDERER_SHAPE(r);
	state = &thiz->state;

	if (features & ENESIM_RENDERER_SHAPE_FEATURE_FILL_RENDERER)
	{
		if (state->current.fill.r &&
				(state->current.draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL))
		{
			enesim_renderer_cleanup(state->current.fill.r, s);
		}
	}
	if (features & ENESIM_RENDERER_SHAPE_FEATURE_STROKE_RENDERER)
	{
		if (state->current.stroke.r &&
				(state->current.draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE))
		{
			enesim_renderer_cleanup(state->current.stroke.r, s);
		}
	}
	/* swap the states */
	_state_commit(state);
}

static Eina_Bool _shape_setup(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop EINA_UNUSED, Enesim_Log **l)
{
	Enesim_Renderer_Shape *thiz;
	Enesim_Renderer_Shape_State *state;
	Eina_Bool fill_renderer = EINA_FALSE;
	int features;

	thiz = ENESIM_RENDERER_SHAPE(r);
	state = &thiz->state;
	features = enesim_renderer_shape_shape_features_get(r);
	/* TODO check the sw hints so we can optimize the case
	 * where the fill and/or the stroke renderers support
	 * the rop operation and thus we can set the hints
	 * to rop too, otherwise use fill as the rop
	 */
	if (features & ENESIM_RENDERER_SHAPE_FEATURE_FILL_RENDERER)
	{
		if (state->current.fill.r &&
				(state->current.draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL))
		{
			fill_renderer = EINA_TRUE;
			if (!enesim_renderer_setup(state->current.fill.r, s, ENESIM_ROP_FILL, l))
			{
				ENESIM_RENDERER_LOG(r, l, "Fill renderer failed");
				return EINA_FALSE;
			}
		}
	}
	if (features & ENESIM_RENDERER_SHAPE_FEATURE_STROKE_RENDERER)
	{
		if (state->current.stroke.r &&
				(state->current.draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE))
		{
			if (!enesim_renderer_setup(state->current.stroke.r, s, ENESIM_ROP_FILL, l))
			{
				ENESIM_RENDERER_LOG(r, l, "Stroke renderer failed");
				/* clean up the fill renderer setup */
				if (fill_renderer)
				{
					enesim_renderer_cleanup(state->current.fill.r, s);

				}
				return EINA_FALSE;
			}
		}
	}
	return EINA_TRUE;
}

static Eina_Bool _enesim_renderer_shape_is_supported(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Shape *thiz;
	Enesim_Renderer_Shape_Class *klass;
	int features;

	features = enesim_renderer_shape_shape_features_get(r);
	thiz = ENESIM_RENDERER_SHAPE(r);
	if (thiz->state.current.fill.r &&
			!(features & ENESIM_RENDERER_SHAPE_FEATURE_FILL_RENDERER))
	{
		WRN("Fill renderer not supported");
		return EINA_FALSE;
	}
	if (thiz->state.current.stroke.r &&
			!(features & ENESIM_RENDERER_SHAPE_FEATURE_STROKE_RENDERER))
	{
		WRN("Stroke renderer not supported");
		return EINA_FALSE;
	}
	klass = ENESIM_RENDERER_SHAPE_CLASS_GET(r);
	if (klass->is_supported)
		return klass->is_supported(r, s);
	return EINA_TRUE;
}

static Eina_Bool _enesim_renderer_shape_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Renderer_Sw_Fill *fill,
		Enesim_Log **l)
{
	Enesim_Renderer_Shape_Class *klass;

	klass = ENESIM_RENDERER_SHAPE_CLASS_GET(r);
	if (!_shape_setup(r, s, rop, l)) return EINA_FALSE;
	if (!klass->sw_setup) return EINA_FALSE;

	if (!klass->sw_setup(r, s, rop, fill, l))
		return EINA_FALSE;

	return EINA_TRUE;
}

static void _enesim_renderer_shape_sw_cleanup(Enesim_Renderer *r,
		Enesim_Surface *s)
{
	Enesim_Renderer_Shape_Class *klass;
	int features;

	klass = ENESIM_RENDERER_SHAPE_CLASS_GET(r);
	features = enesim_renderer_shape_shape_features_get(r);
	_shape_cleanup(r, features, s);
	if (klass->sw_cleanup)
		klass->sw_cleanup(r, s);
}

static Eina_Bool _enesim_renderer_shape_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l)
{
	Enesim_Renderer_Shape_Class *klass;

	klass = ENESIM_RENDERER_SHAPE_CLASS_GET(r);
	if (!_shape_setup(r, s, rop, l)) return EINA_FALSE;
	if (!klass->opengl_setup) return EINA_FALSE;

	return klass->opengl_setup(r, s, rop, draw, l);
}

static void _enesim_renderer_shape_opengl_cleanup(Enesim_Renderer *r,
		Enesim_Surface *s)
{
	Enesim_Renderer_Shape_Class *klass;
	int features;

	klass = ENESIM_RENDERER_SHAPE_CLASS_GET(r);
	features = enesim_renderer_shape_shape_features_get(r);
	_shape_cleanup(r, features, s);
	if (klass->opengl_cleanup)
		klass->opengl_cleanup(r, s);
}

static Eina_Bool _enesim_renderer_shape_opencl_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		const char **program_name, const char **program_source,
		size_t *program_length,
		Enesim_Log **l)
{
	Enesim_Renderer_Shape_Class *klass;

	klass = ENESIM_RENDERER_SHAPE_CLASS_GET(r);
	if (!_shape_setup(r, s, rop, l)) return EINA_FALSE;
	if (!klass->opencl_setup) return EINA_FALSE;

	return klass->opencl_setup(r, s, rop, program_name, program_source,
			program_length, l);
}

static void _enesim_renderer_shape_opencl_cleanup(Enesim_Renderer *r,
		Enesim_Surface *s)
{
	Enesim_Renderer_Shape_Class *klass;
	int features;

	klass = ENESIM_RENDERER_SHAPE_CLASS_GET(r);
	features = enesim_renderer_shape_shape_features_get(r);
	_shape_cleanup(r, features, s);
	if (klass->opencl_cleanup)
		klass->opencl_cleanup(r, s);
}

static Eina_Bool _enesim_renderer_shape_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape_Class *klass;
	Eina_Bool ret = EINA_TRUE;

	klass = ENESIM_RENDERER_SHAPE_CLASS_GET(r);
	/* call the has_changed on the descriptor */
	if (klass->has_changed)
		ret = klass->has_changed(r);
	/* check that the common shape state has changed */
	if (!ret)
		ret = enesim_renderer_shape_state_has_changed(r);

	return ret;
}

static Eina_Bool _enesim_renderer_shape_damage(Enesim_Renderer *r,
		const Eina_Rectangle *old_bounds,
		Enesim_Renderer_Damage cb, void *data)
{
	Enesim_Renderer_Shape *thiz;
	Enesim_Renderer_Shape_Class *klass;
	Enesim_Renderer_Shape_State *state;
	Eina_Rectangle current_bounds;
	Eina_Bool do_send_old = EINA_FALSE;
	int features;

	thiz = ENESIM_RENDERER_SHAPE(r);
	klass = ENESIM_RENDERER_SHAPE_CLASS_GET(r);
	state = &thiz->state;

	/* get the features */
	features = enesim_renderer_shape_shape_features_get(r);

	/* first check if the common properties have changed */
	do_send_old = enesim_renderer_state_has_changed(r);
	if (do_send_old)
	{
		DBG("Common properties have changed");
		goto send_old;
	}

	/* check if the common shape properties have changed */
	do_send_old = _state_changed_basic(state, features);
	if (do_send_old)
	{
		DBG("Common shape properties have changed");
		goto send_old;
	}

	/* check if the shape implementation has changed */
	if (klass->has_changed)
	{
		do_send_old = klass->has_changed(r);
		if (do_send_old)
			DBG("Implementation changed");
	}

send_old:
	if (do_send_old)
	{
		/* get the current bounds */
		enesim_renderer_destination_bounds_get(r, &current_bounds, 0, 0, NULL);
		DBG("Sending old bounds %" EINA_RECTANGLE_FORMAT, EINA_RECTANGLE_ARGS(old_bounds));
		cb(r, old_bounds, EINA_TRUE, data);
		DBG("And new bounds %" EINA_RECTANGLE_FORMAT, EINA_RECTANGLE_ARGS(&current_bounds));
		cb(r, &current_bounds, EINA_FALSE, data);
		return EINA_TRUE;
	}
	else
	{
		/* optimized case */
		int dm = state->current.draw_mode;
		Eina_Bool stroke_changed = EINA_FALSE;
		Eina_Bool fill_changed = EINA_FALSE;

		if (state->current.stroke.r &&
					(dm & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE))
			stroke_changed = enesim_renderer_has_changed(state->current.stroke.r);

		if (state->current.fill.r &&
				(dm & ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL))
			fill_changed = enesim_renderer_has_changed(state->current.fill.r);

		/* if we fill with a renderer which has changed then only
		 * send the damages of that fill
		 */
		if (fill_changed && !stroke_changed)
		{
			Enesim_Renderer_Shape_Damage_Data ddata;

			DBG("Fill renderer changed, damaging fill area");
			enesim_renderer_destination_bounds_get(r, &current_bounds, 0, 0, NULL);
			ddata.real_cb = cb;
			ddata.real_data = data;
			ddata.bounds = &current_bounds;

			return enesim_renderer_damages_get(state->current.fill.r, _shape_damage_cb, &ddata);
		}
		/* otherwise send the current bounds only */
		else
		{
			if (stroke_changed)
			{
				DBG("Stroke changed, sending current bounds");
				enesim_renderer_destination_bounds_get(r, &current_bounds, 0, 0, NULL);
				cb(r, &current_bounds, EINA_FALSE, data);
				return EINA_TRUE;
			}
		}
		return EINA_FALSE;
	}
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_ABSTRACT_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Shape, Enesim_Renderer_Shape_Class,
		enesim_renderer_shape);

static void _enesim_renderer_shape_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->is_supported = _enesim_renderer_shape_is_supported;
	klass->damages_get = _enesim_renderer_shape_damage;
	klass->has_changed = _enesim_renderer_shape_has_changed;
	klass->sw_setup = _enesim_renderer_shape_sw_setup;
	klass->sw_cleanup = _enesim_renderer_shape_sw_cleanup;
	klass->opencl_setup = _enesim_renderer_shape_opencl_setup;
	klass->opencl_cleanup = _enesim_renderer_shape_opencl_cleanup;
	klass->opengl_setup = _enesim_renderer_shape_opengl_setup;
	klass->opengl_cleanup = _enesim_renderer_shape_opengl_cleanup;
}

static void _enesim_renderer_shape_instance_init(void *o)
{
	Enesim_Renderer_Shape *thiz = ENESIM_RENDERER_SHAPE(o);

	/* initialize the state */
	_state_init(&thiz->state);
}

static void _enesim_renderer_shape_instance_deinit(void *o)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(o);
	_state_clear(&thiz->state);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_renderer_shape_state_commit(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	_state_commit(&thiz->state);
}

Eina_Bool enesim_renderer_shape_state_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;
	Eina_Bool ret;
	int features;

	thiz = ENESIM_RENDERER_SHAPE(r);
	features = enesim_renderer_shape_shape_features_get(r);
	ret = _state_changed(&thiz->state, features);
	return ret;
}

const Enesim_Renderer_Shape_State * enesim_renderer_shape_state_get(
		Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	return &thiz->state;
}

void enesim_renderer_shape_propagate(Enesim_Renderer *r, Enesim_Renderer *s)
{
	Enesim_Renderer_Shape *thiz, *other;
	Enesim_Renderer *fill;
	Enesim_Renderer *stroke;
	const Enesim_Renderer_Shape_State *sstate;

	thiz = ENESIM_RENDERER_SHAPE(r);
	other = ENESIM_RENDERER_SHAPE(s);

	sstate = &thiz->state;

	/* TODO we should compare against the state of 'to' */
	enesim_renderer_shape_draw_mode_set(s, sstate->current.draw_mode);
	enesim_renderer_shape_stroke_weight_set(s, sstate->current.stroke.weight);
	enesim_renderer_shape_stroke_color_set(s, sstate->current.stroke.color);
	enesim_renderer_shape_stroke_join_set(s, sstate->current.stroke.join);
	enesim_renderer_shape_stroke_cap_set(s, sstate->current.stroke.cap);
	enesim_renderer_shape_stroke_scalable_set(s, sstate->current.stroke.scalable);
	enesim_renderer_shape_stroke_location_set(s, sstate->current.stroke.location);
	stroke = enesim_renderer_shape_stroke_renderer_get(r);
	enesim_renderer_shape_stroke_renderer_set(s, stroke);

	enesim_renderer_shape_fill_color_set(s, sstate->current.fill.color);
	fill = enesim_renderer_shape_fill_renderer_get(r);
	enesim_renderer_shape_fill_renderer_set(s, fill);
	enesim_renderer_shape_fill_rule_set(s, sstate->current.fill.rule);

	/* add the dashes */
	if (other->state.dashes)
	{
		enesim_list_unref(other->state.dashes);
	}
	other->state.dashes = enesim_list_ref(thiz->state.dashes);
}

void enesim_renderer_shape_stroke_setup(Enesim_Renderer *r,
		Enesim_Color *final_color,
		Enesim_Renderer **orend)
{
	Enesim_Color scolor;
	Enesim_Color color;

	color = enesim_renderer_color_get(r);
	scolor = enesim_renderer_shape_stroke_color_get(r);
	*orend = enesim_renderer_shape_stroke_renderer_get(r);
	/* multiply the color */
	if (scolor != ENESIM_COLOR_FULL)
		*final_color = enesim_color_mul4_sym(color, scolor);
	else
		*final_color = color;
}

void enesim_renderer_shape_stroke_weight_setup(Enesim_Renderer *r,
		double *swx, double *swy)
{
	Enesim_Matrix m;
	Eina_Bool scale;
	double sw;

	enesim_renderer_transformation_get(r, &m);
	sw = enesim_renderer_shape_stroke_weight_get(r);
	scale = enesim_renderer_shape_stroke_scalable_get(r);

	if (scale)
	{
		*swx = sw / 2 * hypot(m.xx, m.yx);
		*swy = sw / 2 * hypot(m.xy, m.yy);
	}
	else
	{
		*swx = sw / 2;
		*swy = sw / 2;
	}
}

void enesim_renderer_shape_fill_setup(Enesim_Renderer *r,
		Enesim_Color *final_color,
		Enesim_Renderer **orend)
{
	Enesim_Color fcolor;
	Enesim_Color color;

	color = enesim_renderer_color_get(r);
	fcolor = enesim_renderer_shape_fill_color_get(r);
	*orend = enesim_renderer_shape_fill_renderer_get(r);
	/* multiply the color */
	if (fcolor != ENESIM_COLOR_FULL)
		*final_color = enesim_color_mul4_sym(color, fcolor);
	else
		*final_color = color;
}

Enesim_List * enesim_renderer_shape_dashes_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	return enesim_list_ref(thiz->state.dashes);
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Sets the stroke weight of a shape renderer
 * @ender_prop{stroke_weight}
 * @param[in] r The shape renderer to set the stroke weight on
 * @param[in] weight The stroke weight to set
 */
EAPI void enesim_renderer_shape_stroke_weight_set(Enesim_Renderer *r, double weight)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	if (thiz->state.current.stroke.weight == weight)
		return;
	thiz->state.current.stroke.weight = weight;
	thiz->state.changed = EINA_TRUE;
}

/**
 * Gets the stroke weight of a shape renderer
 * @ender_prop{stroke_weight}
 * @param[in] r The shape renderer to get the stroke weight from
 * @return The stroke weight
 */
EAPI double enesim_renderer_shape_stroke_weight_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	return thiz->state.current.stroke.weight;
}

/**
 * Sets the stroke location of a shape renderer
 * @ender_prop{stroke_location}
 * @param[in] r The shape renderer to set the stroke location on
 * @param[in] location The stroke location to set
 */
EAPI void enesim_renderer_shape_stroke_location_set(Enesim_Renderer *r, Enesim_Renderer_Shape_Stroke_Location location)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	if (thiz->state.current.stroke.location == location)
		return;
	thiz->state.current.stroke.location = location;
	thiz->state.changed = EINA_TRUE;
}

/**
 * Gets the stroke location of a shape renderer
 * @ender_prop{stroke_location}
 * @param[in] r The shape renderer to get the stroke location from
 * @return The stroke location
 */
EAPI Enesim_Renderer_Shape_Stroke_Location enesim_renderer_shape_stroke_location_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	return thiz->state.current.stroke.location;
}

/**
 * Sets the stroke color of a shape renderer
 * @ender_prop{stroke_color}
 * @param[in] r The shape renderer to set the stroke color on
 * @param[in] color The stroke color to set
 */
EAPI void enesim_renderer_shape_stroke_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	if (thiz->state.current.stroke.color == color)
		return;
	thiz->state.current.stroke.color = color;
	thiz->state.changed = EINA_TRUE;
}

/**
 * Gets the stroke color of a shape renderer
 * @ender_prop{stroke_color}
 * @param[in] r The shape renderer to get the stroke color from
 * @return The stroke color
 */
EAPI Enesim_Color enesim_renderer_shape_stroke_color_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	return thiz->state.current.stroke.color;
}

/**
 * Sets the stroke cap of a shape renderer
 * @ender_prop{stroke_cap}
 * @param[in] r The shape renderer to set the stroke cap on
 * @param[in] cap The stroke cap to set
 */
EAPI void enesim_renderer_shape_stroke_cap_set(Enesim_Renderer *r, Enesim_Renderer_Shape_Stroke_Cap cap)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	if (thiz->state.current.stroke.cap == cap)
		return;
	thiz->state.current.stroke.cap = cap;
	thiz->state.changed = EINA_TRUE;
}

/**
 * Gets the stroke cap of a shape renderer
 * @ender_prop{stroke_cap}
 * @param[in] r The shape renderer to get the stroke cap from
 * @return The stroke cap
 */
EAPI Enesim_Renderer_Shape_Stroke_Cap enesim_renderer_shape_stroke_cap_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	return thiz->state.current.stroke.cap;
}

/**
 * Sets the stroke join of a shape renderer
 * @ender_prop{stroke_join}
 * @param[in] r The shape renderer to set the stroke join on
 * @param[in] join The stroke join to set
 */
EAPI void enesim_renderer_shape_stroke_join_set(Enesim_Renderer *r, Enesim_Renderer_Shape_Stroke_Join join)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	if (thiz->state.current.stroke.join == join)
		return;
	thiz->state.current.stroke.join = join;
	thiz->state.changed = EINA_TRUE;
}

/**
 * Gets the stroke join of a shape renderer
 * @ender_prop{stroke_join}
 * @param[in] r The shape renderer to get the stroke join from
 * @return The stroke join
 */
EAPI Enesim_Renderer_Shape_Stroke_Join enesim_renderer_shape_stroke_join_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	return thiz->state.current.stroke.join;
}

/**
 * Sets the stroke renderer of a shape renderer
 * @ender_prop{stroke_renderer}
 * @param[in] r The shape renderer to set the stroke renderer on
 * @param[in] stroke The stroke renderer to set
 */
EAPI void enesim_renderer_shape_stroke_renderer_set(Enesim_Renderer *r, Enesim_Renderer *stroke)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	if (thiz->state.current.stroke.r == stroke)
	{
		enesim_renderer_unref(stroke);
		return;
	}

	if (thiz->state.current.stroke.r)
		enesim_renderer_unref(thiz->state.current.stroke.r);
	thiz->state.current.stroke.r = stroke;
	thiz->state.changed = EINA_TRUE;
}

/**
 * Gets the stroke renderer of a shape renderer
 * @ender_prop{stroke_renderer}
 * @param[in] r The shape renderer to get the stroke renderer from
 * @return The stroke renderer
 */
EAPI Enesim_Renderer * enesim_renderer_shape_stroke_renderer_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	return enesim_renderer_ref(thiz->state.current.stroke.r);
}

/**
 * Sets the fill color of a shape renderer
 * @ender_prop{fill_color}
 * @param[in] r The shape renderer to set the fill color on
 * @param[in] color The fill color to set
 */
EAPI void enesim_renderer_shape_fill_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	if (thiz->state.current.fill.color == color)
		return;
	thiz->state.current.fill.color = color;
	thiz->state.changed = EINA_TRUE;
}

/**
 * Gets the fill color of a shape renderer
 * @ender_prop{fill_color}
 * @param[in] r The shape renderer to get the fill color from
 * @return The fill color
 */
EAPI Enesim_Color enesim_renderer_shape_fill_color_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	return thiz->state.current.fill.color;
}

/**
 * Sets the fill renderer of a shape renderer
 * @ender_prop{fill_renderer}
 * @param[in] r The shape renderer to set the fill renderer on
 * @param[in] fill The fill renderer to set
 */
EAPI void enesim_renderer_shape_fill_renderer_set(Enesim_Renderer *r, Enesim_Renderer *fill)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	if (thiz->state.current.fill.r == fill)
	{
		enesim_renderer_unref(fill);
		return;
	}
	if (thiz->state.current.fill.r)
		enesim_renderer_unref(thiz->state.current.fill.r);
	thiz->state.current.fill.r = fill;
	thiz->state.changed = EINA_TRUE;
}

/**
 * Gets the fill renderer of a shape renderer
 * @ender_prop{fill_renderer}
 * @param[in] r The shape renderer to get the fill renderer from
 * @return The fill renderer
 */
EAPI Enesim_Renderer * enesim_renderer_shape_fill_renderer_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	return enesim_renderer_ref(thiz->state.current.fill.r);
}

/**
 * Sets the fill rule of a shape renderer
 * @ender_prop{fill_rule}
 * @param[in] r The shape renderer to set the fill rule on
 * @param[in] rule The fill rule to set
 */
EAPI void enesim_renderer_shape_fill_rule_set(Enesim_Renderer *r, Enesim_Renderer_Shape_Fill_Rule rule)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	if (thiz->state.current.fill.rule == rule)
		return;
	thiz->state.current.fill.rule = rule;
	thiz->state.changed = EINA_TRUE;
}

/**
 * Gets the fill rule of a shape renderer
 * @ender_prop{fill_rule}
 * @param[in] r The shape renderer to get the fill rule from
 * @return The fill rule
 */
EAPI Enesim_Renderer_Shape_Fill_Rule enesim_renderer_shape_fill_rule_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	return thiz->state.current.fill.rule;
}

/**
 * Sets the draw mode of a shape renderer
 * @ender_prop{draw_mode}
 * @param[in] r The shape renderer to set the draw mode on
 * @param[in] draw_mode The draw mode to set
 */
EAPI void enesim_renderer_shape_draw_mode_set(Enesim_Renderer *r, int draw_mode)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	if (thiz->state.current.draw_mode == draw_mode)
		return;
	thiz->state.current.draw_mode = draw_mode;
	thiz->state.changed = EINA_TRUE;
}

/**
 * Gets the draw mode of a shape renderer
 * @ender_prop{draw_mode}
 * @param[in] r The shape renderer to get the draw mode from
 * @return The draw mode
 */
EAPI int enesim_renderer_shape_draw_mode_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	return thiz->state.current.draw_mode;
}

/**
 * @brief Get the features a shape renderer support
 * @param[in] r The renderer to get the features from
 * @return The renderer features
 */
EAPI int enesim_renderer_shape_shape_features_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape_Class *klass;
	int features = 0;

	klass = ENESIM_RENDERER_SHAPE_CLASS_GET(r);
	if (klass->features_get)
		klass->features_get(r, &features);
	return features;
}

EAPI void enesim_renderer_shape_stroke_dash_add_simple(Enesim_Renderer *r,
		double length, double gap)
{
	Enesim_Renderer_Shape_Stroke_Dash dash;

	dash.length = length;
	dash.gap = gap;
	enesim_renderer_shape_stroke_dash_add(r, &dash);
}

EAPI void enesim_renderer_shape_stroke_dash_add(Enesim_Renderer *r,
		const Enesim_Renderer_Shape_Stroke_Dash *dash)
{
	Enesim_Renderer_Shape *thiz;
	Enesim_Renderer_Shape_Stroke_Dash *d;

	thiz = ENESIM_RENDERER_SHAPE(r);
	d = malloc(sizeof(Enesim_Renderer_Shape_Stroke_Dash));
	*d = *dash;
	enesim_list_append(thiz->state.dashes, d);
}

EAPI void enesim_renderer_shape_stroke_dash_clear(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	enesim_list_clear(thiz->state.dashes);
}

/**
 * @brief Sets whenever a stroke should be scalable
 * @ender_prop{stroke_scalable}
 * @param[in] r The shape renderer to set the stroke scalable to
 * @param[in] scalable EINA_TRUE to enable scaling the stroke, EINA_FALSE otherwise
 */
EAPI void enesim_renderer_shape_stroke_scalable_set(Enesim_Renderer *r, Eina_Bool scalable)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	thiz->state.current.stroke.scalable = scalable;
	thiz->state.changed = EINA_TRUE;
}

/**
 * @brief Gets whenever a stroke should be scalable
 * @ender_prop{stroke_scalable}
 * @param[in] r The shape renderer to get the stroke scalable from
 * @return EINA_TRUE is the stroke is scalable, EINA_FALSE otherwise
 */
EAPI Eina_Bool enesim_renderer_shape_stroke_scalable_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = ENESIM_RENDERER_SHAPE(r);
	return thiz->state.current.stroke.scalable;
}

/**
 * @brief Get the geometry of a renderer shape
 * @param[in] r The shape renderer to get the geometry from
 * @param[out] geometry The rectangle pointer to write the geometry in
 * @return Eina_True if the functions success on the getting geometry,
 * Eina_False otherwise
 *
 * This function gets the geometry of a shape renderer which is defined
 * only by it's shape and not by the pixels drawn. That is, the stroking
 * of the shape is not taking into account.
 * To know the area of the pixels drawn by the shape renderer use @ref
 * enesim_renderer_bounds_get
 */
EAPI Eina_Bool enesim_renderer_shape_geometry_get(Enesim_Renderer *r, Enesim_Rectangle *geometry)
{
	Enesim_Renderer_Shape_Class *klass;

	klass = ENESIM_RENDERER_SHAPE_CLASS_GET(r);
	if (klass->geometry_get)
		return klass->geometry_get(r, geometry);
	return EINA_FALSE;
}

/**
 * @brief Get the destination geometry of a renderer shape
 * @param[in] r The shape renderer to get the destination geometry from
 * @param[out] geometry The rectangle pointer to write the destination geometry in
 * @return Eina_True if the functions success on the getting geometry,
 * Eina_False otherwise
 *
 * This function is similar to @ref enesim_renderer_shape_geometry_get but the
 * geometry is also transformed by the transformation matrix set using @ref
 * enesim_renderer_transformation_set
 */
EAPI Eina_Bool enesim_renderer_shape_destination_geometry_get(
		Enesim_Renderer *r, Enesim_Rectangle *geometry)
{
	Enesim_Matrix_Type type;

	if (!enesim_renderer_shape_geometry_get(r, geometry))
		return EINA_FALSE;
	type = enesim_renderer_transformation_type_get(r);
	if (type != ENESIM_MATRIX_TYPE_IDENTITY)
	{
		Enesim_Matrix m;
		Enesim_Quad q;

		enesim_renderer_transformation_get(r, &m);
		enesim_matrix_rectangle_transform(&m, geometry, &q);
		enesim_quad_rectangle_to(&q, geometry);
	}
	return EINA_TRUE;
}
