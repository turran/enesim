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
#include "Enesim.h"
#include "enesim_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_SHAPE_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_SHAPE_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_SHAPE_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Shape
{
	EINA_MAGIC
	/* properties */
	Enesim_Renderer_Shape_State current;
	Enesim_Renderer_Shape_State past;
	/* private */
	Eina_Bool changed : 1;
	/* needed for the state */
	Enesim_Renderer_State old_fill;
	Enesim_Renderer_State old_stroke;
	/* interface */
	Enesim_Renderer_Shape_Sw_Setup sw_setup;
	Enesim_Renderer_Shape_OpenCL_Setup opencl_setup;
	Enesim_Renderer_Shape_OpenGL_Setup opengl_setup;
	Enesim_Renderer_Has_Changed has_changed;
	void *data;
} Enesim_Renderer_Shape;

static inline Enesim_Renderer_Shape * _shape_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = enesim_renderer_data_get(r);
	ENESIM_RENDERER_SHAPE_MAGIC_CHECK(thiz);

	return thiz;
}

static Eina_Bool _enesim_renderer_shape_sw_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Shape *thiz;
	const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES];

	thiz = enesim_renderer_data_get(r);
	if (!thiz->sw_setup) return EINA_FALSE;

	sstates[ENESIM_STATE_CURRENT] = &thiz->current;
	sstates[ENESIM_STATE_PAST] = &thiz->past;

	return thiz->sw_setup(r, states, sstates, s, fill, error);
}

static Eina_Bool _enesim_renderer_shape_opengl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		int *num_shaders,
		Enesim_Renderer_OpenGL_Shader **shaders,
		Enesim_Error **error)
{
	Enesim_Renderer_Shape *thiz;
	const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES];

	thiz = enesim_renderer_data_get(r);
	if (!thiz->opengl_setup) return EINA_FALSE;

	sstates[ENESIM_STATE_CURRENT] = &thiz->current;
	sstates[ENESIM_STATE_PAST] = &thiz->past;

	return thiz->opengl_setup(r, states, sstates, s, num_shaders,
		shaders, error);
}

static Eina_Bool _enesim_renderer_shape_opencl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		const char **program_name, const char **program_source,
		size_t *program_length,
		Enesim_Error **error)
{
	Enesim_Renderer_Shape *thiz;
	const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES];

	thiz = enesim_renderer_data_get(r);
	if (!thiz->opengl_setup) return EINA_FALSE;

	sstates[ENESIM_STATE_CURRENT] = &thiz->current;
	sstates[ENESIM_STATE_PAST] = &thiz->past;

	return thiz->opencl_setup(r, states, sstates, s,
		program_name, program_source, program_length, error);
}

static Eina_Bool _enesim_renderer_shape_changed(Enesim_Renderer_Shape *thiz)
{
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
			return EINA_TRUE;
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
			return EINA_TRUE;
		}
	}
	if (!thiz->changed)
		return EINA_FALSE;
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
	/* draw mode */
	if (thiz->current.draw_mode != thiz->past.draw_mode)
		return EINA_TRUE;
	return EINA_FALSE;
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
	thiz->has_changed = descriptor->has_changed;
	thiz->sw_setup = descriptor->sw_setup;
	thiz->opengl_setup = descriptor->opengl_setup;
	thiz->opencl_setup = descriptor->opencl_setup;
	/* set the parent descriptor */
	pdescriptor.version = ENESIM_RENDERER_API;
	pdescriptor.name = descriptor->name;
	pdescriptor.free = descriptor->free;
	pdescriptor.boundings = descriptor->boundings;
	pdescriptor.destination_transform = descriptor->destination_transform;
	pdescriptor.flags = descriptor->flags;
	pdescriptor.is_inside = descriptor->is_inside;
	pdescriptor.damage = descriptor->damage;
	pdescriptor.has_changed = _enesim_renderer_shape_has_changed;
	pdescriptor.sw_setup = _enesim_renderer_shape_sw_setup;
	pdescriptor.sw_cleanup = descriptor->sw_cleanup;
	pdescriptor.opencl_setup = _enesim_renderer_shape_opencl_setup;
	pdescriptor.opencl_kernel_setup = descriptor->opencl_kernel_setup;
	pdescriptor.opencl_cleanup = descriptor->opencl_cleanup;
	pdescriptor.opengl_setup = _enesim_renderer_shape_opengl_setup;
	pdescriptor.opengl_shader_setup = descriptor->opengl_shader_setup;
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
		enesim_renderer_relative_unset(thiz->current.fill.r, &thiz->old_fill);
		enesim_renderer_cleanup(thiz->current.fill.r, s);
	}
	if (thiz->current.stroke.r &&
			(thiz->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE))
	{
		enesim_renderer_relative_unset(thiz->current.stroke.r, &thiz->old_stroke);
		enesim_renderer_cleanup(thiz->current.stroke.r, s);
	}
	thiz->past = thiz->current;
}

Eina_Bool enesim_renderer_shape_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Error **error)
{
	Enesim_Renderer_Shape *thiz;
	const Enesim_Renderer_State *state = states[ENESIM_STATE_CURRENT];
	Eina_Bool fill_renderer = EINA_FALSE;

	thiz = _shape_get(r);
	if (thiz->current.fill.r &&
			(thiz->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_FILL))
	{
		fill_renderer = EINA_TRUE;
		enesim_renderer_relative_set(thiz->current.fill.r, state, &thiz->old_fill);
		if (!enesim_renderer_setup(thiz->current.fill.r, s, error))
		{
			ENESIM_RENDERER_ERROR(r, error, "Fill renderer failed");
			return EINA_FALSE;
		}
	}
	if (thiz->current.stroke.r &&
			(thiz->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE))
	{
		enesim_renderer_relative_set(thiz->current.stroke.r, state, &thiz->old_stroke);
		if (!enesim_renderer_setup(thiz->current.stroke.r, s, error))
		{
			ENESIM_RENDERER_ERROR(r, error, "Stroke renderer failed");
			/* clean up the fill renderer setup */
			if (fill_renderer)
			{
				enesim_renderer_relative_unset(thiz->current.fill.r, &thiz->old_fill);
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
	*color = thiz->current.stroke.color;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_cap_set(Enesim_Renderer *r, Enesim_Shape_Stroke_Cap cap)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
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
	*cap = thiz->current.stroke.cap;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_join_set(Enesim_Renderer *r, Enesim_Shape_Stroke_Join join)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
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
	*join = thiz->current.stroke.join;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_renderer_set(Enesim_Renderer *r, Enesim_Renderer *stroke)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->current.stroke.r)
		enesim_renderer_unref(thiz->current.stroke.r);
	thiz->current.stroke.r = stroke;
	if (thiz->current.stroke.r)
		thiz->current.stroke.r = enesim_renderer_ref(thiz->current.stroke.r);
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_renderer_get(Enesim_Renderer *r, Enesim_Renderer **stroke)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	*stroke = thiz->current.stroke.r;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
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
	*color = thiz->current.fill.color;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_renderer_set(Enesim_Renderer *r, Enesim_Renderer *fill)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->current.fill.r)
		enesim_renderer_unref(thiz->current.fill.r);
	thiz->current.fill.r = fill;
	if (thiz->current.fill.r)
		thiz->current.fill.r = enesim_renderer_ref(thiz->current.fill.r);
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
EAPI void enesim_renderer_shape_draw_mode_set(Enesim_Renderer *r, Enesim_Shape_Draw_Mode draw_mode)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
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
	*draw_mode = thiz->current.draw_mode;
}
