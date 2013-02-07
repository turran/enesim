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
#include "enesim_renderer_shape.h"
#include "enesim_renderer_path.h"

#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_shape_simple_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Renderer_Shape_Simple
{
	Enesim_Renderer *path;
	Enesim_Renderer_Shape_Simple_Descriptor *descriptor;
	void *data;
} Enesim_Renderer_Shape_Simple;

static inline Enesim_Renderer_Shape_Simple * _shape_simple_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	return thiz;
}
/*----------------------------------------------------------------------------*
 *                               Span functions                               *
 *----------------------------------------------------------------------------*/
/* Use the internal path for drawing */
static void _shape_simple_path_span(Enesim_Renderer *r,
		const Enesim_Renderer_State *state EINA_UNUSED,
		const Enesim_Renderer_Shape_State *sstate EINA_UNUSED,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
	enesim_renderer_sw_draw(thiz->path, x, y, len, ddata);
}

#if BUILD_OPENGL
static void _shape_simple_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		const Eina_Rectangle *area, int w, int h)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
	enesim_renderer_opengl_draw(thiz->path, s, area, w, h);
}
#endif
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _shape_simple_name(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
	if (thiz->descriptor->name)
		return thiz->descriptor->name(r);
	else
		return "shape_simple";
}

static Eina_Bool _shape_simple_sw_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES] EINA_UNUSED,
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES] EINA_UNUSED,
		Enesim_Surface *s,
		Enesim_Renderer_Shape_Sw_Draw *draw, Enesim_Error **error)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
	thiz->descriptor->generate(r, thiz->path);
	if (!enesim_renderer_setup(thiz->path, s, error))
		return EINA_FALSE;
	*draw = _shape_simple_path_span;
	return EINA_TRUE;
}

static void _shape_simple_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
	enesim_renderer_cleanup(thiz->path, s);
}

static void _shape_simple_flags(Enesim_Renderer *r EINA_UNUSED, const Enesim_Renderer_State *state EINA_UNUSED,
		Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_AFFINE |
		ENESIM_RENDERER_FLAG_ARGB8888;
}

static void _shape_simple_hints(Enesim_Renderer *r, const Enesim_Renderer_State *state EINA_UNUSED,
		Enesim_Renderer_Hint *hints)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
	enesim_renderer_hints_get(thiz->path, hints);
}

static void _shape_simple_feature_get(Enesim_Renderer *r, Enesim_Shape_Feature *features)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
	if (thiz->descriptor->feature_get)
		thiz->descriptor->feature_get(r, features);
	else
		enesim_renderer_shape_feature_get(thiz->path, features);
}

static void _shape_simple_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
	if (thiz->descriptor->free)
		thiz->descriptor->free(r);
	enesim_renderer_unref(thiz->path);
	free(thiz);
}

static void _shape_simple_bounds(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
	if (thiz->descriptor->bounds)
	{
		thiz->descriptor->bounds(r, states, sstates, bounds);
	}
	else
	{
		thiz->descriptor->generate(r, thiz->path);
		enesim_renderer_bounds(thiz->path, bounds);
	}
}

static void _shape_simple_destination_bounds(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Eina_Rectangle *bounds)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
	if (thiz->descriptor->destination_bounds)
	{
		thiz->descriptor->destination_bounds(r, states, sstates, bounds);
	}
	else
	{
		thiz->descriptor->generate(r, thiz->path);
		enesim_renderer_destination_bounds(thiz->path, bounds, 0, 0);
	}
}

static Eina_Bool _shape_simple_has_changed(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES] EINA_UNUSED)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
	if (enesim_renderer_has_changed(thiz->path))
		return EINA_TRUE;
	if (thiz->descriptor->has_changed)
		return thiz->descriptor->has_changed(r, states);
	return EINA_FALSE;
}

#if BUILD_OPENGL
static Eina_Bool _shape_simple_opengl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES] EINA_UNUSED,
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES] EINA_UNUSED,
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Error **error)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
	if (!enesim_renderer_setup(thiz->path, s, error))
		return EINA_FALSE;

	*draw = _shape_simple_opengl_draw;
	return EINA_TRUE;
}

static void _shape_simple_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
	enesim_renderer_cleanup(thiz->path, s);
}
#endif

#if 0
/* Once the shape is done */
static void _shape_simple_stroke_weight_set(Enesim_Renderer *r, double weight)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_stroke_weight_get(Enesim_Renderer *r, double *weight)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_stroke_location_set(Enesim_Renderer *r, Enesim_Shape_Stroke_Location location)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_stroke_location_get(Enesim_Renderer *r, Enesim_Shape_Stroke_Location *location)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_stroke_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_stroke_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_stroke_cap_set(Enesim_Renderer *r, Enesim_Shape_Stroke_Cap cap)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_stroke_cap_get(Enesim_Renderer *r, Enesim_Shape_Stroke_Cap *cap)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_stroke_join_set(Enesim_Renderer *r, Enesim_Shape_Stroke_Join join)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_stroke_join_get(Enesim_Renderer *r, Enesim_Shape_Stroke_Join *join)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_stroke_renderer_set(Enesim_Renderer *r, Enesim_Renderer *stroke)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_stroke_renderer_get(Enesim_Renderer *r, Enesim_Renderer **stroke)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_fill_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_fill_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_fill_renderer_set(Enesim_Renderer *r, Enesim_Renderer *fill)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_fill_renderer_get(Enesim_Renderer *r, Enesim_Renderer **fill)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_fill_rule_set(Enesim_Renderer *r, Enesim_Shape_Fill_Rule rule)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_fill_rule_get(Enesim_Renderer *r, Enesim_Shape_Fill_Rule *rule)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_draw_mode_set(Enesim_Renderer *r, Enesim_Shape_Draw_Mode draw_mode)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_draw_mode_get(Enesim_Renderer *r, Enesim_Shape_Draw_Mode *draw_mode)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_feature_get(Enesim_Renderer *r, Enesim_Shape_Feature *features)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);

}

static void _shape_simple_stroke_dash_add(Enesim_Renderer *r,
		const Enesim_Shape_Stroke_Dash *dash)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}

static void _shape_simple_stroke_dash_clear(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
}
#endif

static Enesim_Renderer_Shape_Descriptor _shape_simple_descriptor = {
	/* .name = 			*/ _shape_simple_name,
	/* .free = 			*/ _shape_simple_free,
	/* .bounds = 			*/ _shape_simple_bounds,
	/* .destination_bounds = 	*/ _shape_simple_destination_bounds,
	/* .flags = 			*/ _shape_simple_flags,
	/* .hints_get = 		*/ _shape_simple_hints,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _shape_simple_has_changed,
	/* .feature_get =		*/ _shape_simple_feature_get,
	/* .sw_setup = 			*/ _shape_simple_sw_setup,
	/* .sw_cleanup = 		*/ _shape_simple_sw_cleanup,
#if BUILD_OPENCL
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
#else
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
#endif
#if BUILD_OPENGL
	/* .opengl_initialize =         */ NULL,
	/* .opengl_setup =          	*/ _shape_simple_opengl_setup,
	/* .opengl_cleanup =        	*/ _shape_simple_opengl_cleanup,
#else
	/* .opengl_initialize =         */ NULL,
	/* .opengl_setup =          	*/ NULL,
	/* .opengl_cleanup =        	*/ NULL
#endif
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_shape_simple_new(
		Enesim_Renderer_Shape_Simple_Descriptor *descriptor, void *data)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Shape_Simple));
	if (!thiz) return NULL;
	r = enesim_renderer_path_new();
	if (!r)
	{
		free(thiz);
		return NULL;
	}
	thiz->path = r;
	thiz->data = data;
	thiz->descriptor = descriptor;

	r = enesim_renderer_shape_new(&_shape_simple_descriptor, thiz);
	return r;
}

void * enesim_renderer_shape_simple_data_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape_Simple *thiz;

	thiz = _shape_simple_get(r);
	return thiz->data;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

