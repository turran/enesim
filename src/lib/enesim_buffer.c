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
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Buffer *
enesim_buffer_new_data_from(Enesim_Backend b, Enesim_Buffer_Format fmt,
		uint32_t w, uint32_t h, Enesim_Buffer_Data *data)
{
	Enesim_Buffer *buf;

	buf = calloc(1, sizeof(Enesim_Buffer));
	EINA_MAGIC_SET(buf, ENESIM_MAGIC_BUFFER);
	buf->w = w;
	buf->h = h;
	buf->format = fmt;
	buf->data = *data;

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
	void *data;

	if (!p)
		return enesim_buffer_new(b, f, w, h);

	data = enesim_pool_data_alloc(p, b, f, w, h);
	if (!data) return NULL;

	buf = calloc(1, sizeof(Enesim_Buffer));
	EINA_MAGIC_SET(buf, ENESIM_MAGIC_BUFFER);
	buf->w = w;
	buf->h = h;
	buf->format = f;
	buf->pool = p;
	buf->data = data;

	return buf;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Buffer * enesim_buffer_new(Enesim_Backend b, Enesim_Buffer_Format f, int w, int h)
{
	Enesim_Buffer *buf;

	buf = enesim_buffer_new_pool_from(b, f, w, h, &enesim_default_pool);

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
EAPI void
enesim_buffer_delete(Enesim_Buffer *b)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);

	if (b->pool)
	{
		enesim_pool_data_free(b->pool, b->data);
	}
	free(b);
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void *
enesim_buffer_data_get(const Enesim_Buffer *b)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	return b->data;
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
