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
#include "enesim_error.h"
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
	Enesim_Renderer_Shape_State current;
	Enesim_Renderer_Shape_State past;
	/* private */
	Eina_Bool changed : 1;
	Eina_Bool dash_changed : 1;
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
	Enesim_Renderer_Shape_Feature_Get_Cb features_get;
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

static Eina_Bool _enesim_renderer_shape_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill,
		Enesim_Error **error)
{
	Enesim_Renderer_Shape *thiz;

	thiz = enesim_renderer_data_get(r);
	if (!thiz->sw_setup) return EINA_FALSE;

	if (!thiz->sw_setup(r, s, fill, error))
		return EINA_FALSE;

	return EINA_TRUE;
}

static Eina_Bool _enesim_renderer_shape_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Error **error)
{
	Enesim_Renderer_Shape *thiz;

	thiz = enesim_renderer_data_get(r);
	if (!thiz->opengl_setup) return EINA_FALSE;

	return thiz->opengl_setup(r, s, draw, error);
}

static Eina_Bool _enesim_renderer_shape_opencl_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		const char **program_name, const char **program_source,
		size_t *program_length,
		Enesim_Error **error)
{
	Enesim_Renderer_Shape *thiz;

	thiz = enesim_renderer_data_get(r);
	if (!thiz->opencl_setup) return EINA_FALSE;

	return thiz->opencl_setup(r, s, program_name, program_source,
			program_length, error);
}

static Eina_Bool _enesim_renderer_shape_changed_basic(Enesim_Renderer_Shape *thiz)
{
	if (!thiz->changed)
		return EINA_FALSE;
	/* we wont compare the stroke dashes, it has changed, then
	 * modify it directly
	 */
	if (thiz->dash_changed)
		return EINA_TRUE;
	/* the stroke */
	/* color */
	if (thiz->current.stroke.color != thiz->past.stroke.color)
		return EINA_TRUE;
	/* weight */
	if (thiz->current.stroke.weight != thiz->past.stroke.weight)
		return EINA_TRUE;
	/* location */
	if (thiz->current.stroke.location != thiz->past.stroke.location)
		return EINA_TRUE;
	/* join */
	if (thiz->current.stroke.join != thiz->past.stroke.join)
		return EINA_TRUE;
	/* cap */
	if (thiz->current.stroke.cap != thiz->past.stroke.cap)
		return EINA_TRUE;
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

static Eina_Bool _enesim_renderer_shape_changed(Enesim_Renderer_Shape *thiz)
{
	Eina_Bool ret;
	/* FIXME handle the case where it had a stroke/fill and not now
	 * or the opposite
	 */
	/* we should first check if the fill renderer has changed */
	if (thiz->current.fill.r &&
			(thiz->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_FILL))
	{
		if (enesim_renderer_has_changed(thiz->current.fill.r))
		{
			const char *fill_name;

			enesim_renderer_name_get(thiz->current.fill.r, &fill_name);
			DBG("The fill renderer %s has changed", fill_name);
			ret = EINA_TRUE;
			goto done;
		}
	}
	if (thiz->current.stroke.r &&
			(thiz->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE))
	{
		if (enesim_renderer_has_changed(thiz->current.stroke.r))
		{
			const char *stroke_name;

			enesim_renderer_name_get(thiz->current.stroke.r, &stroke_name);
			DBG("The stroke renderer %s has changed", stroke_name);
			ret = EINA_TRUE;
			goto done;
		}
	}
	ret = _enesim_renderer_shape_changed_basic(thiz);
done:
	return ret;
}

static Eina_Bool _enesim_renderer_shape_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;
	Eina_Bool ret = EINA_TRUE;

	thiz = enesim_renderer_data_get(r);
	ret = _enesim_renderer_shape_changed(thiz);
	if (ret)
	{
		return ret;
	}
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
	Eina_Rectangle current_bounds;
	Eina_Bool do_send_old = EINA_FALSE;

	thiz = enesim_renderer_data_get(r);

	/* get the current bounds */
	enesim_renderer_destination_bounds(r, &current_bounds, 0, 0);

	/* first check if the common properties have changed */
	do_send_old = enesim_renderer_common_changed(r);
	if (do_send_old) goto send_old;

	/* check if the common shape properties have changed */
	do_send_old = _enesim_renderer_shape_changed_basic(thiz);
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
		Enesim_Shape_Draw_Mode dm = thiz->current.draw_mode;
		Eina_Bool stroke_changed = EINA_FALSE;

		if (thiz->current.stroke.r &&
					(dm & ENESIM_SHAPE_DRAW_MODE_STROKE))
			stroke_changed = enesim_renderer_has_changed(thiz->current.stroke.r);

		/* if we fill with a renderer which has changed then only
		 * send the damages of that fill
		 */
		if (thiz->current.fill.r &&
				(dm & ENESIM_SHAPE_DRAW_MODE_FILL) &&
				!stroke_changed)
		{
			Enesim_Renderer_Shape_Damage_Data ddata;
			ddata.real_cb = cb;
			ddata.real_data = data;
			ddata.bounds = &current_bounds;

			enesim_renderer_damages_get(thiz->current.fill.r, _shape_damage_cb, &ddata);
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
	/* TODO unref the stroke renderer and fill renderer */
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
	/* set default properties */
	thiz->current.fill.color = thiz->past.fill.color = 0xffffffff;
	thiz->current.stroke.color = thiz->past.stroke.color = 0xffffffff;
	thiz->current.stroke.location = ENESIM_SHAPE_STROKE_CENTER;
	thiz->current.draw_mode = ENESIM_SHAPE_DRAW_MODE_FILL;
	thiz->has_changed = descriptor->has_changed;
	thiz->sw_setup = descriptor->sw_setup;
	thiz->opengl_setup = descriptor->opengl_setup;
	thiz->opencl_setup = descriptor->opencl_setup;
	thiz->features_get = descriptor->feature_get;
	thiz->free = descriptor->free;
	/* set the parent descriptor */
	pdescriptor.version = ENESIM_RENDERER_API;
	pdescriptor.base_name_get = descriptor->base_name_get;
	pdescriptor.free = _enesim_renderer_shape_free;
	pdescriptor.bounds_get = descriptor->bounds_get;
	pdescriptor.destination_bounds_get = descriptor->destination_bounds_get;
	pdescriptor.flags_get = descriptor->flags_get;
	pdescriptor.hints_get = descriptor->hints_get;
	pdescriptor.is_inside = descriptor->is_inside;
	pdescriptor.damages_get = _enesim_renderer_shape_damage;
	pdescriptor.has_changed = _enesim_renderer_shape_has_changed;
	pdescriptor.sw_setup = _enesim_renderer_shape_sw_setup;
	pdescriptor.sw_cleanup = descriptor->sw_cleanup;
	pdescriptor.opencl_setup = _enesim_renderer_shape_opencl_setup;
	pdescriptor.opencl_kernel_setup = descriptor->opencl_kernel_setup;
	pdescriptor.opencl_cleanup = descriptor->opencl_cleanup;
	pdescriptor.opengl_initialize = descriptor->opengl_initialize;
	pdescriptor.opengl_setup = _enesim_renderer_shape_opengl_setup;
	pdescriptor.opengl_cleanup = descriptor->opengl_cleanup;

	r = enesim_renderer_new(&pdescriptor, thiz);
	return r;
}

void enesim_renderer_shape_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);

	if (thiz->current.fill.r &&
			(thiz->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_FILL))
	{
		enesim_renderer_cleanup(thiz->current.fill.r, s);
	}
	if (thiz->current.stroke.r &&
			(thiz->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE))
	{
		enesim_renderer_cleanup(thiz->current.stroke.r, s);
	}
	/* swap the states */
	/* TODO given that we keep a ref, we should this more smartly */
	thiz->past = thiz->current;
	/* unmark the changes */
	thiz->changed = EINA_FALSE;
	thiz->dash_changed = EINA_FALSE;
}

Eina_Bool enesim_renderer_shape_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Error **error)
{
	Enesim_Renderer_Shape *thiz;
	Eina_Bool fill_renderer = EINA_FALSE;

	thiz = _shape_get(r);
	if (thiz->current.fill.r &&
			(thiz->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_FILL))
	{
		fill_renderer = EINA_TRUE;
		if (!enesim_renderer_setup(thiz->current.fill.r, s, error))
		{
			ENESIM_RENDERER_ERROR(r, error, "Fill renderer failed");
			return EINA_FALSE;
		}
	}
	if (thiz->current.stroke.r &&
			(thiz->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE))
	{
		if (!enesim_renderer_setup(thiz->current.stroke.r, s, error))
		{
			ENESIM_RENDERER_ERROR(r, error, "Stroke renderer failed");
			/* clean up the fill renderer setup */
			if (fill_renderer)
			{
				enesim_renderer_cleanup(thiz->current.fill.r, s);

			}
			return EINA_FALSE;
		}
	}
	return EINA_TRUE;
}

void * enesim_renderer_shape_data_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	return thiz->data;
}

void enesim_renderer_shape_state_stroke_weight_set(Enesim_Renderer_Shape_State2 *thiz, double weight)
{
	if (thiz->current.stroke.weight == weight)
		return;
	thiz->current.stroke.weight = weight;
	thiz->changed = EINA_TRUE;
}

Eina_Bool enesim_renderer_shape_state_stroke_weight_get(Enesim_Renderer_Shape_State2 *thiz, double *weight)
{
	if (!weight) return EINA_FALSE;
	*weight = thiz->current.stroke.weight;
	return EINA_TRUE;
}

void enesim_renderer_shape_state_stroke_location_set(Enesim_Renderer_Shape_State2 *thiz, Enesim_Shape_Stroke_Location location)
{
	if (thiz->current.stroke.location == location)
		return;
	thiz->current.stroke.location = location;
	thiz->changed = EINA_TRUE;
}

Eina_Bool enesim_renderer_shape_state_stroke_location_get(Enesim_Renderer_Shape_State2 *thiz, Enesim_Shape_Stroke_Location *location)
{
	if (!location) return EINA_FALSE;
	*location = thiz->current.stroke.location;
	return EINA_TRUE;
}

void enesim_renderer_shape_state_stroke_color_set(Enesim_Renderer_Shape_State2 *thiz, Enesim_Color color)
{
	if (thiz->current.stroke.color == color)
		return;
	thiz->current.stroke.color = color;
	thiz->changed = EINA_TRUE;
}

void enesim_renderer_shape_state_stroke_color_get(Enesim_Renderer_Shape_State2 *thiz, Enesim_Color *color)
{
	if (!color) return EINA_FALSE;
	*color = thiz->current.stroke.color;
	return EINA_TRUE;
}

void enesim_renderer_shape_state_stroke_cap_set(Enesim_Renderer_Shape_State2 *thiz, Enesim_Shape_Stroke_Cap cap)
{
	if (thiz->current.stroke.cap == cap)
		return;
	thiz->current.stroke.cap = cap;
	thiz->changed = EINA_TRUE;
}

void enesim_renderer_shape_state_stroke_cap_get(Enesim_Renderer_Shape_State2 *thiz, Enesim_Shape_Stroke_Cap *cap)
{
	if (cap) *cap = thiz->current.stroke.cap;
}

void enesim_renderer_shape_state_stroke_join_set(Enesim_Renderer_Shape_State2 *thiz, Enesim_Shape_Stroke_Join join)
{
	if (thiz->current.stroke.join == join)
		return;
	thiz->current.stroke.join = join;
	thiz->changed = EINA_TRUE;
}

void enesim_renderer_shape_state_stroke_join_get(Enesim_Renderer_Shape_State2 *thiz, Enesim_Shape_Stroke_Join *join)
{
	if (join) *join = thiz->current.stroke.join;
}

void enesim_renderer_shape_state_stroke_renderer_set(Enesim_Renderer_Shape_State2 *thiz, Enesim_Renderer *stroke)
{
	if (thiz->current.stroke.r == stroke)
		return;

	if (thiz->current.stroke.r)
		enesim_renderer_unref(thiz->current.stroke.r);
	thiz->current.stroke.r = stroke;
	if (stroke)
		thiz->current.stroke.r = enesim_renderer_ref(stroke);
	thiz->changed = EINA_TRUE;
}

void enesim_renderer_shape_state_stroke_renderer_get(Enesim_Renderer_Shape_State2 *thiz, Enesim_Renderer **stroke)
{
	if (!stroke) return;
	*stroke = thiz->current.stroke.r;
	if (thiz->current.stroke.r)
		thiz->current.stroke.r = enesim_renderer_ref(thiz->current.stroke.r);
}

void enesim_renderer_shape_state_fill_color_set(Enesim_Renderer_Shape_State2 *thiz, Enesim_Color color)
{
	if (thiz->current.fill.color == color)
		return;
	thiz->current.fill.color = color;
	thiz->changed = EINA_TRUE;
}

void enesim_renderer_shape_state_fill_color_get(Enesim_Renderer_Shape_State2 *thiz, Enesim_Color *color)
{
	if (color) *color = thiz->current.fill.color;
}

void enesim_renderer_shape_state_fill_renderer_set(Enesim_Renderer_Shape_State2 *thiz, Enesim_Renderer *fill)
{
	if (thiz->current.fill.r == fill)
		return;
	if (thiz->current.fill.r)
		enesim_renderer_unref(thiz->current.fill.r);
	thiz->current.fill.r = fill;
	if (fill)
		thiz->current.fill.r = enesim_renderer_ref(fill);
	thiz->changed = EINA_TRUE;
}

void enesim_renderer_shape_state_fill_renderer_get(Enesim_Renderer_Shape_State2 *thiz, Enesim_Renderer **fill)
{
	if (!fill) return;
	*fill = thiz->current.fill.r;
	if (thiz->current.fill.r)
		thiz->current.fill.r = enesim_renderer_ref(thiz->current.fill.r);
}

void enesim_renderer_shape_state_fill_rule_set(Enesim_Renderer_Shape_State2 *thiz, Enesim_Shape_Fill_Rule rule)
{
	if (thiz->current.fill.rule == rule)
		return;
	thiz->current.fill.rule = rule;
	thiz->changed = EINA_TRUE;
}

void enesim_renderer_shape_state_fill_rule_get(Enesim_Renderer_Shape_State2 *thiz, Enesim_Shape_Fill_Rule *rule)
{
	if (rule) *rule = thiz->current.fill.rule;
}

void enesim_renderer_shape_state_draw_mode_set(Enesim_Renderer_Shape_State2 *thiz, Enesim_Shape_Draw_Mode draw_mode)
{
	if (thiz->current.draw_mode == draw_mode)
		return;
	thiz->current.draw_mode = draw_mode;
	thiz->changed = EINA_TRUE;
}

void enesim_renderer_shape_state_draw_mode_get(Enesim_Renderer_Shape_State2 *thiz, Enesim_Shape_Draw_Mode *draw_mode)
{
	if (draw_mode) *draw_mode = thiz->current.draw_mode;
}

void enesim_renderer_shape_state_stroke_dash_add(Enesim_Renderer_Shape_State2 *thiz,
		const Enesim_Shape_Stroke_Dash *dash)
{
	Enesim_Shape_Stroke_Dash *d;

	d = malloc(sizeof(Enesim_Shape_Stroke_Dash));
	*d = *dash;
	thiz->stroke_dashes = eina_list_append(thiz->stroke_dashes, d);
	thiz->changed = EINA_TRUE;
	thiz->stroke_dashes_changed = EINA_TRUE;
}

void enesim_renderer_shape_state_stroke_dash_clear(Enesim_Renderer_Shape_State2 *thiz)
{
	Enesim_Shape_Stroke_Dash *d;

	EINA_LIST_FREE (thiz->stroke_dashes, d)
	{
		free(d);
	}
	thiz->changed = EINA_TRUE;
	thiz->stroke_dashes_changed = EINA_TRUE;
	thiz->stroke_dashes = NULL;
}

void enesim_renderer_shape_state_commit(Enesim_Renderer_Shape_State2 *thiz)
{
	Enesim_Renderer *old_stroke;
	Enesim_Renderer *old_fill;

	/* keep the referenceable objects */
	/* swap the state */
	thiz->past = thiz->current;
	/* increment the referenceable objects */
	/* release the referenceable objects */
}

Eina_Bool enesim_renderer_shape_state_changed(
		Enesim_Renderer_Shape_State2 *thiz)
{
	if (!thiz->changed)
		return EINA_FALSE;
	/* we wont compare the stroke dashes, it has changed, then
	 * modify it directly
	 */
	if (thiz->stroke_dashes_changed)
		return EINA_TRUE;
	/* the stroke */
	/* color */
	if (thiz->current.stroke.color != thiz->past.stroke.color)
		return EINA_TRUE;
	/* weight */
	if (thiz->current.stroke.weight != thiz->past.stroke.weight)
		return EINA_TRUE;
	/* location */
	if (thiz->current.stroke.location != thiz->past.stroke.location)
		return EINA_TRUE;
	/* join */
	if (thiz->current.stroke.join != thiz->past.stroke.join)
		return EINA_TRUE;
	/* cap */
	if (thiz->current.stroke.cap != thiz->past.stroke.cap)
		return EINA_TRUE;
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

void enesim_renderer_shape_state_clear(Enesim_Renderer_Shape_State2 *thiz)
{
	/* unref */
	/* free */
}

Eina_Bool enesim_renderer_shape_state_setup(Enesim_Renderer_Shape_State2 *thiz)
{

}

void enesim_renderer_shape_state_cleanup(Enesim_Renderer_Shape_State2 *thiz)
{

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
	if (thiz->current.stroke.weight == weight)
		return;
	thiz->current.stroke.weight = weight;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_weight_get(Enesim_Renderer *r, double *weight)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (weight) *weight = thiz->current.stroke.weight;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_location_set(Enesim_Renderer *r, Enesim_Shape_Stroke_Location location)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->current.stroke.location == location)
		return;
	thiz->current.stroke.location = location;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_location_get(Enesim_Renderer *r, Enesim_Shape_Stroke_Location *location)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (location) *location = thiz->current.stroke.location;
}


/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->current.stroke.color == color)
		return;
	thiz->current.stroke.color = color;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (color) *color = thiz->current.stroke.color;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_cap_set(Enesim_Renderer *r, Enesim_Shape_Stroke_Cap cap)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->current.stroke.cap == cap)
		return;
	thiz->current.stroke.cap = cap;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_cap_get(Enesim_Renderer *r, Enesim_Shape_Stroke_Cap *cap)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (cap) *cap = thiz->current.stroke.cap;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_join_set(Enesim_Renderer *r, Enesim_Shape_Stroke_Join join)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->current.stroke.join == join)
		return;
	thiz->current.stroke.join = join;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_join_get(Enesim_Renderer *r, Enesim_Shape_Stroke_Join *join)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (join) *join = thiz->current.stroke.join;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_renderer_set(Enesim_Renderer *r, Enesim_Renderer *stroke)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->current.stroke.r == stroke)
		return;

	if (thiz->current.stroke.r)
		enesim_renderer_unref(thiz->current.stroke.r);
	thiz->current.stroke.r = stroke;
	if (stroke)
		thiz->current.stroke.r = enesim_renderer_ref(stroke);
	thiz->changed = EINA_TRUE;
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
	*stroke = thiz->current.stroke.r;
	if (thiz->current.stroke.r)
		thiz->current.stroke.r = enesim_renderer_ref(thiz->current.stroke.r);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->current.fill.color == color)
		return;
	thiz->current.fill.color = color;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (color) *color = thiz->current.fill.color;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_renderer_set(Enesim_Renderer *r, Enesim_Renderer *fill)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->current.fill.r == fill)
		return;
	if (thiz->current.fill.r)
		enesim_renderer_unref(thiz->current.fill.r);
	thiz->current.fill.r = fill;
	if (fill)
		thiz->current.fill.r = enesim_renderer_ref(fill);
	thiz->changed = EINA_TRUE;
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
	*fill = thiz->current.fill.r;
	if (thiz->current.fill.r)
		thiz->current.fill.r = enesim_renderer_ref(thiz->current.fill.r);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_rule_set(Enesim_Renderer *r, Enesim_Shape_Fill_Rule rule)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->current.fill.rule == rule)
		return;
	thiz->current.fill.rule = rule;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_rule_get(Enesim_Renderer *r, Enesim_Shape_Fill_Rule *rule)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (rule) *rule = thiz->current.fill.rule;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_draw_mode_set(Enesim_Renderer *r, Enesim_Shape_Draw_Mode draw_mode)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->current.draw_mode == draw_mode)
		return;
	thiz->current.draw_mode = draw_mode;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_draw_mode_get(Enesim_Renderer *r, Enesim_Shape_Draw_Mode *draw_mode)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (draw_mode) *draw_mode = thiz->current.draw_mode;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_feature_get(Enesim_Renderer *r, Enesim_Shape_Feature *features)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	*features = 0;
	if (thiz->features_get)
		thiz->features_get(r, features);
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
	thiz->current.stroke.dashes = eina_list_append(thiz->current.stroke.dashes, d);
	thiz->changed = EINA_TRUE;
	thiz->dash_changed = EINA_TRUE;
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
	EINA_LIST_FREE (thiz->current.stroke.dashes, d)
	{
		free(d);
	}
	thiz->changed = EINA_TRUE;
	thiz->dash_changed = EINA_TRUE;
	thiz->current.stroke.dashes = NULL;
}
