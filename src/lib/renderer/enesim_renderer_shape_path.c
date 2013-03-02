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
#include "enesim_renderer_shape_path_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Renderer_Shape_Path
{
	Enesim_Renderer *path;
	Enesim_Renderer_Shape_Path_Descriptor *descriptor;
	void *data;
} Enesim_Renderer_Shape_Path;

static inline Enesim_Renderer_Shape_Path * _shape_path_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	return thiz;
}

static Eina_Bool _shape_path_setup(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Error **error)
{
	Enesim_Renderer_Shape_Path *thiz;
	Enesim_Renderer *fill, *stroke;
	const Enesim_Renderer_State *rstate;
	const Enesim_Renderer_Shape_State *sstate;

	thiz = _shape_path_get(r);
	rstate = enesim_renderer_state_get(r);
	sstate = enesim_renderer_shape_state_get(r);

	/* common properties */
	enesim_renderer_color_set(thiz->path, rstate->current.color);
	enesim_renderer_origin_set(thiz->path, rstate->current.ox, rstate->current.oy);
	enesim_renderer_transformation_set(thiz->path, &rstate->current.transformation);

	/* shape properties */
	fill = sstate->current.fill.r;
	if (fill) fill = enesim_renderer_ref(fill);
	enesim_renderer_shape_fill_renderer_set(thiz->path, fill);
	enesim_renderer_shape_fill_color_set(thiz->path, sstate->current.fill.color);
	enesim_renderer_shape_fill_rule_set(thiz->path, sstate->current.fill.rule);

	stroke = sstate->current.stroke.r;
	if (stroke) stroke = enesim_renderer_ref(stroke);
	enesim_renderer_shape_stroke_renderer_set(thiz->path, stroke);
	enesim_renderer_shape_stroke_weight_set(thiz->path, sstate->current.stroke.weight);
	enesim_renderer_shape_stroke_color_set(thiz->path, sstate->current.stroke.color);
	enesim_renderer_shape_stroke_cap_set(thiz->path, sstate->current.stroke.cap);
	enesim_renderer_shape_stroke_join_set(thiz->path, sstate->current.stroke.join);
	/* FIXME in fact the path renderer does not support the stroke location */
	enesim_renderer_shape_stroke_location_set(thiz->path, sstate->current.stroke.location);

	if (sstate->stroke_dashes_changed)
	{
		Enesim_Shape_Stroke_Dash *d;
		const Eina_List *l;

		enesim_renderer_shape_stroke_dash_clear(thiz->path);
		EINA_LIST_FOREACH (sstate->stroke_dashes, l, d)
		{
			enesim_renderer_shape_stroke_dash_add(thiz->path, d);
		}
	}

	enesim_renderer_shape_draw_mode_set(thiz->path, sstate->current.draw_mode);

	/* now let the implementation override whatever it wants to */
	if (thiz->descriptor->setup)
		if (!thiz->descriptor->setup(r, thiz->path, error))
			return EINA_FALSE;

	if (!enesim_renderer_setup(thiz->path, s, error))
	{
		if (thiz->descriptor->cleanup)
			thiz->descriptor->cleanup(r);
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

static void _shape_path_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
	enesim_renderer_cleanup(thiz->path, s);
	if (thiz->descriptor->cleanup)
		thiz->descriptor->cleanup(r);
}
/*----------------------------------------------------------------------------*
 *                               Span functions                               *
 *----------------------------------------------------------------------------*/
/* Use the internal path for drawing */
static void _shape_path_path_span(Enesim_Renderer *r,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
	enesim_renderer_sw_draw(thiz->path, x, y, len, ddata);
}

#if BUILD_OPENGL
static void _shape_path_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		const Eina_Rectangle *area, int w, int h)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
	enesim_renderer_opengl_draw(thiz->path, s, area, w, h);
}
#endif
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _shape_path_name(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
	if (thiz->descriptor->name)
		return thiz->descriptor->name(r);
	else
		return "shape_simple";
}

static Eina_Bool _shape_path_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *draw, Enesim_Error **error)
{
	if (!_shape_path_setup(r, s, error))
		return EINA_FALSE;
	*draw = _shape_path_path_span;
	return EINA_TRUE;
}

static void _shape_path_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	_shape_path_cleanup(r, s);
}

static void _shape_path_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_AFFINE |
		ENESIM_RENDERER_FEATURE_ARGB8888;
}

static void _shape_path_sw_hints(Enesim_Renderer *r,
		Enesim_Renderer_Sw_Hint *hints)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
	enesim_renderer_sw_hints_get(thiz->path, hints);
}

static void _shape_path_shape_features_get(Enesim_Renderer *r, Enesim_Shape_Feature *features)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
	if (thiz->descriptor->shape_features_get)
		thiz->descriptor->shape_features_get(r, features);
	else
		enesim_renderer_shape_features_get(thiz->path, features);
}

static void _shape_path_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
	if (thiz->descriptor->free)
		thiz->descriptor->free(r);
	enesim_renderer_unref(thiz->path);
	free(thiz);
}

static void _shape_path_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
	if (thiz->descriptor->bounds_get)
	{
		thiz->descriptor->bounds_get(r, bounds);
	}
	else
	{
		thiz->descriptor->setup(r, thiz->path, NULL);
		enesim_renderer_bounds(thiz->path, bounds);
	}
}

static Eina_Bool _shape_path_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
	if (enesim_renderer_has_changed(thiz->path))
		return EINA_TRUE;
	if (thiz->descriptor->has_changed)
		return thiz->descriptor->has_changed(r);
	return EINA_FALSE;
}

#if BUILD_OPENGL
static Eina_Bool _shape_path_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Error **error)
{
	if (!_shape_path_setup(r, s, error))
		return EINA_FALSE;

	*draw = _shape_path_opengl_draw;
	return EINA_TRUE;
}

static void _shape_path_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	_shape_path_cleanup(r, s);
}
#endif

#if 0
/* Once the shape is done */
static void _shape_path_stroke_weight_set(Enesim_Renderer *r, double weight)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_stroke_weight_get(Enesim_Renderer *r, double *weight)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_stroke_location_set(Enesim_Renderer *r, Enesim_Shape_Stroke_Location location)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_stroke_location_get(Enesim_Renderer *r, Enesim_Shape_Stroke_Location *location)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_stroke_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_stroke_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_stroke_cap_set(Enesim_Renderer *r, Enesim_Shape_Stroke_Cap cap)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_stroke_cap_get(Enesim_Renderer *r, Enesim_Shape_Stroke_Cap *cap)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_stroke_join_set(Enesim_Renderer *r, Enesim_Shape_Stroke_Join join)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_stroke_join_get(Enesim_Renderer *r, Enesim_Shape_Stroke_Join *join)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_stroke_renderer_set(Enesim_Renderer *r, Enesim_Renderer *stroke)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_stroke_renderer_get(Enesim_Renderer *r, Enesim_Renderer **stroke)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_fill_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_fill_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_fill_renderer_set(Enesim_Renderer *r, Enesim_Renderer *fill)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_fill_renderer_get(Enesim_Renderer *r, Enesim_Renderer **fill)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_fill_rule_set(Enesim_Renderer *r, Enesim_Shape_Fill_Rule rule)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_fill_rule_get(Enesim_Renderer *r, Enesim_Shape_Fill_Rule *rule)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_draw_mode_set(Enesim_Renderer *r, Enesim_Shape_Draw_Mode draw_mode)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_draw_mode_get(Enesim_Renderer *r, Enesim_Shape_Draw_Mode *draw_mode)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_shape_features_get(Enesim_Renderer *r, Enesim_Shape_Feature *features)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);

}

static void _shape_path_stroke_dash_add(Enesim_Renderer *r,
		const Enesim_Shape_Stroke_Dash *dash)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}

static void _shape_path_stroke_dash_clear(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
}
#endif

static Enesim_Renderer_Shape_Descriptor _shape_path_descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _shape_path_name,
	/* .free = 			*/ _shape_path_free,
	/* .bounds_get = 		*/ _shape_path_bounds_get,
	/* .features_get = 		*/ _shape_path_features_get,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _shape_path_has_changed,
	/* .sw_hints_get = 		*/ _shape_path_sw_hints,
	/* .sw_setup = 			*/ _shape_path_sw_setup,
	/* .sw_cleanup = 		*/ _shape_path_sw_cleanup,
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
	/* .opengl_setup =          	*/ _shape_path_opengl_setup,
	/* .opengl_cleanup =        	*/ _shape_path_opengl_cleanup,
#else
	/* .opengl_initialize =         */ NULL,
	/* .opengl_setup =          	*/ NULL,
	/* .opengl_cleanup =        	*/ NULL
#endif
	/* .shape_features_get =		*/ _shape_path_shape_features_get,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_shape_path_new(
		Enesim_Renderer_Shape_Path_Descriptor *descriptor, void *data)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Shape_Path *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Shape_Path));
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

	r = enesim_renderer_shape_new(&_shape_path_descriptor, thiz);
	return r;
}

void * enesim_renderer_shape_path_data_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Shape_Path *thiz;

	thiz = _shape_path_get(r);
	return thiz->data;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

