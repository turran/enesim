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
#include "enesim_error.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"

#include "enesim_renderer_private.h"
#include "enesim_vector_private.h"
#include "enesim_rasterizer_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RASTERIZER_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RASTERIZER_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RASTERIZER_MAGIC);\
	} while(0)

typedef struct _Enesim_Rasterizer
{
	EINA_MAGIC
	/* public */
	/* private */
	Enesim_Rasterizer_State state;
	void *data;
	/* interface */
	Enesim_Renderer_Base_Name_Get_Cb base_name_get;
	Enesim_Renderer_Delete free;
	Enesim_Rasterizer_Figure_Set figure_set;
	Enesim_Renderer_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
} Enesim_Rasterizer;

static inline Enesim_Rasterizer * _rasterizer_get(Enesim_Renderer *r)
{
	Enesim_Rasterizer *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	ENESIM_RASTERIZER_MAGIC_CHECK(thiz);

	return thiz;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _rasterizer_base_name_get(Enesim_Renderer *r)
{
	Enesim_Rasterizer *thiz;

	thiz = _rasterizer_get(r);
	if (!thiz->base_name_get) return "rasterizer";
	return thiz->base_name_get(r);
}

static Eina_Bool _rasterizer_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *draw, Enesim_Error **error)
{
	Enesim_Rasterizer *thiz;
	Eina_Bool ret;

	thiz = _rasterizer_get(r);
	if (!thiz->sw_setup) return EINA_FALSE;

	/* setup the renderer state */
	if (!enesim_renderer_state_setup(&thiz->state.rstate))
		goto err_renderer;
	/* setup the shape state */
	if (!enesim_renderer_shape_state_setup(&thiz->state.sstate))
		goto err_shape;
	/* finally call the interface */
	if (!thiz->sw_setup(r, s, draw, error))
		goto err_rasterizer;
	return EINA_TRUE;

err_rasterizer:
	enesim_renderer_shape_state_cleanup(&thiz->state.sstate);
err_shape:
	enesim_renderer_state_cleanup(&thiz->state.rstate);
err_renderer:
	return EINA_FALSE;
}

static void _rasterizer_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Rasterizer *thiz;

	thiz = _rasterizer_get(r);
	if (thiz->sw_cleanup)
		thiz->sw_cleanup(r, s);
	enesim_renderer_shape_state_cleanup(&thiz->state.sstate);
	enesim_renderer_state_cleanup(&thiz->state.rstate);
}

static void _rasterizer_free(Enesim_Renderer *r)
{
	Enesim_Rasterizer *thiz;

	thiz = _rasterizer_get(r);
	if (thiz->free) thiz->free(r);
	free(thiz);
}

static void _rasterizer_flags(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Flag *flags)
{
	/* FIXME we should use the rasterizer implementation flags */
	*flags = ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_PROJECTIVE |
			ENESIM_RENDERER_FLAG_ARGB8888;
}

static void _rasterizer_hints(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_COLORIZE;
}

static void _rasterizer_feature_get(Enesim_Renderer *r EINA_UNUSED, Enesim_Shape_Feature *features)
{
	*features = ENESIM_SHAPE_FLAG_FILL_RENDERER | ENESIM_SHAPE_FLAG_STROKE_RENDERER;
}

static Enesim_Renderer_Shape_Descriptor _descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .base_name_get = 		*/ _rasterizer_base_name_get,
	/* .free = 			*/ _rasterizer_free,
	/* .bounds_get = 		*/ NULL,
	/* .destination_bounds_get = 	*/ NULL,
	/* .flags_get = 		*/ NULL,
	/* .hints_get = 		*/ NULL,
	/* .is_inside = 		*/ NULL,
	/* .damages_get = 		*/ NULL,
	/* .has_changed = 		*/ NULL,
	/* .origin_x_set = 		*/ NULL,
	/* .origin_x_get = 		*/ NULL,
	/* .origin_y_set = 		*/ NULL,
	/* .origin_y_get = 		*/ NULL,
	/* .transformation_set = 	*/ NULL,
	/* .transformation_get = 	*/ NULL,
	/* .quality_set = 		*/ NULL,
	/* .quality_get = 		*/ NULL,
	/* .sw_setup = 			*/ NULL,
	/* .sw_cleanup = 		*/ NULL,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
	/* .opengl_initialize =         */ NULL,
	/* .opengl_setup =   		*/ NULL,
	/* .opengl_cleanup =        	*/ NULL,
	/* .feature_get = 		*/ NULL,
	/* .draw_mode_set = 		*/ NULL,
	/* .draw_mode_get = 		*/ NULL,
	/* .stroke_color_set = 		*/ NULL,
	/* .stroke_color_get = 		*/ NULL,
	/* .stroke_renderer_set = 	*/ NULL,
	/* .stroke_renderer_get = 	*/ NULL,
	/* .stroke_weight_set = 	*/ NULL,
	/* .stroke_weight_get = 	*/ NULL,
	/* .stroke_location_set = 	*/ NULL,
	/* .stroke_location_get = 	*/ NULL,
	/* .stroke_cap_set = 		*/ NULL,
	/* .stroke_cap_get = 		*/ NULL,
	/* .stroke_join_set = 		*/ NULL,
	/* .stroke_join_get = 		*/ NULL,
	/* .stroke_add = 		*/ NULL,
	/* .stroke_clear = 		*/ NULL,
	/* .fill_color_set = 		*/ NULL,
	/* .fill_color_get = 		*/ NULL,
	/* .fill_renderer_set = 	*/ NULL,
	/* .fill_renderer_get = 	*/ NULL,
	/* .fill_rule_set = 		*/ NULL,
	/* .fill_rule_get = 		*/ NULL,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_rasterizer_new(Enesim_Rasterizer_Descriptor *d, void *data)
{
	Enesim_Renderer *r;
	Enesim_Rasterizer *thiz;
	Enesim_Renderer_Shape_Descriptor descriptor;

	thiz = calloc(1, sizeof(Enesim_Rasterizer));
	if (!thiz) return NULL;

	descriptor = _descriptor;

	EINA_MAGIC_SET(thiz, ENESIM_RASTERIZER_MAGIC);
	thiz->base_name_get = d->base_name_get;
	thiz->free = d->free;
	thiz->figure_set = d->figure_set;
	thiz->data = data;

	r = enesim_renderer_shape_new(&descriptor, thiz);
	return r;
}

void * enesim_rasterizer_data_get(Enesim_Renderer *r)
{
	Enesim_Rasterizer *thiz;

	thiz = _rasterizer_get(r);
	return thiz->data;
}

void enesim_rasterizer_figure_set(Enesim_Renderer *r, const Enesim_Figure *f)
{
	Enesim_Rasterizer *thiz;

	thiz = _rasterizer_get(r);
	if (thiz->figure_set)
		thiz->figure_set(r, f);
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

