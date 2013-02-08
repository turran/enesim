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
/* We need to start removing all the extra logic in the renderer and move it
 * here for simple usage
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

#include "enesim_renderer_private.h"
/* We should not implement:
 * damages
 * bounds
 * destination bounds
 * hints
 * flags
 * We should implement:
 * changed
 *
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Renderer_Simple
{
	Enesim_Renderer_State2 state;
} Enesim_Renderer_Simple;

static inline Enesim_Renderer_Simple * _simple_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Simple *thiz;

	thiz = enesim_renderer_data_get(r);
	return thiz;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static void _enesim_renderer_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_clear(&thiz->state);
	free(thiz);
}

static void _enesim_renderer_transformation_set(Enesim_Renderer *r,
		const Enesim_Matrix *m)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_transformation_set(&thiz->state, m);
}

static void _enesim_renderer_transformation_get(Enesim_Renderer *r,
		Enesim_Matrix *m)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_transformation_get(&thiz->state, m);
}

static void _enesim_renderer_rop_set(Enesim_Renderer *r,
		Enesim_Rop rop)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_rop_set(&thiz->state, rop);
}

static void _enesim_renderer_rop_get(Enesim_Renderer *r,
		Enesim_Rop *rop)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_rop_get(&thiz->state, rop);
}

static void _enesim_renderer_visibility_set(Enesim_Renderer *r,
		Eina_Bool visibility)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_visibility_set(&thiz->state, visibility);
}

static void _enesim_renderer_visibility_get(Enesim_Renderer *r,
		Eina_Bool *visibility)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_visibility_get(&thiz->state, visibility);
}

static void _enesim_renderer_color_set(Enesim_Renderer *r,
		Enesim_Color color)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_color_set(&thiz->state, color);
}

static void _enesim_renderer_color_get(Enesim_Renderer *r,
		Enesim_Color *color)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_color_get(&thiz->state, color);
}

static void _enesim_renderer_x_set(Enesim_Renderer *r,
		double x)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_x_set(&thiz->state, x);
}

static void _enesim_renderer_x_get(Enesim_Renderer *r,
		double *x)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_x_get(&thiz->state, x);
}

static void _enesim_renderer_y_set(Enesim_Renderer *r,
		double y)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_y_set(&thiz->state, y);
}

static void _enesim_renderer_y_get(Enesim_Renderer *r,
		double *y)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_y_get(&thiz->state, y);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

