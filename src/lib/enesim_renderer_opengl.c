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
/*
 * Shall we use only one framebuffer? One framebuffer per renderer?
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static Eina_Bool _fragment_shader_support = EINA_FALSE;
static Eina_Bool _geometry_shader_support = EINA_FALSE;

/* FIXME this should be ported to the new glsl 3.1 standard */
static const char _vertex_passthrough[] = "					\
void main(void)									\
{										\
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; 		\
	gl_TexCoord[0]  = gl_TextureMatrix[0] * gl_MultiTexCoord0;		\
}";

/* gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex; */

static Enesim_Renderer_OpenGL_Shader _vertex_shader = {
	/* .type = 	*/ ENESIM_SHADER_VERTEX,
	/* .name = 	*/ "vertex_passthrough",
	/* .source =	*/ _vertex_passthrough,
	/* .size =	*/ sizeof(_vertex_passthrough),
};

/* FIXME for debugging purposes */
#define GLERR {\
        GLenum err; \
        err = glGetError(); \
        printf("Error %d\n", err); \
        }

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

static void _opengl_rop_set(Enesim_Renderer *r)
{
	Enesim_Rop rop;

	enesim_renderer_rop_get(r, &rop);
	glBlendEquation(GL_FUNC_ADD);
	switch (rop)
	{
		case ENESIM_BLEND:
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		break;

		case ENESIM_FILL:
		glBlendFunc(GL_ONE, GL_ZERO);
		break;

		default:
		break;
	}
}

#if 0
/* Disabled for now until we find a solution with the unresolved symbol */
static void _opengl_geometry_shader_setup(Enesim_Renderer_OpenGL_Shader *shader)
{
	st = GL_GEOMETRY_SHADER_EXT;
	glProgramParameteriEXT(program, GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);
	glProgramParameteriEXT(program, GL_GEOMETRY_INPUT_TYPE_EXT, GL_POINTS);
	glProgramParameteriEXT(program, GL_GEOMETRY_VERTICES_OUT_EXT, 10);
}

static void _opengl_vertex_shader_setup(Enesim_Renderer_OpenGL_Shader *shader)
{
	st = GL_VERTEX_SHADER_ARB;
}
#endif

static Eina_Bool _opengl_shader_compile(GLenum pid,
		Enesim_Renderer_OpenGL_Shader *shader,
		GLenum *sid)
{
	GLenum st;
	GLenum sh;
	int ok = 0;

	/* FIXME for now we only support fragment shaders */
	if (shader->type != ENESIM_SHADER_FRAGMENT)
		return EINA_FALSE;

	st = GL_FRAGMENT_SHADER_ARB;
	/* setup the shaders */
	sh = glCreateShaderObjectARB(st);
	glShaderSourceARB(sh, 1, (const GLcharARB **)&shader->source, (const GLint *)&shader->size);
	/* compile it */
	glCompileShaderARB(sh);
	glGetObjectParameterivARB(sh, GL_OBJECT_COMPILE_STATUS_ARB, &ok);
	if (!ok)
	{
		char log[PATH_MAX];
		glGetInfoLogARB(sh, sizeof(log), NULL, log);
		printf("Failed to compile %s\n", log);
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
		char log[PATH_MAX];
		glGetInfoLogARB(cp->id, sizeof(log), NULL, log);
		printf("Failed to Link %s\n", log);
		/* FIXME destroy all the shaders */

		return EINA_FALSE;
	}
	return EINA_TRUE;
}

static Eina_Bool _opengl_compiled_program_new(
		Enesim_Renderer_OpenGL_Compiled_Program *cp,
		Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Data *rdata,
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
	/* use this program and setup each shader */
	glUseProgramObjectARB(cp->id);
	for (i = 0; i < p->num_shaders; i++)
	{
		Enesim_Renderer_OpenGL_Shader *shader;

		shader = p->shaders[i];
		if (!rdata->shader_setup(r, s, p, shader))
		{
			printf("Cannot setup the shader\n");
			i = p->num_shaders;
			goto error;
		}
	}

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
	Enesim_Renderer_OpenGL_Program *programs;
	Enesim_Renderer_OpenGL_Define_Geometry define_geometry;
	Enesim_Renderer_OpenGL_Shader_Setup shader_setup;
	Enesim_Buffer_OpenGL_Data *sdata;
	GLenum status;
	int num;
	int i;

	sdata = enesim_surface_backend_data_get(s);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	/* create the data in case does not exist */
	if (!rdata)
	{
		rdata = calloc(1, sizeof(Enesim_Renderer_OpenGL_Data));
		enesim_renderer_backend_data_set(r, ENESIM_BACKEND_OPENGL, rdata);
	}
	/* do the setup */
	if (!r->descriptor.opengl_setup) return EINA_FALSE;
	if (!r->descriptor.opengl_setup(r, states, s,
			&define_geometry,
			&shader_setup,
			&programs, &num, error))
		return EINA_FALSE;

	/* FIXME we need to know if we should create, compile and link the programs
	 * again or not .... */

	/* store the data returned by the setup */
	rdata->define_geometry = define_geometry;
	rdata->shader_setup = shader_setup;
	rdata->programs = programs;

	/* FIXME for now we just free them */
	for (i = 0; i < rdata->num_programs; i++)
	{
		Enesim_Renderer_OpenGL_Compiled_Program *cp;

		cp = &rdata->c_programs[i];
		_opengl_compiled_program_free(cp);
	}
	rdata->num_programs = num;
	rdata->c_programs = calloc(rdata->num_programs, sizeof(GLenum));
	for (i = 0; i < rdata->num_programs; i++)
	{
		Enesim_Renderer_OpenGL_Program *p;
		Enesim_Renderer_OpenGL_Compiled_Program *cp;

		p = &rdata->programs[i];
		cp = &rdata->c_programs[i];
		/* TODO check return value */
		_opengl_compiled_program_new(cp, r, s, rdata, p);
	}

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

	return EINA_TRUE;
}

void enesim_renderer_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void enesim_renderer_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s, Eina_Rectangle *area,
		int x, int y)
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
	/* get the compiled shader from the renderer backend data */
	glUseProgramObjectARB(rdata->program);
	/* create the geometry to render to */
	glEnable(GL_BLEND);

	enesim_surface_size_get(s, &width, &height);
	/* FIXME we should define a pre_render function
	 * to for example set/store some attributes
	 */
	_opengl_rop_set(r);
	/* now draw */
	if (rdata->define_geometry)
	{
		/* FIXME for now */
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();

		glOrtho(0, width, 0, height, -1, 1);
		rdata->define_geometry(r, area);
	}
#if 0
	else
	{
		_opengl_draw_own_geometry(rdata, area, width, height);
	}
#endif
	/* FIXME we should define a post_render function
	 * to put the state as it was before */
	glUseProgramObjectARB(0);
}

void enesim_renderer_opengl_init(void)
{
	/* maybe we should add a hash to match between kernel name and program */
	_opengl_extensions_setup();
	printf("Fragment shader %d Geometry shader %d\n", _fragment_shader_support, _geometry_shader_support);
}

void enesim_renderer_opengl_shutdown(void)
{

}

/* we need a way to destroy the renderer data
void enesim_renderer_opengl_delete(Enesim_Renderer *r)
{
	Enesim_Renderer_OpenGL_Data *rdata;
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	glDeleteFramebuffersEXT(1, &rdata->fbo);
}
*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

