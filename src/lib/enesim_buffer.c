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
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Eina_Bool enesim_buffer_setup(Enesim_Buffer *buf, Enesim_Backend b,
		Enesim_Buffer_Format fmt, uint32_t w, uint32_t h,
		Enesim_Buffer_Data *data, Enesim_Pool *pool)
{
	EINA_MAGIC_SET(buf, ENESIM_MAGIC_BUFFER);
	buf->w = w;
	buf->h = h;
	buf->format = fmt;
	if (!pool)
	{
		if (data) buf->data = *data;
	}
	else
	{
		if (!enesim_pool_data_alloc(pool, data, b, fmt, w, h))
			return EINA_FALSE;
	}
	buf->pool = pool;
	return EINA_TRUE;
}

void enesim_buffer_cleanup(Enesim_Buffer *buf)
{
	if (buf->pool)
	{
		 enesim_pool_data_free(buf->pool, &buf->data, buf->backend,
				buf->format);
	}
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Buffer *
enesim_buffer_new_data_from(Enesim_Backend b, Enesim_Buffer_Format fmt,
		uint32_t w, uint32_t h, Enesim_Buffer_Data *data)
{
	Enesim_Buffer *buf;

	buf = calloc(1, sizeof(Enesim_Buffer));
	if (!enesim_buffer_setup(buf, b, fmt, w, h, data, NULL))
	{
		free(buf);
		return NULL;
	}

	return buf;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Buffer *
enesim_buffer_new_pool_from(Enesim_Backend b, Enesim_Buffer_Format f,
		uint32_t w, uint32_t h, Enesim_Pool *p)
{
	Enesim_Buffer *buf;

	if (!p) return enesim_buffer_new(b, f, w, h);

	buf = calloc(1, sizeof(Enesim_Buffer));
	if (!enesim_buffer_setup(buf, b, f, w, h, &buf->data, p))
	{
		free(buf);
		return NULL;
	}

	return buf;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Buffer *
enesim_buffer_new(Enesim_Backend b, Enesim_Buffer_Format f, uint32_t w, uint32_t h)
{
	Enesim_Buffer *buf;

	buf = enesim_buffer_new_pool_from(b, f, w, h, &enesim_default_pool);

	return buf;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void
enesim_buffer_size_get(const Enesim_Buffer *b, int *w, int *h)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	if (w) *w = b->w;
	if (h) *h = b->h;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Buffer_Format
enesim_buffer_format_get(const Enesim_Buffer *b)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	return b->format;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Backend
enesim_buffer_backend_get(const Enesim_Buffer *b)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	return b->backend;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void
enesim_buffer_delete(Enesim_Buffer *b)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);

	enesim_buffer_cleanup(b);
	free(b);
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void
enesim_buffer_data_get(const Enesim_Buffer *b, Enesim_Buffer_Data *data)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	if (data) *data = b->data;
}
/**
 * Store a private data pointer into the buffer
 */
EAPI void
enesim_buffer_private_set(Enesim_Buffer *b, void *data)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	b->user = data;
}

/**
 * Retrieve the private data pointer from the buffer
 */
EAPI void *
enesim_buffer_private_get(Enesim_Buffer *b)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	return b->user;
}

EAPI size_t
enesim_buffer_format_size_get(Enesim_Buffer_Format fmt, uint32_t w, uint32_t h)
{
	switch (fmt)
	{
		case ENESIM_CONVERTER_ARGB8888:
		case ENESIM_CONVERTER_ARGB8888_PRE:
		return w * h * 4;
		break;

		case ENESIM_CONVERTER_RGB565:
		return w * h * 2;
		break;

		case ENESIM_CONVERTER_RGB888:
		return w * h * 3;
		break;

		case ENESIM_CONVERTER_A8:
		case ENESIM_CONVERTER_GRAY:
		return w * h;
		break;
	}
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
		return ENESIM_CONVERTER_RGB565;

	if ((boffset == 0) && (blen == 8) && (goffset == 8) && (glen == 8) &&
			(roffset == 16) && (rlen == 8) && (aoffset == 24) && (alen == 8))
	{
		if (premul)
			return ENESIM_CONVERTER_ARGB8888_PRE;
		else
			return ENESIM_CONVERTER_ARGB8888;
	}

	if ((boffset == 0) && (blen == 0) && (goffset == 0) && (glen == 0) &&
			(roffset == 0) && (rlen == 0) && (aoffset == 0) && (alen == 8))
		return ENESIM_CONVERTER_A8;
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
		case ENESIM_CONVERTER_RGB565:
		return 16;

		case ENESIM_CONVERTER_ARGB8888:
		case ENESIM_CONVERTER_ARGB8888_PRE:
		return 32;

		case ENESIM_CONVERTER_A8:
		case ENESIM_CONVERTER_GRAY:
		return 8;

		case ENESIM_CONVERTER_RGB888:
		return 24;

		default:
		return 0;
	}
}
