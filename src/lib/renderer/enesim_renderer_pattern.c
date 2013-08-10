/* ENESIM - Drawing Library
 * Copyright (C) 2007-2013 Jorge Luis Zapata
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
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_pattern.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

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

#define ENESIM_RENDERER_PATTERN(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Pattern,					\
		enesim_renderer_pattern_descriptor_get())

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
	Enesim_Renderer base;
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

typedef struct _Enesim_Renderer_Pattern_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Pattern_Class;

static void _pattern_tile_source_size(Enesim_Renderer_Pattern *thiz,
		Enesim_Renderer *r,
		Enesim_Rectangle *size)
{
	Enesim_Rectangle src;
	Enesim_Matrix m;
	Enesim_Quad q;

	enesim_renderer_transformation_get(r, &m);
	enesim_rectangle_coords_from(&src, thiz->current.x, thiz->current.y,
			thiz->current.width, thiz->current.height);
	enesim_matrix_rectangle_transform(&m, &src, &q);
	enesim_quad_rectangle_to(&q, size);
}

static void _pattern_tile_destination_size(Enesim_Renderer_Pattern *thiz,
		Enesim_Renderer *r,
		Eina_Rectangle *size)
{
	Enesim_Rectangle src;
	Enesim_Matrix m;
	Enesim_Quad q;

	enesim_renderer_transformation_get(r, &m);
	enesim_rectangle_coords_from(&src, thiz->current.x, thiz->current.y,
			thiz->current.width, thiz->current.height);
	enesim_matrix_rectangle_transform(&m, &src, &q);
	enesim_quad_eina_rectangle_to(&q, size);
}

static Eina_Bool _pattern_state_setup(Enesim_Renderer_Pattern *thiz,
		Enesim_Renderer *r, Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Log **l)
{
	Eina_Rectangle dst_bounds;
	Enesim_Format format;
	Enesim_Color color;
	Eina_Bool changed = EINA_FALSE;

	/* setup the renderer */
	if (!thiz->current.source)
	{
		ENESIM_RENDERER_LOG(r, l, "You need to set a source renderer");
		return EINA_FALSE;
	}

	/* setup the surface */
	_pattern_tile_destination_size(thiz, r, &dst_bounds);

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
		if (!thiz->cache)
		{
			ENESIM_RENDERER_LOG(r, l,
					"Impossible to create the surface of size %d %d",
					dst_bounds.w, dst_bounds.h);
			return EINA_FALSE;
		}
		thiz->cache_w = dst_bounds.w;
		thiz->cache_h = dst_bounds.h;
		changed = EINA_TRUE;
	}

	color = enesim_renderer_color_get(r);
	thiz->src_color = enesim_renderer_color_get(thiz->current.source);
	color = argb8888_mul4_sym(color, thiz->src_color);
	enesim_renderer_color_set(thiz->current.source, color);

	if (enesim_renderer_has_changed(thiz->current.source) || changed)
	{
		/* TODO The surface backend might be different, still there are some
		 * issues with the API on the surface/pool side
		 */
		enesim_renderer_draw(thiz->current.source, thiz->cache, rop, NULL,
				0, 0, NULL);
	}

	return EINA_TRUE;
}

/*----------------------------------------------------------------------------*
 *                               Span functions                               *
 *----------------------------------------------------------------------------*/
static void _argb8888_repeat_span_identity(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Pattern *thiz = ENESIM_RENDERER_PATTERN(r);
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

static void _argb8888_reflect_span_identity(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Pattern *thiz = ENESIM_RENDERER_PATTERN(r);
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

static void _argb8888_restrict_span_identity(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Pattern *thiz = ENESIM_RENDERER_PATTERN(r);
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

static void _argb8888_pad_span_identity(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Pattern *thiz = ENESIM_RENDERER_PATTERN(r);
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

static Enesim_Renderer_Sw_Fill  _spans[ENESIM_REPEAT_MODES][ENESIM_MATRIX_TYPES];
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _pattern_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "pattern";
}

static Eina_Bool _pattern_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l)
{
	Enesim_Renderer_Pattern *thiz;
	Enesim_Matrix_Type type;

 	thiz = ENESIM_RENDERER_PATTERN(r);

	/* do the common setup */
	enesim_renderer_transformation_type_get(r, &type);
	if (!_pattern_state_setup(thiz, r, s, rop, l)) return EINA_FALSE;
	*fill = _spans[thiz->current.repeat_mode][type];

	return EINA_TRUE;
}

static void _pattern_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Pattern *thiz;

 	thiz = ENESIM_RENDERER_PATTERN(r);
	//enesim_renderer_color_set(thiz->current.source, thiz->src_color);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

static void _pattern_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_ARGB8888;
}

static void _pattern_sw_hints(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Rop rop EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_COLORIZE;
}

static void _pattern_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	if (thiz->current.repeat_mode == ENESIM_RESTRICT)
	{
		_pattern_tile_source_size(thiz, r, bounds);
	}
	else
	{
		enesim_rectangle_coords_from(bounds, INT_MIN / 2, INT_MIN / 2, INT_MAX, INT_MAX);
	}
}

static Eina_Bool _pattern_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	if (thiz->current.source)
	{
		if (enesim_renderer_has_changed(thiz->current.source))
		{
			DBG("The source renderer %s has changed",
					enesim_renderer_name_get(thiz->current.source));
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
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Pattern, Enesim_Renderer_Pattern_Class,
		enesim_renderer_pattern);

static void _enesim_renderer_pattern_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _pattern_name;
	klass->bounds_get = _pattern_bounds_get;
	klass->features_get = _pattern_features_get;
	klass->has_changed = _pattern_has_changed;
	klass->sw_hints_get = _pattern_sw_hints;
	klass->sw_setup = _pattern_sw_setup;
	klass->sw_cleanup = _pattern_sw_cleanup;
	memset(_spans, 0, sizeof(_spans));
	_spans[ENESIM_REPEAT][ENESIM_MATRIX_IDENTITY] = _argb8888_repeat_span_identity;
	_spans[ENESIM_REFLECT][ENESIM_MATRIX_IDENTITY] = _argb8888_reflect_span_identity;
	_spans[ENESIM_RESTRICT][ENESIM_MATRIX_IDENTITY] = _argb8888_restrict_span_identity;
	_spans[ENESIM_PAD][ENESIM_MATRIX_IDENTITY] = _argb8888_pad_span_identity;
}

static void _enesim_renderer_pattern_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_renderer_pattern_instance_deinit(void *o)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(o);
	if (thiz->cache)
		enesim_surface_unref(thiz->cache);
}
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

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_pattern);
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

	thiz = ENESIM_RENDERER_PATTERN(r);
	thiz->current.width = width;
	thiz->changed = EINA_TRUE;
}
/**
 * Gets the width of the pattern
 * @param[in] r The pattern renderer
 * @param[out] width The pattern width
 */
EAPI void enesim_renderer_pattern_width_get(Enesim_Renderer *r, double *width)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
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

	thiz = ENESIM_RENDERER_PATTERN(r);
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

	thiz = ENESIM_RENDERER_PATTERN(r);
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

	thiz = ENESIM_RENDERER_PATTERN(r);
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

	thiz = ENESIM_RENDERER_PATTERN(r);
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

	thiz = ENESIM_RENDERER_PATTERN(r);
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

	thiz = ENESIM_RENDERER_PATTERN(r);
	if (y) *y = thiz->current.y;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_pattern_position_set(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Pattern *thiz;
	thiz = ENESIM_RENDERER_PATTERN(r);
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

	thiz = ENESIM_RENDERER_PATTERN(r);
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

	thiz = ENESIM_RENDERER_PATTERN(r);
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

	thiz = ENESIM_RENDERER_PATTERN(r);
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

	thiz = ENESIM_RENDERER_PATTERN(r);
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

	thiz = ENESIM_RENDERER_PATTERN(r);
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

	thiz = ENESIM_RENDERER_PATTERN(r);
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

	thiz = ENESIM_RENDERER_PATTERN(r);
	*mode = thiz->current.repeat_mode;
}

