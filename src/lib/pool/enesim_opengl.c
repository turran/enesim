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
typedef struct _Enesim_Buffer_OpenGL_Data
{
	GLuint texture;
	GLuint fbo;
} Enesim_Buffer_OpenGL_Data;

typedef struct _Enesim_OpenGL_Pool
{
} Enesim_OpenGL_Pool;

static Eina_Bool _data_alloc(void *prv, Enesim_Backend *backend,
		void **backend_data,
		Enesim_Buffer_Format fmt, uint32_t w, uint32_t h)
{
	Enesim_OpenGL_Pool *thiz = prv;
	Enesim_Buffer_OpenGL_Data *data;

	data = malloc(sizeof(Enesim_Buffer_OpenGL_Data));
	*backend_data = data;
	glGenTexture(1, &data->texture);
	glGenFramebuffersEXT(1, &data->fbo);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, data->fbo);
	/* attach the texture to the first color attachment */
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
		GL_TEXTURE_2D, data->texture, 0);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);

	return EINA_TRUE;
}

static Eina_Bool _data_from(void *prv,
		Enesim_Backend *backend,
		void **backend_data,
		Enesim_Buffer_Format fmt,
		uint32_t w, uint32_t h,
		Eina_Bool copy,
		Enesim_Buffer_Sw_Data *src)
{
	Enesim_OpenGL_Pool *thiz = prv;
	Enesim_Buffer_OpenGL_Data *data = backend_data;

	return EINA_TRUE;
}

static void _data_free(void *prv, void *backend_data,
		Enesim_Buffer_Format fmt,
		Eina_Bool external_allocated)
{
	Enesim_Buffer_OpenGL_Data *data = backend_data;
}

static Eina_Bool _data_get(void *prv, void *backend_data,
		uint32_t w, uint32_t h,
		Enesim_Buffer_Sw_Data *dst)
{
	//Enesim_Buffer_OpenGL_Data *data = backend_data;

	return EINA_TRUE;
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

	thiz = calloc(1, sizeof(Enesim_OpenGL_Pool));

	p = enesim_pool_new(&_descriptor, thiz);
	if (!p)
	{
		free(thiz);
		return NULL;
	}

	return p;
}
