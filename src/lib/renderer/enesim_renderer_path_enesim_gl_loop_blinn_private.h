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

typedef struct _Enesim_Renderer_Path_Enesim_OpenGL_Loop_Blinn_Polygon
{
	GLenum type;
	Enesim_Polygon *polygon;
} Enesim_Renderer_Path_Enesim_OpenGL_Loop_Blinn_Polygon;

typedef struct _Enesim_Renderer_Path_Enesim_OpenGL_Loop_Blinn_Figure
{
	Eina_List *polygons;
	Enesim_Surface *tmp;
	Enesim_Surface *renderer_s;
	Eina_Bool needs_tesselate : 1;
} Enesim_Renderer_Path_Enesim_OpenGL_Loop_Blinn_Figure;

typedef struct _Enesim_Renderer_Path_Enesim_OpenGL_Loop_Blinn
{
	Enesim_Renderer_Path_Enesim_OpenGL_Loop_Blinn_Figure stroke;
	Enesim_Renderer_Path_Enesim_OpenGL_Loop_Blinn_Figure fill;
} Enesim_Renderer_Path_Enesim_OpenGL_Loop_Blinn;

Eina_Bool enesim_renderer_path_enesim_gl_loop_blinn_initialize(
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***programs);

Eina_Bool enesim_renderer_path_enesim_gl_loop_blinn_setup(
		Enesim_Renderer *r,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l EINA_UNUSED);

#endif
#endif

