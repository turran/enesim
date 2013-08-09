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
#ifndef ENESIM_OPENGL_H_
#define ENESIM_OPENGL_H_

#define ENESIM_OPENGL_SHADER(k) #k

#define GL_GLEXT_PROTOTYPES
#include "GL/gl.h"
#include "GL/glu.h"
#include "GL/glext.h"

/**
 * @page Enesim_OpenGL_Page OpenGL Backend
 * @file
 * @brief Enesim OpenGL API
 */

EAPI Enesim_Pool * enesim_pool_opengl_new(void);
EAPI Enesim_Buffer * enesim_buffer_new_opengl_data_from(Enesim_Buffer_Format f, uint32_t w, uint32_t h,
		GLuint *texture, unsigned int num_textures);
EAPI Enesim_Surface * enesim_surface_new_opengl_data_from(Enesim_Format f, uint32_t w, uint32_t h,
		GLuint texture);

typedef struct _Enesim_Buffer_OpenGL_Data
{
	GLuint texture;
	unsigned int num_textures;
} Enesim_Buffer_OpenGL_Data;

#endif
