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
#include "libargb.h"

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"
#include "enesim_renderer_blur.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_renderer_private.h"
/*
 * A box-like blur filter - initial slow version.
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define MUL_A_256(a, c) \
 ( (((((c) >> 8) & 0x00ff00ff) * (a)) & 0xff00ff00) + \
   (((((c) & 0x00ff00ff) * (a)) >> 8) & 0x00ff00ff) )

#define ENESIM_RENDERER_BLUR(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Blur,					\
		enesim_renderer_blur_descriptor_get())


static int _atable[256];
static void _init_atable(void)
{
	double  a = M_PI/549.667, b;
	int  p = 0;

	while (p < 256)
	{
		b = cos(p * a);
		_atable[255 - p] = 0.5 + (256 * b);
		p++;
	}
}


typedef struct _Enesim_Renderer_Blur
{
	Enesim_Renderer parent;
	Enesim_Surface *src;
	Enesim_Blur_Channel channel;
	double rx, ry;
	/* The state variables */
	Enesim_Color color;
//	int irx, iry;
//	int iaxx, iayy;
	int ibxx, ibyy;
} Enesim_Renderer_Blur;

typedef struct _Enesim_Renderer_Blur_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Blur_Class;

static Eina_Bool _blur_state_setup(Enesim_Renderer_Blur *thiz,
		Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED,
		Enesim_Log **l)
{
	if (!thiz->src)
	{
		ENESIM_RENDERER_LOG(r, l, "No surface set");
		return EINA_FALSE;
	}
	/* lock the surface for read only */
	enesim_surface_lock(thiz->src, EINA_FALSE);
	return EINA_TRUE;
}

static void _blur_state_cleanup(Enesim_Renderer_Blur *thiz,
		Enesim_Renderer *r EINA_UNUSED, Enesim_Surface *s EINA_UNUSED)
{
	if (thiz->src)
		enesim_surface_unlock(thiz->src);
}

static Enesim_Renderer_Sw_Fill _spans[ENESIM_BLUR_CHANNELS];
/*----------------------------------------------------------------------------*
 *                        The Software fill variants                          *
 *----------------------------------------------------------------------------*/
static void _argb8888_span_identity(Enesim_Renderer *r,
		int x, int y, unsigned int len,
		void *ddata)
{
	Enesim_Renderer_Blur *thiz;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	uint32_t *src, *pbuf;
	size_t sstride;
	int sw, sh;
	Enesim_Color color;
	Eina_F16p16 xx, yy;
	int ibyy, ibxx;
	int ix, ix0, iy, iy0;
	int txx0, ntxx0, tx0, ntx0;
	int tyy0, ntyy0, ty0, nty0;

	thiz = ENESIM_RENDERER_BLUR(r);
 	color = thiz->color;
	ibyy = thiz->ibyy, ibxx = thiz->ibxx;
	/* setup the parameters */

	ix = ix0 = x;  iy = iy0 = y;
	xx = (ibxx * x) + (ibxx >> 1) - 32768;
	yy = (ibyy * y) + (ibyy >> 1) - 32768;
	x = xx >> 16;
	y = yy >> 16;

	enesim_surface_size_get(thiz->src, &sw, &sh);
	if ((iy < 0) || (iy >= sh))
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	enesim_surface_data_get(thiz->src, (void **)&src, &sstride);
	src = argb8888_at(src, sstride, ix, iy);

	tyy0 = yy & 0xffff0000;  ty0 = (tyy0 >> 16);
	ntyy0 = tyy0 + ibyy;  nty0 = (ntyy0 >> 16);

	txx0 = xx & 0xffff0000;  tx0 = (txx0 >> 16);
	ntxx0 = txx0 + ibxx;  ntx0 = (ntxx0 >> 16);

	while (dst < end)
	{
		unsigned int p0 = 0;

		if ( ((unsigned)ix < sw) )
		{
			unsigned int ag0 = 0, rb0 = 0, ag3 = 0, rb3 = 0, dag = 0, drb = 0;
			int tyy = tyy0, ty = ty0, ntyy = ntyy0, nty = nty0;

			pbuf = src + (ix - ix0);  iy = iy0;

			while (iy < sh)
			{
				unsigned int ag2 = 0, rb2 = 0, pag2 = 0, prb2 = 0, pp3 = 0;
				unsigned int *p = pbuf;
				int txx = txx0, tx = tx0, ntxx = ntxx0, ntx = ntx0;
				int a;

				while (1)
				{
					unsigned int p3 = *p;

					if (ntx != tx)
					{
						if (ntx != x)
						{
							a = (65536 - (txx & 0xffff));
							ag2 += ((((p3 >> 16) & 0xff00) * a) & 0xffff0000) +
								(((p3 & 0xff00) * a) >> 16);
							rb2 += ((((p3 >> 8) & 0xff00) * a) & 0xffff0000) +
								(((p3 & 0xff) * a) >> 8);
							break;
						}
						a = (1 + (ntxx & 0xffff));
						ag2 = ((((p3 >> 16) & 0xff00) * a) & 0xffff0000) +
							(((p3 & 0xff00) * a) >> 16);
						rb2 = ((((p3 >> 8) & 0xff00) * a) & 0xffff0000) +
							(((p3 & 0xff) * a) >> 8);
						tx = ntx;
					}
					else if (p3)
					{
						if (p3 != pp3)
						{
							pag2 = ((((p3 >> 16) & 0xff00) * ibxx) & 0xffff0000) +
								(((p3 & 0xff00) * ibxx) >> 16);
							prb2 = ((((p3 >> 8) & 0xff00) * ibxx) & 0xffff0000) +
								(((p3 & 0xff) * ibxx) >> 8);
							pp3 = p3;
						}
						ag2 += pag2;   rb2 += prb2;
					}

					p++;  txx = ntxx;  ntxx += ibxx;  ntx = (ntxx >> 16);
				}

				if (nty != ty)
				{
					if (nty != y)
					{
						a = (65536 - (tyy & 0xffff));
						ag0 += (((ag2 >> 16) * a) & 0xffff0000) +
							(((ag2 & 0xffff) * a) >> 16);
						rb0 += (((rb2 >> 16) * a) & 0xffff0000) +
							(((rb2 & 0xffff) * a) >> 16);
						break;
					}
					a = (1 + (ntyy & 0xffff));
					ag0 = (((ag2 >> 16) * a) & 0xffff0000) +
						(((ag2 & 0xffff) * a) >> 16);
					rb0 = (((rb2 >> 16) * a) & 0xffff0000) +
						(((rb2 & 0xffff) * a) >> 16);
					ty = nty;
				}
				else if (ag2 | rb2)
				{
					if ((ag2 != ag3) || (rb2 != rb3))
					{
						dag = (((ag2 >> 16) * ibyy) & 0xffff0000) +
							(((ag2 & 0xffff) * ibyy) >> 16);
						drb = (((rb2 >>16) * ibyy) & 0xffff0000) +
							(((rb2 & 0xffff) * ibyy) >> 16);
							ag3 = ag2;  rb3 = rb2;
					}
					ag0 += dag;   rb0 += drb;
				}

				pbuf += sw;  tyy = ntyy;  ntyy += ibyy;  nty = (ntyy >> 16);  iy++;

				p0 = ((ag0 + 0xff00ff) & 0xff00ff00) +
					(((rb0 + 0xff00ff) >> 8) & 0xff00ff);
				// apply an alpha modifier to dampen alphas more smoothly
				if ((a = (p0 >> 24)) && (a < 234))
				{ a = _atable[a];  p0 = MUL_A_256(a, p0); }

				if (color != 0xffffffff)
					p0 = argb8888_mul4_sym(p0, color);
			}
		}

		*dst++ = p0;  xx += ibxx;  x = (xx >> 16);  ix++;
		txx0 = xx & 0xffff0000;  tx0 = (txx0 >> 16);
		ntxx0 = (txx0 + ibxx);  ntx0 = (ntxx0 >> 16);
	}
}

static void _a8_span_identity(Enesim_Renderer *r,
		int x, int y, unsigned int len,
		void *ddata)
{
	Enesim_Renderer_Blur *thiz;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	uint32_t *src, *pbuf;
	size_t sstride;
	int sw, sh;
	Enesim_Color color;
	Eina_F16p16 xx, yy;
	int ibyy, ibxx;
	int ix, ix0, iy, iy0;
	int txx0, ntxx0, tx0, ntx0;
	int tyy0, ntyy0, ty0, nty0;

	thiz = ENESIM_RENDERER_BLUR(r);
	color = thiz->color;

	ibyy = thiz->ibyy, ibxx = thiz->ibxx;
	/* setup the parameters */

	ix = ix0 = x;  iy = iy0 = y;
	xx = (ibxx * x) + (ibxx >> 1) - 32768;
	yy = (ibyy * y) + (ibyy >> 1) - 32768;
	x = xx >> 16;
	y = yy >> 16;

	enesim_surface_size_get(thiz->src, &sw, &sh);
	if ((iy < 0) || (iy >= sh))
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	enesim_surface_data_get(thiz->src, (void **)&src, &sstride);
	src = argb8888_at(src, sstride, ix, iy);

	tyy0 = yy & 0xffff0000;  ty0 = (tyy0 >> 16);
	ntyy0 = tyy0 + ibyy;  nty0 = (ntyy0 >> 16);

	txx0 = xx & 0xffff0000;  tx0 = (txx0 >> 16);
	ntxx0 = txx0 + ibxx;  ntx0 = (ntxx0 >> 16);

	while (dst < end)
	{
		unsigned int p0 = 0;

		if ( ((unsigned)ix < sw) )
		{
			unsigned int a0 = 0, a3 = 0, da = 0;
			int tyy = tyy0, ty = ty0, ntyy = ntyy0, nty = nty0;

			pbuf = src + (ix - ix0);
			iy = iy0;
			while (iy < sh)
			{
				unsigned int a2 = 0, pa2 = 0, pp3 = 0;
				int txx = txx0, tx = tx0, ntxx = ntxx0, ntx = ntx0;
				unsigned int *p = pbuf;

				while (1)
				{
					unsigned int p3 = *p;

					if (ntx != tx)
					{
						if (ntx != x)
						{
							if (p3)
							{
								a2 += ((((p3 >> 16) & 0xff00) * (65536 - (txx & 0xffff))) & 0xffff0000);
							}
							break;
						}
						a2 = 0;
						if (p3)
							a2 = ((((p3 >> 16) & 0xff00) * (1 + (ntxx & 0xffff))) & 0xffff0000);
						tx = ntx;
					}
					else if (p3)
					{
						if (p3 != pp3)
						{
							pa2 = ((((p3 >> 16) & 0xff00) * ibxx) & 0xffff0000);
							pp3 = p3;
						}
						a2 += pa2;
					}

					p++; txx = ntxx;  ntxx += ibxx;  ntx = (ntxx >> 16);
				}

				if (nty != ty)
				{
					if (nty != y)
					{
						a0 += (((a2 >> 16) * (65536 - (tyy & 0xffff))) & 0xffff0000);
						break;
					}
					a0 = (((a2 >> 16) * (1 + (ntyy & 0xffff))) & 0xffff0000);
					ty = nty;
				}
				else if (a2)
				{
					if (a2 != a3)
					{
						da = (((a2 >> 16) * ibyy) & 0xffff0000);
						a3 = a2;
					}
					a0 += da;
				}

				pbuf += sw;  tyy = ntyy;  ntyy += ibyy;  nty = (ntyy >> 16);  iy++;
				p0 = ((a0 + 0xff0000) & 0xff000000);
				// apply an alpha modifier to dampen alphas more smoothly
				if ((da = (p0 >> 24)) && (da < 234))
				{ da = _atable[da];  p0 = MUL_A_256(da, p0); }

				if (color != 0xffffffff)
					p0 = MUL_A_256(1 + (p0 >> 24), color);
			}
		}

		*dst++ = p0;  xx += ibxx;  x = (xx >> 16);  ix++;
		txx0 = xx & 0xffff0000;  tx0 = (txx0 >> 16);
		ntxx0 = (txx0 + ibxx);  ntx0 = (ntxx0 >> 16);
	}
}

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _blur_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "blur";
}

static Eina_Bool _blur_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l)
{
	Enesim_Renderer_Blur *thiz;
	double rx, ry;

	thiz = ENESIM_RENDERER_BLUR(r);
	enesim_renderer_color_get(r, &thiz->color);
	if (!_blur_state_setup(thiz, r, s, l))
		return EINA_FALSE;

	rx = ((2 * thiz->rx) + 1.01) / 2.0;
	if (rx <= 1) rx = 1.005;
	if (rx > 16) rx = 16;

	thiz->ibxx = 65536 / rx;

	ry = ((2 * thiz->ry) + 1.01) / 2.0;
	if (ry <= 1) ry = 1.005;
	if (ry > 16) ry = 16;

	thiz->ibyy = 65536 / ry;

	*fill = _spans[thiz->channel];
	if (!*fill) return EINA_FALSE;

	return EINA_TRUE;
}


static void _blur_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Blur *thiz;

	thiz = ENESIM_RENDERER_BLUR(r);
	_blur_state_cleanup(thiz, r, s);
}

static void _blur_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Blur *thiz;

	thiz = ENESIM_RENDERER_BLUR(r);
	if (!thiz->src)
	{
		rect->x = 0;
		rect->y = 0;
		rect->w = 0;
		rect->h = 0;
	}
	else
	{
		int sw, sh;

		enesim_surface_size_get(thiz->src, &sw, &sh);
		rect->x = 0;
		rect->y = 0;
		rect->w = sw;
		rect->h = sh;
	}
}

static void _blur_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_ARGB8888;
}

static void _blur_sw_hints_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_COLORIZE;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Blur, Enesim_Renderer_Blur_Class,
		enesim_renderer_blur);

static void _enesim_renderer_blur_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _blur_name;
	klass->bounds_get = _blur_bounds_get;
	klass->features_get = _blur_features_get;
	klass->sw_hints_get = _blur_sw_hints_get;
	klass->sw_setup = _blur_sw_setup;
	klass->sw_cleanup = _blur_sw_cleanup;
	/* initialize the static information */
	_init_atable();
	_spans[ENESIM_BLUR_CHANNEL_COLOR]
		= _argb8888_span_identity;
	_spans[ENESIM_BLUR_CHANNEL_ALPHA]
		= _a8_span_identity;
}

static void _enesim_renderer_blur_instance_init(void *o)
{
	Enesim_Renderer_Blur *thiz = ENESIM_RENDERER_BLUR(o);

	/* specific renderer setup */
	thiz->channel = ENESIM_BLUR_CHANNEL_COLOR;
	thiz->rx = thiz->ry = 0.5;
	/* common renderer setup */
}

static void _enesim_renderer_blur_instance_deinit(void *o EINA_UNUSED)
{
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a new blur filter renderer
 *
 * @return The renderer
 */
EAPI Enesim_Renderer * enesim_renderer_blur_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_blur);
	return r;
}
/**
 * Sets the channel to use in the src data
 * @param[in] r The blur filter renderer
 * @param[in] channel The channel to use
 */
EAPI void enesim_renderer_blur_channel_set(Enesim_Renderer *r,
	Enesim_Blur_Channel channel)
{
	Enesim_Renderer_Blur *thiz;

	thiz = ENESIM_RENDERER_BLUR(r);

	thiz->channel = channel;
}
/**
 * Gets the channel used in the src data
 * @param[in] r The blur filter renderer
 * returns the channel used
 */
EAPI Enesim_Blur_Channel enesim_renderer_blur_channel_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Blur *thiz;

	thiz = ENESIM_RENDERER_BLUR(r);

	return thiz->channel;
}

/**
 * Sets the surface to use as the src data
 * @param[in] r The blur filter renderer
 * @param[in] src The surface to use
 */
EAPI void enesim_renderer_blur_src_set(Enesim_Renderer *r, Enesim_Surface *src)
{
	Enesim_Renderer_Blur *thiz;

	thiz = ENESIM_RENDERER_BLUR(r);
	if (thiz->src)
		enesim_surface_unref(thiz->src);
	thiz->src = src;
	if (thiz->src)
		thiz->src = enesim_surface_ref(thiz->src);
}
/**
 *
 */
EAPI void enesim_renderer_blur_src_get(Enesim_Renderer *r, Enesim_Surface **src)
{
	Enesim_Renderer_Blur *thiz;

	if (!src) return;
	thiz = ENESIM_RENDERER_BLUR(r);
	*src = thiz->src;
	if (thiz->src)
		thiz->src = enesim_surface_ref(thiz->src);
}
/**
 * Sets the blur radius in the x direction
 * @param[in] r The blur filter renderer
 * @param[in] rx The blur radius in the x direction
 */
EAPI void enesim_renderer_blur_radius_x_set(Enesim_Renderer *r, double rx)
{
	Enesim_Renderer_Blur *thiz;

	thiz = ENESIM_RENDERER_BLUR(r);

	thiz->rx = rx;
}
/**
 * Gets the blur radius used in the x direction
 * @param[in] r The blur filter renderer
 * returns the blur x direction radius used
 */
EAPI double enesim_renderer_blur_radius_x_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Blur *thiz;

	thiz = ENESIM_RENDERER_BLUR(r);

	return thiz->rx;
}

/**
 * Sets the blur radius in the y direction
 * @param[in] r The blur filter renderer
 * @param[in] ry The blur radius in the y direction
 */
EAPI void enesim_renderer_blur_radius_y_set(Enesim_Renderer *r, double ry)
{
	Enesim_Renderer_Blur *thiz;

	thiz = ENESIM_RENDERER_BLUR(r);

	thiz->ry = ry;
}
/**
 * Gets the blur radius used in the y direction
 * @param[in] r The blur filter renderer
 * returns the blur y direction radius used
 */
EAPI double enesim_renderer_blur_radius_y_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Blur *thiz;

	thiz = ENESIM_RENDERER_BLUR(r);

	return thiz->ry;
}

