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
#include "enesim_eina.h"
#include "enesim_error.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"
#include "enesim_renderer_gradient.h"

#include "enesim_renderer_private.h"
#include "enesim_renderer_gradient_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_renderer_gradient

#define ENESIM_RENDERER_GRADIENT_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_GRADIENT_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_GRADIENT_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Gradient
{
	EINA_MAGIC
	/* properties */
	Enesim_Renderer_Gradient_State state;
	/* generated at state setup */
	Enesim_Renderer_Gradient_Sw_State sw;
	uint32_t *src;
	int slen;
	/* private */
	Enesim_Repeat_Mode past_mode;
	Eina_Bool changed : 1;
	Eina_Bool stops_changed : 1;
	Enesim_Renderer_Gradient_Descriptor *descriptor;
	Enesim_Renderer_Gradient_Sw_Draw draw;
	void *data;
} Enesim_Renderer_Gradient;

static inline Enesim_Renderer_Gradient * _gradient_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = enesim_renderer_data_get(r);
	ENESIM_RENDERER_GRADIENT_MAGIC_CHECK(thiz);

	return thiz;
}

static Eina_Bool _gradient_generate_1d_span(Enesim_Renderer_Gradient *thiz, Enesim_Renderer *r,
		Enesim_Error **error)
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
		ENESIM_RENDERER_ERROR(r, error, "No valid offset between stops");
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
		*dst++ = curr->argb;
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

static void _gradient_draw(Enesim_Renderer *r,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Gradient *thiz;
	Enesim_Renderer_Gradient_Sw_Draw_Data data;

	thiz = _gradient_get(r);

	data.gstate = &thiz->state;
	data.sw_state = &thiz->sw;

	thiz->draw(r, &data, x, y, len, ddata);
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static void _gradient_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	thiz->sw.len = 0;
	thiz->changed = EINA_FALSE;
	thiz->stops_changed = EINA_FALSE;
	thiz->past_mode = thiz->state.mode;

	if (thiz->descriptor->sw_cleanup)
	{
		thiz->descriptor->sw_cleanup(r, s);
	}
}

static Eina_Bool _gradient_state_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Gradient *thiz;
	int slen;

	thiz = _gradient_get(r);
	/* check that we have at least two stops */
	if (eina_list_count(thiz->state.stops) < 2)
	{
		ENESIM_RENDERER_ERROR(r, error, "Less than two stops");
		return EINA_FALSE;
	}
	/* always call our own fill */
	*fill = _gradient_draw;
	/* setup the implementation */
	if (!thiz->descriptor->sw_setup(r, &thiz->state, s, &thiz->draw, error))
	{
		ENESIM_RENDERER_ERROR(r, error, "Gradient implementation failed");
		return EINA_FALSE;
	}
	if (!thiz->draw)
	{
		ENESIM_RENDERER_ERROR(r, error, "Gradient implementation didnt return a draw function");
		return EINA_FALSE;
	}
	/* setup the matrix. TODO this should be done on every sw based renderer */
	enesim_matrix_f16p16_matrix_to(&thiz->state.state.current.transformation,
			&thiz->sw.matrix);
	/* get the length */
	slen = thiz->descriptor->length(r);
	if (slen <= 0)
	{
		ENESIM_RENDERER_ERROR(r, error, "Gradient length %d <= 0", slen);
		return EINA_FALSE;
	}
	if (!thiz->sw.src || slen != thiz->sw.len)
	{
		thiz->sw.len = slen;
		if (thiz->sw.src)
			free(thiz->sw.src);
		thiz->sw.src = malloc(sizeof(uint32_t) * thiz->sw.len);
	}
	if (!_gradient_generate_1d_span(thiz, r, error))
	{
		return EINA_FALSE;
	}

	return EINA_TRUE;
}

static void _gradient_bounds(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	if (thiz->state.mode == ENESIM_RESTRICT && thiz->descriptor->bounds)
	{
		thiz->descriptor->bounds(r, bounds);
	}
	else
	{
		bounds->x = INT_MIN / 2;
		bounds->y = INT_MIN / 2;
		bounds->w = INT_MAX;
		bounds->h = INT_MAX;
	}
}

static void _gradient_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	if (thiz->sw.src)
		free(thiz->sw.src);
	if (thiz->descriptor->free)
		thiz->descriptor->free(r);
	free(thiz);
}

static void _gradient_flags(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_PROJECTIVE |
			ENESIM_RENDERER_FLAG_ARGB8888;
}

static const char * _gradient_name(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	if (thiz->descriptor->name)
		return thiz->descriptor->name(r);
	else
		return "gradient";
}

static Eina_Bool _gradient_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;
	Eina_Bool ret = EINA_TRUE;

	thiz = _gradient_get(r);
	ret = _gradient_changed(thiz);
	if (ret)
		goto done;
	if (thiz->descriptor->has_changed)
		ret = thiz->descriptor->has_changed(r);
done:
	return ret;
}

#if BUILD_OPENGL
static Eina_Bool _gradient_opengl_initialize(Enesim_Renderer *r,
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***programs)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	if (!thiz->descriptor->opengl_initialize)
	{
		return EINA_TRUE;
	}
	return thiz->descriptor->opengl_initialize(r, num_programs, programs);
}

static Eina_Bool _gradient_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Error **error)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	if (!thiz->descriptor->opengl_setup)
	{
		return EINA_FALSE;
	}
	return thiz->descriptor->opengl_setup(r, s, draw, error);
}
#endif

static Enesim_Renderer_Descriptor _gradient_descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _gradient_name,
	/* .free = 			*/ _gradient_free,
	/* .bounds = 		*/ _gradient_bounds,
	/* .destination_bounds = 	*/ NULL,
	/* .flags = 			*/ _gradient_flags,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _gradient_has_changed,
	/* .sw_hints_get = 		*/ NULL,
	/* .sw_setup = 			*/ _gradient_state_setup,
	/* .sw_cleanup = 		*/ _gradient_state_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
#if BUILD_OPENGL
	/* .opengl_initialize = 	*/ _gradient_opengl_initialize,
	/* .opengl_setup =          	*/ _gradient_opengl_setup,
	/* .opengl_cleanup =        	*/ NULL
#else
	/* .opengl_initialize = 	*/ NULL,
	/* .opengl_setup = 		*/ NULL,
	/* .opengl_cleanup = 		*/ NULL
#endif
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_gradient_new(Enesim_Renderer_Gradient_Descriptor *gdescriptor, void *data)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Gradient *thiz;
	if (!gdescriptor->length)
	{
		ERR("No suitable gradient length function");
		return NULL;
	}

	thiz = calloc(1, sizeof(Enesim_Renderer_Gradient));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_GRADIENT_MAGIC);

	thiz->data = data;
	thiz->descriptor = gdescriptor;
	/* set default properties */
	thiz->state.stops = NULL;

	r = enesim_renderer_new(&_gradient_descriptor, thiz);
	return r;
}

void * enesim_renderer_gradient_data_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	return thiz->data;
}

int enesim_renderer_gradient_natural_length_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;
	Enesim_Renderer_Gradient_Stop *curr_stop, *next_stop;
	Eina_List *curr, *next, *last;
	double dp = 1.0, small = 1.0 / 16384.0;

	thiz = _gradient_get(r);

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
 * FIXME
 * To be documented
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

	thiz = _gradient_get(r);
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
		Eina_List *tmp;

		for (tmp = thiz->state.stops; tmp; tmp = eina_list_next(tmp))
		{
			Enesim_Renderer_Gradient_Stop *p = eina_list_data_get(tmp);

			if (p->pos > s->pos)
				break;
		}
		thiz->state.stops = eina_list_append_relative_list(thiz->state.stops, s, tmp);
	}
	thiz->stops_changed = EINA_TRUE;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_stop_clear(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;
	Enesim_Renderer_Gradient_Stop *stop;
	Eina_List *l;
	Eina_List *l_next;

	thiz = _gradient_get(r);
	EINA_LIST_FOREACH_SAFE(thiz->state.stops, l, l_next, stop)
	{
		thiz->state.stops = eina_list_remove_list(thiz->state.stops, l);
		free(stop);
	}
	thiz->stops_changed = EINA_TRUE;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_stop_set(Enesim_Renderer *r,
		Eina_List *list)
{
	Enesim_Renderer_Gradient_Stop *stop;
	Eina_List *l;

	EINA_LIST_FOREACH(list, l, stop)
	{
		enesim_renderer_gradient_stop_add(r, stop);
	}
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_mode_set(Enesim_Renderer *r,
		Enesim_Repeat_Mode mode)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	thiz->state.mode = mode;
	thiz->changed = EINA_TRUE;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_mode_get(Enesim_Renderer *r, Enesim_Repeat_Mode *mode)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	*mode = thiz->state.mode;
}
