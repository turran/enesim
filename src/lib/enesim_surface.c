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
/* FIXME export this */
void * enesim_surface_backend_data_get(Enesim_Surface *s)
{
	return enesim_buffer_backend_data_get(s->buffer);
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Surface * enesim_surface_new_data_from(Enesim_Format fmt,
		uint32_t w, uint32_t h, Eina_Bool copy,
		Enesim_Buffer_Sw_Data *data)
{
	Enesim_Surface *s;
	Enesim_Buffer *b;
	Enesim_Buffer_Format buf_fmt;

	if ((!w) || (!h)) return NULL;
	if (!_format_to_buffer_format(fmt, &buf_fmt))
		return NULL;

	b = enesim_buffer_new_data_from(buf_fmt, w, h, copy, data);
	if (!b) return NULL;

	s = calloc(1, sizeof(Enesim_Surface));
	EINA_MAGIC_SET(s, ENESIM_MAGIC_SURFACE);
	s->format = fmt;
	s->buffer = b;

	return s;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Surface * enesim_surface_new_pool_from(Enesim_Format f,
		uint32_t w, uint32_t h, Enesim_Pool *p)
{
	Enesim_Surface *s;
	Enesim_Buffer *b;
	Enesim_Buffer_Format buf_fmt;

	if ((!w) || (!h)) return NULL;
	if (!_format_to_buffer_format(f, &buf_fmt))
		return NULL;

	b = enesim_buffer_new_pool_from(f, w, h, p);
	if (!b) return NULL;

	s = calloc(1, sizeof(Enesim_Surface));
	EINA_MAGIC_SET(s, ENESIM_MAGIC_SURFACE);
	s->format = f;
	s->buffer = b;

	return s;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Surface * enesim_surface_new(Enesim_Format f, uint32_t w, uint32_t h)
{
	Enesim_Surface *s;

	s = enesim_surface_new_pool_from(f, w, h, NULL);

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
	if (w) *w = s->buffer->w;
	if (h) *h = s->buffer->h;
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
	return enesim_buffer_backend_get(s->buffer);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void
enesim_surface_delete(Enesim_Surface *s)
{
	ENESIM_MAGIC_CHECK_SURFACE(s);

	enesim_buffer_delete(s->buffer);
	free(s);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Eina_Bool enesim_surface_data_get(Enesim_Surface *s, uint32_t **data, size_t *stride)
{
	Enesim_Buffer_Sw_Data sw_data;

	if (!data) return EINA_FALSE;
	ENESIM_MAGIC_CHECK_SURFACE(s);
	if (!enesim_buffer_data_get(s->buffer, &sw_data)) return EINA_FALSE;

	data = s->buffer->backend_data;
	switch (s->format)
	{
		case ENESIM_FORMAT_ARGB8888:
		case ENESIM_FORMAT_ARGB8888_SPARSE:
		case ENESIM_FORMAT_XRGB8888:
		*data = sw_data.argb8888_pre.plane0;
		if (stride) *stride = sw_data.argb8888_pre.plane0_stride;

		case ENESIM_FORMAT_A8:
		*data = sw_data.a8.plane0;
		if (stride) *stride = sw_data.a8.plane0_stride;
		break;

		default:
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

/**
 * Store a private data pointer into the surface
 */
EAPI void enesim_surface_private_set(Enesim_Surface *s, void *data)
{
	ENESIM_MAGIC_CHECK_SURFACE(s);
	s->user = data;
}

/**
 * Retrieve the private data pointer from the surface
 */
EAPI void * enesim_surface_private_get(Enesim_Surface *s)
{
	ENESIM_MAGIC_CHECK_SURFACE(s);
	return s->user;
}


