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

#include "Enesim_OpenGL.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
#if 0
EAPI void enesim_renderer_opengl_compile_program(Enesim_Renderer *r ...)
{

}
#endif

EAPI GLenum enesim_opengl_texture_new(int width, int height)
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

EAPI void enesim_opengl_texture_free(GLenum id)
{
	glDeleteTextures(1, &id);
}

EAPI void enesim_opengl_compiled_program_set(Enesim_Renderer_OpenGL_Compiled_Program *cp)
{
	if (!cp) glUseProgramObjectARB(0);
	else glUseProgramObjectARB(cp->id);
}

EAPI void enesim_opengl_rop_set(Enesim_Rop rop)
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

EAPI void enesim_opengl_clip_unset(void)
{
	glDisable(GL_SCISSOR_TEST);
}

EAPI void enesim_opengl_clip_set(const Eina_Rectangle *area, int ww, int hh)
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

	x = ww - area->x;
	y = hh - area->y;
	w = area->w;
	h = area->h;

	glEnable(GL_SCISSOR_TEST);
	glScissor(x, y, w, h);
}


