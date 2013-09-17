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
	Enesim_F16p16_Matrix matrix;
	Eina_F16p16 s_scale;
} Enesim_Renderer_Dispmap;

typedef struct _Enesim_Renderer_Dispmap_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Dispmap_Class;

/* TODO Move this to a common header */
static inline uint8_t _argb8888_alpha(uint32_t argb8888)
{
	return (argb8888 >> 24);
}

static inline uint8_t _argb8888_red(uint32_t argb8888)
{
	return (argb8888 >> 16) & 0xff;
}

static inline uint8_t _argb8888_green(uint32_t argb8888)
{
	return (argb8888 >> 8) & 0xff;
}

static inline uint8_t _argb8888_blue(uint32_t argb8888)
{
	return argb8888 & 0xff;
}

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
	uint32_t *dst = ddata;							\
	uint32_t *end = dst + len;						\
	uint32_t *map, *src;							\
	size_t mstride;								\
	size_t sstride;								\
	int sw, sh, mw, mh;							\
	Eina_F16p16 xx, yy;							\
										\
	thiz = ENESIM_RENDERER_DISPMAP(r);					\
	/* setup the parameters */						\
	enesim_surface_size_get(thiz->src, &sw, &sh);				\
	enesim_surface_size_get(thiz->map, &mw, &mh);				\
	enesim_surface_data_get(thiz->map, (void **)&map, &mstride);		\
	enesim_surface_data_get(thiz->src, (void **)&src, &sstride);		\
										\
	enesim_coord_identity_setup(&xx, &yy, x, y, thiz->ox, thiz->oy);	\
	x = eina_f16p16_int_to(xx);						\
	y = eina_f16p16_int_to(yy);						\
	map = argb8888_at(map, mstride, x, y);					\
										\
	while (dst < end)							\
	{									\
		Eina_F16p16 sxx, syy;						\
		int sx, sy;							\
		uint32_t p0 = 0;						\
		uint16_t m0;							\
		uint16_t m1;							\
										\
		/* TODO fix this, no need for it */				\
		x = eina_f16p16_int_to(xx);					\
		if (x < 0 || x >= mw || y < 0 || y >= mh)			\
			goto next;						\
										\
		m1 = yfunction(*map);						\
		m0 = xfunction(*map);						\
		sxx = _displace(xx, m0, thiz->s_scale);				\
		syy = _displace(yy, m1, thiz->s_scale);				\
										\
		sx = eina_f16p16_int_to(sxx);					\
		sy = eina_f16p16_int_to(syy);					\
		p0 = argb8888_sample_good(src, sstride, sw, sh, sxx, syy,	\
				sx, sy);					\
										\
next:										\
		*dst++ = p0;							\
		map++;								\
		xx += EINA_F16P16_ONE;					\
	}									\
}

#define DISPMAP_AFFINE(xch, ych, xfunction, yfunction) \
static void _argb8888_##xch##_##ych##_span_affine(Enesim_Renderer *r,		\
		int x, int y, int len,						\
		void *ddata)							\
{										\
	Enesim_Renderer_Dispmap *thiz;						\
	uint32_t *dst = ddata;							\
	uint32_t *end = dst + len;						\
	uint32_t *src;								\
	uint32_t *map;								\
	size_t mstride;								\
	size_t sstride;								\
	int sw, sh, mw, mh;							\
	Eina_F16p16 xx, yy;							\
										\
	thiz = ENESIM_RENDERER_DISPMAP(r);					\
	/* setup the parameters */						\
	enesim_surface_size_get(thiz->src, &sw, &sh);				\
	enesim_surface_size_get(thiz->map, &mw, &mh);				\
	enesim_surface_data_get(thiz->map, (void **)&map, &mstride);		\
	enesim_surface_data_get(thiz->src, (void **)&src, &sstride);		\
										\
	/* TODO move by the origin */						\
	enesim_coord_affine_setup(&xx, &yy, x, y, thiz->ox, thiz->oy,		\
			&thiz->matrix);						\
										\
	while (dst < end)							\
	{									\
		Eina_F16p16 sxx, syy;						\
		int sx, sy;							\
		uint32_t p0 = 0;						\
		uint32_t *m;							\
		uint16_t m0;							\
		uint16_t m1;							\
										\
		x = eina_f16p16_int_to(xx);					\
		y = eina_f16p16_int_to(yy);					\
										\
		if (x < 0 || x >= mw || y < 0 || y >= mh)			\
			goto next;						\
										\
		m = argb8888_at(map, mstride, x, y);				\
		m1 = yfunction(*m);						\
		m0 = xfunction(*m);						\
										\
		sxx = _displace(xx, m0, thiz->s_scale);				\
		syy = _displace(yy, m1, thiz->s_scale);				\
										\
		sx = eina_f16p16_int_to(sxx);					\
		sy = eina_f16p16_int_to(syy);					\
		p0 = argb8888_sample_good(src, sstride, sw, sh, sxx, syy, sx,	\
				sy);						\
										\
next:										\
		*dst++ = p0;							\
		map++;								\
		yy += thiz->matrix.yx;						\
		xx += thiz->matrix.xx;						\
	}									\
}

DISPMAP_AFFINE(r, g, _argb8888_red, _argb8888_green);
DISPMAP_AFFINE(a, b, _argb8888_alpha, _argb8888_blue);
DISPMAP_IDENTITY(a, b, _argb8888_alpha, _argb8888_blue);
DISPMAP_IDENTITY(r, g, _argb8888_red, _argb8888_green);

static Enesim_Renderer_Sw_Fill _spans[ENESIM_CHANNELS][ENESIM_CHANNELS][ENESIM_MATRIX_TYPES];
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _dispmap_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "dispmap";
}

static void _dispmap_sw_cleanup(Enesim_Renderer *r EINA_UNUSED, Enesim_Surface *s EINA_UNUSED)
{

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

	enesim_renderer_origin_get(r, &thiz->ox, &thiz->oy);
	enesim_renderer_transformation_get(r, &m);
	enesim_renderer_transformation_type_get(r, &type);
	enesim_matrix_f16p16_matrix_to(&m, &thiz->matrix);

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
	klass->sw_setup = _dispmap_sw_setup;
	klass->sw_cleanup = _dispmap_sw_cleanup;

	_spans[ENESIM_CHANNEL_ALPHA][ENESIM_CHANNEL_BLUE][ENESIM_MATRIX_IDENTITY]
		= _argb8888_a_b_span_identity;
	_spans[ENESIM_CHANNEL_ALPHA][ENESIM_CHANNEL_BLUE][ENESIM_MATRIX_AFFINE]
		= _argb8888_a_b_span_affine;
	_spans[ENESIM_CHANNEL_RED][ENESIM_CHANNEL_GREEN][ENESIM_MATRIX_IDENTITY]
		= _argb8888_r_g_span_identity;
	_spans[ENESIM_CHANNEL_RED][ENESIM_CHANNEL_GREEN][ENESIM_MATRIX_AFFINE]
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
 * @param[in] r The displacement map renderer
 * @param[in] map The surface map [transfer full]
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
 * @param[in] r The displacement map renderer
 * @param[in] map The surface map [transfer full]
 */
EAPI Enesim_Surface * enesim_renderer_dispmap_map_surface_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Dispmap *thiz;

	thiz = ENESIM_RENDERER_DISPMAP(r);
	return enesim_surface_ref(thiz->map);
}

/**
 * @brief Sets the source surface that will be displaced using the map surface
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
 * @param[in] r The displacement map renderer
 * @param[factor] The factor to use
 */
EAPI void enesim_renderer_dispmap_factor_set(Enesim_Renderer *r, double factor)
{
	Enesim_Renderer_Dispmap *thiz;

	thiz = ENESIM_RENDERER_DISPMAP(r);

	thiz->scale = factor;
}

/**
 * @brief Gets the factor of displacement for the displacement renderer
 * @param[in] r The displacement map renderer
 * @return The factor to use
 */
EAPI double enesim_renderer_dispmap_factor_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Dispmap *thiz;

	thiz = ENESIM_RENDERER_DISPMAP(r);

	return thiz->scale;
}
