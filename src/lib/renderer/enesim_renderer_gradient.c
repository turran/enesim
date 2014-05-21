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
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_gradient.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#ifdef BUILD_OPENGL
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
#endif

#include "enesim_renderer_private.h"
#include "enesim_renderer_gradient_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_renderer_gradient

static Eina_Bool _gradient_generate_1d_span(Enesim_Renderer_Gradient *thiz, Enesim_Renderer *r,
		Enesim_Log **l)
{
	Enesim_Renderer_Gradient_Stop *curr, *next, *last;
	Eina_F16p16 xx, inc;
	Eina_List *tmp;
	double diff;
	int slen;
	int start;
	int end;
	int i;
	uint32_t *dst;

	dst = thiz->sw.src;
	slen = thiz->sw.len;

	curr = eina_list_data_get(thiz->state.stops);
	tmp = eina_list_next(thiz->state.stops);
	next = eina_list_data_get(tmp);
	last = eina_list_data_get(eina_list_last(thiz->state.stops));
	diff = next->pos - curr->pos;
	/* get a valid start */
	while (!diff)
	{
		tmp = eina_list_next(tmp);
		curr = next;
		next = eina_list_data_get(tmp);
		if (!next)
			break;
		diff = next->pos - curr->pos;
	}
	if (!diff)
	{
		ENESIM_RENDERER_LOG(r, l, "No valid offset between stops");
		return EINA_FALSE;
	}
	inc = eina_f16p16_double_from(1.0 / (diff * slen));
	xx = 0;

	start = curr->pos * slen;
	end = last->pos * slen;

	dst = thiz->sw.src;

	/* in case we dont start at 0.0 */
	for (i = 0; i < start; i++)
		*dst++ = curr->argb;

	/* FIXME Im not sure if we increment xx by the 1 / ((next - curr) * len) value
	 * as it might not be too accurate
	 */
	for (i = start; i < end; i++)
	{
		uint16_t off;
		uint32_t p0;

		/* advance the curr and next */
		if (xx >= 65536)
		{
			tmp = eina_list_next(tmp);
			/* we advanced but there's no other stop? */
			if (!tmp)
				break;
			curr = next;
			next = eina_list_data_get(tmp);
			diff = next->pos - curr->pos;
			/* the stop position is the same as the previous position, just skip it */
			if (!diff)
			{
				continue;
			}
			inc = eina_f16p16_double_from(1.0 / (diff * slen));
			xx = 0;
		}
		off = 1 + (eina_f16p16_fracc_get(xx) >> 8);
		p0 = argb8888_interp_256(off, next->argb, curr->argb);
		*dst++ = enesim_color_argb_from(p0);
		xx += inc;
	}
	/* in case we dont end at 1.0 */
	for (i = end; i < thiz->sw.len; i++)
		*dst++ = next->argb;
	return EINA_TRUE;
}

static Eina_Bool _gradient_changed(Enesim_Renderer_Gradient *thiz)
{
	if (thiz->stops_changed)
		return EINA_TRUE;
	if (thiz->changed)
	{
		if (thiz->state.mode != thiz->past_mode)
			return EINA_TRUE;
	}
	return EINA_FALSE;
}

/* common setup process */
static Eina_Bool _gradient_setup(Enesim_Renderer *r, Enesim_Log **l)
{
	Enesim_Renderer_Gradient *thiz;
	Enesim_Renderer_Gradient_Class *klass;
	int len;

	thiz = ENESIM_RENDERER_GRADIENT(r);
	/* check that we have at least two stops */
	if (eina_list_count(thiz->state.stops) < 2)
	{
		ENESIM_RENDERER_LOG(r, l, "Less than two stops");
		return EINA_FALSE;
	}

	klass = ENESIM_RENDERER_GRADIENT_CLASS_GET(r);
	len = klass->length(r);
	if (len <= 0)
	{
		ENESIM_RENDERER_LOG(r, l, "Gradient length %d <= 0", len);
		return EINA_FALSE;
	}

	if (!thiz->sw.src || len != thiz->sw.len)
	{
		thiz->sw.len = len;
		if (thiz->sw.src)
			free(thiz->sw.src);
		thiz->sw.src = malloc(sizeof(uint32_t) * thiz->sw.len);
	}

	if (!_gradient_generate_1d_span(thiz, r, l))
	{
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

static void _gradient_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = ENESIM_RENDERER_GRADIENT(r);
	thiz->changed = EINA_FALSE;
	thiz->stops_changed = EINA_FALSE;
	thiz->past_mode = thiz->state.mode;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static void _gradient_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Gradient_Class *klass;

	/* the common clean up */
	_gradient_cleanup(r);

	/* the implementation clean up */
	klass = ENESIM_RENDERER_GRADIENT_CLASS_GET(r);
	if (klass->sw_cleanup)
	{
		klass->sw_cleanup(r, s);
	}
}

static Eina_Bool _gradient_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l)
{
	Enesim_Renderer_Gradient_Class *klass;

	klass = ENESIM_RENDERER_GRADIENT_CLASS_GET(r);

	/* first do the implementation setup */
	if (!klass->sw_setup(r, s, rop, fill, l))
	{
		ENESIM_RENDERER_LOG(r, l, "Gradient implementation failed");
		return EINA_FALSE;
	}

	/* now the common setup */
	if (!_gradient_setup(r, l))
	{
		if (klass->sw_cleanup)
			klass->sw_cleanup(r, s);
		return EINA_FALSE;
	}
	
	return EINA_TRUE;
}

static void _gradient_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Gradient *thiz;
	Enesim_Renderer_Gradient_Class *klass;

	thiz = ENESIM_RENDERER_GRADIENT(r);
	klass = ENESIM_RENDERER_GRADIENT_CLASS_GET(r);
	if (thiz->state.mode == ENESIM_REPEAT_MODE_RESTRICT && klass->bounds_get)
	{
		klass->bounds_get(r, bounds);
	}
	else
	{
		bounds->x = INT_MIN / 2;
		bounds->y = INT_MIN / 2;
		bounds->w = INT_MAX;
		bounds->h = INT_MAX;
	}
}

static void _gradient_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_PROJECTIVE |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static Eina_Bool _gradient_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;
	Enesim_Renderer_Gradient_Class *klass;
	Eina_Bool ret = EINA_TRUE;

	thiz = ENESIM_RENDERER_GRADIENT(r);
	ret = _gradient_changed(thiz);
	if (ret)
		goto done;
	klass = ENESIM_RENDERER_GRADIENT_CLASS_GET(r);
	if (klass->has_changed)
		ret = klass->has_changed(r);
done:
	return ret;
}

#if BUILD_OPENGL
static Eina_Bool _gradient_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Gradient *thiz;
	Enesim_Renderer_Gradient_Class *klass;

	/* first do the implementation setup */
	klass = ENESIM_RENDERER_GRADIENT_CLASS_GET(r);
	if (!klass->opengl_setup)
	{
		ENESIM_RENDERER_LOG(r, l, "Gradient implementation not found");
		return EINA_FALSE;
	}

	if (!klass->opengl_setup(r, s, rop, draw, l))
	{
		ENESIM_RENDERER_LOG(r, l, "Gradient implementation failed");
		return EINA_FALSE;
	}
	/* now the common setup */
	if (!_gradient_setup(r, l))
	{
		if (klass->opengl_cleanup)
			klass->opengl_cleanup(r, s);
		return EINA_FALSE;
	}
	/* upload the spans to a texture */
	thiz = ENESIM_RENDERER_GRADIENT(r);
	if (thiz->gl.len != thiz->sw.len)
	{
		thiz->gl.len = thiz->sw.len;
		if (thiz->gl.gen_stops)
			enesim_opengl_span_free(thiz->gl.gen_stops);
		thiz->gl.gen_stops = enesim_opengl_span_new(thiz->gl.len, thiz->sw.src);
	}

	return EINA_TRUE;
}

static void _gradient_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Gradient_Class *klass;

	/* the common clean up */
	_gradient_cleanup(r);

	/* the implementation clean up */
	klass = ENESIM_RENDERER_GRADIENT_CLASS_GET(r);
	if (klass->opengl_cleanup)
	{
		klass->opengl_cleanup(r, s);
	}
}
#endif
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_ABSTRACT_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Gradient, Enesim_Renderer_Gradient_Class,
		enesim_renderer_gradient);

static void _enesim_renderer_gradient_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->bounds_get = _gradient_bounds_get;
	klass->features_get = _gradient_features_get;
	klass->has_changed = _gradient_has_changed;
	klass->sw_setup = _gradient_sw_setup;
	klass->sw_cleanup = _gradient_sw_cleanup;
#if BUILD_OPENGL
	klass->opengl_setup = _gradient_opengl_setup;
	klass->opengl_cleanup = _gradient_opengl_cleanup;
#endif
}

static void _enesim_renderer_gradient_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_renderer_gradient_instance_deinit(void *o)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = ENESIM_RENDERER_GRADIENT(o);
	if (thiz->sw.src)
		free(thiz->sw.src);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
int enesim_renderer_gradient_natural_length_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;
	Enesim_Renderer_Gradient_Stop *curr_stop, *next_stop;
	Eina_List *curr, *next, *last;
	double dp = 1.0, small = 1.0 / 16384.0;

	thiz = ENESIM_RENDERER_GRADIENT(r);

	curr = thiz->state.stops;
	curr_stop = eina_list_data_get(curr);
	last = eina_list_last(thiz->state.stops);
	while (curr != last)
	{
		double del;

		next = eina_list_next(curr);
		next_stop = eina_list_data_get(next);

		del = next_stop->pos - curr_stop->pos;
		if ((del > small) && (del < dp))
			dp = del;

		curr = next;
		curr_stop = next_stop;
	}

	return 2 * ceil(1.0 / dp);
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Add a stop on a gradient renderer
 * @param[in] r The gradient renderer to add the stop in
 * @param[in] stop The gradient stop to add
 */
EAPI void enesim_renderer_gradient_stop_add(Enesim_Renderer *r, Enesim_Renderer_Gradient_Stop *stop)
{
	Enesim_Renderer_Gradient *thiz;
	Enesim_Renderer_Gradient_Stop *s;
	double pos;

	if (!stop) return;

	pos = stop->pos;
	if (pos < 0)
		pos = 0;
	else if (pos > 1)
		pos = 1;

	thiz = ENESIM_RENDERER_GRADIENT(r);
	s = malloc(sizeof(Enesim_Renderer_Gradient_Stop));
	s->argb = stop->argb;
	s->pos = pos;

	/* if pos == 0.0 set to first */
	if (pos == 0.0)
	{
		thiz->state.stops = eina_list_prepend(thiz->state.stops, s);
	}
	/* if pos == 1.0 set to last */
	else if (pos == 1.0)
	{
		thiz->state.stops = eina_list_append(thiz->state.stops, s);
	}
	/* else iterate until pos > prev && pos < next */
	else
	{
		Enesim_Renderer_Gradient_Stop *p;
		Eina_List *l;
		Eina_Bool found = EINA_FALSE;

		EINA_LIST_FOREACH(thiz->state.stops, l, p)
		{
			if (s->pos < p->pos)
			{
				found = EINA_TRUE;
				break;
			}
		}
		if (found)
			thiz->state.stops = eina_list_prepend_relative_list(thiz->state.stops, s, l);
		else
			thiz->state.stops = eina_list_append(thiz->state.stops, s);
	}
	thiz->stops_changed = EINA_TRUE;
}

/**
 * Clear every stop added on a gradient renderer
 * @param[in] r The gradient renderer to clear the stops on
 */
EAPI void enesim_renderer_gradient_stop_clear(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;
	Enesim_Renderer_Gradient_Stop *stop;
	Eina_List *l;
	Eina_List *l_next;

	thiz = ENESIM_RENDERER_GRADIENT(r);
	EINA_LIST_FOREACH_SAFE(thiz->state.stops, l, l_next, stop)
	{
		thiz->state.stops = eina_list_remove_list(thiz->state.stops, l);
		free(stop);
	}
	thiz->stops_changed = EINA_TRUE;
}

/**
 * Set the repeat mode of a gradient renderer
 * @param[in] r The gradient renderer to set the repeat mode on
 * @param[in] mode The repeat mode
 */
EAPI void enesim_renderer_gradient_repeat_mode_set(Enesim_Renderer *r,
		Enesim_Repeat_Mode mode)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = ENESIM_RENDERER_GRADIENT(r);
	thiz->state.mode = mode;
	thiz->changed = EINA_TRUE;
}

/**
 * Get the repeat mode of a gradient renderer
 * @param[in] r The gradient renderer to get the repeat mode from
 * @return The repeat mode
 */
EAPI Enesim_Repeat_Mode enesim_renderer_gradient_repeat_mode_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = ENESIM_RENDERER_GRADIENT(r);
	return thiz->state.mode;
}
