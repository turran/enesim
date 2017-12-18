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
#include "enesim_format.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"

#include "enesim_pool_private.h"
#include "enesim_buffer_private.h"
#include "enesim_converter_private.h"

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
/** @cond internal */
#define ENESIM_LOG_DEFAULT enesim_log_buffer

#if BUILD_OPENGL
static void _buffer_opengl_backend_free(void *data, void *user_data EINA_UNUSED)
{
	Enesim_Buffer_OpenGL_Data *backend_data = data;
	enesim_opengl_buffer_data_free(backend_data);
}
#endif

static void _buffer_data_free(void *data, void *user_data EINA_UNUSED)
{
	free(data);
}

static Eina_Bool _buffer_format_has_alpha(Enesim_Buffer_Format fmt)
{
	switch (fmt)
	{
		case ENESIM_BUFFER_FORMAT_RGB565:
		case ENESIM_BUFFER_FORMAT_XRGB8888:
		case ENESIM_BUFFER_FORMAT_RGB888:
		case ENESIM_BUFFER_FORMAT_BGR888:
		case ENESIM_BUFFER_FORMAT_GRAY:
		case ENESIM_BUFFER_FORMAT_CMYK:
		case ENESIM_BUFFER_FORMAT_CMYK_ADOBE:
		return EINA_FALSE;

		case ENESIM_BUFFER_FORMAT_ARGB8888_PRE:
		case ENESIM_BUFFER_FORMAT_ARGB8888:
		case ENESIM_BUFFER_FORMAT_A8:
		return EINA_TRUE;

		default:
		return EINA_FALSE;
	}
}

static Enesim_Buffer * _buffer_new(uint32_t w, uint32_t h, Enesim_Backend backend,
		void *backend_data, Enesim_Buffer_Format f, Enesim_Pool *p,
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
	/* set the default alpha hints */
	if (_buffer_format_has_alpha(f))
		thiz->alpha_hint = ENESIM_ALPHA_HINT_NORMAL;
	else
		thiz->alpha_hint = ENESIM_ALPHA_HINT_OPAQUE;

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

Eina_Bool enesim_buffer_sw_data_alloc(Enesim_Buffer_Sw_Data *data,
		Enesim_Buffer_Format fmt, uint32_t w, uint32_t h)
{
	Eina_Bool ret;
	switch (fmt)
	{
		/* packed case */
		case ENESIM_BUFFER_FORMAT_ARGB8888:
		case ENESIM_BUFFER_FORMAT_ARGB8888_PRE:
		case ENESIM_BUFFER_FORMAT_CMYK:
		case ENESIM_BUFFER_FORMAT_CMYK_ADOBE:
		case ENESIM_BUFFER_FORMAT_BGR888:
		case ENESIM_BUFFER_FORMAT_RGB888:
		case ENESIM_BUFFER_FORMAT_RGB565:
		case ENESIM_BUFFER_FORMAT_A8:
		case ENESIM_BUFFER_FORMAT_GRAY:
		{
			size_t bytes;
			int stride;
			void *alloc_data;

			bytes = enesim_buffer_format_size_get(fmt, w, h);
			stride = enesim_buffer_format_size_get(fmt, w, 1);
			alloc_data = calloc(bytes, sizeof(char));
			ret = enesim_buffer_sw_data_set(data, fmt, alloc_data, stride);
			if (!ret)
			{
				free(alloc_data);
			}
		}
		break;

		/* planar case */
		default:
		ERR("Format not supported");
		ret = EINA_FALSE;
		break;
	}
	return ret;
}

/* FIXME change this to pass void[] content and int[] strides
 * TODO add a function to get the number of planes
 */
Eina_Bool enesim_buffer_sw_data_set(Enesim_Buffer_Sw_Data *data,
		Enesim_Buffer_Format fmt, void *content0, int stride0)
{
	Eina_Bool ret = EINA_TRUE;
	switch (fmt)
	{
		/* 32 bpp */
		case ENESIM_BUFFER_FORMAT_ARGB8888:
		data->argb8888.plane0 = content0;
		data->argb8888.plane0_stride = stride0;
		break;

		case ENESIM_BUFFER_FORMAT_ARGB8888_PRE:
		data->argb8888_pre.plane0 = content0;
		data->argb8888_pre.plane0_stride = stride0;
		break;

		case ENESIM_BUFFER_FORMAT_CMYK:
		case ENESIM_BUFFER_FORMAT_CMYK_ADOBE:
		data->cmyk.plane0 = content0;
		data->cmyk.plane0_stride = stride0;
		break;

		/* 24 bpp */
		case ENESIM_BUFFER_FORMAT_BGR888:
		data->bgr888.plane0 = content0;
		data->bgr888.plane0_stride = stride0;
		break;

		case ENESIM_BUFFER_FORMAT_RGB888:
		data->rgb888.plane0 = content0;
		data->rgb888.plane0_stride = stride0;
		break;

		/* 16 bpp */
		case ENESIM_BUFFER_FORMAT_RGB565:
		data->rgb565.plane0 = content0;
		data->rgb565.plane0_stride = stride0;
		break;

		/* 8 bpp */
		case ENESIM_BUFFER_FORMAT_A8:
		data->a8.plane0 = content0;
		data->a8.plane0_stride = stride0;
		break;

		case ENESIM_BUFFER_FORMAT_GRAY:
		default:
		ERR("Unsupported format %d", fmt);
		ret = EINA_FALSE;
		break;
	}
	return ret;
}

Eina_Bool enesim_buffer_sw_data_free(Enesim_Buffer_Sw_Data *data,
		Enesim_Buffer_Format fmt,
		Enesim_Buffer_Free free_func,
		void *free_func_data)
{
	Eina_Bool ret = EINA_TRUE;
	switch (fmt)
	{
		/* 32 bpp */
		case ENESIM_BUFFER_FORMAT_ARGB8888:
		free_func(data->argb8888.plane0, free_func_data);
		break;

		case ENESIM_BUFFER_FORMAT_ARGB8888_PRE:
		free_func(data->argb8888_pre.plane0, free_func_data);
		break;

		case ENESIM_BUFFER_FORMAT_CMYK:
		case ENESIM_BUFFER_FORMAT_CMYK_ADOBE:
		free_func(data->cmyk.plane0, free_func_data);
		break;

		/* 24 bpp */
		case ENESIM_BUFFER_FORMAT_BGR888:
		free_func(data->bgr888.plane0, free_func_data);
		break;

		case ENESIM_BUFFER_FORMAT_RGB888:
		free_func(data->rgb888.plane0, free_func_data);
		break;

		/* 16 bpp */
		case ENESIM_BUFFER_FORMAT_RGB565:
		free_func(data->rgb565.plane0, free_func_data);
		break;

		/* 8 bpp */
		case ENESIM_BUFFER_FORMAT_A8:
		free_func(data->a8.plane0, free_func_data);
		break;

		case ENESIM_BUFFER_FORMAT_GRAY:
		default:
		ERR("Unsupported format %d", fmt);
		ret = EINA_FALSE;
		break;
	}
	return ret;
}

Eina_Bool enesim_buffer_sw_data_at(Enesim_Buffer_Sw_Data *data,
		Enesim_Buffer_Format fmt, int x, int y,
		Enesim_Buffer_Sw_Data *at)
{
	Eina_Bool ret = EINA_TRUE;
	switch (fmt)
	{
		/* 32 bpp */
		case ENESIM_BUFFER_FORMAT_ARGB8888:
		at->argb8888.plane0 = (uint32_t *)((uint8_t *)data->argb8888.plane0 +
				(y * data->argb8888.plane0_stride) + x);
		at->argb8888.plane0_stride = data->argb8888.plane0_stride;
		break;

		case ENESIM_BUFFER_FORMAT_ARGB8888_PRE:
		at->argb8888_pre.plane0 = (uint32_t *)((uint8_t *)data->argb8888_pre.plane0 +
				(y * data->argb8888_pre.plane0_stride) + x);
		at->argb8888_pre.plane0_stride = data->argb8888_pre.plane0_stride;
		break;

		/* 24 bpp */
		case ENESIM_BUFFER_FORMAT_BGR888:
		at->bgr888.plane0 = data->bgr888.plane0 +
				(y * data->bgr888.plane0_stride) + x;
		at->bgr888.plane0_stride = data->bgr888.plane0_stride;
		break;

		case ENESIM_BUFFER_FORMAT_RGB888:
		at->rgb888.plane0 = data->rgb888.plane0 +
				(y * data->rgb888.plane0_stride) + x;
		at->rgb888.plane0_stride = data->rgb888.plane0_stride;
		break;

		case ENESIM_BUFFER_FORMAT_CMYK:
		case ENESIM_BUFFER_FORMAT_CMYK_ADOBE:
		at->cmyk.plane0 = data->cmyk.plane0 +
				(y * data->cmyk.plane0_stride) + x;
		at->cmyk.plane0_stride = data->cmyk.plane0_stride;
		break;

		/* 16 bpp */
		case ENESIM_BUFFER_FORMAT_RGB565:
		at->rgb565.plane0 = (uint16_t *)((uint8_t *)data->rgb565.plane0 +
				(y * data->rgb565.plane0_stride) + x);
		at->rgb565.plane0_stride = data->rgb565.plane0_stride;
		break;

		/* 8 bpp */
		case ENESIM_BUFFER_FORMAT_A8:
		at->a8.plane0 = data->a8.plane0 +
				(y * data->a8.plane0_stride) + x;
		at->a8.plane0_stride = data->a8.plane0_stride;
		break;
		case ENESIM_BUFFER_FORMAT_GRAY:
		at->a8.plane0 = data->a8.plane0 +
				(y * data->a8.plane0_stride) + x;
		at->a8.plane0_stride = data->a8.plane0_stride;
		break;

		default:
		ERR("Unsupported format %d", fmt);
		ret = EINA_FALSE;
		break;
	}
	return ret;
}

/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Create a new buffer using a pool and an user provided data
 * @param[in] f The format of the buffer
 * @param[in] w The width of the buffer
 * @param[in] h The height of the buffer
 * @param[in] p The pool to use
 * @param[in] copy In case the data needs to be copied to create the buffer
 * or used directly
 * @param[in] sw_data The data of the buffer pixels
 * @param[in] free_func The function to be called whenever the data of the buffer
 * needs to be freed
 * @param[in] free_func_data The private data for the @a free_func callback
 * @return The newly created buffer
 */
EAPI Enesim_Buffer * enesim_buffer_new_pool_and_data_from(Enesim_Buffer_Format f,
		uint32_t w, uint32_t h, Enesim_Pool *p, Eina_Bool copy,
		Enesim_Buffer_Sw_Data *sw_data, Enesim_Buffer_Free free_func,
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
			sw_data))
	{
		enesim_pool_unref(p);
		return NULL;
	}

	buf = _buffer_new(w, h, backend, backend_data, f, p, !copy, free_func,
			free_func_data);

	return buf;
}

/**
 * @brief Create a new buffer using an user provided data
 * @param[in] f The format of the buffer
 * @param[in] w The width of the buffer
 * @param[in] h The height of the buffer
 * @param[in] copy In case the data needs to be copied to create the buffer
 * or used directly
 * @param[in] sw_data The data of the buffer pixels
 * @param[in] free_func The function to be called whenever the data of the buffer
 * needs to be freed
 * @param[in] user_data The private data for the @a free_func callback
 * @return The newly created buffer
 */
EAPI Enesim_Buffer * enesim_buffer_new_data_from(Enesim_Buffer_Format f,
		uint32_t w, uint32_t h, Eina_Bool copy,
		Enesim_Buffer_Sw_Data *sw_data, Enesim_Buffer_Free free_func,
		void *user_data)
{
	Enesim_Buffer *buf;

	buf = enesim_buffer_new_pool_and_data_from(f, w, h, NULL, copy, sw_data,
			free_func, user_data);

	return buf;
}

/**
 * @brief Create a new buffer using a pool
 * @param[in] f The format of the buffer
 * @param[in] w The width of the buffer
 * @param[in] h The height of the buffer
 * @param[in] p The pool to use
 * @return The newly created buffer
 */
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

/**
 * @brief Create a new buffer using the default memory pool
 * @param[in] f The format of the buffer
 * @param[in] w The width of the buffer
 * @param[in] h The height of the buffer
 * @return The newly created buffer
 * @see enesim_pool_default_get()
 * @see enesim_pool_default_set()
 */
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
	/* set the textures */
	backend_data = calloc(1, sizeof(Enesim_Buffer_OpenGL_Data));
	backend_data->textures = calloc(sizeof(GLuint), num_textures);
	memcpy(backend_data->textures, textures, sizeof(GLuint) * num_textures);
	backend_data->num_textures = num_textures;

	b = _buffer_new(w, h, ENESIM_BACKEND_OPENGL, backend_data, f, NULL,
			EINA_TRUE, _buffer_opengl_backend_free, NULL);
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
 * @return Eina_True if it is possible to get the data, Eina_False otherwise
 */
EAPI Eina_Bool enesim_buffer_sw_data_get(const Enesim_Buffer *b, Enesim_Buffer_Sw_Data *data)
{
	Enesim_Buffer_Sw_Data *sw_data;

	ENESIM_MAGIC_CHECK_BUFFER(b);
	if (b->backend != ENESIM_BACKEND_SOFTWARE)
	{
		ERR("Backend is not of type software");
		return EINA_FALSE;
	}
	sw_data = b->backend_data;
	*data = *sw_data;
	return EINA_TRUE;
}

/**
 * Maps the buffer into user space memory
 * @param[in] b The buffer to map
 * @param[out] data The pointer to store the buffer data
 * @return EINA_TRUE if sucessfull, EINA_FALSE otherwise
 */
EAPI Eina_Bool enesim_buffer_map(const Enesim_Buffer *b, Enesim_Buffer_Sw_Data *data)
{
	Eina_Bool ret;

	ENESIM_MAGIC_CHECK_BUFFER(b);
	/* for software backend, just get the sw_data */
	if (b->backend == ENESIM_BACKEND_SOFTWARE)
	{
		return enesim_buffer_sw_data_get(b, data);
	}

	enesim_buffer_sw_data_alloc(data, b->format, b->w, b->h);
	if (b->external_allocated)
	{
		switch (b->backend)
		{
#if BUILD_OPENGL
			case ENESIM_BACKEND_OPENGL:
			{
				Enesim_Buffer_OpenGL_Data *backend_data = b->backend_data;
				ret = enesim_opengl_buffer_data_get(backend_data, b->format,
						b->w, b->h, data);
			}
			break;
#endif
			default:
			WRN("Unsupported backend");
			ret = EINA_FALSE;
			break;
		}
	}
	else
	{
		ret = enesim_pool_data_get(b->pool, b->backend_data, b->format, b->w, b->h, data);
	}

	if (!ret)
	{
		/* free the buffer allocated data */
		enesim_buffer_sw_data_free(data, b->format, _buffer_data_free, NULL);
	}
	return ret;
}

/**
 * @brief Unmaps the buffer
 * Call this function when the mapped data of a buffer is no longer
 * needed.
 * @param[in] b The buffer to unmap
 * @param[in] data The pointer where the buffer data is mapped
 * @param[in] written EINA_TRUE in case the mapped data has been written, EINA_FALSE otherwise
 * @return EINA_TRUE if sucessfull, EINA_FALSE otherwise
 */
EAPI Eina_Bool enesim_buffer_unmap(const Enesim_Buffer *b, Enesim_Buffer_Sw_Data *data, Eina_Bool written)
{
	Eina_Bool ret = EINA_TRUE;

	ENESIM_MAGIC_CHECK_BUFFER(b);
	if (!data) return EINA_FALSE;
	/* for software backend, do nothing */
	if (b->backend == ENESIM_BACKEND_SOFTWARE)
	{
		return EINA_TRUE;
	}

	if (written)
	{
		if (b->external_allocated)
		{
			switch (b->backend)
			{
#if BUILD_OPENGL
				case ENESIM_BACKEND_OPENGL:
				{
				ret = EINA_FALSE;
				}
				break;
#endif
				default:
				WRN("Unsupported backend");
				ret = EINA_FALSE;
				break;
			}
		}
		else
		{
			ret = enesim_pool_data_put(b->pool, b->backend_data, b->format, b->w, b->h, data);
		}
	}
	/* free the buffer allocated data */
	enesim_buffer_sw_data_free(data, b->format, _buffer_data_free, NULL);
	return ret;
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
		break;

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
 * @param[in] depth The pixel depth
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
EAPI Eina_Bool enesim_buffer_format_rgb_components_from(
		Enesim_Buffer_Format *fmt, int depth,
		uint8_t aoffset, uint8_t alen,
		uint8_t roffset, uint8_t rlen,
		uint8_t goffset, uint8_t glen,
		uint8_t boffset, uint8_t blen, Eina_Bool premul)
{
	Eina_Bool ret = EINA_FALSE;

	if (blen == 5 && glen == 6 && rlen == 5 && alen == 0 && depth == 16)
	{
		if (boffset == 0 && goffset == 5 && roffset == 11 && aoffset == 0)
		{
			ret = EINA_TRUE;
			*fmt = ENESIM_BUFFER_FORMAT_RGB565;
		}
	}
	else if (blen == 8 && glen == 8 && rlen == 8)
	{
		if (alen == 8 && depth == 32)
		{
			if (boffset == 0 && goffset == 8 && roffset == 16 && aoffset == 24)
			{
				ret = EINA_TRUE;
				if (premul)
					*fmt = ENESIM_BUFFER_FORMAT_ARGB8888_PRE;
				else
					*fmt = ENESIM_BUFFER_FORMAT_ARGB8888;
			}
		}
		else if (alen == 0)
		{
			if (boffset == 0 && goffset == 8 && roffset == 16)
			{
				if (depth == 24)
				{
					ret = EINA_TRUE;
					*fmt = ENESIM_BUFFER_FORMAT_RGB888;
				}
				else if (depth == 32)
				{
					ret = EINA_TRUE;
					*fmt = ENESIM_BUFFER_FORMAT_XRGB8888;
				}
			}
			else if (boffset == 16 && goffset == 8 && roffset == 0)
			{
				ret = EINA_TRUE;
				*fmt = ENESIM_BUFFER_FORMAT_BGR888;
			}
		}
	}
	else if ((boffset == 0) && (blen == 0) && (goffset == 0) && (glen == 0) &&
			(roffset == 0) && (rlen == 0) && (aoffset == 0) &&
			(alen == 8) && (depth == 8))
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
 * @param[in] write Lock for writing
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

/**
 * Set the alpha hint on a buffer
 * @param[in] thiz The buffer to set the alpha hint
 * @param[in] hint The alpha hint to set
 */
EAPI void enesim_buffer_alpha_hint_set(Enesim_Buffer *thiz, Enesim_Alpha_Hint hint)
{
	if (!_buffer_format_has_alpha(thiz->format))
		return;
	thiz->alpha_hint = hint;
}

/**
 * Get the alpha hint on a buffer
 * @param[in] thiz The buffer to get the alpha hint from
 * @return The buffer alpha hint
 */
EAPI Enesim_Alpha_Hint enesim_buffer_alpha_hint_get(Enesim_Buffer *thiz)
{
	return thiz->alpha_hint;

}

/**
 * Converts a buffer into another buffer. Basically it will do a color space
 * conversion.
 * @param[in] thiz The buffer to convert
 * @param[in] dst The destination buffer
 * @return EINA_TRUE if the conversion was correct, EINA_FALSE otherwise
 */
EAPI Eina_Bool enesim_buffer_convert(Enesim_Buffer *thiz, Enesim_Buffer *dst)
{
	Enesim_Converter_2D converter;
	Enesim_Buffer_Format dfmt;
	Enesim_Buffer_Format sfmt;
	Enesim_Buffer_Sw_Data ddata;
	Enesim_Buffer_Sw_Data sdata;
	int w, h, dstw, dsth;

	sfmt = enesim_buffer_format_get(thiz);
	dfmt = enesim_buffer_format_get(dst);
	if (sfmt == dfmt)
		return EINA_FALSE;

	enesim_buffer_size_get(thiz, &w, &h);
	enesim_buffer_size_get(dst, &dstw, &dsth);
	if (dstw != w || dsth != h)
		return EINA_FALSE;

	converter = enesim_converter_surface_get(dfmt, ENESIM_ANGLE_NONE, sfmt);
	if (!converter)
		return EINA_FALSE;

	if (!enesim_buffer_map(thiz, &sdata))
		return EINA_FALSE;

	if (!enesim_buffer_map(dst, &ddata))
	{
		enesim_buffer_unmap(thiz, &sdata, EINA_FALSE);
		return EINA_FALSE;
	}

	/* FIXME check the stride too */
	/* TODO check the clip and x, y */
	converter(&ddata, w, h, &sdata, w, h);
	enesim_buffer_unmap(thiz, &sdata, EINA_FALSE);
	enesim_buffer_unmap(dst, &ddata, EINA_TRUE);

	return EINA_TRUE;
}

/**
 * Converts a buffer into another buffer. Basically it will do a color space
 * conversion.
 * @param[in] thiz The buffer to convert
 * @param[in] dst The destination buffer
 * @param[in] clips A list of clipping areas on the destination surface to limit the conversion. @ender_nullable
 * @return EINA_TRUE if the conversion was correct, EINA_FALSE otherwise
 */
EAPI Eina_Bool enesim_buffer_convert_list(Enesim_Buffer *thiz, Enesim_Buffer *dst, Eina_List *clips)
{
	Enesim_Converter_2D converter;
	Enesim_Buffer_Format dfmt;
	Enesim_Buffer_Format sfmt;
	Enesim_Buffer_Sw_Data ddata;
	Enesim_Buffer_Sw_Data sdata;
	Eina_Rectangle *area;
	Eina_List *l;
	int w, h, dstw, dsth;

	sfmt = enesim_buffer_format_get(thiz);
	dfmt = enesim_buffer_format_get(dst);
	if (sfmt == dfmt)
		return EINA_FALSE;

	enesim_buffer_size_get(thiz, &w, &h);
	enesim_buffer_size_get(dst, &dstw, &dsth);
	if (dstw != w || dsth != h)
		return EINA_FALSE;

	converter = enesim_converter_surface_get(dfmt, ENESIM_ANGLE_NONE, sfmt);
	if (!converter)
		return EINA_FALSE;

	enesim_buffer_sw_data_get(dst, &ddata);
	enesim_buffer_sw_data_get(thiz, &sdata);

	EINA_LIST_FOREACH(clips, l, area)
	{
		Enesim_Buffer_Sw_Data dat;
		Enesim_Buffer_Sw_Data sat;

		/* FIXME check the stride too */
		/* TODO check the clip and x, y */

		enesim_buffer_sw_data_at(&ddata, dfmt, area->x, area->y, &dat);
		enesim_buffer_sw_data_at(&sdata, sfmt, area->x, area->y, &sat);
		converter(&dat, area->w, area->h, &sat, area->w, area->h);
	}

	return EINA_TRUE;
}
