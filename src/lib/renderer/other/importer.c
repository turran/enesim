/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2008 Jorge Luis Zapata
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
typedef struct _Enesim_Renderer_Importer
{
	Enesim_Buffer *buffer;
	Enesim_Buffer_Data cdata;
	Enesim_Buffer_Format cfmt;
	Enesim_Angle angle;
} Enesim_Renderer_Importer;

static inline Enesim_Renderer_Importer * _importer_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Importer *thiz;

	thiz = enesim_renderer_data_get(r);

	return thiz;
}

static void _span_argb8888_none_argb8888(Enesim_Renderer *r, int x, int y, unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Importer *thiz = (Enesim_Renderer_Importer *)r;
	uint32_t *ssrc = thiz->cdata.argb8888.plane0;

	ssrc = ssrc + (thiz->cdata.argb8888.plane0_stride * y) + x;
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

static void _span_a8_none_argb8888(Enesim_Renderer *r, int x, int y, unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Importer *thiz;
	uint8_t *ssrc;

	thiz = _importer_get(r);
 	ssrc = thiz->cdata.a8.plane0;
	ssrc = ssrc + (thiz->cdata.a8.plane0_stride * y) + x;
	while (len--)
	{
		*dst = *ssrc << 24;;

		dst++;
		ssrc++;
	}
}

static void _span_rgb888_none_argb8888(Enesim_Renderer *r, int x, int y, unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Importer *thiz;
	uint8_t *ssrc;

	thiz = _importer_get(r);
 	ssrc = thiz->cdata.rgb888.plane0;
	ssrc = ssrc + (thiz->cdata.rgb888.plane0_stride * y * 3) + (x * 3);
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
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static Eina_Bool _importer_state_setup(Enesim_Renderer *r, Enesim_Renderer_Sw_Fill *fill)
{
	Enesim_Renderer_Importer *thiz;

	thiz = _importer_get(r);
	if (!thiz->buffer) return EINA_FALSE;

	enesim_buffer_data_get(thiz->buffer, &thiz->cdata);
	thiz->cfmt = enesim_buffer_format_get(thiz->buffer);
	/* TODO use a LUT for this */
	switch (thiz->cfmt)
	{
		case ENESIM_CONVERTER_ARGB8888:
		*fill = _span_argb8888_none_argb8888;
		break;

		case ENESIM_CONVERTER_A8:
		*fill = _span_a8_none_argb8888;
		break;

		case ENESIM_CONVERTER_RGB888:
		*fill = _span_rgb888_none_argb8888;
		break;

		default:
		break;
	}
	return EINA_TRUE;
}

static void _importer_boundings(Enesim_Renderer *r, Enesim_Rectangle *rect)
{
	Enesim_Renderer_Importer *thiz;

	thiz = _importer_get(r);
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

static void _importer_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Importer *thiz;

	thiz = _importer_get(r);
	free(thiz);
}

static void _importer_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Renderer_Importer *thiz;

	thiz = _importer_get(r);
	if (!thiz)
	{
		*flags = 0;
		return;
	}
	*flags = ENESIM_RENDERER_FLAG_ARGB8888;
}

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version =    */ ENESIM_RENDERER_API,
	/* .free =       */ _importer_free,
	/* .boundings =  */ _importer_boundings,
	/* .flags =      */ _importer_flags,
	/* .is_inside =  */ NULL,
	/* .sw_setup =   */ _importer_state_setup,
	/* .sw_cleanup = */ NULL
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
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
	Enesim_Renderer_Importer *thiz;
	Enesim_Renderer *r;

	thiz = calloc(1, sizeof(Enesim_Renderer_Importer));
	if (!thiz) return NULL;

	r = enesim_renderer_new(&_descriptor, thiz);

	return r;
}
/**
 * Sets the angle
 * @param[in] r The importer renderer
 * @param[in] angle The angle
 */
EAPI void enesim_renderer_importer_angle_set(Enesim_Renderer *r, Enesim_Angle angle)
{
	Enesim_Renderer_Importer *thiz;

	thiz = _importer_get(r);
	thiz->angle = angle;
}

/**
 * Sets the buffer to import pixels from
 * @param[in] r The importer renderer
 * @param[in] buffer The buffer
 */
EAPI void enesim_renderer_importer_buffer_set(Enesim_Renderer *r, Enesim_Buffer *buffer)
{
	Enesim_Renderer_Importer *thiz;

	thiz = _importer_get(r);
	thiz->buffer = buffer;
}

