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

EAPI void enesim_opengl_clip_set(const Eina_Rectangle *area, int ww, int wh)
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

EAPI void enesim_opengl_clip_unset(void)
{
	glDisable(GL_SCISSOR_TEST);
}
