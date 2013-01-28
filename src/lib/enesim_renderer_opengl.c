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
#include "enesim_private.h"

#include "enesim_main.h"
#include "enesim_error.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"

#if BUILD_OPENGL
#include "Enesim_OpenGL.h"
#endif

#include "enesim_buffer_private.h"
#include "enesim_renderer_private.h"
/*
 * Shall we use only one framebuffer? One framebuffer per renderer?
 * Add a program cache
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static Eina_Hash *_program_lut = NULL;

static Eina_Bool _fragment_shader_support = EINA_FALSE;
static Eina_Bool _geometry_shader_support = EINA_FALSE;

/* FIXME for debugging purposes */
#define GLERR {\
        GLenum err; \
        err = glGetError(); \
        printf("Error %d\n", err); \
        }

#if 0
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

static Eina_Bool _opengl_compiled_program_link(Enesim_Renderer_OpenGL_Compiled_Program *cp)
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
		Enesim_Renderer_OpenGL_Compiled_Program *cp,
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
		Enesim_Renderer_OpenGL_Compiled_Program *cp)
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

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Eina_Bool enesim_renderer_opengl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Error **error)
{
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_Renderer_OpenGL_Draw draw;
	Enesim_Buffer_OpenGL_Data *sdata;
	GLenum status;
	int i;

	sdata = enesim_surface_backend_data_get(s);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);

	/* create the data in case does not exist */
	if (!rdata)
	{
		/* initialize the opengl system */
		enesim_opengl_init();

		rdata = calloc(1, sizeof(Enesim_Renderer_OpenGL_Data));
		enesim_renderer_backend_data_set(r, ENESIM_BACKEND_OPENGL, rdata);
		if (r->descriptor.opengl_initialize)
		{
			Enesim_Renderer_OpenGL_Program **programs = NULL;
			Enesim_Renderer_OpenGL_Program_Data *pdata;
			const char *name;
			int num = 0;

			name = r->descriptor.name(r);
			if (!name)
			{
				ENESIM_RENDERER_ERROR(r, error, "Renderer with no name?");
				return EINA_FALSE;
			}

			pdata = eina_hash_find(_program_lut, name);
			if (pdata)
			{
				rdata->program = pdata;
				goto setup;
			}
			pdata = calloc(1, sizeof(Enesim_Renderer_OpenGL_Program_Data));
			if (!r->descriptor.opengl_initialize(r, &num, &programs))
				return EINA_FALSE;
			pdata->programs = programs;
			pdata->num_programs = num;

			eina_hash_add(_program_lut, name, pdata);

			if (!programs)
				goto setup;

			pdata->compiled = calloc(pdata->num_programs, sizeof(Enesim_Renderer_OpenGL_Compiled_Program));
			for (i = 0; i < pdata->num_programs; i++)
			{
				Enesim_Renderer_OpenGL_Program *p;
				Enesim_Renderer_OpenGL_Compiled_Program *cp;

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
	if (!r->descriptor.opengl_setup) return EINA_FALSE;
	if (!r->descriptor.opengl_setup(r, states, s,
			&draw,
			error))
		return EINA_FALSE;

	if (!draw)
	{
		return EINA_FALSE;
	}

	/* FIXME we need to know if we should create, compile and link the programs
	 * again or not .... */

	/* create the fbos */
	if (!rdata->fbo)
	{
		glGenFramebuffersEXT(1, &rdata->fbo);
	}
	/* attach the texture to the first color attachment */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, rdata->fbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			GL_TEXTURE_2D, sdata->texture, 0);

        status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
        if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
        {
		ENESIM_RENDERER_ERROR(r, error, "Impossible too setup the framebuffer %d", status);
        }

	/* store the data returned by the setup */
	rdata->draw = draw;

	return EINA_TRUE;
}

void enesim_renderer_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	if (!r->descriptor.opengl_cleanup) return;
	r->descriptor.opengl_cleanup(r, s);
}

/* FIXME this should be exported and normalize in the same way as the one on the enesim_renderer_sw.c
 */
void enesim_renderer_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s, const Eina_Rectangle *area,
		int x EINA_UNUSED, int y EINA_UNUSED)
{
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_Buffer_OpenGL_Data *sdata;
	int width;
	int height;

	sdata = enesim_surface_backend_data_get(s);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);

	/* run it on the renderer fbo */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, rdata->fbo);
	glBindTexture(GL_TEXTURE_2D, 0);
	glEnable(GL_BLEND);

	/* create the geometry to render to */
	enesim_surface_size_get(s, &width, &height);
	/* now draw */
	if (rdata->draw)
	{
		rdata->draw(r, s, area, width, height);
	}
	glUseProgramObjectARB(0);
}

void enesim_renderer_opengl_init(void)
{
	_program_lut = eina_hash_string_superfast_new(NULL);
}

void enesim_renderer_opengl_shutdown(void)
{
	/* TODO destroy the lut */
}

/* we need a way to destroy the renderer data */
void enesim_renderer_opengl_free(Enesim_Renderer *r)
{
	Enesim_Renderer_OpenGL_Data *rdata;
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	if (!rdata) return;

	glDeleteFramebuffersEXT(1, &rdata->fbo);
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
#if 0
EAPI void enesim_renderer_opengl_target_set(Enesim_Renderer *r, Enesim_Surface *s)
{

}

EAPI void enesim_renderer_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s)
{

}

#endif
