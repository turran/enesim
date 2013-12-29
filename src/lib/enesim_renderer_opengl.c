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
#include "libargb.h"

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#if BUILD_OPENGL
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
#endif

#include "enesim_buffer_private.h"
#include "enesim_surface_private.h"
#include "enesim_renderer_private.h"

#define ENESIM_LOG_DEFAULT enesim_log_renderer
/*
 * TODO
 * Add a program cache
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static Eina_Hash *_program_lut = NULL;
static GLuint _fbo = 0;

#if 0
static Eina_Bool _fragment_shader_support = EINA_FALSE;
static Eina_Bool _geometry_shader_support = EINA_FALSE;

static void _opengl_extensions_setup(void)
{
	char *extensions;

	extensions  = (char *)glGetString(GL_EXTENSIONS);
	if (!extensions) return;
	if (!strstr(extensions, "GL_ARB_shader_objects"))
		return;
	if (!strstr(extensions, "GL_ARB_shading_language_100"))
		return;
	if (strstr(extensions, "GL_ARB_fragment_shader"))
		_fragment_shader_support = EINA_TRUE;
	if (strstr(extensions, "GL_EXT_geometry_shader4"))
		_geometry_shader_support = EINA_TRUE;
}

/* Disabled for now until we find a solution with the unresolved symbol */
static void _opengl_geometry_shader_setup(Enesim_Renderer_OpenGL_Shader *shader)
{
	st = GL_GEOMETRY_SHADER_EXT;
	glProgramParameteriEXT(program, GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);
	glProgramParameteriEXT(program, GL_GEOMETRY_INPUT_TYPE_EXT, GL_POINTS);
	glProgramParameteriEXT(program, GL_GEOMETRY_VERTICES_OUT_EXT, 10);
}

#endif

static Eina_Bool _opengl_shader_compile(GLenum pid,
		Enesim_Renderer_OpenGL_Shader *shader,
		GLenum *sid)
{
	GLenum st;
	GLenum sh;
	size_t size;
	int ok = 0;

	/* FIXME for now we only support fragment shaders */
	switch (shader->type)
	{
		case ENESIM_SHADER_FRAGMENT:
		st = GL_FRAGMENT_SHADER_ARB;
		break;

		case ENESIM_SHADER_VERTEX:
		st = GL_VERTEX_SHADER_ARB;
		break;

		default:
		return EINA_FALSE;
	}

	/* setup the shaders */
	sh = glCreateShaderObjectARB(st);
	size = strlen(shader->source);
	glShaderSourceARB(sh, 1, (const GLcharARB **)&shader->source, (const GLint *)&size);
	/* compile it */
	glCompileShaderARB(sh);
	glGetObjectParameterivARB(sh, GL_OBJECT_COMPILE_STATUS_ARB, &ok);
	if (!ok)
	{
		char info_log[PATH_MAX];
		glGetInfoLogARB(sh, sizeof(info_log), NULL, info_log);
		printf("Failed to compile %s\n", info_log);
		return EINA_FALSE;
	}
	/* attach the shader to the program */
	glAttachObjectARB(pid, sh);
	*sid = sh;

	return EINA_TRUE;
}

static Eina_Bool _opengl_compiled_program_link(Enesim_OpenGL_Compiled_Program *cp)
{
	int ok = 0;

	/* link it */
	glLinkProgramARB(cp->id);
	glGetObjectParameterivARB(cp->id, GL_OBJECT_LINK_STATUS_ARB, &ok);
	if (!ok)
	{
		char info_log[PATH_MAX];
		glGetInfoLogARB(cp->id, sizeof(info_log), NULL, info_log);
		printf("Failed to Link %s\n", info_log);
		/* FIXME destroy all the shaders */

		return EINA_FALSE;
	}
	return EINA_TRUE;
}

static Eina_Bool _opengl_compiled_program_new(
		Enesim_OpenGL_Compiled_Program *cp,
		Enesim_Renderer *r EINA_UNUSED,
		Enesim_Surface *s EINA_UNUSED,
		Enesim_Renderer_OpenGL_Data *rdata EINA_UNUSED,
		Enesim_Renderer_OpenGL_Program *p)
{
	int i;

	cp->num_shaders = p->num_shaders;
	cp->shaders = calloc(cp->num_shaders, sizeof(GLenum));
	cp->id = glCreateProgramObjectARB();

	/* compile every shader */
	for (i = 0; i < p->num_shaders; i++)
	{
		Enesim_Renderer_OpenGL_Shader *shader;
		GLenum sh;

		shader = p->shaders[i];

		if (!_opengl_shader_compile(cp->id, shader, &sh))
		{
			i++;
			goto error;
		}

		cp->shaders[i] = sh;
	}
	/* now link */
	if (!_opengl_compiled_program_link(cp))
		goto error;
	return EINA_TRUE;

error:
	for (i--; i >= 0; i--)
	{
		/* TODO destroy each shader */
	}
	free(cp->shaders);

	return EINA_FALSE;
}

static void _opengl_compiled_program_free(
		Enesim_OpenGL_Compiled_Program *cp)
{
	int i;

	for (i = 0; i < cp->num_shaders; i++)
	{
		/* TODO destroy each shader */
	}
	free(cp->shaders);
}

#if 0
static void _opengl_draw_own_geometry(Enesim_Renderer_OpenGL_Data *rdata,
		const Eina_Rectangle *area,
		int width, int height)
{
	/* check the types of shaders
	 * if only geometry then we need the common vertex
	 */
	if (rdata->has_geometry)
	{
		int pixel_transform;

		pixel_transform = glGetUniformLocationARB(rdata->program, "pixel_transform");
		printf("pixel_transform = %d\n", pixel_transform);
		if (pixel_transform >= 0)
		{
			double sx = 2.0/width;
			double sy = 2.0/height;

			//const GLfloat values[] = {-sx, 0, 0, 0, -sy, 0, 0, 0, 1};
			const GLfloat values[] = {sx, 0, -1, 0, -sy, 1, 0, 0, 1};
			glUniformMatrix3fv(pixel_transform, 1, GL_FALSE, values);
		}
		/* in this case we need to setup some common uniforms
		 * like the pixel transformation, etc
		 */
		glBegin(GL_POINTS);
			glVertex2d(area->x, area->y);
		glEnd();
	}
	/* simplest case */
	else
	{
		glMatrixMode(GL_TEXTURE);
		glLoadIdentity();
		/* FIXME for now */
		//glScalef(0.02, 0.02, 1.0);
		glRotatef(30, 0.0, 0.0, 1.0);

		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, width, 0, height, -1, 1);

		glBegin(GL_QUADS);
			glTexCoord2d(area->x, area->y); glVertex2d(area->x, area->y);
			glTexCoord2d(area->x + area->w, area->y); glVertex2d(area->x + area->w, area->y);
			glTexCoord2d(area->x + area->w, area->y + area->h); glVertex2d(area->x + area->w, area->y + area->h);
			glTexCoord2d(area->x, area->y + area->h); glVertex2d(area->x, area->y + area->h);
		glEnd();
	}
}
#endif

static void _program_free(void *data)
{
	Enesim_Renderer_OpenGL_Program_Data *d = data;
	free(d);
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Eina_Bool enesim_renderer_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop, Enesim_Log **error)
{
	Enesim_Renderer_Class *klass;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_Renderer_OpenGL_Draw draw;
	int i;

	klass = ENESIM_RENDERER_CLASS_GET(r);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);

	/* create the data in case does not exist */
	if (!rdata)
	{
		/* initialize the opengl system */
		enesim_opengl_init();

		rdata = calloc(1, sizeof(Enesim_Renderer_OpenGL_Data));
		enesim_renderer_backend_data_set(r, ENESIM_BACKEND_OPENGL, rdata);
		if (klass->opengl_initialize)
		{
			Enesim_Renderer_OpenGL_Program **programs = NULL;
			Enesim_Renderer_OpenGL_Program_Data *pdata;
			const char *name;
			int num = 0;

			name = enesim_renderer_name_get(r);
			if (!name)
			{
				ENESIM_RENDERER_LOG(r, error, "Renderer with no name?");
				return EINA_FALSE;
			}

			pdata = eina_hash_find(_program_lut, name);
			if (pdata)
			{
				rdata->program = pdata;
				goto setup;
			}
			pdata = calloc(1, sizeof(Enesim_Renderer_OpenGL_Program_Data));
			if (!klass->opengl_initialize(r, &num, &programs))
				return EINA_FALSE;
			pdata->programs = programs;
			pdata->num_programs = num;

			eina_hash_add(_program_lut, name, pdata);

			if (!programs)
				goto setup;

			pdata->compiled = calloc(pdata->num_programs, sizeof(Enesim_OpenGL_Compiled_Program));
			for (i = 0; i < pdata->num_programs; i++)
			{
				Enesim_Renderer_OpenGL_Program *p;
				Enesim_OpenGL_Compiled_Program *cp;

				p = pdata->programs[i];
				cp = &pdata->compiled[i];

				/* TODO try to find the program */
				/* TODO check return value */
				_opengl_compiled_program_new(cp, r, s, rdata, p);
			}
			rdata->program = pdata;
		}
	}
setup:
	/* do the setup */
	if (!klass->opengl_setup) return EINA_FALSE;
	if (!klass->opengl_setup(r, s, rop, &draw, error))
		return EINA_FALSE;

	if (!draw)
	{
		return EINA_FALSE;
	}

	/* FIXME we need to know if we should create, compile and link the programs
	 * again or not .... */

	/* store the data returned by the setup */
	rdata->draw = draw;

	return EINA_TRUE;
}

void enesim_renderer_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS_GET(r);
	if (!klass->opengl_cleanup) return;
	klass->opengl_cleanup(r, s);
}

/* TODO later we need to decide what to do with the area that we dont draw */
void enesim_renderer_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, const Eina_Rectangle *area, int x, int y)
{
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_Buffer_OpenGL_Data *sdata;
#if 0
	Eina_Rectangle final;
	Eina_Bool intersect;
	Eina_Bool visible;
#endif

	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	sdata = enesim_surface_backend_data_get(s);
#if 0
	final = r->current_destination_bounds;
	intersect = eina_rectangle_intersection(&final, area);

	visible = enesim_renderer_visibility_get(r);
	if (!visible)
		return;
	if (!intersect || !eina_rectangle_is_valid(&final))
		return;
#endif
	/* check that we have a valid fbo */
	if (!sdata->fbo)
	{
		glGenFramebuffersEXT(1, &sdata->fbo);
		if (!sdata->fbo)
		{
			WRN("Impossible to create the fbo");
			return;
		}
	}
	/* now draw */
	if (rdata->draw)
	{
		GLenum error;
#if 0
		rdata->draw(r, s, rop, &final, x, y);
#else
		rdata->draw(r, s, rop, area, x, y);
#endif
		error = glGetError();
		if (error)
		{
			WRN("Rendering '%s' gave error 0x%08x", r->name, error);
		}
	}
	/* don't use any program */
	glUseProgramObjectARB(0);
	/* set the draw rop back to fill */
	enesim_opengl_rop_set(ENESIM_ROP_FILL);
}

void enesim_renderer_opengl_init(void)
{
	_program_lut = eina_hash_string_superfast_new(_program_free);
}

void enesim_renderer_opengl_shutdown(void)
{
	eina_hash_free(_program_lut);
}

/* we need a way to destroy the renderer data */
void enesim_renderer_opengl_free(Enesim_Renderer *r)
{
	Enesim_Renderer_OpenGL_Data *gl_data;

	gl_data = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	if (!gl_data) return;
	free(gl_data);
}

/*----------------------------------------------------------------------------*
 *                                Shaders                                     *
 *----------------------------------------------------------------------------*/
Eina_Bool enesim_renderer_opengl_shader_ambient_setup(GLenum pid,
		Enesim_Color color)
{
	int final_color_u;

	glUseProgramObjectARB(pid);
	final_color_u = glGetUniformLocationARB(pid, "ambient_color");
	glUniform4fARB(final_color_u,
			argb8888_red_get(color) / 255.0,
			argb8888_green_get(color) / 255.0,
			argb8888_blue_get(color) / 255.0,
			argb8888_alpha_get(color) / 255.0);

	return EINA_TRUE;
}

Eina_Bool enesim_renderer_opengl_shader_texture_setup(GLenum pid,
		GLint texture_unit, Enesim_Surface *s, Enesim_Color color)
{
	Enesim_Buffer_OpenGL_Data *backend_data;
	int w, h;
	int texture_u;
	int size_u;
	int color_u;

	glUseProgramObjectARB(pid);
	color_u = glGetUniformLocationARB(pid, "texture_color");
	texture_u = glGetUniformLocationARB(pid, "texture_texture");
	size_u = glGetUniformLocationARB(pid, "texture_size");

	glUniform4fARB(color_u,
			argb8888_red_get(color) / 255.0,
			argb8888_green_get(color) / 255.0,
			argb8888_blue_get(color) / 255.0,
			argb8888_alpha_get(color) / 255.0);


	enesim_surface_size_get(s, &w, &h);
	glUniform2i(size_u, w, h);

	backend_data = enesim_surface_backend_data_get(s);
	glActiveTexture(GL_TEXTURE0 + texture_unit);
	glBindTexture(GL_TEXTURE_2D, backend_data->textures[0]);
	glUniform1i(texture_u, texture_unit);

	return EINA_TRUE;
}

Enesim_Renderer_OpenGL_Shader enesim_renderer_opengl_shader_texture = {
	/* .type 	= */ ENESIM_SHADER_FRAGMENT,
	/* .name 	= */ "enesim_renderer_opengl_shader_texture",
	/* .source	= */
#include "enesim_renderer_opengl_common_texture.glsl"
};

Enesim_Renderer_OpenGL_Shader enesim_renderer_opengl_shader_ambient = {
	/* .type	= */ ENESIM_SHADER_FRAGMENT,
	/* .name	= */ "enesim_renderer_opengl_shader_ambient",
	/* .source	= */
#include "enesim_renderer_opengl_common_ambient.glsl"
};

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
