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
#ifndef ENESIM_RENDERER_PATH_ENESIM_GL_LOOP_BLINN_H_
#define ENESIM_RENDERER_PATH_ENESIM_GL_LOOP_BLINN_H_

#if BUILD_OPENGL

Eina_Bool enesim_renderer_path_enesim_gl_loop_blinn_initialize(
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***programs);

Eina_Bool enesim_renderer_path_enesim_gl_loop_blinn_setup(
		Enesim_Renderer *r,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l EINA_UNUSED);

#endif
#endif

