/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2008 Jorge Luis Zapata
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
typedef struct _Stop
{
	Enesim_Color color;
	double pos;
	/* TODO replace float with Eina_F16p16 */
} Stop;

static inline Enesim_Renderer_Gradient * _gradient_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = enesim_renderer_data_get(r);
	return thiz;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_gradient_new(Enesim_Renderer_Descriptor *descriptor, void *data)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Gradient *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Gradient));
	thiz->data = data;
	if (!thiz) return NULL;
	/* set default properties */
	thiz->stops = NULL;

	r = enesim_renderer_new(descriptor, thiz);
	return r;
}

void enesim_renderer_gradient_state_setup(Enesim_Renderer *r, int len)
{
	Enesim_Renderer_Gradient *thiz;
	Eina_List *tmp;
	Stop *curr, *next;
	Eina_F16p16 xx, inc;
	int end = len;
	uint32_t *dst;

	thiz = _gradient_get(r);
	/* TODO check that we have at least two stops */
	/* TODO check that we have one at 0 and one at 1 */
	curr = eina_list_data_get(thiz->stops);
	tmp = eina_list_next(thiz->stops);
	next = eina_list_data_get(tmp);
	/* Check that we dont divide by 0 */
	inc = eina_f16p16_float_from(1.0 / ((next->pos - curr->pos) * len));
	xx = 0;

	thiz->src = dst = malloc(sizeof(uint32_t) * len);
	memset(thiz->src, 0xff, len);
	/* FIXME Im not sure if we increment xx by the 1 / ((next - curr) * len) value
	 * as it might not be too accurate
	 */
	while (end--)
	{
		uint16_t off;
		uint32_t p0;

		/* advance the curr and next */
		if (xx >= 65536)
		{
			tmp = eina_list_next(tmp);
			curr = next;
			next = eina_list_data_get(tmp);
			inc = eina_f16p16_float_from(1.0 / ((next->pos - curr->pos) * len));
			xx = 0;
		}
		off = 1 + (eina_f16p16_fracc_get(xx) >> 8);
		p0 = argb8888_interp_256(off, next->color, curr->color);
		*dst++ = p0;
		xx += inc;
	}
	thiz->slen = len;
}

void enesim_renderer_gradient_pixels_get(Enesim_Renderer *r, uint32_t **pixels, unsigned int *len)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	*pixels = thiz->src;
	*len = thiz->slen;
}

void * enesim_renderer_gradient_data_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	return thiz->data;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_stop_add(Enesim_Renderer *r, Enesim_Color c,
		double pos)
{
	Enesim_Renderer_Gradient *thiz;
	Stop *s;

	if (pos < 0)
		pos = 0;
	else if (pos > 1)
		pos = 1;

	thiz = _gradient_get(r);
	s = malloc(sizeof(Stop));
	s->color = c;
	s->pos = pos;
	/* if pos == 0.0 set to first */
	if (pos == 0.0)
	{
		thiz->stops = eina_list_prepend(thiz->stops, s);
	}
	/* if pos == 1.0 set to last */
	else if (pos == 1.0)
	{
		thiz->stops = eina_list_append(thiz->stops, s);
	}
	/* else iterate until pos > prev && pos < next */
	else
	{
		Eina_List *tmp;

		for (tmp = thiz->stops; tmp; tmp = eina_list_next(tmp))
		{
			Stop *p = eina_list_data_get(tmp);

			if (p->pos > s->pos)
				break;
		}
		thiz->stops = eina_list_append_relative_list(thiz->stops, s, tmp);
	}
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_clear(Enesim_Renderer *r)
{

}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_stop_set(Enesim_Renderer *r,
		Eina_List *list)
{
	Enesim_Renderer_Gradient *thiz;
	Enesim_Renderer_Gradient_Stop *stop;
	Eina_List *l;

	thiz = _gradient_get(r);
	EINA_LIST_FOREACH(list, l, stop)
	{
		enesim_renderer_gradient_stop_add(r, stop->color, stop->pos);
	}
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_mode_set(Enesim_Renderer *r,
		Enesim_Renderer_Gradient_Mode mode)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	thiz->mode = mode;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_mode_get(Enesim_Renderer *r, Enesim_Renderer_Gradient_Mode *mode)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	*mode = thiz->mode;
}
