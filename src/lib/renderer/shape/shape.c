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
static inline Enesim_Renderer_Shape * _background_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = enesim_renderer_data_get(r);
	return thiz;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_shape_new(Enesim_Renderer_Descriptor *descriptor, void *data)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Shape *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Shape));
	thiz->data = data;
	if (!thiz) return NULL;
	r = enesim_renderer_new(descriptor, thiz);
	return r;
}

void enesim_renderer_shape_init(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *s = (Enesim_Renderer_Shape *)r;

	s->fill.color = 0xffffffff;
	s->stroke.color = 0xffffffff;
	enesim_renderer_init(r);
}

void enesim_renderer_shape_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *s = (Enesim_Renderer_Shape *)r;

	if (s->fill.rend && (s->draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL ||
			(s->draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL)))
	{
		enesim_renderer_relative_unset(r, s->fill.rend, &s->fill.original, s->fill.ox, s->fill.oy);
	}
}

Eina_Bool enesim_renderer_shape_setup(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *s = (Enesim_Renderer_Shape *)r;

	if (s->fill.rend && (s->draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL ||
			(s->draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL)))
	{
		enesim_renderer_relative_set(r, s->fill.rend, &s->fill.original, &s->fill.ox, &s->fill.oy);
	}
	return EINA_TRUE;
}

Eina_Bool enesim_renderer_shape_sw_setup(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *s = (Enesim_Renderer_Shape *)r;

	if (!enesim_renderer_shape_setup(r))
	{
		WRN("Shape setup on %p failed", r);
		return EINA_FALSE;
	}
	if (s->fill.rend && (s->draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL ||
			(s->draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL)))
	{
		if (!enesim_renderer_sw_setup(s->fill.rend))
		{
			WRN("Shape setup on fill renderer %p of %p failed", s->fill.rend, r);
			return EINA_FALSE;
		}
	}
	return EINA_TRUE;
}

void enesim_renderer_shape_sw_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *s = (Enesim_Renderer_Shape *)r;

	if (s->fill.rend && (s->draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL ||
			(s->draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL)))
	{
		enesim_renderer_sw_cleanup(s->fill.rend);
	}
	enesim_renderer_shape_cleanup(r);
}

void * enesim_renderer_shape_data_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *s = (Enesim_Renderer_Shape *)r;

	return s->data;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_outline_weight_set(Enesim_Renderer *r, double weight)
{
	Enesim_Renderer_Shape *s;

	if (weight < 1)
		weight = 1;
	s = (Enesim_Renderer_Shape *)r;
	s->stroke.weight = weight;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_outline_weight_get(Enesim_Renderer *r, double *weight)
{
	Enesim_Renderer_Shape *s;

	s = (Enesim_Renderer_Shape *)r;
	if (weight) *weight = s->stroke.weight;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_outline_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Shape *s;

	s = (Enesim_Renderer_Shape *)r;
	s->stroke.color = color;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_outline_renderer_set(Enesim_Renderer *r, Enesim_Renderer *outline)
{
	Enesim_Renderer_Shape *s;

	s = (Enesim_Renderer_Shape *)r;
	s->stroke.rend = outline;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Shape *s;

	s = (Enesim_Renderer_Shape *)r;
	s->fill.color = color;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_renderer_set(Enesim_Renderer *r, Enesim_Renderer *fill)
{
	Enesim_Renderer_Shape *s;

	s = (Enesim_Renderer_Shape *)r;
	s->fill.rend = fill;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_draw_mode_set(Enesim_Renderer *r, Enesim_Shape_Draw_Mode draw_mode)
{
	Enesim_Renderer_Shape *s;

	s = (Enesim_Renderer_Shape *)r;
	s->draw_mode = draw_mode;
}
