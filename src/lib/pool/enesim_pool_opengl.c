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

#include "enesim_error.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_surface.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"

#include "Enesim_OpenGL.h"

#include "enesim_pool_private.h"
#include "enesim_buffer_private.h"

/**
 * @todo
 * - Create as many textures as we need to create the desired surface
 *
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_OpenGL_Pool
{
} Enesim_OpenGL_Pool;

static Eina_Bool _data_alloc(void *prv EINA_UNUSED, Enesim_Backend *backend,
		void **backend_data,
		Enesim_Buffer_Format fmt, uint32_t w, uint32_t h)
{
	Enesim_Buffer_OpenGL_Data *data;

	data = malloc(sizeof(Enesim_Buffer_OpenGL_Data));
	*backend = ENESIM_BACKEND_OPENGL;
	*backend_data = data;
	switch (fmt)
	{
		case ENESIM_BUFFER_FORMAT_ARGB8888:
		case ENESIM_BUFFER_FORMAT_ARGB8888_PRE:
		data->num_textures = 1;
		glGenTextures(1, &data->texture);
		glBindTexture(GL_TEXTURE_2D, data->texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		break;

		case ENESIM_BUFFER_FORMAT_RGB565:
		case ENESIM_BUFFER_FORMAT_RGB888:
		case ENESIM_BUFFER_FORMAT_A8:
		case ENESIM_BUFFER_FORMAT_GRAY:
		default:
		free(data);
		return EINA_FALSE;
		break;


	}

	return EINA_TRUE;
}

static Eina_Bool _data_from(void *prv EINA_UNUSED,
		Enesim_Backend *backend,
		void **backend_data,
		Enesim_Buffer_Format fmt,
		uint32_t w, uint32_t h,
		Eina_Bool copy,
		Enesim_Buffer_Sw_Data *src)
{
	Enesim_Buffer_OpenGL_Data *data;

	if (!copy) return EINA_FALSE;

	data = malloc(sizeof(Enesim_Buffer_OpenGL_Data));
	glGenTextures(1, &data->texture);
	*backend = ENESIM_BACKEND_OPENGL;
	*backend_data = data;
	switch (fmt)
	{
		case ENESIM_BUFFER_FORMAT_ARGB8888:
		glBindTexture(GL_TEXTURE_2D, data->texture);
        	glPixelStorei(GL_UNPACK_ALIGNMENT, 4);
	        glPixelStorei(GL_UNPACK_ROW_LENGTH, w);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, src->argb8888.plane0);
		break;

		case ENESIM_BUFFER_FORMAT_ARGB8888_PRE:
		case ENESIM_BUFFER_FORMAT_RGB565:
		case ENESIM_BUFFER_FORMAT_RGB888:
		case ENESIM_BUFFER_FORMAT_A8:
		case ENESIM_BUFFER_FORMAT_GRAY:
		default:
		return EINA_FALSE;
		break;
	}

	return EINA_TRUE;
}

static void _data_free(void *prv EINA_UNUSED, void *backend_data,
		Enesim_Buffer_Format fmt EINA_UNUSED,
		Eina_Bool external_allocated EINA_UNUSED)
{
	Enesim_Buffer_OpenGL_Data *data = backend_data;

	glDeleteTextures(1, &data->texture);
}

static Eina_Bool _data_get(void *prv EINA_UNUSED, void *backend_data,
		Enesim_Buffer_Format fmt,
		uint32_t w, uint32_t h EINA_UNUSED,
		Enesim_Buffer_Sw_Data *dst)
{
	Enesim_Buffer_OpenGL_Data *data = backend_data;
	switch (fmt)
	{
		case ENESIM_BUFFER_FORMAT_ARGB8888:
		case ENESIM_BUFFER_FORMAT_ARGB8888_PRE:
		glBindTexture(GL_TEXTURE_2D, data->texture);
        	glPixelStorei(GL_PACK_ALIGNMENT, 4);
	        glPixelStorei(GL_PACK_ROW_LENGTH, w);
		glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, dst->argb8888.plane0);
		dst->argb8888.plane0_stride = w * 4;
		break;

		case ENESIM_BUFFER_FORMAT_RGB565:
		case ENESIM_BUFFER_FORMAT_RGB888:
		case ENESIM_BUFFER_FORMAT_A8:
		case ENESIM_BUFFER_FORMAT_GRAY:
		default:
		return EINA_FALSE;
		break;
	}

	return EINA_FALSE;
}

static void _free(void *prv)
{
	Enesim_OpenGL_Pool *thiz = prv;

	free(thiz);
}

static Enesim_Pool_Descriptor _descriptor = {
	/* .data_alloc = */ _data_alloc,
	/* .data_free =  */ _data_free,
	/* .data_from =  */ _data_from,
	/* .data_get =   */ _data_get,
	/* .free =       */ _free,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Pool * enesim_pool_opengl_new(void)
{
	Enesim_OpenGL_Pool *thiz;
	Enesim_Pool *p;

	/* initialize the opengl system */
	enesim_opengl_init();
	thiz = calloc(1, sizeof(Enesim_OpenGL_Pool));

	p = enesim_pool_new(&_descriptor, thiz);
	/* FIXME we should put a simple initializer */
	glEnable(GL_TEXTURE_2D);
	if (!p)
	{
		free(thiz);
		return NULL;
	}

	return p;
}
