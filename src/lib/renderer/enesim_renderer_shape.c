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
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"

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
#define ENESIM_LOG_DEFAULT enesim_log_renderer_shape

#define ENESIM_RENDERER_SHAPE_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_SHAPE_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_SHAPE_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Shape_Damage_Data
{
	Eina_Rectangle *bounds;
	Enesim_Renderer_Damage_Cb real_cb;
	void *real_data;
} Enesim_Renderer_Shape_Damage_Data;

typedef struct _Enesim_Renderer_Shape
{
	EINA_MAGIC
	/* properties */
	Enesim_Renderer_Shape_State state;
	/* private */
	/* interface */
	Enesim_Renderer_Delete_Cb free;
	Enesim_Renderer_Has_Changed_Cb has_changed;
	/* software based functions */
	Enesim_Renderer_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	/* opencl based functions */
	Enesim_Renderer_OpenCL_Setup opencl_setup;
	Enesim_Renderer_OpenCL_Kernel_Setup opencl_kernel_setup;
	Enesim_Renderer_OpenCL_Cleanup opencl_cleanup;
	/* opengl based functions */
	Enesim_Renderer_OpenGL_Setup opengl_setup;
	Enesim_Renderer_OpenGL_Cleanup opengl_cleanup;
	/* shape functions */
	Enesim_Renderer_Shape_Features_Get_Cb shape_features_get;
	void *data;
} Enesim_Renderer_Shape;

static inline Enesim_Renderer_Shape * _shape_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = enesim_renderer_data_get(r);
	ENESIM_RENDERER_SHAPE_MAGIC_CHECK(thiz);

	return thiz;
}

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
		Enesim_Shape_Feature features)
{
	if (!thiz->changed)
		return EINA_TRUE;
	/* optional properties */
	/* the stroke */
	/* color */
	if (thiz->current.stroke.color != thiz->past.stroke.color)
		return EINA_TRUE;
	/* weight */
	if (thiz->current.stroke.weight != thiz->past.stroke.weight)
		return EINA_TRUE;
	/* location */
	if (features & ENESIM_SHAPE_FLAG_STROKE_LOCATION)
	{
		if (thiz->current.stroke.location != thiz->past.stroke.location)
			return EINA_TRUE;
	}
	/* join */
	if (thiz->current.stroke.join != thiz->past.stroke.join)
		return EINA_TRUE;
	/* cap */
	if (thiz->current.stroke.cap != thiz->past.stroke.cap)
		return EINA_TRUE;
	/* dashes */
	if (features & ENESIM_SHAPE_FLAG_STROKE_DASH)
	{
		/* we wont compare the stroke dashes, it has changed, then
		 * modify it directly
		 */
		if (thiz->stroke_dashes_changed)
			return EINA_TRUE;
	}

	/* fill */
	/* color */
	if (thiz->current.fill.color != thiz->past.fill.color)
		return EINA_TRUE;
	/* fill rule */
	if (thiz->current.fill.rule != thiz->past.fill.rule)
		return EINA_TRUE;
	/* draw mode */
	if (thiz->current.draw_mode != thiz->past.draw_mode)
		return EINA_TRUE;
	/* we wont compare the stroke dashes, it has changed, then
	 * modify it directly
	 */
	/* color */
	if (thiz->current.fill.color != thiz->past.fill.color)
		return EINA_TRUE;
	/* renderer */
	if (thiz->current.fill.r != thiz->past.fill.r)
		return EINA_TRUE;
	/* fill rule */
	if (thiz->current.fill.rule != thiz->past.fill.rule)
		return EINA_TRUE;
	/* draw mode */
	if (thiz->current.draw_mode != thiz->past.draw_mode)
		return EINA_TRUE;

	return EINA_FALSE;
}

static Eina_Bool _state_changed(Enesim_Renderer_Shape_State *thiz,
		Enesim_Shape_Feature features)
{
	if (_state_changed_basic(thiz, features))
		return EINA_TRUE;

	/* renderer */
	if (features & ENESIM_SHAPE_FLAG_STROKE_RENDERER)
	{
		if (thiz->current.stroke.r != thiz->past.stroke.r)
			return EINA_TRUE;
		if (thiz->current.stroke.r &&
			(thiz->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE))
		{
			if (enesim_renderer_has_changed(thiz->current.stroke.r))
			{
				const char *stroke_name;

				enesim_renderer_name_get(thiz->current.stroke.r, &stroke_name);
				DBG("The stroke renderer %s has changed", stroke_name);
				return EINA_TRUE;
			}
		}
	}
	/* renderer */
	if (features & ENESIM_SHAPE_FLAG_FILL_RENDERER)
	{
		if (thiz->current.fill.r != thiz->past.fill.r)
			return EINA_TRUE;
		/* we should first check if the fill renderer has changed */
		if (thiz->current.fill.r &&
				(thiz->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_FILL))
		{
			if (enesim_renderer_has_changed(thiz->current.fill.r))
			{
				const char *fill_name;

				enesim_renderer_name_get(thiz->current.fill.r, &fill_name);
				DBG("The fill renderer %s has changed", fill_name);
				return EINA_TRUE;
			}
		}
	}
	return EINA_FALSE;
}

static void _state_clear(Enesim_Renderer_Shape_State *thiz)
{
	Enesim_Shape_Stroke_Dash *d;

	if (thiz->current.fill.r)
		enesim_renderer_unref(thiz->current.fill.r);
	if (thiz->past.fill.r)
		enesim_renderer_unref(thiz->past.fill.r);
	if (thiz->current.stroke.r)
		enesim_renderer_unref(thiz->current.stroke.r);
	if (thiz->past.stroke.r)
		enesim_renderer_unref(thiz->past.stroke.r);
	EINA_LIST_FREE (thiz->stroke_dashes, d)
	{
		free(d);
	}
}

static void _state_init(Enesim_Renderer_Shape_State *thiz)
{
	/* set default properties */
	thiz->current.fill.color = thiz->past.fill.color = 0xffffffff;
	thiz->current.stroke.color = thiz->past.stroke.color = 0xffffffff;
	thiz->current.stroke.location = ENESIM_SHAPE_STROKE_CENTER;
	thiz->current.draw_mode = ENESIM_SHAPE_DRAW_MODE_FILL;
}

static void _state_commit(Enesim_Renderer_Shape_State *thiz)
{
	/* TODO given that we keep a ref, we should this more smartly */
	thiz->past = thiz->current;
	/* unmark the changes */
	thiz->changed = EINA_FALSE;
	thiz->stroke_dashes_changed = EINA_FALSE;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static void _shape_cleanup(Enesim_Renderer_Shape *thiz,
		Enesim_Shape_Feature features, Enesim_Surface *s)
{
	Enesim_Renderer_Shape_State *state = &thiz->state;

	if (features & ENESIM_SHAPE_FLAG_FILL_RENDERER)
	{
		if (state->current.fill.r &&
				(state->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_FILL))
		{
			enesim_renderer_cleanup(state->current.fill.r, s);
		}
	}
	if (features & ENESIM_SHAPE_FLAG_STROKE_RENDERER)
	{
		if (state->current.stroke.r &&
				(state->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE))
		{
			enesim_renderer_cleanup(state->current.stroke.r, s);
		}
	}
	/* swap the states */
	_state_commit(state);
}

static Eina_Bool _shape_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Log **log)
{
	Enesim_Renderer_Shape *thiz;
	Enesim_Renderer_Shape_State *state;
	Enesim_Shape_Feature features;
	Eina_Bool fill_renderer = EINA_FALSE;

	thiz = _shape_get(r);
	state = &thiz->state;
	enesim_renderer_shape_features_get(r, &features);
	if (features & ENESIM_SHAPE_FLAG_FILL_RENDERER)
	{
		if (state->current.fill.r &&
				(state->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_FILL))
		{
			fill_renderer = EINA_TRUE;
			if (!enesim_renderer_setup(state->current.fill.r, s, log))
			{
				ENESIM_RENDERER_LOG(r, log, "Fill renderer failed");
				return EINA_FALSE;
			}
		}
	}
	if (features & ENESIM_SHAPE_FLAG_STROKE_RENDERER)
	{
		if (state->current.stroke.r &&
				(state->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE))
		{
			if (!enesim_renderer_setup(state->current.stroke.r, s, log))
			{
				ENESIM_RENDERER_LOG(r, log, "Stroke renderer failed");
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

static Eina_Bool _enesim_renderer_shape_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill,
		Enesim_Log **log)
{
	Enesim_Renderer_Shape *thiz;

	thiz = enesim_renderer_data_get(r);
	if (!_shape_setup(r, s, log)) return EINA_FALSE;
	if (!thiz->sw_setup) return EINA_FALSE;

	if (!thiz->sw_setup(r, s, fill, log))
		return EINA_FALSE;

	return EINA_TRUE;
}

static void _enesim_renderer_shape_sw_cleanup(Enesim_Renderer *r,
		Enesim_Surface *s)
{
	Enesim_Renderer_Shape *thiz;
	Enesim_Shape_Feature features;

	thiz = enesim_renderer_data_get(r);
	enesim_renderer_shape_features_get(r, &features);
	_shape_cleanup(thiz, features, s);
	if (thiz->sw_cleanup)
		thiz->sw_cleanup(r, s);
}


static Eina_Bool _enesim_renderer_shape_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **log)
{
	Enesim_Renderer_Shape *thiz;

	thiz = enesim_renderer_data_get(r);
	if (!_shape_setup(r, s, log)) return EINA_FALSE;
	if (!thiz->opengl_setup) return EINA_FALSE;

	return thiz->opengl_setup(r, s, draw, log);
}

static void _enesim_renderer_shape_opengl_cleanup(Enesim_Renderer *r,
		Enesim_Surface *s)
{
	Enesim_Renderer_Shape *thiz;
	Enesim_Shape_Feature features;

	thiz = enesim_renderer_data_get(r);
	enesim_renderer_shape_features_get(r, &features);
	_shape_cleanup(thiz, features, s);
	if (thiz->opengl_cleanup)
		thiz->opengl_cleanup(r, s);
}

static Eina_Bool _enesim_renderer_shape_opencl_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		const char **program_name, const char **program_source,
		size_t *program_length,
		Enesim_Log **log)
{
	Enesim_Renderer_Shape *thiz;

	thiz = enesim_renderer_data_get(r);
	if (!_shape_setup(r, s, log)) return EINA_FALSE;
	if (!thiz->opencl_setup) return EINA_FALSE;

	return thiz->opencl_setup(r, s, program_name, program_source,
			program_length, log);
}

static void _enesim_renderer_shape_opencl_cleanup(Enesim_Renderer *r,
		Enesim_Surface *s)
{
	Enesim_Renderer_Shape *thiz;
	Enesim_Shape_Feature features;

	thiz = enesim_renderer_data_get(r);
	enesim_renderer_shape_features_get(r, &features);
	_shape_cleanup(thiz, features, s);
	if (thiz->opencl_cleanup)
		thiz->opencl_cleanup(r, s);
}

static Eina_Bool _enesim_renderer_shape_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;
	Eina_Bool ret = EINA_TRUE;

	ret = enesim_renderer_state_has_changed(r);
	if (ret)
	{
		return ret;
	}
	thiz = enesim_renderer_data_get(r);
	/* call the has_changed on the descriptor */
	if (thiz->has_changed)
		ret = thiz->has_changed(r);

	return ret;
}

static void _enesim_renderer_shape_damage(Enesim_Renderer *r,
		const Eina_Rectangle *old_bounds,
		Enesim_Renderer_Damage_Cb cb, void *data)
{
	Enesim_Renderer_Shape *thiz;
	Enesim_Renderer_Shape_State *state;
	Enesim_Shape_Feature features;
	Eina_Rectangle current_bounds;
	Eina_Bool do_send_old = EINA_FALSE;

	thiz = enesim_renderer_data_get(r);
	state = &thiz->state;

	/* get the current bounds */
	enesim_renderer_destination_bounds(r, &current_bounds, 0, 0);
	/* get the features */
	enesim_renderer_shape_features_get(r, &features);

	/* first check if the common properties have changed */
	do_send_old = enesim_renderer_state_has_changed(r);
	if (do_send_old) goto send_old;

	/* check if the common shape properties have changed */
	do_send_old = _state_changed_basic(state, features);
	if (do_send_old) goto send_old;

	/* check if the shape implementation has changed */
	if (thiz->has_changed)
		do_send_old = thiz->has_changed(r);

send_old:
	if (do_send_old)
	{
		cb(r, old_bounds, EINA_TRUE, data);
		cb(r, &current_bounds, EINA_FALSE, data);
	}
	else
	{
		/* optimized case */
		Enesim_Shape_Draw_Mode dm = state->current.draw_mode;
		Eina_Bool stroke_changed = EINA_FALSE;

		if (state->current.stroke.r &&
					(dm & ENESIM_SHAPE_DRAW_MODE_STROKE))
			stroke_changed = enesim_renderer_has_changed(state->current.stroke.r);

		/* if we fill with a renderer which has changed then only
		 * send the damages of that fill
		 */
		if (state->current.fill.r &&
				(dm & ENESIM_SHAPE_DRAW_MODE_FILL) &&
				!stroke_changed)
		{
			Enesim_Renderer_Shape_Damage_Data ddata;
			ddata.real_cb = cb;
			ddata.real_data = data;
			ddata.bounds = &current_bounds;

			enesim_renderer_damages_get(state->current.fill.r, _shape_damage_cb, &ddata);
		}
		/* otherwise send the current bounds only */
		else
		{
			if (stroke_changed)
			{
				cb(r, &current_bounds, EINA_FALSE, data);
			}
		}
	}
}

static void _enesim_renderer_shape_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->free)
		thiz->free(r);
	_state_clear(&thiz->state);
	free(thiz);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_shape_new(Enesim_Renderer_Shape_Descriptor *descriptor,
		void *data)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Shape *thiz;
	Enesim_Renderer_Descriptor pdescriptor;

	thiz = calloc(1, sizeof(Enesim_Renderer_Shape));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_SHAPE_MAGIC);
	thiz->data = data;
	/* initialize the state */
	_state_init(&thiz->state);
	thiz->has_changed = descriptor->has_changed;
	thiz->sw_setup = descriptor->sw_setup;
	thiz->sw_cleanup = descriptor->sw_cleanup;
	thiz->opengl_setup = descriptor->opengl_setup;
	thiz->opengl_cleanup = descriptor->opengl_cleanup;
	thiz->opencl_setup = descriptor->opencl_setup;
	thiz->opencl_cleanup = descriptor->opencl_cleanup;
	thiz->shape_features_get = descriptor->shape_features_get;
	thiz->free = descriptor->free;
	/* set the parent descriptor */
	pdescriptor.version = ENESIM_RENDERER_API;
	pdescriptor.base_name_get = descriptor->base_name_get;
	pdescriptor.free = _enesim_renderer_shape_free;
	pdescriptor.bounds_get = descriptor->bounds_get;
	pdescriptor.features_get = descriptor->features_get;
	pdescriptor.is_inside = descriptor->is_inside;
	pdescriptor.damages_get = _enesim_renderer_shape_damage;
	pdescriptor.has_changed = _enesim_renderer_shape_has_changed;
	pdescriptor.sw_hints_get = descriptor->sw_hints_get;
	pdescriptor.sw_setup = _enesim_renderer_shape_sw_setup;
	pdescriptor.sw_cleanup = _enesim_renderer_shape_sw_cleanup;
	pdescriptor.opencl_setup = _enesim_renderer_shape_opencl_setup;
	pdescriptor.opencl_kernel_setup = descriptor->opencl_kernel_setup;
	pdescriptor.opencl_cleanup = _enesim_renderer_shape_opencl_cleanup;
	pdescriptor.opengl_initialize = descriptor->opengl_initialize;
	pdescriptor.opengl_setup = _enesim_renderer_shape_opengl_setup;
	pdescriptor.opengl_cleanup = _enesim_renderer_shape_opengl_cleanup;

	r = enesim_renderer_new(&pdescriptor, thiz);
	return r;
}

void * enesim_renderer_shape_data_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	return thiz->data;
}

Eina_Bool enesim_renderer_shape_state_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;
	Enesim_Shape_Feature features;
	Eina_Bool ret;

	thiz = _shape_get(r);
	enesim_renderer_shape_features_get(r, &features);
	ret = _state_changed(&thiz->state, features);
	return ret;
}

const Enesim_Renderer_Shape_State * enesim_renderer_shape_state_get(
		Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	return &thiz->state;
}

void enesim_renderer_shape_propagate(Enesim_Renderer *r, Enesim_Renderer *to)
{
	Enesim_Renderer_Shape *thiz;
	Enesim_Renderer *fill;
	Enesim_Renderer *stroke;
	const Enesim_Renderer_Shape_State *sstate;
	

	thiz = _shape_get(r);
	sstate = &thiz->state;

	/* TODO we should compare agains the state of 'to' */
	enesim_renderer_shape_draw_mode_set(to, sstate->current.draw_mode);
	enesim_renderer_shape_stroke_weight_set(to, sstate->current.stroke.weight);
	enesim_renderer_shape_stroke_color_set(to, sstate->current.stroke.color);
	enesim_renderer_shape_stroke_renderer_get(r, &stroke);
	enesim_renderer_shape_stroke_renderer_set(to, stroke);

	enesim_renderer_shape_fill_color_set(to, sstate->current.fill.color);
	enesim_renderer_shape_fill_renderer_get(r, &fill);
	enesim_renderer_shape_fill_renderer_set(to, fill);
	enesim_renderer_shape_fill_rule_set(to, sstate->current.fill.rule);

	/* TODO add the dashes */
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_weight_set(Enesim_Renderer *r, double weight)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->state.current.stroke.weight == weight)
		return;
	thiz->state.current.stroke.weight = weight;
	thiz->state.changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_weight_get(Enesim_Renderer *r, double *weight)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (weight) *weight = thiz->state.current.stroke.weight;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_location_set(Enesim_Renderer *r, Enesim_Shape_Stroke_Location location)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->state.current.stroke.location == location)
		return;
	thiz->state.current.stroke.location = location;
	thiz->state.changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_location_get(Enesim_Renderer *r, Enesim_Shape_Stroke_Location *location)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (location) *location = thiz->state.current.stroke.location;
}


/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->state.current.stroke.color == color)
		return;
	thiz->state.current.stroke.color = color;
	thiz->state.changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (color) *color = thiz->state.current.stroke.color;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_cap_set(Enesim_Renderer *r, Enesim_Shape_Stroke_Cap cap)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->state.current.stroke.cap == cap)
		return;
	thiz->state.current.stroke.cap = cap;
	thiz->state.changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_cap_get(Enesim_Renderer *r, Enesim_Shape_Stroke_Cap *cap)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (cap) *cap = thiz->state.current.stroke.cap;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_join_set(Enesim_Renderer *r, Enesim_Shape_Stroke_Join join)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->state.current.stroke.join == join)
		return;
	thiz->state.current.stroke.join = join;
	thiz->state.changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_join_get(Enesim_Renderer *r, Enesim_Shape_Stroke_Join *join)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (join) *join = thiz->state.current.stroke.join;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_renderer_set(Enesim_Renderer *r, Enesim_Renderer *stroke)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->state.current.stroke.r == stroke)
		return;

	if (thiz->state.current.stroke.r)
		enesim_renderer_unref(thiz->state.current.stroke.r);
	thiz->state.current.stroke.r = stroke;
	if (stroke)
		thiz->state.current.stroke.r = enesim_renderer_ref(stroke);
	thiz->state.changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_renderer_get(Enesim_Renderer *r, Enesim_Renderer **stroke)
{
	Enesim_Renderer_Shape *thiz;

	if (!stroke) return;
	thiz = _shape_get(r);
	*stroke = thiz->state.current.stroke.r;
	if (thiz->state.current.stroke.r)
		thiz->state.current.stroke.r = enesim_renderer_ref(thiz->state.current.stroke.r);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->state.current.fill.color == color)
		return;
	thiz->state.current.fill.color = color;
	thiz->state.changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (color) *color = thiz->state.current.fill.color;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_renderer_set(Enesim_Renderer *r, Enesim_Renderer *fill)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->state.current.fill.r == fill)
		return;
	if (thiz->state.current.fill.r)
		enesim_renderer_unref(thiz->state.current.fill.r);
	thiz->state.current.fill.r = fill;
	if (fill)
		thiz->state.current.fill.r = enesim_renderer_ref(fill);
	thiz->state.changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_renderer_get(Enesim_Renderer *r, Enesim_Renderer **fill)
{
	Enesim_Renderer_Shape *thiz;

	if (!fill) return;
	thiz = _shape_get(r);
	*fill = thiz->state.current.fill.r;
	if (thiz->state.current.fill.r)
		thiz->state.current.fill.r = enesim_renderer_ref(thiz->state.current.fill.r);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_rule_set(Enesim_Renderer *r, Enesim_Shape_Fill_Rule rule)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->state.current.fill.rule == rule)
		return;
	thiz->state.current.fill.rule = rule;
	thiz->state.changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_rule_get(Enesim_Renderer *r, Enesim_Shape_Fill_Rule *rule)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (rule) *rule = thiz->state.current.fill.rule;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_draw_mode_set(Enesim_Renderer *r, Enesim_Shape_Draw_Mode draw_mode)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->state.current.draw_mode == draw_mode)
		return;
	thiz->state.current.draw_mode = draw_mode;
	thiz->state.changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_draw_mode_get(Enesim_Renderer *r, Enesim_Shape_Draw_Mode *draw_mode)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (draw_mode) *draw_mode = thiz->state.current.draw_mode;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_features_get(Enesim_Renderer *r, Enesim_Shape_Feature *features)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	*features = 0;
	if (thiz->shape_features_get)
		thiz->shape_features_get(r, features);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_dash_add_simple(Enesim_Renderer *r,
		double length, double gap)
{
	Enesim_Shape_Stroke_Dash dash;

	dash.length = length;
	dash.gap = gap;
	enesim_renderer_shape_stroke_dash_add(r, &dash);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_dash_add(Enesim_Renderer *r,
		const Enesim_Shape_Stroke_Dash *dash)
{
	Enesim_Renderer_Shape *thiz;
	Enesim_Shape_Stroke_Dash *d;

	thiz = _shape_get(r);
	d = malloc(sizeof(Enesim_Shape_Stroke_Dash));
	*d = *dash;
	thiz->state.stroke_dashes = eina_list_append(thiz->state.stroke_dashes, d);
	thiz->state.changed = EINA_TRUE;
	thiz->state.stroke_dashes_changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_dash_clear(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;
	Enesim_Shape_Stroke_Dash *d;

	thiz = _shape_get(r);
	EINA_LIST_FREE (thiz->state.stroke_dashes, d)
	{
		free(d);
	}
	thiz->state.changed = EINA_TRUE;
	thiz->state.stroke_dashes_changed = EINA_TRUE;
	thiz->state.stroke_dashes = NULL;
}
