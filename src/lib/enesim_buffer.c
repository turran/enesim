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
#include "enesim_pool.h"
#include "enesim_buffer.h"

#include "enesim_pool_private.h"
#include "enesim_buffer_private.h"

#if BUILD_OPENGL
#include "enesim_surface.h"
#include "enesim_log.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_color.h"
#include "enesim_renderer.h"
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
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
		enesim_pool_unref(b->pool);
	}
	eina_rwlock_free(&b->lock);
	free(b);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void * enesim_buffer_backend_data_get(Enesim_Buffer *b)
{
	return b->backend_data;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Create a new buffer using a pool and data
 * @param[in] f The format of the buffer
 * @param[in] w The width of the buffer
 * @param[in] h The height of the buffer
 * @param[in] p The pool to use
 * @param[in] copy In case the data needs to be copied to create the buffer
 * or used directly
 * @param[in] data The data of the buffer pixels
 * @param[in] free_func The function to be called whenever the data of the buffer
 * needs to be freed
 * @param[in] free_func_data The private data for the @a free_func callback
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
	{
		enesim_pool_unref(p);
		return NULL;
	}

	buf = _buffer_new(w, h, backend, backend_data, f, p, !copy, free_func,
			free_func_data);

	return buf;
}

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

EAPI Enesim_Buffer * enesim_buffer_new_pool_from(Enesim_Buffer_Format f,
		uint32_t w, uint32_t h, Enesim_Pool *p)
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
	{
		enesim_pool_unref(p);
		return NULL;
	}

	buf = _buffer_new(w, h, backend, backend_data, f, p, EINA_FALSE, NULL,
			NULL);

	return buf;
}

EAPI Enesim_Buffer * enesim_buffer_new(Enesim_Buffer_Format f,
			uint32_t w, uint32_t h)
{
	Enesim_Buffer *buf;

	buf = enesim_buffer_new_pool_from(f, w, h, NULL);

	return buf;
}

#if BUILD_OPENGL
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
 * @brief Gets the size of a buffer
 * @param[in] b The buffer to get the size from
 * @param[out] w The width of the buffer
 * @param[out] h The height of the buffer
 */
EAPI void enesim_buffer_size_get(const Enesim_Buffer *b, int *w, int *h)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	if (w) *w = b->w;
	if (h) *h = b->h;
}

/**
 * @brief Gets the format of a buffer
 * @param[in] b The buffer to get the format from
 * @return The format of the buffer
 */
EAPI Enesim_Buffer_Format enesim_buffer_format_get(const Enesim_Buffer *b)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	return b->format;
}

/**
 * @brief Gets the backend of a buffer
 * @param[in] b The buffer to get the backend from
 * @return The backend of the buffer
 */
EAPI Enesim_Backend enesim_buffer_backend_get(const Enesim_Buffer *b)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	return b->backend;
}


/**
 * @brief Gets the pool of a buffer
 * @param[in] b The buffer to get the pool from
 * @return The pool of the buffer
 */
EAPI Enesim_Pool * enesim_buffer_pool_get(Enesim_Buffer *b)
{
	return enesim_pool_ref(b->pool);
}

/**
 * @brief Increase the reference counter of a buffer
 * @param[in] b The buffer
 * @return The input parameter @a b for programming convenience
 */
EAPI Enesim_Buffer * enesim_buffer_ref(Enesim_Buffer *b)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	b->ref++;
	return b;
}

/**
 * @brief Decrease the reference counter of a buffer
 * @param[in] b The buffer
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
 * @brief Gets the data of a buffer
 * @param[in] b The buffer to get the data from
 * @param[out] data The data of the buffer
 */
EAPI Eina_Bool enesim_buffer_data_get(const Enesim_Buffer *b, Enesim_Buffer_Sw_Data *data)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	return enesim_pool_data_get(b->pool, b->backend_data, b->format, b->w, b->h, data);
}

/**
 * @brief Store a private data pointer into the buffer
 * @param[in] b The buffer to store the data in
 * @param[in] data The user provided data
 */
EAPI void enesim_buffer_private_set(Enesim_Buffer *b, void *data)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	b->user = data;
}

/**
 * @brief Retrieve the private data pointer from the buffer
 * @param[in] b The buffer to retrieve the data from
 */
EAPI void * enesim_buffer_private_get(Enesim_Buffer *b)
{
	ENESIM_MAGIC_CHECK_BUFFER(b);
	return b->user;
}

/**
 * @brief Get the size in bytes given a format and a size
 * @param[in] fmt The format to get the size from
 * @param[in] w The width to get the size from
 * @param[in] h The height to get the size from
 * @return The size in bytes
 */
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

/**
 * @brief Get the color components offsets and lengths given a format
 * @param[in] fmt The format to get the components from
 * @param[out] aoffset Alpha offset
 * @param[out] alen Alpha length
 * @param[out] roffset Red offset
 * @param[out] rlen Red length
 * @param[out] goffset Green offset
 * @param[out] glen Green length
 * @param[out] boffset Blue offset
 * @param[out] blen Blue length
 * @param[out] premul Flag to indicate if the components are premultiplied
 * @return EINA_TRUE if the format can be described as ARGB components,
 * EINA_FALSE otherwise
 */
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

		case ENESIM_BUFFER_FORMAT_XRGB8888:
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
 * @brief Get the format based on the components description
 * @param[out] fmt The format associated with the components
 * @param[in] aoffset Alpha offset
 * @param[in] alen Alpha length
 * @param[in] roffset Red offset
 * @param[in] rlen Red length
 * @param[in] goffset Green offset
 * @param[in] glen Green length
 * @param[in] boffset Blue offset
 * @param[in] blen Blue length
 * @param[in] premul Flag to indicate if the components are premultiplied
 * @return EINA_TRUE if the format can be described by the provided ARGB
 * components, EINA_FALSE otherwise
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
 * Gets the pixel depth of the format
 * @param fmt The format to get the depth from
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
		case ENESIM_BUFFER_FORMAT_XRGB8888:
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
 * @brief Locks a buffer
 * @param[in] b The buffer to lock
 */
EAPI void enesim_buffer_lock(Enesim_Buffer *b, Eina_Bool write)
{
	if (write)
		eina_rwlock_take_write(&b->lock);
	else
		eina_rwlock_take_read(&b->lock);
}

/**
 * @brief Unlocks a buffer
 * @param[in] b The buffer to unlock
 */
EAPI void enesim_buffer_unlock(Enesim_Buffer *b)
{
	eina_rwlock_release(&b->lock);
}
