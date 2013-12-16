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

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"

#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"

#include "enesim_surface_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
GLenum enesim_opengl_texture_new(int width, int height)
{
	GLenum id;

	glGenTextures(1, &id);
	glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glBindTexture(GL_TEXTURE_2D, 0);

	return id;
}

void enesim_opengl_texture_free(GLenum id)
{
	glDeleteTextures(1, &id);
}

void enesim_opengl_compiled_program_set(Enesim_OpenGL_Compiled_Program *cp)
{
	if (!cp) glUseProgramObjectARB(0);
	else glUseProgramObjectARB(cp->id);
}

void enesim_opengl_rop_set(Enesim_Rop rop)
{
	glBlendEquation(GL_FUNC_ADD);
	switch (rop)
	{
		case ENESIM_ROP_BLEND:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;

		case ENESIM_ROP_FILL:
		glBlendFunc(GL_ONE, GL_ZERO);
		break;

		default:
		break;
	}
}

void enesim_opengl_clip_unset(void)
{
	glDisable(GL_SCISSOR_TEST);
}

void enesim_opengl_clip_set(const Eina_Rectangle *area, int ww EINA_UNUSED, int hh)
{
	GLint x;
	GLint y;
	GLsizei w;
	GLsizei h;

	if (!area)
	{
		enesim_opengl_clip_unset();
		return;
	}

	x = area->x;
	y = hh - (area->y + area->h);
	w = area->w;
	h = area->h;

	glEnable(GL_SCISSOR_TEST);
	glScissor(x, y, w, h);
}

/* area is the destination of area of a viewport of size WxH */
void enesim_opengl_draw_area(const Eina_Rectangle *area)
{
	glBegin(GL_QUADS);
		glTexCoord2d(area->x, area->y);
		glVertex2d(area->x, area->y);

		glTexCoord2d(area->x + area->w, area->y);
		glVertex2d(area->x + area->w, area->y);

		glTexCoord2d(area->x + area->w, area->y + area->h);
		glVertex2d(area->x + area->w, area->y + area->h);

		glTexCoord2d(area->x, area->y + area->h);
		glVertex2d(area->x, area->y + area->h);
	glEnd();
}

Eina_Bool enesim_opengl_target_surface_set(Enesim_Surface *s, int x, int y)
{
	if (s)
	{
		Enesim_Buffer_OpenGL_Data *backend_data;
		Enesim_Backend backend;
		int w, h;

		backend = enesim_surface_backend_get(s);
		if (backend != ENESIM_BACKEND_OPENGL)
			return EINA_FALSE;
 
		enesim_surface_size_get(s, &w, &h);
		backend_data = enesim_surface_backend_data_get(s);

		/* The viewport of the whole framebuffer should be
		 * of size of the surface
		 */
		glViewport(0, 0, w, h);
		/* sets the texture and projection matrix */
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(-x, w + x, h + y, -y, -1, 1);

		/* sets the fbo as the destination buffer */
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, backend_data->fbo);
		glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			GL_TEXTURE_2D, backend_data->textures[0], 0);
	}
	else
	{
		glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
	}
	return EINA_TRUE;
}

Eina_Bool enesim_opengl_source_surface_set(Enesim_Surface *s)
{
	int w, h;

	enesim_surface_size_get(s, &w, &h);
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glOrtho(-w, w, -h, h, -1, 1);
	return EINA_TRUE;
}

void enesim_opengl_buffer_data_free(Enesim_Buffer_OpenGL_Data *data)
{
	if (data->textures)
	{
		glDeleteTextures(data->num_textures, data->textures);
		free(data->textures);
	}
	if (data->fbo)
	{
		glDeleteFramebuffersEXT(1, &data->fbo);
	}
	free(data);
}

void enesim_opengl_init(void)
{
	static int _init = 0;
	glewExperimental = GL_TRUE;

	if (++_init != 1) return;
	glewInit();
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

