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
#include "enesim_renderer_importer.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_renderer_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_RENDERER_IMPORTER(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Importer,					\
		enesim_renderer_importer_descriptor_get())

typedef struct _Enesim_Renderer_Importer
{
	Enesim_Renderer parent;
	Enesim_Buffer *buffer;
	Enesim_Buffer_Sw_Data cdata;
	Enesim_Buffer_Format cfmt;
} Enesim_Renderer_Importer;

typedef struct _Enesim_Renderer_Importer_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Importer_Class;

static void _span_argb8888_none_argb8888(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Importer *thiz;
	uint32_t *dst = ddata;
	uint32_t *ssrc;
	size_t stride;

	thiz = ENESIM_RENDERER_IMPORTER(r);
	ssrc = thiz->cdata.argb8888.plane0;
	stride = thiz->cdata.argb8888.plane0_stride;
	ssrc = (uint32_t *)((uint8_t *)ssrc + (stride * y) + x);
	while (len--)
	{
		uint16_t a = (*ssrc >> 24) + 1;

		if (a != 256)
		{
			*dst = (*ssrc & 0xff000000) + (((((*ssrc) >> 8) & 0xff) * a) & 0xff00) +
			(((((*ssrc) & 0x00ff00ff) * a) >> 8) & 0x00ff00ff);
		}
		else
			*dst = *ssrc;

		dst++;
		ssrc++;
	}
}

static void _span_a8_none_argb8888(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Importer *thiz;
	uint32_t *dst = ddata;
	uint8_t *ssrc;
	size_t stride;

	thiz = ENESIM_RENDERER_IMPORTER(r);
 	ssrc = thiz->cdata.a8.plane0;
	stride = thiz->cdata.a8.plane0_stride;
	ssrc = ssrc + (stride * y) + x;
	while (len--)
	{
		*dst = *ssrc << 24;;

		dst++;
		ssrc++;
	}
}

static void _span_rgb888_none_argb8888(Enesim_Renderer *rend,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Importer *thiz;
	uint32_t *dst = ddata;
	uint8_t *ssrc;
	size_t stride;

	thiz = ENESIM_RENDERER_IMPORTER(rend);
 	ssrc = thiz->cdata.rgb888.plane0;
	stride = thiz->cdata.rgb888.plane0_stride;
	ssrc = ssrc + (stride * y) + x;
	while (len--)
	{
		uint8_t r, g, b;

		r = *ssrc++;
		g = *ssrc++;
		b = *ssrc++;
		*dst = 0xff000000 | r << 16 | g << 8 | b;

		dst++;
	}
}

static void _span_bgr888_none_argb8888(Enesim_Renderer *rend,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Importer *thiz;
	uint32_t *dst = ddata;
	uint8_t *ssrc;
	size_t stride;

	thiz = ENESIM_RENDERER_IMPORTER(rend);
 	ssrc = thiz->cdata.bgr888.plane0;
	stride = thiz->cdata.bgr888.plane0_stride;
	ssrc = ssrc + (stride * y) + x;
	while (len--)
	{
		uint8_t r, g, b;

		b = *ssrc++;
		g = *ssrc++;
		r = *ssrc++;
		*dst = 0xff000000 | r << 16 | g << 8 | b;

		dst++;
	}
}

/* Conversion from CMYK to RGB is done in 2 steps: */
/* CMYK => CMY => RGB (see http://www.easyrgb.com/index.php?X=MATH) */
/* after computation, if C, M, Y and K are between 0 and 1, we have: */
/* R = (1 - C) * (1 - K) * 255 */
/* G = (1 - M) * (1 - K) * 255 */
/* B = (1 - Y) * (1 - K) * 255 */
/* libjpeg stores CMYK values between 0 and 255, */
/* so we replace C by C * 255 / 255, etc... and we obtain: */
/* R = (255 - C) * (255 - K) / 255 */
/* G = (255 - M) * (255 - K) / 255 */
/* B = (255 - Y) * (255 - K) / 255 */
/* with C, M, Y and K between 0 and 255. */
/*
 * Note: a / 255 is well approximated by (a + 255) / 256
 */
static void _span_cmyk_none_argb8888(Enesim_Renderer *rend,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Importer *thiz;
	uint32_t *dst = ddata;
	uint8_t *ssrc;
	size_t stride;

	thiz = ENESIM_RENDERER_IMPORTER(rend);
 	ssrc = thiz->cdata.cmyk.plane0;
	stride = thiz->cdata.cmyk.plane0_stride;
	ssrc = ssrc + (stride * y) + x;
	while (len--)
	{
		uint8_t r, g, b;

		r = ((255 - ssrc[0]) * (255 - ssrc[3]) + 255) >> 8;
		g = ((255 - ssrc[1]) * (255 - ssrc[3]) + 255) >> 8;
		b = ((255 - ssrc[2]) * (255 - ssrc[3]) + 255) >> 8;
		*dst = 0xff000000 | r << 16 | g << 8 | b;

		dst++;
		ssrc += 4;
	}
}

/* According to libjpeg doc, Photoshop inverse the values of C, M, Y and K, */
/* that is C is replaces by 255 - C, etc...*/
/* See the comment above for the computation of RGB values from CMYK ones. */
static void _span_cmyk_adobe_none_argb8888(Enesim_Renderer *rend,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Importer *thiz;
	uint32_t *dst = ddata;
	uint8_t *ssrc;
	size_t stride;

	thiz = ENESIM_RENDERER_IMPORTER(rend);
 	ssrc = thiz->cdata.cmyk.plane0;
	stride = thiz->cdata.cmyk.plane0_stride;
	ssrc = ssrc + (stride * y) + x;
	while (len--)
	{
		uint8_t r, g, b;

		r = (ssrc[0] * ssrc[3] + 255) >> 8;
		g = (ssrc[1] * ssrc[3] + 255) >> 8;
		b = (ssrc[2] * ssrc[3] + 255) >> 8;
		*dst = 0xff000000 | r << 16 | g << 8 | b;

		dst++;
		ssrc += 4;
	}
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char *_importer_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "importer";
}

static Eina_Bool _importer_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l)
{
	Enesim_Renderer_Importer *thiz;

	thiz = ENESIM_RENDERER_IMPORTER(r);
	if (!thiz->buffer) return EINA_FALSE;

	enesim_buffer_data_get(thiz->buffer, &thiz->cdata);
	thiz->cfmt = enesim_buffer_format_get(thiz->buffer);
	/* TODO use a LUT for this */
	switch (thiz->cfmt)
	{
		case ENESIM_BUFFER_FORMAT_ARGB8888:
		*fill = _span_argb8888_none_argb8888;
		break;

		case ENESIM_BUFFER_FORMAT_A8:
		*fill = _span_a8_none_argb8888;
		break;

		case ENESIM_BUFFER_FORMAT_RGB888:
		*fill = _span_rgb888_none_argb8888;
		break;

		case ENESIM_BUFFER_FORMAT_BGR888:
		*fill = _span_bgr888_none_argb8888;
		break;

		case ENESIM_BUFFER_FORMAT_CMYK:
		*fill = _span_cmyk_none_argb8888;
		break;

		case ENESIM_BUFFER_FORMAT_CMYK_ADOBE:
		*fill = _span_cmyk_adobe_none_argb8888;
		break;

		default:
		ENESIM_RENDERER_LOG(r, l, "Invalid format %d", thiz->cfmt);
		return EINA_FALSE;
		break;
	}
	return EINA_TRUE;
}

static void _importer_bounds(Enesim_Renderer *r,
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Importer *thiz;

	thiz = ENESIM_RENDERER_IMPORTER(r);
	if (thiz->buffer)
	{
		int w, h;

		enesim_buffer_size_get(thiz->buffer, &w, &h);
		rect->x = 0;
		rect->y = 0;
		rect->w = w;
		rect->h = h;
	}
	else
	{
		rect->x = 0;
		rect->y = 0;
		rect->w = 0;
		rect->h = 0;
	}
}

static void _importer_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_ARGB8888;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Importer, Enesim_Renderer_Importer_Class,
		enesim_renderer_importer);

static void _enesim_renderer_importer_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _importer_name;
	klass->bounds_get = _importer_bounds;
 	klass->features_get = _importer_features_get;
	klass->sw_setup =  _importer_sw_setup;
}

static void _enesim_renderer_importer_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_renderer_importer_instance_deinit(void *o EINA_UNUSED)
{
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates an importer renderer
 *
 * An importer renderer imports pixels from different surface formats into
 * a native enesim format
 * @return The renderer
 */
EAPI Enesim_Renderer * enesim_renderer_importer_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_importer);
	return r;
}

/**
 * Sets the buffer to import pixels from
 * @param[in] r The importer renderer
 * @param[in] buffer The buffer
 */
EAPI void enesim_renderer_importer_buffer_set(Enesim_Renderer *r, Enesim_Buffer *buffer)
{
	Enesim_Renderer_Importer *thiz;

	thiz = ENESIM_RENDERER_IMPORTER(r);
	if (thiz->buffer)
		enesim_buffer_unref(thiz->buffer);
	thiz->buffer = buffer;
}

/**
 * Gets the buffer to import pixels from
 * @param[in] r The importer renderer
 * @return The buffer [transfer none]
 */
EAPI Enesim_Buffer * enesim_renderer_importer_buffer_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Importer *thiz;

	thiz = ENESIM_RENDERER_IMPORTER(r);
	if (thiz->buffer)
		return enesim_buffer_ref(thiz->buffer);
	return NULL;
}


