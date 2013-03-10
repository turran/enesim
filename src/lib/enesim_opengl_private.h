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
#ifndef ENESIM_OPENGL_PRIVATE_H_
#define ENESIM_OPENGL_PRIVATE_H_

typedef struct _Enesim_OpenGL_Compiled_Program
{
	GLenum id;
	GLenum *shaders;
	int num_shaders;
} Enesim_OpenGL_Compiled_Program;

void enesim_opengl_init(void);
GLenum enesim_opengl_texture_new(int width, int height);
void enesim_opengl_texture_free(GLenum id);
void enesim_opengl_compiled_program_set(Enesim_OpenGL_Compiled_Program *cp);
void enesim_opengl_rop_set(Enesim_Rop rop);
void enesim_opengl_clip_unset(void);
void enesim_opengl_clip_set(const Eina_Rectangle *area, int ww, int hh);

#endif
