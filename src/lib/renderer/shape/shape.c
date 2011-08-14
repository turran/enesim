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
static inline Enesim_Renderer_Shape * _shape_get(Enesim_Renderer *r)
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
	/* set default properties */
	thiz->fill.color = 0xffffffff;
	thiz->stroke.color = 0xffffffff;

	r = enesim_renderer_new(descriptor, thiz);
	return r;
}

void enesim_renderer_shape_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->fill.rend && (thiz->draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL ||
			(thiz->draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL)))
	{
		enesim_renderer_relative_unset(r, thiz->fill.rend, &thiz->fill.original, thiz->fill.ox, thiz->fill.oy);
	}
}

Eina_Bool enesim_renderer_shape_setup(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->fill.rend && (thiz->draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL ||
			(thiz->draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL)))
	{
		enesim_renderer_relative_set(r, thiz->fill.rend, &thiz->fill.original, &thiz->fill.ox, &thiz->fill.oy);
	}
	return EINA_TRUE;
}

Eina_Bool enesim_renderer_shape_sw_setup(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (!enesim_renderer_shape_setup(r))
	{
		WRN("Shape setup on %p failed", r);
		return EINA_FALSE;
	}
	if (thiz->fill.rend && (thiz->draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL ||
			(thiz->draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL)))
	{
		if (!enesim_renderer_sw_setup(thiz->fill.rend))
		{
			WRN("Shape setup on fill renderer %p of %p failed", thiz->fill.rend, r);
			return EINA_FALSE;
		}
	}
	return EINA_TRUE;
}

void enesim_renderer_shape_sw_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->fill.rend && (thiz->draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL ||
			(thiz->draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL)))
	{
		enesim_renderer_sw_cleanup(thiz->fill.rend);
	}
	enesim_renderer_shape_cleanup(r);
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

	if (weight < 1)
		weight = 1;
	thiz = _shape_get(r);
	thiz->stroke.weight = weight;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_weight_get(Enesim_Renderer *r, double *weight)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (weight) *weight = thiz->stroke.weight;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	thiz->stroke.color = color;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	*color = thiz->stroke.color;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_renderer_set(Enesim_Renderer *r, Enesim_Renderer *stroke)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	thiz->stroke.rend = stroke;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_stroke_renderer_get(Enesim_Renderer *r, Enesim_Renderer **stroke)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	*stroke = thiz->stroke.rend;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	thiz->fill.color = color;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	*color = thiz->fill.color;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_fill_renderer_set(Enesim_Renderer *r, Enesim_Renderer *fill)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	if (thiz->fill.rend)
		enesim_renderer_unref(thiz->fill.rend)
	thiz->fill.rend = fill;
	if (thiz->fill.rend)
		thiz->fill.rend = enesim_renderer_ref(thiz->fill.rend);
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
	*fill = thiz->fill.rend;
	if (thiz->fill.rend)
		thiz->fill.rend = enesim_renderer_ref(thiz->fill.rend);
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_draw_mode_set(Enesim_Renderer *r, Enesim_Shape_Draw_Mode draw_mode)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	thiz->draw_mode = draw_mode;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_draw_mode_get(Enesim_Renderer *r, Enesim_Shape_Draw_Mode *draw_mode)
{
	Enesim_Renderer_Shape *thiz;

	thiz = _shape_get(r);
	*draw_mode = thiz->draw_mode;
}

