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
	Enesim_Repeat_Mode repeat_mode;
} Enesim_Renderer_Pattern_State;

typedef struct _Enesim_Renderer_Pattern {
	Enesim_Renderer base;
	/* the properties */
	Enesim_Renderer_Pattern_State current;
	Enesim_Renderer_Pattern_State past;
	Enesim_Renderer *src_r;
	Enesim_Surface *src_s;
	/* generated at state setup */
	Enesim_Surface *src;
	Enesim_Surface *cache;
	int src_w;
	int src_h;
	/* private */
	Eina_Bool changed : 1;
	Eina_Bool force_redraw : 1;
} Enesim_Renderer_Pattern;

typedef struct _Enesim_Renderer_Pattern_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Pattern_Class;

static Eina_Bool _pattern_state_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Log **l)
{
	Enesim_Renderer_Pattern *thiz;
	Eina_Rectangle bounds;

 	thiz = ENESIM_RENDERER_PATTERN(r);
	thiz->force_redraw = EINA_FALSE;

	/* setup the renderer/surface */
	if (!thiz->src_r && !thiz->src_s)
	{
		ENESIM_RENDERER_LOG(r, l, "No surface or renderer set");
		return EINA_FALSE;
	}

	if (thiz->src_r)
	{
		thiz->force_redraw = enesim_renderer_has_changed(thiz->src_r);

		enesim_renderer_destination_bounds_get(thiz->src_r, &bounds, 0, 0);
		if (thiz->cache)
		{
			int sw, sh;

			enesim_surface_size_get(thiz->cache, &sw, &sh);
			if (sw != bounds.w || sh != bounds.h)
			{
				enesim_surface_unref(thiz->cache);
				thiz->cache = NULL;
			}
		}

		if (!thiz->cache)
		{
			Enesim_Pool *pool;

			pool = enesim_surface_pool_get(s);
			thiz->cache = enesim_surface_new_pool_from(ENESIM_FORMAT_ARGB8888,
					bounds.w, bounds.h, pool);
			if (!thiz->cache)
			{
				ENESIM_RENDERER_LOG(r, l,
						"Impossible to create the surface of size %d %d",
						bounds.w, bounds.h);
				return EINA_FALSE;
			}
			thiz->force_redraw = EINA_TRUE;
		}
		if (thiz->force_redraw)
		{
			if (!enesim_renderer_draw(thiz->src_r, thiz->cache, ENESIM_ROP_FILL,
					NULL, -bounds.x, -bounds.y, NULL))
			{
				ENESIM_RENDERER_LOG(r, l, "Failed to draw the source renderer");
				return EINA_FALSE;
			}
		}
		thiz->src = enesim_surface_ref(thiz->cache);
		enesim_surface_size_get(thiz->cache, &thiz->src_w, &thiz->src_h);
	}
	else
	{
		thiz->src = enesim_surface_ref(thiz->src_s);
		enesim_surface_size_get(thiz->src, &thiz->src_w, &thiz->src_h);
	}

	return EINA_TRUE;
}

static void _pattern_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Pattern *thiz;

 	thiz = ENESIM_RENDERER_PATTERN(r);
	thiz->force_redraw = EINA_FALSE;
	if (thiz->src)
	{
		enesim_surface_unref(thiz->src);
		thiz->src = NULL;
	}
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

/*----------------------------------------------------------------------------*
 *                               Span functions                               *
 *----------------------------------------------------------------------------*/
static void _argb8888_repeat_span_identity(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Pattern *thiz = ENESIM_RENDERER_PATTERN(r);
	uint32_t *src;
	int sw = thiz->src_w;
	int sh = thiz->src_h;
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
	enesim_surface_data_get(thiz->src, (void **)&src, &sstride);
	src = argb8888_at(src, sstride, x, y);

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
	int sw = thiz->src_w;
	int sh = thiz->src_h;
	size_t sstride;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;

	x = x - thiz->current.x;
	y = y - thiz->current.y;
	y = y % (2 * sh);
	if (y < 0) y += 2 * sh;
	if (y >= sh) y = (2 * sh) - y - 1;
	enesim_surface_data_get(thiz->src, (void **)&src, &sstride);
	src = argb8888_at(src, sstride, x, y);

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
	int sw = thiz->src_w;
	int sh = thiz->src_h;
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
	enesim_surface_data_get(thiz->src, (void **)&src, &sstride);
	src = argb8888_at(src, sstride, x, y);

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
	enesim_surface_data_get(thiz->src, (void **)&src, &sstride);
	src = argb8888_at(src, sstride, x, y);

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
		Enesim_Surface *s, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l)
{
	Enesim_Renderer_Pattern *thiz;
	Enesim_Matrix_Type type;

 	thiz = ENESIM_RENDERER_PATTERN(r);

	/* do the common setup */
	type = enesim_renderer_transformation_type_get(r);
	if (!_pattern_state_setup(r, s, l))
		return EINA_FALSE;
	*fill = _spans[thiz->current.repeat_mode][type];

	return EINA_TRUE;
}

static void _pattern_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	_pattern_state_cleanup(r, s);
}

static void _pattern_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_ARGB8888;
}

static void _pattern_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	enesim_rectangle_coords_from(rect, thiz->current.x, thiz->current.y,
			thiz->current.width, thiz->current.height);
}

static Eina_Bool _pattern_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	if (thiz->src_r)
	{
		if (enesim_renderer_has_changed(thiz->src_r))
		{
			DBG("The source renderer %s has changed",
					enesim_renderer_name_get(thiz->src_r));
			return EINA_TRUE;
		}
	}

	if (!thiz->changed) return EINA_FALSE;

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
	if (thiz->src_r)
		enesim_renderer_unref(thiz->src_r);
	if (thiz->src_s)
		enesim_surface_unref(thiz->src_s);
	if (thiz->src)
		enesim_surface_unref(thiz->src);
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
 * @return The pattern width
 */
EAPI double enesim_renderer_pattern_width_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	return thiz->current.width;
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
 * @return The pattern height
 */
EAPI double enesim_renderer_pattern_height_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	return thiz->current.height;
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
 * @return The pattern x coordinate
 */
EAPI double enesim_renderer_pattern_x_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	return thiz->current.x;
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
 * @return The pattern y coordinate
 */
EAPI double enesim_renderer_pattern_y_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	return thiz->current.y;
}

EAPI void enesim_renderer_pattern_position_set(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Pattern *thiz;
	thiz = ENESIM_RENDERER_PATTERN(r);
	thiz->current.x = x;
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
}
EAPI void enesim_renderer_pattern_position_get(Enesim_Renderer *r, double *x, double *y)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	if (x) *x = thiz->current.x;
	if (y) *y = thiz->current.y;
}

EAPI void enesim_renderer_pattern_size_set(Enesim_Renderer *r, double width, double height)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	thiz->current.width = width;
	thiz->current.height = height;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_pattern_size_get(Enesim_Renderer *r, double *width, double *height)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	if (width) *width = thiz->current.width;
	if (height) *height = thiz->current.height;
}

/**
 * @brief Sets the surface to use as the src data
 * @param[in] r The pattern renderer
 * @param[in] src The surface to use [transfer full]
 */
EAPI void enesim_renderer_pattern_source_surface_set(Enesim_Renderer *r, Enesim_Surface *src)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	if (thiz->src_s)
	{
		enesim_surface_unref(thiz->src_s);
		thiz->src_s = NULL;
	}
	thiz->src_s = src;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the surface to pattern
 * @param[in] r The pattern renderer
 * @return The surface to pattern [transfer none]
 */
EAPI Enesim_Surface * enesim_renderer_pattern_source_surface_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	return enesim_surface_ref(thiz->src_s);
}

/**
 * @brief Sets the renderer to pattern
 * @param[in] r The pattern renderer
 * @param[in] sr The renderer to use [transfer full]
 */
EAPI void enesim_renderer_pattern_source_renderer_set(Enesim_Renderer *r, Enesim_Renderer *sr)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	if (thiz->src_r)
	{
		enesim_renderer_unref(thiz->src_r);
		thiz->src_r = NULL;
	}
	thiz->src_r = sr;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the renderer to pattern
 * @param[in] r The pattern renderer
 * @return The renderer to pattern [transfer none]
 */
EAPI Enesim_Renderer * enesim_renderer_pattern_source_renderer_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	return enesim_renderer_ref(thiz->src_r);
}

EAPI void enesim_renderer_pattern_repeat_mode_set(Enesim_Renderer *r, Enesim_Repeat_Mode mode)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	thiz->current.repeat_mode = mode;
	thiz->changed = EINA_TRUE;
}

EAPI Enesim_Repeat_Mode enesim_renderer_pattern_repeat_mode_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	return thiz->current.repeat_mode;
}

