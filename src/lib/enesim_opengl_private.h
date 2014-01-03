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
#ifndef ENESIM_OPENGL_PRIVATE_H_
#define ENESIM_OPENGL_PRIVATE_H_

#define ENESIM_OPENGL_ERROR() 			\
{						\
	GLenum _err;				\
	_err = glGetError(); 			\
	ERR("OpenGL error 0x%08x", _err);	\
}

typedef struct _Enesim_OpenGL_Compiled_Program
{
	GLenum id;
	GLenum *shaders;
	int num_shaders;
} Enesim_OpenGL_Compiled_Program;

Eina_Bool enesim_opengl_init(void);

void enesim_opengl_matrix_convert(Enesim_Matrix *m, float fm[16]);

GLuint enesim_opengl_texture_new(int width, int height, void *data);
void enesim_opengl_texture_free(GLuint id);

GLuint enesim_opengl_span_new(int len, void *data);
void enesim_opengl_span_free(GLuint id);

void enesim_opengl_compiled_program_set(Enesim_OpenGL_Compiled_Program *cp);
void enesim_opengl_rop_set(Enesim_Rop rop);
void enesim_opengl_clip_unset(void);
void enesim_opengl_clip_set(const Eina_Rectangle *area, int ww, int hh);

void enesim_opengl_draw_area(const Eina_Rectangle *area);
Eina_Bool enesim_opengl_target_surface_set(Enesim_Surface *s);
void enesim_opengl_buffer_data_free(Enesim_Buffer_OpenGL_Data *data);

#endif
