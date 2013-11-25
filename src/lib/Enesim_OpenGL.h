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

#define GL_GLEXT_PROTOTYPES
#include "GL/gl.h"
#include "GL/glu.h"
#include "GL/glext.h"

/**
 * @defgroup Enesim_OpenGL_Group OpenGL
 * @brief OpenGL specific functions
 * @{
 */

/**
 * Helper macro to stringify a shader code
 */
#define ENESIM_OPENGL_SHADER(k) #k

/**
 * The data associated with an OpenGL buffer
 */
typedef struct _Enesim_Buffer_OpenGL_Data
{
	GLuint *textures; /**< The textures id */
	unsigned int num_textures; /**< The number of textures */
} Enesim_Buffer_OpenGL_Data;

EAPI Enesim_Pool * enesim_pool_opengl_new(void);
EAPI Enesim_Buffer * enesim_buffer_new_opengl_data_from(Enesim_Buffer_Format f,
		uint32_t w, uint32_t h, GLuint *texture,
		unsigned int num_textures);
EAPI Enesim_Surface * enesim_surface_new_opengl_data_from(Enesim_Format f,
		uint32_t w, uint32_t h, GLuint texture);
EAPI const Enesim_Buffer_OpenGL_Data * enesim_surface_opengl_data_get(
		Enesim_Surface *thiz);

/**
 * @}
 */

#endif
