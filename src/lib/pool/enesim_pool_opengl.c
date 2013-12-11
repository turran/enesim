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

#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"

#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"

#include "enesim_pool_private.h"
#include "enesim_buffer_private.h"

#define ENESIM_LOG_DEFAULT enesim_log_pool

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
	/* we use our internal fbo to zero the buffers */
	GLuint fbo;
} Enesim_OpenGL_Pool;

static void _zero_buffer(Enesim_OpenGL_Pool *thiz, Enesim_Buffer_OpenGL_Data *data)
{
	GLenum status;
	unsigned int i;

	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, thiz->fbo);
	for (i = 0; i < data->num_textures; i++)
	{
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT,
			GL_COLOR_ATTACHMENT0_EXT, GL_TEXTURE_2D,
			data->textures[0], 0);

	        status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
		if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
		{
			/* FIXME add a message in the pool log */
			continue;
		}
		glClearColor(0.0, 0.0, 0.0, 1.0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	}
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

static Eina_Bool _data_alloc(void *prv, Enesim_Backend *backend,
		void **backend_data,
		Enesim_Buffer_Format fmt, uint32_t w, uint32_t h)
{
	Enesim_OpenGL_Pool *thiz = prv;
	Enesim_Buffer_OpenGL_Data *data;

	data = calloc(1, sizeof(Enesim_Buffer_OpenGL_Data));
	data->textures = calloc(1, sizeof(GLuint));
	data->num_textures = 1;

	*backend = ENESIM_BACKEND_OPENGL;
	*backend_data = data;
	switch (fmt)
	{
		case ENESIM_BUFFER_FORMAT_ARGB8888:
		case ENESIM_BUFFER_FORMAT_ARGB8888_PRE:
		glGenTextures(1, data->textures);
		glBindTexture(GL_TEXTURE_2D, data->textures[0]);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
		break;

		case ENESIM_BUFFER_FORMAT_RGB565:
		case ENESIM_BUFFER_FORMAT_RGB888:
		case ENESIM_BUFFER_FORMAT_A8:
		case ENESIM_BUFFER_FORMAT_GRAY:
		default:
		free(data->textures);
		free(data);
		return EINA_FALSE;
		break;


	}
	_zero_buffer(thiz, data);
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

	data = calloc(1, sizeof(Enesim_Buffer_OpenGL_Data));
	data->textures = calloc(1, sizeof(GLuint));
	data->num_textures = 1;
	glGenTextures(1, data->textures);
	*backend = ENESIM_BACKEND_OPENGL;
	*backend_data = data;
	switch (fmt)
	{
		case ENESIM_BUFFER_FORMAT_ARGB8888:
		glBindTexture(GL_TEXTURE_2D, data->textures[0]);
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
		free(data->textures);
		free(data);
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
	enesim_opengl_buffer_data_free(data);
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
		glBindTexture(GL_TEXTURE_2D, data->textures[0]);
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

	glDeleteFramebuffersEXT(1, &thiz->fbo);
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
	if (!GLEW_ARB_framebuffer_object)
	{
		ERR("No ARB_framebuffer_object extension found");
		return NULL;
	}

	glEnable(GL_TEXTURE_2D);

	thiz = calloc(1, sizeof(Enesim_OpenGL_Pool));
	glGenFramebuffersEXT(1, &thiz->fbo);

	p = enesim_pool_new(&_descriptor, thiz);
	if (!p)
	{
		_free(thiz);
		return NULL;
	}

	return p;
}
