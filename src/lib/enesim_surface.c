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
static inline void _surface_setup(Enesim_Surface *s, Enesim_Format fmt)
{
	EINA_MAGIC_SET(s, ENESIM_MAGIC_SURFACE);
	s->format = fmt;
}

static inline Eina_Bool _format_to_buffer_format(Enesim_Format fmt,
		Enesim_Buffer_Format *buf_fmt)
{
	switch (fmt)
	{
		case ENESIM_FORMAT_ARGB8888:
		case ENESIM_FORMAT_ARGB8888_SPARSE:
		case ENESIM_FORMAT_XRGB8888:
		*buf_fmt = ENESIM_CONVERTER_ARGB8888_PRE;
		return EINA_TRUE;
		
		case ENESIM_FORMAT_A8:
		*buf_fmt = ENESIM_CONVERTER_A8;
		return EINA_TRUE;

		default:
		return EINA_FALSE;
	}
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Surface *
enesim_surface_new_data_from(Enesim_Backend b, Enesim_Format fmt,
		uint32_t w, uint32_t h, Enesim_Buffer_Data *data)
{
	Enesim_Surface *s;
	Enesim_Buffer_Format buf_fmt;

	if ((!w) || (!h)) return NULL;
	if (!_format_to_buffer_format(fmt, &buf_fmt))
		return NULL;

	s = calloc(1, sizeof(Enesim_Surface));
	if (!enesim_buffer_setup(&s->buffer, b,
		buf_fmt, w, h, data, NULL))
	{
		free(s);
		return NULL;
	}
	_surface_setup(s, fmt);

	return s;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Surface *
enesim_surface_new_pool_from(Enesim_Backend b, Enesim_Format f,
		uint32_t w, uint32_t h, Enesim_Pool *p)
{
	Enesim_Surface *s;
	Enesim_Buffer_Format buf_fmt;

	if ((!w) || (!h)) return NULL;
	if (!p) return enesim_surface_new(b, f, w, h);
	
	if (!_format_to_buffer_format(f, &buf_fmt))
		return NULL;

	s = calloc(1, sizeof(Enesim_Surface));
	if (!enesim_buffer_setup(&s->buffer, b, buf_fmt,
			w, h, &s->buffer.data, p))
	{
		free(s);
		return NULL;
	}
	_surface_setup(s, f);

	return s;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Surface *
enesim_surface_new(Enesim_Backend b, Enesim_Format f, uint32_t w, uint32_t h)
{
	Enesim_Surface *s;

	s = enesim_surface_new_pool_from(b, f, w, h, &enesim_default_pool);

	return s;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void
enesim_surface_size_get(const Enesim_Surface *s, int *w, int *h)
{
	ENESIM_MAGIC_CHECK_SURFACE(s);
	if (w) *w = s->buffer.w;
	if (h) *h = s->buffer.h;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Format enesim_surface_format_get(const Enesim_Surface *s)
{
	ENESIM_MAGIC_CHECK_SURFACE(s);
	return s->format;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Backend enesim_surface_backend_get(const Enesim_Surface *s)
{
	ENESIM_MAGIC_CHECK_SURFACE(s);
	return s->buffer.backend;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void
enesim_surface_delete(Enesim_Surface *s)
{
	ENESIM_MAGIC_CHECK_SURFACE(s);

	enesim_buffer_cleanup(&s->buffer);
	free(s);
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void *
enesim_surface_data_get(const Enesim_Surface *s)
{
	ENESIM_MAGIC_CHECK_SURFACE(s);
	switch (s->format)
	{
		case ENESIM_FORMAT_ARGB8888:
		case ENESIM_FORMAT_ARGB8888_SPARSE:
		case ENESIM_FORMAT_XRGB8888:
		return s->buffer.data.argb8888_pre.plane0;

		case ENESIM_FORMAT_A8:
		return s->buffer.data.a8.plane0;

		default:
		return NULL;
	}
}
/**
 * FIXME rename this to enesim_color_components_from
 * Create a pixel from the given unpremultiplied components
 */
EAPI void enesim_surface_pixel_components_from(uint32_t *color,
		Enesim_Format f, uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
	switch (f)
	{
		case ENESIM_FORMAT_ARGB8888:
		{
			uint16_t alpha = a + 1;
			*color = (a << 24) | (((r * alpha) >> 8) << 16)
					| (((g * alpha) >> 8) << 8)
					| ((b * alpha) >> 8);
		}
		break;

		case ENESIM_FORMAT_A8:
		*color = a;
		break;

		default:
		break;
	}
}

/**
 * FIXME rename this to enesim_color_components_to
 */
EAPI void enesim_surface_pixel_components_to(uint32_t color,
		Enesim_Format f, uint8_t *a, uint8_t *r, uint8_t *g, uint8_t *b)
{
	switch (f)
	{
		case ENESIM_FORMAT_ARGB8888:
		{
			uint8_t pa;
			pa = (color >> 24);
			if ((pa > 0) && (pa < 255))
			{
				if (a) *a = pa;
				if (r) *r = (argb8888_red_get(color) * 255) / pa;
				if (g) *g = (argb8888_green_get(color) * 255) / pa;
				if (b) *b = (argb8888_blue_get(color) * 255) / pa;
			}
			else
			{
				if (a) *a = pa;
				if (r) *r = argb8888_red_get(color);
				if (g) *g = argb8888_green_get(color);
				if (b) *b = argb8888_blue_get(color);
			}
		}
		break;

		case ENESIM_FORMAT_A8:
		if (a) *a = (uint8_t)color;
		break;

		default:
		break;
	}
}

/**
 *
 */
EAPI uint32_t
enesim_surface_stride_get(Enesim_Surface *s)
{
	ENESIM_MAGIC_CHECK_SURFACE(s);
	return s->buffer.data.argb8888.plane0_stride;
}
/**
 * Store a private data pointer into the surface
 */
EAPI void
enesim_surface_private_set(Enesim_Surface *s, void *data)
{
	ENESIM_MAGIC_CHECK_SURFACE(s);
	s->user = data;
}

/**
 * Retrieve the private data pointer from the surface
 */
EAPI void *
enesim_surface_private_get(Enesim_Surface *s)
{
	ENESIM_MAGIC_CHECK_SURFACE(s);
	return s->user;
}


