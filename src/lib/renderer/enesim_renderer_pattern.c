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

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_pattern.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"
#include "enesim_coord_private.h"

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
/** @cond internal */
#define ENESIM_LOG_DEFAULT enesim_log_renderer_pattern

#define ENESIM_RENDERER_PATTERN(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Pattern,					\
		enesim_renderer_pattern_descriptor_get())

typedef struct _Enesim_Renderer_Pattern_State
{
	Enesim_Repeat_Mode repeat_mode;
} Enesim_Renderer_Pattern_State;

typedef struct _Enesim_Renderer_Pattern {
	Enesim_Renderer base;
	/* the properties */
	Enesim_Renderer_Pattern_State current;
	Enesim_Renderer_Pattern_State past;
	Enesim_Renderer *src_r;
	Enesim_Surface *src_s;
	/* private */
	/* generated at state setup */
	Enesim_Surface *src;
	Enesim_Surface *cache;
	int src_xx;
	int src_yy;
	int src_ww;
	int src_hh;
	int src_w;
	int src_h;
	Enesim_Matrix_F16p16 matrix;
	Eina_Bool changed : 1;
	Eina_Bool force_redraw : 1;
	/* for sw */
	size_t sstride;
	uint32_t *ssrc;
} Enesim_Renderer_Pattern;

typedef struct _Enesim_Renderer_Pattern_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Pattern_Class;

static Eina_Bool _pattern_state_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Log **l)
{
	Enesim_Renderer_Pattern *thiz;
	Eina_Rectangle bounds;
	int sx, sy, sw, sh;

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
			int old_sw, old_sh;

			enesim_surface_size_get(thiz->cache, &old_sw, &old_sh);
			if (old_sw != bounds.w || old_sh != bounds.h)
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
		enesim_surface_size_get(thiz->cache, &sw, &sh);
		sx = bounds.x;
		sy = bounds.y;
	}
	else
	{
		thiz->src = enesim_surface_ref(thiz->src_s);
		enesim_surface_size_get(thiz->src, &sw, &sh);
		sx = 0;
		sy = 0;
	}
	thiz->src_xx = eina_f16p16_int_from(sx);
	thiz->src_yy = eina_f16p16_int_from(sy);
	thiz->src_ww = eina_f16p16_int_from(sw);
	thiz->src_hh = eina_f16p16_int_from(sh);
	thiz->src_w = sw;
	thiz->src_h = sh;

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
#define PATTERN_IDENTITY(rmode) \
static void _enesim_renderer_pattern_argb8888_##rmode##_identity_span(		\
		Enesim_Renderer *r, int x, int y, int len, void *ddata)		\
{										\
	Enesim_Renderer_Pattern *thiz;						\
	Eina_F16p16 xx, yy;							\
	uint32_t *dst = ddata;							\
	uint32_t *end = dst + len;						\
	double ox, oy;								\
										\
	thiz = ENESIM_RENDERER_PATTERN(r);					\
	enesim_renderer_origin_get(r, &ox, &oy);				\
	enesim_coord_identity_setup(&xx, &yy, x, y, ox, oy);			\
	/* translate the origin by the image origin */				\
	xx = eina_f16p16_sub(xx, thiz->src_xx);					\
	yy = eina_f16p16_sub(yy, thiz->src_yy);					\
	yy = enesim_coord_##rmode(yy, thiz->src_hh);				\
	while (dst < end)							\
	{									\
		Eina_F16p16 xxx;						\
										\
		xxx = enesim_coord_##rmode(xx, thiz->src_ww);			\
		*dst++ = enesim_coord_sample_good_restrict(thiz->ssrc,		\
				thiz->sstride, 	thiz->src_w, thiz->src_h, 	\
				xxx, yy);					\
		xx += EINA_F16P16_ONE;						\
	}									\
}

#define PATTERN_AFFINE(rmode) \
static void _enesim_renderer_pattern_argb8888_##rmode##_affine_span(		\
		Enesim_Renderer *r, int x, int y, int len, void *ddata)		\
{										\
	Enesim_Renderer_Pattern *thiz;						\
	Eina_F16p16 xx, yy;							\
	uint32_t *dst = ddata;							\
	uint32_t *end = dst + len;						\
	double ox, oy;								\
										\
	thiz = ENESIM_RENDERER_PATTERN(r);					\
	enesim_renderer_origin_get(r, &ox, &oy);				\
	enesim_coord_affine_setup(&xx, &yy, x, y, ox, oy, &thiz->matrix);	\
	/* translate the origin by the image origin */				\
	xx = eina_f16p16_sub(xx, thiz->src_xx);					\
	yy = eina_f16p16_sub(yy, thiz->src_yy);					\
	while (dst < end)							\
	{									\
		*dst++ = enesim_coord_sample_good_##rmode(thiz->ssrc, 		\
				thiz->sstride, 	thiz->src_w, thiz->src_h, 	\
				xx, yy);					\
		yy += thiz->matrix.yx;						\
		xx += thiz->matrix.xx;						\
	}									\
}

PATTERN_IDENTITY(reflect)
PATTERN_IDENTITY(repeat)

PATTERN_AFFINE(reflect)
PATTERN_AFFINE(repeat)
PATTERN_AFFINE(restrict)

static Enesim_Renderer_Sw_Fill  _spans[ENESIM_REPEAT_MODE_LAST][ENESIM_MATRIX_TYPE_LAST];
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
	Enesim_Matrix m;
	Enesim_Matrix_Type type;

 	thiz = ENESIM_RENDERER_PATTERN(r);

	/* do the common setup */
	if (!_pattern_state_setup(r, s, l))
		return EINA_FALSE;
	if (!enesim_surface_map(thiz->src, (void **)&thiz->ssrc, &thiz->sstride))
	{
		_pattern_state_cleanup(r, s);
		return EINA_FALSE;
	}
	/* convert the matrix */
	enesim_renderer_transformation_get(r, &m);
	enesim_matrix_inverse(&m, &m);
	enesim_matrix_matrix_f16p16_to(&m, &thiz->matrix);
	type = enesim_renderer_transformation_type_get(r);
	*fill = _spans[thiz->current.repeat_mode][type];

	return EINA_TRUE;
}

static void _pattern_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	enesim_surface_unmap(thiz->src, (void **)&thiz->ssrc, EINA_FALSE);
	_pattern_state_cleanup(r, s);
}

static void _pattern_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_ARGB8888;
}

static void _pattern_bounds_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Rectangle *rect)
{
	/* TODO in case of restrict, do not do this */
	enesim_rectangle_coords_from(rect, INT_MIN / 2, INT_MIN / 2, INT_MAX, INT_MAX);
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
	_spans[ENESIM_REPEAT_MODE_REPEAT][ENESIM_MATRIX_TYPE_IDENTITY] = _enesim_renderer_pattern_argb8888_repeat_identity_span;
	_spans[ENESIM_REPEAT_MODE_REFLECT][ENESIM_MATRIX_TYPE_IDENTITY] = _enesim_renderer_pattern_argb8888_reflect_identity_span;
	_spans[ENESIM_REPEAT_MODE_REPEAT][ENESIM_MATRIX_TYPE_AFFINE] = _enesim_renderer_pattern_argb8888_repeat_affine_span;
	_spans[ENESIM_REPEAT_MODE_REFLECT][ENESIM_MATRIX_TYPE_AFFINE] = _enesim_renderer_pattern_argb8888_reflect_affine_span;
	_spans[ENESIM_REPEAT_MODE_RESTRICT][ENESIM_MATRIX_TYPE_AFFINE] = _enesim_renderer_pattern_argb8888_restrict_affine_span;
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
/** @endcond */
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
 * @brief Sets the surface to use as the pattern source
 * @ender_prop{source_surface}
 * @param[in] r The pattern renderer
 * @param[in] src The surface to use @ender_transfer{full}
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
 * @brief Gets the surface used as pattern source
 * @ender_prop{source_surface}
 * @param[in] r The pattern renderer
 * @return The surface to pattern @ender_transfer{none}
 */
EAPI Enesim_Surface * enesim_renderer_pattern_source_surface_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	return enesim_surface_ref(thiz->src_s);
}

/**
 * @brief Sets the renderer to use as pattern source
 * @ender_prop{source_renderer}
 * @param[in] r The pattern renderer
 * @param[in] sr The renderer to use @ender_transfer{full}
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
 * @brief Gets the renderer used as pattern source
 * @ender_prop{source_renderer}
 * @param[in] r The pattern renderer
 * @return The renderer to pattern @ender_transfer{none}
 */
EAPI Enesim_Renderer * enesim_renderer_pattern_source_renderer_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	return enesim_renderer_ref(thiz->src_r);
}

/**
 * @brief Sets the repeat mode of a pattern renderer
 * @ender_prop{repeat_mode}
 * @param[in] r The pattern renderer
 * @param[in] mode The repeat mode
 */
EAPI void enesim_renderer_pattern_repeat_mode_set(Enesim_Renderer *r, Enesim_Repeat_Mode mode)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	thiz->current.repeat_mode = mode;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the repeat mode of a pattern renderer
 * @ender_prop{repeat_mode}
 * @param[in] r The pattern renderer
 * @return mode The repeat mode
 */
EAPI Enesim_Repeat_Mode enesim_renderer_pattern_repeat_mode_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = ENESIM_RENDERER_PATTERN(r);
	return thiz->current.repeat_mode;
}

