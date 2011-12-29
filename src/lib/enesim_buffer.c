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
#include "Enesim.h"
#include "enesim_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
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
EAPI Enesim_Buffer * enesim_buffer_new_pool_and_data_from(Enesim_Buffer_Format f,
		uint32_t w, uint32_t h, Enesim_Pool *p, Eina_Bool copy,
		Enesim_Buffer_Sw_Data *data)
{
	Enesim_Buffer *buf;
	Enesim_Backend backend;
	void *backend_data;

	if (!p)
	{
		p = enesim_pool_default_get();
		if (!p) return NULL;
	}

	if (!enesim_pool_data_from(p, &backend, &backend_data, f, w, h, copy, data))
		return NULL;

	buf = calloc(1, sizeof(Enesim_Buffer));
	EINA_MAGIC_SET(buf, ENESIM_MAGIC_BUFFER);
	buf->w = w;
	buf->h = h;
	buf->backend = backend;
	buf->backend_data = backend_data;
	buf->format = f;
	buf->pool = p;
	buf->external_allocated = !copy;
	buf = enesim_buffer_ref(buf);

	return buf;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Buffer * enesim_buffer_new_data_from(Enesim_Buffer_Format f,
		uint32_t w, uint32_t h, Eina_Bool copy,
		Enesim_Buffer_Sw_Data *data)
{
	Enesim_Buffer *buf;

	buf = enesim_buffer_new_pool_and_data_from(f, w, h, NULL, copy, data);

	return buf;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Buffer *
enesim_buffer_new_pool_from(Enesim_Buffer_Format f, uint32_t w,
		uint32_t h, Enesim_Pool *p)
{
	Enesim_Buffer *buf;
	Enesim_Backend backend;
	void *backend_data;

	if (!p)
	{
		p = enesim_pool_default_get();
		if (!p) return NULL;
	}

	if (!enesim_pool_data_alloc(p, &backend, &backend_data, f, w, h))
		return NULL;

	buf = calloc(1, sizeof(Enesim_Buffer));
	EINA_MAGIC_SET(buf, ENESIM_MAGIC_BUFFER);
	buf->w = w;
	buf->h = h;
	buf->backend = backend;
	buf->backend_data = backend_data;
	buf->format = f;
	buf->pool = p;
	buf->external_allocated = EINA_FALSE;
	buf = enesim_buffer_ref(buf);

	return buf;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Buffer * enesim_buffer_new(Enesim_Buffer_Format f,
			uint32_t w, uint32_t h)
{
	Enesim_Buffer *buf;

	buf = enesim_buffer_new_pool_from(f, w, h, NULL);

	return buf;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_buffer_size_get(const Enesim_Buffer *b, int *w, int *h)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	if (w) *w = b->w;
	if (h) *h = b->h;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Buffer_Format enesim_buffer_format_get(const Enesim_Buffer *b)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	return b->format;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Backend enesim_buffer_backend_get(const Enesim_Buffer *b)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	return b->backend;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Buffer * enesim_buffer_ref(Enesim_Buffer *b)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	b->ref++;
	return b;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_buffer_unref(Enesim_Buffer *b)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);

	b->ref--;
	if (!b->ref)
	{
		enesim_pool_data_free(b->pool, b->backend_data, b->format, b->external_allocated);
		free(b);
	}
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Eina_Bool enesim_buffer_data_get(const Enesim_Buffer *b, Enesim_Buffer_Sw_Data *data)
{
	void *buffer_data;

	ENESIM_MAGIC_CHECK_BUFFER(b);
	return enesim_pool_data_get(b->pool, b->backend_data, b->format, b->w, b->h, data);
}
/**
 * Store a private data pointer into the buffer
 */
EAPI void enesim_buffer_private_set(Enesim_Buffer *b, void *data)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	b->user = data;
}

/**
 * Retrieve the private data pointer from the buffer
 */
EAPI void * enesim_buffer_private_get(Enesim_Buffer *b)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	return b->user;
}

EAPI size_t enesim_buffer_format_size_get(Enesim_Buffer_Format fmt, uint32_t w, uint32_t h)
{
	switch (fmt)
	{
		case ENESIM_BUFFER_FORMAT_ARGB8888:
		case ENESIM_BUFFER_FORMAT_ARGB8888_PRE:
		return w * h * 4;
		break;

		case ENESIM_BUFFER_FORMAT_RGB565:
		return w * h * 2;
		break;

		case ENESIM_BUFFER_FORMAT_RGB888:
		case ENESIM_BUFFER_FORMAT_BGR888:
		return w * h * 3;
		break;

		case ENESIM_BUFFER_FORMAT_A8:
		case ENESIM_BUFFER_FORMAT_GRAY:
		return w * h;
		break;

		default:
		return 0;
	}
	return 0;
}

/**
 * To be documented
 * FIXME: To be fixed
 * FIXME how to handle the gray?
 */
EAPI Enesim_Buffer_Format enesim_buffer_format_rgb_get(uint8_t aoffset, uint8_t alen,
		uint8_t roffset, uint8_t rlen, uint8_t goffset, uint8_t glen,
		uint8_t boffset, uint8_t blen, Eina_Bool premul)
{
	if ((boffset == 0) && (blen == 5) && (goffset == 5) && (glen == 6) &&
			(roffset == 11) && (rlen == 5) && (aoffset == 0) && (alen == 0))
		return ENESIM_BUFFER_FORMAT_RGB565;

	if ((boffset == 0) && (blen == 8) && (goffset == 8) && (glen == 8) &&
			(roffset == 16) && (rlen == 8) && (aoffset == 24) && (alen == 8))
	{
		if (premul)
			return ENESIM_BUFFER_FORMAT_ARGB8888_PRE;
		else
			return ENESIM_BUFFER_FORMAT_ARGB8888;
	}

	if ((boffset == 0) && (blen == 0) && (goffset == 0) && (glen == 0) &&
			(roffset == 0) && (rlen == 0) && (aoffset == 0) && (alen == 8))
		return ENESIM_BUFFER_FORMAT_A8;
}

/**
 * Gets the pixel depth of the converter format
 * @param fmt The converter format to get the depth from
 * @return The depth in bits per pixel
 */
EAPI uint8_t enesim_buffer_format_rgb_depth_get(Enesim_Buffer_Format fmt)
{
	switch (fmt)
	{
		case ENESIM_BUFFER_FORMAT_RGB565:
		return 16;

		case ENESIM_BUFFER_FORMAT_ARGB8888:
		case ENESIM_BUFFER_FORMAT_ARGB8888_PRE:
		return 32;

		case ENESIM_BUFFER_FORMAT_A8:
		case ENESIM_BUFFER_FORMAT_GRAY:
		return 8;

		case ENESIM_BUFFER_FORMAT_RGB888:
		case ENESIM_BUFFER_FORMAT_BGR888:
		return 24;

		default:
		return 0;
	}
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void * enesim_buffer_backend_data_get(Enesim_Buffer *b)
{
	return b->backend_data;
}

