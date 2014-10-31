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
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_dispmap.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_coord_private.h"
#include "enesim_renderer_private.h"
/*
 * P'(x,y) <- P(x + scale * (XC(x,y) - .5), y + scale * (YC(x,y) - .5))
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_RENDERER_DISPMAP(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Dispmap,					\
		enesim_renderer_dispmap_descriptor_get())

typedef struct _Enesim_Renderer_Dispmap
{
	Enesim_Renderer parent;
	Enesim_Surface *map;
	Enesim_Surface *src;
	Enesim_Channel x_channel;
	Enesim_Channel y_channel;
	double scale;
	/* The state variables */
	double ox;
	double oy;
	Enesim_Matrix_F16p16 matrix;
	Eina_F16p16 s_scale;
	/* for sw */
	uint32_t *ssrc;
	size_t sstride;
	uint32_t *msrc;
	size_t mstride;
	Enesim_Color color;
	Enesim_Renderer *mask;
	Eina_Bool do_mask;
} Enesim_Renderer_Dispmap;

typedef struct _Enesim_Renderer_Dispmap_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Dispmap_Class;

static inline Eina_F16p16 _displace(Eina_F16p16 coord, uint8_t distance, Eina_F16p16 scale)
{
	Eina_F16p16 vx;

	/* FIXME define fixed(255) as a constant */
	vx = eina_f16p16_int_from(distance - 127);
	vx = eina_f16p16_mul((((int64_t)(vx) << 16) / eina_f16p16_int_from(255)), scale);

	return vx + coord;
}

#define DISPMAP_IDENTITY(xch, ych, xfunction, yfunction) \
static void _argb8888_##xch##_##ych##_span_identity(Enesim_Renderer *r,		\
		int x, int y, int len,						\
		void *ddata)							\
{										\
	Enesim_Renderer_Dispmap *thiz;						\
	Enesim_Color color;							\
	uint32_t *dst = ddata;							\
	uint32_t *end = dst + len;						\
	uint32_t *map;								\
	int sw, sh, mw, mh;							\
	Eina_F16p16 xx, yy;							\
										\
	thiz = ENESIM_RENDERER_DISPMAP(r);					\
	color = thiz->color;							\
	if (color == 0xffffffff)						\
		color = 0;							\
	/* setup the parameters */						\
	enesim_surface_size_get(thiz->src, &sw, &sh);				\
	enesim_surface_size_get(thiz->map, &mw, &mh);				\
										\
	enesim_coord_identity_setup(&xx, &yy, x, y, thiz->ox, thiz->oy);	\
	x = eina_f16p16_int_to(xx);						\
	y = eina_f16p16_int_to(yy);						\
	map = argb8888_at(thiz->msrc, thiz->mstride, x, y);			\
										\
	/* do mask */								\
	if (thiz->do_mask) 							\
		enesim_renderer_sw_draw(thiz->mask, x, y, len, dst);		\
										\
	while (dst < end)							\
	{									\
		Eina_F16p16 sxx, syy;						\
		uint32_t p0 = 0;						\
		int ma = 255;							\
		uint16_t m0;							\
		uint16_t m1;							\
										\
		/* TODO fix this, no need for it */				\
		x = eina_f16p16_int_to(xx);					\
		if (x < 0 || x >= mw || y < 0 || y >= mh)			\
			goto set;						\
										\
		if (thiz->do_mask)						\
		{								\
			ma = (*dst) >> 24;					\
			if (!ma)						\
				goto next;					\
		}								\
										\
		m1 = yfunction(*map);						\
		m0 = xfunction(*map);						\
		sxx = _displace(xx, m0, thiz->s_scale);				\
		syy = _displace(yy, m1, thiz->s_scale);				\
										\
		p0 = enesim_coord_sample_good_restrict(thiz->ssrc, 		\
				thiz->sstride, sw, sh, sxx, syy);		\
										\
		if (ma < 255)							\
			p0 = argb8888_mul_sym(ma, p0);				\
set:										\
		*dst = p0;							\
next:										\
		dst++;								\
		map++;								\
		xx += EINA_F16P16_ONE;						\
	}									\
}

#define DISPMAP_AFFINE(xch, ych, xfunction, yfunction) \
static void _argb8888_##xch##_##ych##_span_affine(Enesim_Renderer *r,		\
		int x, int y, int len,						\
		void *ddata)							\
{										\
	Enesim_Renderer_Dispmap *thiz;						\
	Enesim_Color color;							\
	uint32_t *dst = ddata;							\
	uint32_t *end = dst + len;						\
	uint32_t *map;								\
	int sw, sh, mw, mh;							\
	Eina_F16p16 xx, yy;							\
										\
	thiz = ENESIM_RENDERER_DISPMAP(r);					\
	color = thiz->color;							\
	if (color == 0xffffffff)						\
		color = 0;							\
	/* setup the parameters */						\
	enesim_surface_size_get(thiz->src, &sw, &sh);				\
	enesim_surface_size_get(thiz->map, &mw, &mh);				\
										\
	/* TODO move by the origin */						\
	enesim_coord_affine_setup(&xx, &yy, x, y, thiz->ox, thiz->oy,		\
			&thiz->matrix);						\
										\
	/* do mask */								\
	if (thiz->do_mask)							\
		enesim_renderer_sw_draw(thiz->mask, x, y, len, dst);		\
										\
	while (dst < end)							\
	{									\
		Eina_F16p16 sxx, syy;						\
		uint32_t p0 = 0;						\
		int ma = 255;							\
		uint32_t *m;							\
		uint16_t m0;							\
		uint16_t m1;							\
										\
		x = eina_f16p16_int_to(xx);					\
		y = eina_f16p16_int_to(yy);					\
										\
		if (x < 0 || x >= mw || y < 0 || y >= mh)			\
			goto set;						\
										\
		if (thiz->do_mask)						\
		{								\
			ma = (*dst) >> 24;					\
			if (!ma)						\
				goto next;					\
		}								\
										\
		m = argb8888_at(thiz->msrc, thiz->mstride, x, y);		\
		m1 = yfunction(*m);						\
		m0 = xfunction(*m);						\
										\
		sxx = _displace(xx, m0, thiz->s_scale);				\
		syy = _displace(yy, m1, thiz->s_scale);				\
										\
		p0 = enesim_coord_sample_good_restrict(thiz->ssrc, 		\
				thiz->sstride, sw, sh, sxx, syy);		\
										\
		if (color)							\
			p0 = argb8888_mul4_sym(p0, color);			\
		if (ma < 255)							\
			p0 = argb8888_mul_sym(ma, p0);				\
set:										\
		*dst = p0;							\
next:										\
		dst++;								\
		map++;								\
		yy += thiz->matrix.yx;						\
		xx += thiz->matrix.xx;						\
	}									\
}

DISPMAP_AFFINE(r, g, argb8888_red_get, argb8888_green_get);
DISPMAP_AFFINE(a, b, argb8888_alpha_get, argb8888_blue_get);
DISPMAP_IDENTITY(a, b, argb8888_alpha_get, argb8888_blue_get);
DISPMAP_IDENTITY(r, g, argb8888_red_get, argb8888_green_get);

static Enesim_Renderer_Sw_Fill _spans[ENESIM_CHANNEL_LAST][ENESIM_CHANNEL_LAST][ENESIM_MATRIX_TYPE_LAST];
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _dispmap_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "dispmap";
}

static void _dispmap_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Dispmap *thiz;

	thiz = ENESIM_RENDERER_DISPMAP(r);
	if (thiz->map)
		enesim_surface_unmap(thiz->map, thiz->msrc, EINA_FALSE);
	if (thiz->src)
		enesim_surface_unmap(thiz->src, thiz->ssrc, EINA_FALSE);
	if (thiz->mask)
	{
		enesim_renderer_unref(thiz->mask);
		thiz->mask = NULL;
	}
	thiz->do_mask = EINA_FALSE;
}

static Eina_Bool _dispmap_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Dispmap *thiz;
	Enesim_Matrix m;
	Enesim_Matrix_Type type;

	thiz = ENESIM_RENDERER_DISPMAP(r);
	if (!thiz->map || !thiz->src) return EINA_FALSE;

	enesim_surface_map(thiz->map, (void **)&thiz->msrc, &thiz->mstride);
	enesim_surface_map(thiz->src, (void **)&thiz->ssrc, &thiz->sstride);

	enesim_renderer_origin_get(r, &thiz->ox, &thiz->oy);
	enesim_renderer_transformation_get(r, &m);
	type = enesim_renderer_transformation_type_get(r);
	enesim_matrix_matrix_f16p16_to(&m, &thiz->matrix);

	thiz->color = enesim_renderer_color_get(r);
	thiz->mask = enesim_renderer_mask_get(r);
	if (thiz->mask)
	{
		Enesim_Channel mchannel;
		mchannel = enesim_renderer_mask_channel_get(r);
		if (mchannel == ENESIM_CHANNEL_ALPHA)
			thiz->do_mask = EINA_TRUE;
	}

	*fill = _spans[thiz->x_channel][thiz->y_channel][type];
	if (!*fill) return EINA_FALSE;

	thiz->s_scale = eina_f16p16_double_from(thiz->scale);

	return EINA_TRUE;
}

static void _dispmap_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Dispmap *thiz;

	thiz = ENESIM_RENDERER_DISPMAP(r);
	if (!thiz->src || !thiz->map)
	{
		rect->x = 0;
		rect->y = 0;
		rect->w = 0;
		rect->h = 0;
	}
	else
	{
		int sw, sh;
		int mw, mh;

		enesim_surface_size_get(thiz->src, &sw, &sh);
		enesim_surface_size_get(thiz->map, &mw, &mh);
		rect->x = 0;
		rect->y = 0;
		rect->w = mw < sw ? mw : sw;
		rect->h = mh < sh ? mh : sh;
	}
}

static void _dispmap_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static void _dispmap_sw_hints_get(Enesim_Renderer *r,
		Enesim_Rop rop EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	Enesim_Channel mchannel;

	*hints = ENESIM_RENDERER_SW_HINT_COLORIZE;

	mchannel = enesim_renderer_mask_channel_get(r);
	if (mchannel == ENESIM_CHANNEL_ALPHA)
		*hints |= ENESIM_RENDERER_SW_HINT_MASK;
}

/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Dispmap, Enesim_Renderer_Dispmap_Class,
		enesim_renderer_dispmap);

static void _enesim_renderer_dispmap_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _dispmap_name;
	klass->bounds_get = _dispmap_bounds_get;
	klass->features_get = _dispmap_features_get;
	klass->sw_hints_get = _dispmap_sw_hints_get;
	klass->sw_setup = _dispmap_sw_setup;
	klass->sw_cleanup = _dispmap_sw_cleanup;

	_spans[ENESIM_CHANNEL_ALPHA][ENESIM_CHANNEL_BLUE][ENESIM_MATRIX_TYPE_IDENTITY]
		= _argb8888_a_b_span_identity;
	_spans[ENESIM_CHANNEL_ALPHA][ENESIM_CHANNEL_BLUE][ENESIM_MATRIX_TYPE_AFFINE]
		= _argb8888_a_b_span_affine;
	_spans[ENESIM_CHANNEL_RED][ENESIM_CHANNEL_GREEN][ENESIM_MATRIX_TYPE_IDENTITY]
		= _argb8888_r_g_span_identity;
	_spans[ENESIM_CHANNEL_RED][ENESIM_CHANNEL_GREEN][ENESIM_MATRIX_TYPE_AFFINE]
		= _argb8888_r_g_span_affine;
}

static void _enesim_renderer_dispmap_instance_init(void *o)
{
	Enesim_Renderer_Dispmap *thiz = ENESIM_RENDERER_DISPMAP(o);
	/* specific renderer setup */
	thiz->x_channel = ENESIM_CHANNEL_RED;
	thiz->y_channel = ENESIM_CHANNEL_GREEN;
}

static void _enesim_renderer_dispmap_instance_deinit(void *o EINA_UNUSED)
{
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a new displacement map renderer
 *
 * @return The renderer
 */
EAPI Enesim_Renderer * enesim_renderer_dispmap_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_dispmap);
	return r;
}

/**
 * @brief Sets the channel to use as the x coordinate displacement
 * @ender_prop{x_channel}
 * @param[in] r The displacement map renderer
 * @param[in] channel The channel to use
 */
EAPI void enesim_renderer_dispmap_x_channel_set(Enesim_Renderer *r,
	Enesim_Channel channel)
{
	Enesim_Renderer_Dispmap *thiz;

	thiz = ENESIM_RENDERER_DISPMAP(r);

	thiz->x_channel = channel;
}

/**
 * @brief Sets the channel to use as the y coordinate displacement
 * @ender_prop{y_channel}
 * @param[in] r The displacement map renderer
 * @param[in] channel The channel to use
 */
EAPI void enesim_renderer_dispmap_y_channel_set(Enesim_Renderer *r,
	Enesim_Channel channel)
{
	Enesim_Renderer_Dispmap *thiz;

	thiz = ENESIM_RENDERER_DISPMAP(r);

	thiz->y_channel = channel;
}

/**
 * @brief Sets the map surface to use for displacing the source surface
 * @ender_prop{map_surface}
 * @param[in] r The displacement map renderer
 * @param[in] map The surface map @ender_transfer{full}
 */
EAPI void enesim_renderer_dispmap_map_surface_set(Enesim_Renderer *r, Enesim_Surface *map)
{
	Enesim_Renderer_Dispmap *thiz;

	thiz = ENESIM_RENDERER_DISPMAP(r);
	if (thiz->map)
		enesim_surface_unref(thiz->map);
	thiz->map = map;
}

/**
 * @brief Sets the map surface to use for displacing the source surface
 * @ender_prop{map_surface}
 * @param[in] r The displacement map renderer
 * @return The surface map @ender_transfer{full}
 */
EAPI Enesim_Surface * enesim_renderer_dispmap_map_surface_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Dispmap *thiz;

	thiz = ENESIM_RENDERER_DISPMAP(r);
	return enesim_surface_ref(thiz->map);
}

/**
 * @brief Sets the source surface that will be displaced using the map surface
 * @ender_prop{source_surface}
 * @param[in] r The displacement map renderer
 * @param[in] src The source surface to displace
 */
EAPI void enesim_renderer_dispmap_source_surface_set(Enesim_Renderer *r, Enesim_Surface *src)
{
	Enesim_Renderer_Dispmap *thiz;

	thiz = ENESIM_RENDERER_DISPMAP(r);
	if (thiz->src)
		enesim_surface_unref(thiz->src);
	thiz->src = src;
}

/**
 * @brief Gets the source surface that will be displaced using the map surface
 * @ender_prop{source_surface}
 * @param[in] r The displacement map renderer
 * @return The source surface
 */
EAPI Enesim_Surface * enesim_renderer_dispmap_source_surface_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Dispmap *thiz;

	thiz = ENESIM_RENDERER_DISPMAP(r);
	return enesim_surface_ref(thiz->src);
}

/**
 * @brief Sets the factor of displacement for the displacement renderer
 * @ender_prop{factor}
 * @param[in] r The displacement map renderer
 * @param[in] factor The factor to use
 */
EAPI void enesim_renderer_dispmap_factor_set(Enesim_Renderer *r, double factor)
{
	Enesim_Renderer_Dispmap *thiz;

	thiz = ENESIM_RENDERER_DISPMAP(r);

	thiz->scale = factor;
}

/**
 * @brief Gets the factor of displacement for the displacement renderer
 * @ender_prop{factor}
 * @param[in] r The displacement map renderer
 * @return The factor to use
 */
EAPI double enesim_renderer_dispmap_factor_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Dispmap *thiz;

	thiz = ENESIM_RENDERER_DISPMAP(r);

	return thiz->scale;
}
