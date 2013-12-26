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
#ifndef ENESIM_RENDERER_GL_PRIVATE_H_
#define ENESIM_RENDERER_GL_PRIVATE_H_

#if BUILD_OPENGL
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
#endif

typedef struct _Enesim_Renderer_OpenGL_Shader Enesim_Renderer_OpenGL_Shader;
typedef struct _Enesim_Renderer_OpenGL_Program Enesim_Renderer_OpenGL_Program;

/**
 * The draw function that every gl based renderer should implement
 * @param r The renderer to draw
 * @param s The destination surface to draw
 * @param area ??
 * @param dw ??
 * @param dh ??
 */
typedef void (*Enesim_Renderer_OpenGL_Draw)(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, const Eina_Rectangle *area, int x, int y);

typedef enum _Enesim_Renderer_OpenGL_Shader_Type
{
	ENESIM_SHADER_VERTEX,
	ENESIM_SHADER_GEOMETRY,
	ENESIM_SHADER_FRAGMENT,
	ENESIM_SHADERS,
} Enesim_Renderer_OpenGL_Shader_Type;

struct _Enesim_Renderer_OpenGL_Shader
{
	Enesim_Renderer_OpenGL_Shader_Type type;
	const char *name;
	const char *source;
};

struct _Enesim_Renderer_OpenGL_Program
{
	const char *name;
	Enesim_Renderer_OpenGL_Shader **shaders;
	int num_shaders;
};

#if BUILD_OPENGL
typedef struct _Enesim_Renderer_OpenGL_Program_Data
{
	Enesim_Renderer_OpenGL_Program **programs;
	int num_programs;
	/* generated */
	Enesim_OpenGL_Compiled_Program *compiled;
} Enesim_Renderer_OpenGL_Program_Data;

typedef struct _Enesim_Renderer_OpenGL_Data
{
	/* data fetch on the initialize */
	Enesim_Renderer_OpenGL_Program_Data *program;
	/* data fetch on the setup */
	Enesim_Renderer_OpenGL_Draw draw;
} Enesim_Renderer_OpenGL_Data;
#endif

Eina_Bool enesim_renderer_opengl_shader_ambient_setup(GLenum pid,
		Enesim_Color color);
Eina_Bool enesim_renderer_opengl_shader_texture_setup(GLenum pid,
		GLint texture_unit, Enesim_Surface *s, Enesim_Color color);

extern Enesim_Renderer_OpenGL_Shader enesim_renderer_opengl_shader_texture;
extern Enesim_Renderer_OpenGL_Shader enesim_renderer_opengl_shader_ambient;

#endif

