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
#include "enesim_private.h"
#include "libargb.h"

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
#include "enesim_renderer_pattern.h"

#ifdef BUILD_OPENCL
#include "Enesim_OpenCL.h"
#endif

#ifdef BUILD_OPENGL
#include "Enesim_OpenGL.h"
#endif

#include "enesim_renderer_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_renderer_pattern

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
	Enesim_Surface *cache;
	int cache_w;
	int cache_h;
	Enesim_Color src_color;
	/* private */
	Eina_Bool changed : 1;
} Enesim_Renderer_Pattern;

static inline Enesim_Renderer_Pattern * _pattern_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = enesim_renderer_data_get(r);
	return thiz;
}

static void _pattern_tile_source_size(Enesim_Renderer_Pattern *thiz,
		const Enesim_Renderer_State *state,
		Enesim_Rectangle *size)
{
	Enesim_Rectangle src;
	Enesim_Quad q;

	enesim_rectangle_coords_from(&src, thiz->current.x, thiz->current.y,
			thiz->current.width, thiz->current.height);
	enesim_matrix_rectangle_transform(&state->geometry_transformation, &src, &q);
	enesim_quad_rectangle_to(&q, size);
}

static void _pattern_tile_destination_size(Enesim_Renderer_Pattern *thiz,
		const Enesim_Renderer_State *state,
		Eina_Rectangle *size)
{
	Enesim_Rectangle src;
	Enesim_Quad q;

	enesim_rectangle_coords_from(&src, thiz->current.x, thiz->current.y,
			thiz->current.width, thiz->current.height);
	enesim_matrix_rectangle_transform(&state->geometry_transformation, &src, &q);
	enesim_quad_eina_rectangle_to(&q, size);
}

static Eina_Bool _pattern_state_setup(Enesim_Renderer_Pattern *thiz,
		Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s, Enesim_Error **error)
{
	Eina_Rectangle dst_bounds;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	Enesim_Backend backend;
	Enesim_Format format;
	Enesim_Color color;
	Eina_Bool changed = EINA_FALSE;

	/* setup the renderer */
	if (!thiz->current.source)
	{
		ENESIM_RENDERER_ERROR(r, error, "You need to set a source renderer");
		return EINA_FALSE;
	}

	/* setup the surface */
	_pattern_tile_destination_size(thiz, cs, &dst_bounds);

	/* for now we just create a surface of the size of the pattern transformed */
	format = enesim_surface_format_get(s);
	if (thiz->cache)
	{
		Enesim_Format sf;

		sf = enesim_surface_format_get(thiz->cache);
		if (dst_bounds.w != thiz->cache_w || dst_bounds.h != thiz->cache_h || format != sf)
		{
			enesim_surface_unref(thiz->cache);
			thiz->cache = NULL;
		}
	}

	if (!thiz->cache)
	{
		thiz->cache = enesim_surface_new(format, dst_bounds.w, dst_bounds.h);
		printf("%d %d\n", dst_bounds.w, dst_bounds.h);
		if (!thiz->cache)
		{
			ENESIM_RENDERER_ERROR(r, error,
					"Impossible to create the surface of size %d %d",
					dst_bounds.w, dst_bounds.h);
			return EINA_FALSE;
		}
		thiz->cache_w = dst_bounds.w;
		thiz->cache_h = dst_bounds.h;
		changed = EINA_TRUE;
	}

	color = cs->color;
	enesim_renderer_color_get(thiz->current.source, &thiz->src_color);
	color = argb8888_mul4_sym(color, thiz->src_color);
	enesim_renderer_color_set(thiz->current.source, color);

	if (enesim_renderer_has_changed(thiz->current.source) || changed)
	{
		/* TODO The surface backend might be different, still there are some
		 * issues with the API on the surface/pool side
		 */
		if (!enesim_renderer_setup(thiz->current.source, thiz->cache, error))
		{
			ENESIM_RENDERER_ERROR(r, error,
					"Impossible to setup the source renderer");
			return EINA_FALSE;
		}
		enesim_renderer_draw(thiz->current.source, thiz->cache, NULL,
				0, 0, NULL);
	}

	return EINA_TRUE;
}

/*----------------------------------------------------------------------------*
 *                               Span functions                               *
 *----------------------------------------------------------------------------*/
static void _argb8888_repeat_span_identity(Enesim_Renderer *r,
		const Enesim_Renderer_State *state EINA_UNUSED,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Pattern *thiz = _pattern_get(r);
	uint32_t *src;
	int sw = thiz->cache_w;
	int sh = thiz->cache_h;
	size_t sstride;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;

	x = x - thiz->current.x;
	y = y - thiz->current.y;
	if ((y > sh - 1) || (y < 0))
	{
		y = y % sh;
		if (y < 0)
			y += sh;
	}
	enesim_surface_data_get(thiz->cache, (void **)&src, &sstride);
	src = argb8888_at(src, sstride, 0, y);

	while (dst < end)
	{
		if ((x > sw - 1) || (x < 0))
		{
			x = x % sw;
			if (x < 0)
				x += sw;
		}
		*dst++ = *(src + x);
		x++;
	}
}

static void _argb8888_repeat_span_affine(Enesim_Renderer *r,
		const Enesim_Renderer_State *state EINA_UNUSED,
		int x EINA_UNUSED, int y EINA_UNUSED,
		unsigned int len EINA_UNUSED, void *ddata EINA_UNUSED)
{
	Enesim_Renderer_Pattern *thiz = _pattern_get(r);

}

static void _argb8888_repeat_span_projective(Enesim_Renderer *r,
		const Enesim_Renderer_State *state EINA_UNUSED,
		int x EINA_UNUSED, int y EINA_UNUSED,
		unsigned int len EINA_UNUSED, void *ddata EINA_UNUSED)
{
	Enesim_Renderer_Pattern *thiz = _pattern_get(r);

}

static void _argb8888_reflect_span_identity(Enesim_Renderer *r,
		const Enesim_Renderer_State *state EINA_UNUSED,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Pattern *thiz = _pattern_get(r);
	uint32_t *src;
	int sw = thiz->cache_w;
	int sh = thiz->cache_h;
	size_t sstride;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;

	x = x - thiz->current.x;
	y = y - thiz->current.y;
	y = y % (2 * sh);
	if (y < 0) y += 2 * sh;
	if (y >= sh) y = (2 * sh) - y - 1;
	enesim_surface_data_get(thiz->cache, (void **)&src, &sstride);
	src = argb8888_at(src, sstride, 0, y);

	while (dst < end)
	{
		x = x % (2 * sw);
		if (x < 0) x += 2 * sw;
		if (x >= sw) x = (2 * sw) - x - 1;
		*dst++ = *(src + x);
		x++;
	}
}

static void _argb8888_reflect_span_affine(Enesim_Renderer *r,
		const Enesim_Renderer_State *state EINA_UNUSED,
		int x EINA_UNUSED, int y EINA_UNUSED,
		unsigned int len EINA_UNUSED, void *ddata EINA_UNUSED)
{
	Enesim_Renderer_Pattern *thiz = _pattern_get(r);

}

static void _argb8888_reflect_span_projective(Enesim_Renderer *r,
		const Enesim_Renderer_State *state EINA_UNUSED,
		int x EINA_UNUSED, int y EINA_UNUSED,
		unsigned int len EINA_UNUSED, void *ddata EINA_UNUSED)
{
	Enesim_Renderer_Pattern *thiz = _pattern_get(r);

}

static void _argb8888_restrict_span_identity(Enesim_Renderer *r,
		const Enesim_Renderer_State *state EINA_UNUSED,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Pattern *thiz = _pattern_get(r);
	uint32_t *src;
	int sw = thiz->cache_w;
	int sh = thiz->cache_h;
	size_t sstride;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;

	x = x - thiz->current.x;
	y = y - thiz->current.y;
	if ((y < 0) || (y >= sh) || (x >= sw) || (x + len < 0))
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	enesim_surface_data_get(thiz->cache, (void **)&src, &sstride);
	src = argb8888_at(src, sstride, 0, y);

	while (dst < end)
	{
		if ((x >= 0) && (x < sw))
			*dst++ = *(src + x);
		else
			*dst++ = 0;
		x++;
	}

}

static void _argb8888_restrict_span_affine(Enesim_Renderer *r,
		const Enesim_Renderer_State *state EINA_UNUSED,
		int x EINA_UNUSED, int y EINA_UNUSED,
		unsigned int len EINA_UNUSED, void *ddata EINA_UNUSED)
{
	Enesim_Renderer_Pattern *thiz = _pattern_get(r);

}

static void _argb8888_restrict_span_projective(Enesim_Renderer *r,
		const Enesim_Renderer_State *state EINA_UNUSED,
		int x EINA_UNUSED, int y EINA_UNUSED,
		unsigned int len EINA_UNUSED, void *ddata EINA_UNUSED)
{
	Enesim_Renderer_Pattern *thiz = _pattern_get(r);

}

static void _argb8888_pad_span_identity(Enesim_Renderer *r,
		const Enesim_Renderer_State *state EINA_UNUSED,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Pattern *thiz = _pattern_get(r);
	uint32_t *src;
	int sw = thiz->current.width;
	int sh = thiz->current.height;
	size_t sstride;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;

	x = x - thiz->current.x;
	y = y - thiz->current.y;
	if (y < 0)
		y = 0;
	else if (y >= sh)
		y = sh - 1;
	enesim_surface_data_get(thiz->cache, (void **)&src, &sstride);
	src = argb8888_at(src, sstride, 0, y);

	while (dst < end)
	{
		if (x < 0)
			x = 0;
		else if (x >= sw)
			x = sw - 1;
		*dst++ = *(src + x);
		x++;
	}

}

static void _argb8888_pad_span_affine(Enesim_Renderer *r,
		const Enesim_Renderer_State *state EINA_UNUSED,
		int x EINA_UNUSED, int y EINA_UNUSED,
		unsigned int len EINA_UNUSED, void *ddata EINA_UNUSED)
{
	Enesim_Renderer_Pattern *thiz = _pattern_get(r);

}

static void _argb8888_pad_span_projective(Enesim_Renderer *r,
		const Enesim_Renderer_State *state EINA_UNUSED,
		int x EINA_UNUSED, int y EINA_UNUSED,
		unsigned int len EINA_UNUSED, void *ddata EINA_UNUSED)
{
	Enesim_Renderer_Pattern *thiz = _pattern_get(r);

}

static Enesim_Renderer_Sw_Fill  _spans[ENESIM_REPEAT_MODES][ENESIM_MATRIX_TYPES];
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _pattern_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "pattern";
}

static Eina_Bool _pattern_sw_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Pattern *thiz;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];

 	thiz = _pattern_get(r);

	/* do the common setup */
	if (!_pattern_state_setup(thiz, r, states, s, error)) return EINA_FALSE;
	*fill = _spans[thiz->current.repeat_mode][cs->transformation_type];
	printf("repeat mode %d %p\n", thiz->current.repeat_mode, *fill);

	return EINA_TRUE;
}

static void _pattern_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Pattern *thiz;

 	thiz = _pattern_get(r);
	enesim_renderer_color_set(thiz->current.source, thiz->src_color);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

static void _pattern_flags(Enesim_Renderer *r EINA_UNUSED, const Enesim_Renderer_State *state EINA_UNUSED,
		Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_PROJECTIVE |
			ENESIM_RENDERER_FLAG_ARGB8888;
}

static void _pattern_hints(Enesim_Renderer *r EINA_UNUSED, const Enesim_Renderer_State *state EINA_UNUSED,
		Enesim_Renderer_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_COLORIZE;
}

static void _pattern_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	if (thiz->cache)
		enesim_surface_unref(thiz->cache);
	free(thiz);
}

static void _pattern_bounds(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	if (thiz->current.repeat_mode == ENESIM_RESTRICT)
	{
		const Enesim_Renderer_State *cs;

		cs = states[ENESIM_STATE_CURRENT];
		_pattern_tile_source_size(thiz, cs, bounds);
	}
	else
	{
		enesim_rectangle_coords_from(bounds, INT_MIN / 2, INT_MIN / 2, INT_MAX, INT_MAX);
	}
}

static void _pattern_destination_bounds(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Eina_Rectangle *bounds)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	if (thiz->current.repeat_mode == ENESIM_RESTRICT)
	{
		const Enesim_Renderer_State *cs;

		cs = states[ENESIM_STATE_CURRENT];
		_pattern_tile_destination_size(thiz, cs, bounds);
	}
	else
	{
		eina_rectangle_coords_from(bounds, INT_MIN / 2, INT_MIN / 2, INT_MAX, INT_MAX);
	}

}

static Eina_Bool _pattern_has_changed(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES] EINA_UNUSED)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	if (thiz->current.source)
	{
		if (enesim_renderer_has_changed(thiz->current.source))
		{
			const char *source_name;

			enesim_renderer_name_get(thiz->current.source, &source_name);
			DBG("The source renderer %s has changed", source_name);
			return EINA_TRUE;
		}
	}

	if (!thiz->changed) return EINA_FALSE;

	/* the source */
	if (thiz->current.source != thiz->past.source)
		return EINA_TRUE;
	/* the width */
	if (thiz->current.width != thiz->past.width)
		return EINA_TRUE;
	/* the height */
	if (thiz->current.height != thiz->past.height)
		return EINA_TRUE;
	/* the x */
	if (thiz->current.x != thiz->past.x)
		return EINA_TRUE;
	/* the y */
	if (thiz->current.y != thiz->past.y)
		return EINA_TRUE;
	/* the repeat mode */
	if (thiz->current.repeat_mode != thiz->past.repeat_mode)
		return EINA_TRUE;

	return EINA_FALSE;
}

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _pattern_name,
	/* .free = 			*/ _pattern_free,
	/* .bounds = 		*/ _pattern_bounds,
	/* .destination_bounds = 	*/ _pattern_destination_bounds,
	/* .flags = 			*/ _pattern_flags,
	/* .hints_get = 			*/ _pattern_hints,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _pattern_has_changed,
	/* .sw_setup = 			*/ _pattern_sw_setup,
	/* .sw_cleanup = 		*/ _pattern_sw_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
	/* .opengl_setup =          	*/ NULL,
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
	static Eina_Bool spans_initialized = EINA_FALSE;

	if (!spans_initialized)
	{
		spans_initialized = EINA_TRUE;
		_spans[ENESIM_REPEAT][ENESIM_MATRIX_IDENTITY] = _argb8888_repeat_span_identity;
		_spans[ENESIM_REPEAT][ENESIM_MATRIX_AFFINE] = _argb8888_repeat_span_affine;
		_spans[ENESIM_REPEAT][ENESIM_MATRIX_PROJECTIVE] = _argb8888_repeat_span_projective;
		_spans[ENESIM_REFLECT][ENESIM_MATRIX_IDENTITY] = _argb8888_reflect_span_identity;
		_spans[ENESIM_REFLECT][ENESIM_MATRIX_AFFINE] = _argb8888_reflect_span_affine;
		_spans[ENESIM_REFLECT][ENESIM_MATRIX_PROJECTIVE] = _argb8888_reflect_span_projective;
		_spans[ENESIM_RESTRICT][ENESIM_MATRIX_IDENTITY] = _argb8888_restrict_span_identity;
		_spans[ENESIM_RESTRICT][ENESIM_MATRIX_AFFINE] = _argb8888_restrict_span_affine;
		_spans[ENESIM_RESTRICT][ENESIM_MATRIX_PROJECTIVE] = _argb8888_restrict_span_projective;
		_spans[ENESIM_PAD][ENESIM_MATRIX_IDENTITY] = _argb8888_pad_span_identity;
		_spans[ENESIM_PAD][ENESIM_MATRIX_AFFINE] = _argb8888_pad_span_affine;
		_spans[ENESIM_PAD][ENESIM_MATRIX_PROJECTIVE] = _argb8888_pad_span_projective;
	}

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

/**
 * To be documented
 * FIXME: To be fixed
 */
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

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_pattern_source_get(Enesim_Renderer *r, Enesim_Renderer **source)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	*source = thiz->current.source;
	if (thiz->current.source)
		thiz->current.source = enesim_renderer_ref(thiz->current.source);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_pattern_repeat_mode_set(Enesim_Renderer *r, Enesim_Repeat_Mode mode)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	thiz->current.repeat_mode = mode;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_pattern_repeat_mode_get(Enesim_Renderer *r, Enesim_Repeat_Mode *mode)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	*mode = thiz->current.repeat_mode;
}

