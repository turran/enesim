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
typedef struct _Enesim_Renderer_Pattern_State
{
	double x;
	double y;
	double width;
	double height;
	Enesim_Renderer *source;
	Enesim_Repeat_Mode repeat_mode;
} Enesim_Renderer_Pattern_State;

typedef struct _Enesim_Renderer_Pattern {
	/* the properties */
	Enesim_Renderer_Pattern_State current;
	Enesim_Renderer_Pattern_State past;
	/* generated at state setup */
	/* private */
	Eina_Bool changed : 1;
} Enesim_Renderer_Pattern;

static inline Enesim_Renderer_Pattern * _pattern_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = enesim_renderer_data_get(r);
	return thiz;
}

static void _repeat_span(Enesim_Renderer *p, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Pattern *thiz = _pattern_get(p);

}

static void _pad_span(Enesim_Renderer *p, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Pattern *thiz = _pattern_get(p);

}

static void _restrict_span(Enesim_Renderer *p, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Pattern *thiz = _pattern_get(p);

}

static void _reflect_span(Enesim_Renderer *p, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Pattern *thiz = _pattern_get(p);

}

static Eina_Bool _pattern_state_setup(Enesim_Renderer_Pattern *thiz, Enesim_Renderer *r)
{
 	thiz = _pattern_get(r);
	return EINA_TRUE;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _pattern_name(Enesim_Renderer *r)
{
	return "pattern";
}

static Eina_Bool _pattern_sw_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Pattern *thiz;

 	thiz = _pattern_get(r);

	if (!_pattern_state_setup(thiz, r)) return EINA_FALSE;
	*fill = _repeat_span;

	return EINA_TRUE;
}

static void _pattern_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Pattern *thiz;

 	thiz = _pattern_get(r);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

static void _pattern_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	if (!thiz)
	{
		*flags = 0;
		return;
	}

	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_PROJECTIVE |
			ENESIM_RENDERER_FLAG_ARGB8888 |
			ENESIM_RENDERER_FLAG_ROP;

}

static void _pattern_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	free(thiz);
}

static void _pattern_boundings(Enesim_Renderer *r, Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	bounds->x = thiz->current.x;
	bounds->y = thiz->current.y;
	bounds->w = thiz->current.width;
	bounds->h = thiz->current.height;
}

static Eina_Bool _pattern_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	if (!thiz->changed) return EINA_FALSE;
	/* FIXME */
	return EINA_TRUE;
}

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _pattern_name,
	/* .free = 			*/ _pattern_free,
	/* .boundings = 		*/ _pattern_boundings,
	/* .destination_transform = 	*/ NULL,
	/* .flags = 			*/ _pattern_flags,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _pattern_has_changed,
	/* .sw_setup = 			*/ _pattern_sw_setup,
	/* .sw_cleanup = 		*/ _pattern_sw_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
	/* .opengl_setup =          	*/ NULL,
	/* .opengl_shader_setup = 	*/ NULL,
	/* .opengl_cleanup =        	*/ NULL
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a pattern renderer
 * @return The new renderer
 */
EAPI Enesim_Renderer * enesim_renderer_pattern_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Pattern *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Pattern));
	if (!thiz) return NULL;
	r = enesim_renderer_new(&_descriptor, thiz);
	return r;
}

/**
 * Sets the width of the pattern
 * @param[in] r The pattern renderer
 * @param[in] width The pattern width
 */
EAPI void enesim_renderer_pattern_width_set(Enesim_Renderer *r, double width)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	thiz->current.width = width;
	thiz->changed = EINA_TRUE;
}
/**
 * Gets the width of the pattern
 * @param[in] r The pattern renderer
 * @param[out] w The pattern width
 */
EAPI void enesim_renderer_pattern_width_get(Enesim_Renderer *r, double *width)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	if (width) *width = thiz->current.width;
}
/**
 * Sets the height of the pattern
 * @param[in] r The pattern renderer
 * @param[in] height The pattern height
 */
EAPI void enesim_renderer_pattern_height_set(Enesim_Renderer *r, double height)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	thiz->current.height = height;
	thiz->changed = EINA_TRUE;
}
/**
 * Gets the height of the pattern
 * @param[in] r The pattern renderer
 * @param[out] height The pattern height
 */
EAPI void enesim_renderer_pattern_height_get(Enesim_Renderer *r, double *height)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	if (height) *height = thiz->current.height;
}

/**
 * Sets the x of the pattern
 * @param[in] r The pattern renderer
 * @param[in] x The pattern x coordinate
 */
EAPI void enesim_renderer_pattern_x_set(Enesim_Renderer *r, double x)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	thiz->current.x = x;
	thiz->changed = EINA_TRUE;
}
/**
 * Gets the x of the pattern
 * @param[in] r The pattern renderer
 * @param[out] w The pattern x coordinate
 */
EAPI void enesim_renderer_pattern_x_get(Enesim_Renderer *r, double *x)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	if (x) *x = thiz->current.x;
}
/**
 * Sets the y of the pattern
 * @param[in] r The pattern renderer
 * @param[in] y The pattern y coordinate
 */
EAPI void enesim_renderer_pattern_y_set(Enesim_Renderer *r, double y)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
}
/**
 * Gets the y of the pattern
 * @param[in] r The pattern renderer
 * @param[out] y The pattern y coordinate
 */
EAPI void enesim_renderer_pattern_y_get(Enesim_Renderer *r, double *y)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	if (y) *y = thiz->current.y;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_pattern_position_set(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Pattern *thiz;
	thiz = _pattern_get(r);
	thiz->current.x = x;
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_pattern_position_get(Enesim_Renderer *r, double *x, double *y)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	if (x) *x = thiz->current.x;
	if (y) *y = thiz->current.y;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_pattern_size_set(Enesim_Renderer *r, double width, double height)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	thiz->current.width = width;
	thiz->current.height = height;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_pattern_size_get(Enesim_Renderer *r, double *width, double *height)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	if (width) *width = thiz->current.width;
	if (height) *height = thiz->current.height;
}

EAPI void enesim_renderer_pattern_source_set(Enesim_Renderer *r, Enesim_Renderer *source)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	if (thiz->current.source)
		enesim_renderer_unref(thiz->current.source);
	thiz->current.source = source;
	if (thiz->current.source)
		thiz->current.source = enesim_renderer_ref(thiz->current.source);
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_pattern_source_get(Enesim_Renderer *r, Enesim_Renderer **source)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	*source = thiz->current.source;
	if (thiz->current.source)
		thiz->current.source = enesim_renderer_ref(thiz->current.source);
}

EAPI void enesim_renderer_pattern_repeat_mode_set(Enesim_Renderer *r, Enesim_Repeat_Mode mode)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	thiz->current.repeat_mode = mode;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_pattern_repeat_mode_get(Enesim_Renderer *r, Enesim_Repeat_Mode *mode)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	*mode = thiz->current.repeat_mode;
}

