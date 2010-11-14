/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2010 Jorge Luis Zapata
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
typedef struct _Radial
{
	Enesim_Renderer_Gradient base;
	struct {
		float x, y;
	} center, radius;
} Radial;

static void _state_cleanup(Enesim_Renderer *r)
{

}

static Eina_Bool _state_setup(Enesim_Renderer *r, Enesim_Renderer_Sw_Fill *fill)
{
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * FIXME
 * To be documented
 */
EAPI Enesim_Renderer * enesim_renderer_gradient_radial_new(void)
{
	Radial *l;
	Enesim_Renderer *r;

	l = calloc(1, sizeof(Radial));

	r = (Enesim_Renderer *)l;
	enesim_renderer_gradient_init(r);
	r->sw_setup = _state_setup;
	r->sw_cleanup = _state_cleanup;

	return r;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_center_x_set(Enesim_Renderer *r, float v)
{
	Radial *rad = (Radial *)r;

	rad->center.x = v;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_center_y_set(Enesim_Renderer *r, float v)
{
	Radial *rad = (Radial *)r;

	rad->center.y = v;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_radius_y_set(Enesim_Renderer *r, float v)
{
	Radial *rad = (Radial *)r;

	rad->radius.y = v;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_radius_x_set(Enesim_Renderer *r, float v)
{
	Radial *rad = (Radial *)r;

	rad->radius.x = v;
}

