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
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"

#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
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
		case ENESIM_BLEND:
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		break;

		case ENESIM_FILL:
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
void enesim_opengl_draw_area(GLenum fb, GLenum t, Eina_Rectangle *area,
		int w, int h, int tx EINA_UNUSED, int ty EINA_UNUSED)
{
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fb);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			GL_TEXTURE_2D, t, 0);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, 0, h, -1, 1);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glOrtho(-w, w, -h, h, -1, 1);

	/* trigger the area */
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
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, 0);
}

void enesim_opengl_init(void)
{
	static int _init = 0;

	if (++_init != 1) return;
	glewInit();
}/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
#if 0
EAPI void enesim_renderer_opengl_compile_program(Enesim_Renderer *r ...)
{

}
#endif


