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

#include "enesim_main.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"

#include "enesim_pool_private.h"
#include "enesim_buffer_private.h"

#if BUILD_OPENGL
#include "enesim_surface.h"
#include "enesim_error.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_color.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"
#include "Enesim_OpenGL.h"
#endif
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_buffer

static Enesim_Buffer * _buffer_new(uint32_t w, uint32_t h, Enesim_Backend backend,
		void *backend_data, Enesim_Format f, Enesim_Pool *p,
		Eina_Bool external, Enesim_Buffer_Free free_func, void *free_func_data)
{
	Enesim_Buffer *thiz;

	thiz = calloc(1, sizeof(Enesim_Buffer));
	EINA_MAGIC_SET(thiz, ENESIM_MAGIC_BUFFER);
	thiz->w = w;
	thiz->h = h;
	thiz->backend = backend;
	thiz->backend_data = backend_data;
	thiz->format = f;
	thiz->pool = p;
	thiz->external_allocated = external;
	if (thiz->external_allocated)
	{
		thiz->free_func = free_func;
		thiz->free_func_data = free_func_data;
	}
	thiz = enesim_buffer_ref(thiz);
	/* now create the needed locks */
	eina_rwlock_new(&thiz->lock);

	return thiz;
}

static void _buffer_free(Enesim_Buffer *b)
{
	if (b->external_allocated)
	{
		if (b->free_func)
			b->free_func(b->backend_data, b->free_func_data);
	}
	if (b->pool)
	{
		enesim_pool_data_free(b->pool, b->backend_data, b->format,
				b->external_allocated);
	}
	eina_rwlock_free(&b->lock);
	free(b);
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
EAPI Enesim_Buffer * enesim_buffer_new_pool_and_data_from(Enesim_Buffer_Format f,
		uint32_t w, uint32_t h, Enesim_Pool *p, Eina_Bool copy,
		Enesim_Buffer_Sw_Data *data, Enesim_Buffer_Free free_func,
		void *free_func_data)
{
	Enesim_Buffer *buf;
	Enesim_Backend backend;
	void *backend_data;

	if (!p)
	{
		p = enesim_pool_default_get();
		if (!p) return NULL;
	}

	if (!enesim_pool_data_from(p, &backend, &backend_data, f, w, h, copy,
			data))
		return NULL;

	buf = _buffer_new(w, h, backend, backend_data, f, p, !copy, free_func,
			free_func_data);

	return buf;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Buffer * enesim_buffer_new_data_from(Enesim_Buffer_Format f,
		uint32_t w, uint32_t h, Eina_Bool copy,
		Enesim_Buffer_Sw_Data *data, Enesim_Buffer_Free free_func,
		void *user_data)
{
	Enesim_Buffer *buf;

	buf = enesim_buffer_new_pool_and_data_from(f, w, h, NULL, copy, data,
			free_func, user_data);

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

	buf = _buffer_new(w, h, backend, backend_data, f, p, EINA_FALSE, NULL,
			NULL);

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

#if BUILD_OPENGL
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Buffer * enesim_buffer_new_opengl_data_from(Enesim_Buffer_Format f,
		uint32_t w, uint32_t h,
		GLuint *textures, unsigned int num_textures)
{
	Enesim_Buffer *b;
	Enesim_Buffer_OpenGL_Data *backend_data;

	if (!w || !h) return NULL;
	backend_data = calloc(1, sizeof(Enesim_Buffer_OpenGL_Data));
	/* FIXME for now */
	backend_data->texture = textures[0];
	backend_data->num_textures = num_textures;

	b = _buffer_new(w, h, ENESIM_BACKEND_OPENGL, backend_data, f, NULL,
			EINA_TRUE, NULL, NULL);
	return b;
}
#endif

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
EAPI Enesim_Pool * enesim_buffer_pool_get(Enesim_Buffer *b)
{
	return b->pool;
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
		DBG("Unreffing buffer %p", b);
		_buffer_free(b);
	}
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Eina_Bool enesim_buffer_data_get(const Enesim_Buffer *b, Enesim_Buffer_Sw_Data *data)
{
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
		case ENESIM_BUFFER_FORMAT_CMYK:
		case ENESIM_BUFFER_FORMAT_CMYK_ADOBE:
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

EAPI Eina_Bool enesim_buffer_format_rgb_components_to(Enesim_Buffer_Format fmt,
		uint8_t *aoffset, uint8_t *alen,
		uint8_t *roffset, uint8_t *rlen,
		uint8_t *goffset, uint8_t *glen,
		uint8_t *boffset, uint8_t *blen, Eina_Bool *premul)
{
	Eina_Bool ret = EINA_TRUE;

	*aoffset = 0; *alen = 0;
	*roffset = 0; *rlen = 0;
	*goffset = 0; *glen = 0;
	*boffset = 0; *blen = 0;
	*premul = EINA_FALSE;

	switch (fmt)
	{
		case ENESIM_BUFFER_FORMAT_ARGB8888:
		*aoffset = 24; *alen = 8;
		*roffset = 16; *rlen = 8;
		*goffset = 8; *glen = 8;
		*boffset = 0; *blen = 8;
		break;

		case ENESIM_BUFFER_FORMAT_ARGB8888_PRE:
		*aoffset = 24; *alen = 8;
		*roffset = 16; *rlen = 8;
		*goffset = 8; *glen = 8;
		*boffset = 0; *blen = 8;
		*premul = EINA_TRUE;
		break;

		case ENESIM_BUFFER_FORMAT_RGB888:
		*roffset = 16; *rlen = 8;
		*goffset = 8; *glen = 8;
		*boffset = 0; *blen = 8;

		case ENESIM_BUFFER_FORMAT_BGR888:
		*aoffset = 0; *alen = 0;
		*roffset = 0; *rlen = 8;
		*goffset = 8; *glen = 8;
		*boffset = 16; *blen = 8;
		break;

		case ENESIM_BUFFER_FORMAT_A8:
		*aoffset = 0; *alen = 8;
		break;

		default:
		ret = EINA_FALSE;
		break;
	}
	return ret;
}

/**
 * To be documented
 * FIXME: To be fixed
 * FIXME how to handle the gray?
 */
EAPI Eina_Bool enesim_buffer_format_rgb_components_from(Enesim_Buffer_Format *fmt,
		uint8_t aoffset, uint8_t alen,
		uint8_t roffset, uint8_t rlen,
		uint8_t goffset, uint8_t glen,
		uint8_t boffset, uint8_t blen, Eina_Bool premul)
{
	Eina_Bool ret = EINA_FALSE;

	if ((boffset == 0) && (blen == 5) && (goffset == 5) && (glen == 6) &&
			(roffset == 11) && (rlen == 5) && (aoffset == 0) && (alen == 0))
	{
		ret = EINA_TRUE;
		*fmt = ENESIM_BUFFER_FORMAT_RGB565;
	}

	if ((boffset == 0) && (blen == 8) && (goffset == 8) && (glen == 8) &&
			(roffset == 16) && (rlen == 8) && (aoffset == 24) && (alen == 8))
	{
		ret = EINA_TRUE;
		if (premul)
			*fmt = ENESIM_BUFFER_FORMAT_ARGB8888_PRE;
		else
			*fmt = ENESIM_BUFFER_FORMAT_ARGB8888;
	}

	if ((boffset == 0) && (blen == 0) && (goffset == 0) && (glen == 0) &&
			(roffset == 0) && (rlen == 0) && (aoffset == 0) && (alen == 8))
	{
		ret = EINA_TRUE;
		*fmt = ENESIM_BUFFER_FORMAT_A8;
	}

	return ret;
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

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_buffer_lock(Enesim_Buffer *b, Eina_Bool write)
{
	if (write)
		eina_rwlock_take_write(&b->lock);
	else
		eina_rwlock_take_read(&b->lock);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_buffer_unlock(Enesim_Buffer *b)
{
	eina_rwlock_release(&b->lock);
}
