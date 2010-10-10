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
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
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
		enesim_renderer_relative_unset(r, s->fill.rend, &s->fill.original);
	}
}

Eina_Bool enesim_renderer_shape_setup(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *s = (Enesim_Renderer_Shape *)r;

	if (s->fill.rend && (s->draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL ||
			(s->draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL)))
	{
		enesim_renderer_relative_set(r, s->fill.rend, &s->fill.original);
	}
	return EINA_TRUE;
}

Eina_Bool enesim_renderer_shape_sw_setup(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape *s = (Enesim_Renderer_Shape *)r;

	if (!enesim_renderer_shape_setup(r))
		return EINA_FALSE;

	if (s->fill.rend && (s->draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL ||
			(s->draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL)))
	{
		if (!enesim_renderer_sw_setup(s->fill.rend))
			return EINA_FALSE;
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
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_shape_outline_weight_set(Enesim_Renderer *r, float weight)
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
