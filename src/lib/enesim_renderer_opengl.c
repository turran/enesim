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

static Eina_Bool _shader_compile(GLenum program, Enesim_Renderer_OpenGL_Shader *shader, GLenum *sh_id)
{
	GLenum st;
	GLenum sh;
	int ok = 0;

	if (shader->type == ENESIM_SHADER_GEOMETRY)
	{
		st = GL_GEOMETRY_SHADER_EXT;
		glProgramParameteriEXT(program, GL_GEOMETRY_OUTPUT_TYPE_EXT, GL_TRIANGLE_STRIP);
		glProgramParameteriEXT(program, GL_GEOMETRY_INPUT_TYPE_EXT, GL_POINTS);
		glProgramParameteriEXT(program, GL_GEOMETRY_VERTICES_OUT_EXT, 10);
	}
	else if (shader->type == ENESIM_SHADER_FRAGMENT)
		st = GL_FRAGMENT_SHADER_ARB;
	else if (shader->type == ENESIM_SHADER_VERTEX)
		st = GL_VERTEX_SHADER_ARB;
	else
	{
		return EINA_FALSE;
	}
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
	glAttachObjectARB(program, sh);
	*sh_id = sh;
	return EINA_TRUE;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Eina_Bool enesim_renderer_opengl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		Enesim_Surface *s,
		Enesim_Error **error)
{
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_Renderer_OpenGL_Shader *shaders;
	Enesim_Buffer_OpenGL_Data *sdata;
	Eina_Bool has_vertex = EINA_FALSE;
	Eina_Bool has_geometry = EINA_FALSE;
	Eina_Bool ret;
	GLenum status;
	int ok = 0;
	int i;
	int num;

	sdata = enesim_surface_backend_data_get(s);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	if (!rdata)
	{
		rdata = calloc(1, sizeof(Enesim_Renderer_OpenGL_Data));
		enesim_renderer_backend_data_set(r, ENESIM_BACKEND_OPENGL, rdata);
	}
	if (!r->descriptor.opengl_setup) return EINA_FALSE;
	ret = r->descriptor.opengl_setup(r, state, s, &num, &shaders, error);
	if (rdata->num_shaders != num)
	{
		rdata->num_shaders = num;
		if (rdata->shaders)
			free(rdata->shaders);
		if (rdata->shader_types)
			free(rdata->shader_types);
		rdata->shaders = calloc(num, sizeof(GLenum));
		rdata->shader_types = calloc(num, sizeof(Enesim_Renderer_OpenGL_Shader_Type));
	}

	if (!ret) return EINA_FALSE;

	if (!rdata->program)
	{
		rdata->program = glCreateProgramObjectARB();
	}

	for (i = 0; i < num; i++)
	{
		Enesim_Renderer_OpenGL_Shader *shader;
		GLenum sh;

		shader = &shaders[i];

		if (!_shader_compile(rdata->program, shader, &sh))
			return EINA_FALSE;

		rdata->shaders[i] = sh;
		rdata->shader_types[i] = shader->type;;
		if (rdata->shader_types[i] == ENESIM_SHADER_GEOMETRY)
			has_geometry = EINA_TRUE;
		if (rdata->shader_types[i] == ENESIM_SHADER_VERTEX)
			has_vertex = EINA_TRUE;
	}
	if (has_geometry)
	{
		rdata->has_geometry = EINA_TRUE;
		if (!has_vertex)
		{
			GLenum sh;
			/* FIXME we could have this already compiled */
			_shader_compile(rdata->program, &_vertex_shader, &sh);
		}
	}

	/* link it */
	glLinkProgramARB(rdata->program);
	glGetObjectParameterivARB(rdata->program, GL_OBJECT_LINK_STATUS_ARB, &ok);
	if (!ok)
	{
		char log[PATH_MAX];
		glGetInfoLogARB(rdata->program, sizeof(log), NULL, log);
		printf("Failed to Link %s\n", log);
		/* FIXME destroy all the shaders */

		return EINA_FALSE;
	}

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

	glUseProgramObjectARB(rdata->program);
	if (r->descriptor.opengl_shader_setup)
	{
		if (!r->descriptor.opengl_shader_setup(r, s))
		{
			printf("Cannot setup the shader\n");
			return EINA_FALSE;
		}
	}

	return EINA_TRUE;
}

void enesim_renderer_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void enesim_renderer_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s, Eina_Rectangle *area,
		int x, int y, Enesim_Renderer_Flag flags)
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

	_opengl_rop_set(r);
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
			GLERR;
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

