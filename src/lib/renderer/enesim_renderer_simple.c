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
#include "enesim_renderer_simple_private.h"
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
typedef struct _Enesim_Renderer_Simple_Descriptor_Private
{
	Enesim_Renderer_Delete_Cb free;
	Enesim_Renderer_Has_Changed_Cb has_changed;
} Enesim_Renderer_Simple_Descriptor_Private;

typedef struct _Enesim_Renderer_Simple
{
	Enesim_Renderer_State state;
	Enesim_Renderer_Simple_Descriptor_Private descriptor;
	void *data;
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
static void _simple_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	if (thiz->descriptor.free)
		thiz->descriptor.free(r);
	enesim_renderer_state_clear(&thiz->state);
	free(thiz);
}

static void _simple_transformation_set(Enesim_Renderer *r,
		const Enesim_Matrix *m)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_transformation_set(&thiz->state, m);
}

static void _simple_transformation_get(Enesim_Renderer *r,
		Enesim_Matrix *m)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_transformation_get(&thiz->state, m);
}

static void _simple_rop_set(Enesim_Renderer *r,
		Enesim_Rop rop)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_rop_set(&thiz->state, rop);
}

static void _simple_rop_get(Enesim_Renderer *r,
		Enesim_Rop *rop)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_rop_get(&thiz->state, rop);
}

static void _simple_visibility_set(Enesim_Renderer *r,
		Eina_Bool visibility)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_visibility_set(&thiz->state, visibility);
}

static void _simple_visibility_get(Enesim_Renderer *r,
		Eina_Bool *visibility)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_visibility_get(&thiz->state, visibility);
}

static void _simple_color_set(Enesim_Renderer *r,
		Enesim_Color color)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_color_set(&thiz->state, color);
}

static void _simple_color_get(Enesim_Renderer *r,
		Enesim_Color *color)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_color_get(&thiz->state, color);
}

static void _simple_x_set(Enesim_Renderer *r,
		double x)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_x_set(&thiz->state, x);
}

static void _simple_x_get(Enesim_Renderer *r,
		double *x)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_x_get(&thiz->state, x);
}

static void _simple_y_set(Enesim_Renderer *r,
		double y)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_y_set(&thiz->state, y);
}

static void _simple_y_get(Enesim_Renderer *r,
		double *y)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_y_get(&thiz->state, y);
}

static Eina_Bool _simple_has_changed(Enesim_Renderer *r,
		const Enesim_Renderer_State *state[ENESIM_RENDERER_STATES])
{
	Enesim_Renderer_Simple *thiz;
	Eina_Bool ret;

	thiz = _simple_get(r);
	ret = enesim_renderer_state_changed(&thiz->state);
	if (ret) return ret;

	if (thiz->descriptor.has_changed)
		ret = thiz->descriptor.has_changed(r, state);

	return ret;
}

Enesim_Renderer_Descriptor _simple_descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ NULL,
	/* .free = 			*/ _simple_free,
	/* .bounds = 			*/ NULL,
	/* .destination_bounds =	*/ NULL,
	/* .flags = 			*/ NULL,
	/* .hints_get = 		*/ NULL,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _simple_has_changed,
	/* .sw_setup = 			*/ NULL,
	/* .sw_cleanup = 		*/ NULL,
	/* .opencl_setup = 		*/ NULL,
	/* .opencl_kernel_setup = 	*/ NULL,
	/* .opencl_cleanup = 		*/ NULL,
	/* .opengl_initialize = 	*/ NULL,
	/* .opengl_setup = 		*/ NULL,
	/* .opengl_cleanup = 		*/ NULL
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_simple_new(
		Enesim_Renderer_Simple_Descriptor *descriptor, void *data)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Descriptor d;
	Enesim_Renderer_Simple *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Simple));
	if (!thiz) return NULL;
	thiz->data = data;

	d = _simple_descriptor;
	d.name = descriptor->name_get;
	d.bounds = descriptor->bounds_get;
	d.destination_bounds = descriptor->destination_bounds_get;
	d.flags = descriptor->flags_get;
	d.hints_get = descriptor->hints_get;
	d.damage = descriptor->damages_get;
	d.sw_setup = descriptor->sw_setup;
	d.sw_cleanup = descriptor->sw_cleanup;
	d.opencl_setup = descriptor->opencl_setup;
	d.opencl_kernel_setup = descriptor->opencl_kernel_setup;
	d.opencl_cleanup = descriptor->opencl_cleanup;
	d.opengl_initialize = descriptor->opengl_initialize;
	d.opengl_setup = descriptor->opengl_setup;
	d.opengl_cleanup = descriptor->opengl_cleanup;

	thiz->descriptor.free = descriptor->free;
	thiz->descriptor.has_changed = descriptor->has_changed;

	r = enesim_renderer_new(&d, thiz);
	return r;
}

void * enesim_renderer_simple_data_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	return thiz->data;
}

void enesim_renderer_simple_state_get(Enesim_Renderer *r,
		const Enesim_Renderer_State **state)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	*state = thiz->state.
}

void enesim_renderer_simple_transformation_type_get(Enesim_Renderer *r,
		Enesim_Matrix_Type *type)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	*type = thiz->state.transformation_type;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

