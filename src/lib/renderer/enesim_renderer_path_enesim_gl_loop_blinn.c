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
#include "enesim_renderer_path_enesim_private.h"

/* The idea is to tesselate the whole path without taking into account the
 * curves, just normalize theme to quadratic/cubic (i.e remove the arc).
 * once that's done we can use a shader and set the curve coefficients on the
 * vertex data, thus provide a simple in/out check to define the real curve
 * http://www.mdk.org.pl/2007/10/27/curvy-blues
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#if BUILD_OPENGL
static void _path_opengl_figure_draw(GLenum fbo,
		GLenum texture,
		Enesim_Renderer_Path_Enesim_OpenGL_Figure *gf,
		Enesim_Figure *f,
		Enesim_Color color,
		Enesim_Renderer *rel EINA_UNUSED,
		Enesim_Renderer_OpenGL_Data *rdata,
		Eina_Bool silhoutte,
		const Eina_Rectangle *area)
{

}

static void _path_opengl_fill_and_stroke_draw(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop, const Eina_Rectangle *area,
		int x, int y)
{

}

static void _path_opengl_fill_or_stroke_draw(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop, const Eina_Rectangle *area,
		int x, int y)
{

}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Eina_Bool enesim_renderer_path_enesim_gl_loop_blinn_initialize(
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***programs)
{
	*programs = NULL;
	*num_programs = 0;
	return EINA_TRUE;
}

Eina_Bool enesim_renderer_path_enesim_gl_loop_blinn_setup(
		Enesim_Renderer *r,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Shape_Draw_Mode dm;
	const Enesim_Renderer_Shape_State *css;

	css = enesim_renderer_shape_state_get(r);

	/* check what to draw, stroke, fill or stroke + fill */
	dm = css->current.draw_mode;
	/* fill + stroke */
	if (dm == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE_FILL)
	{
		*draw = _path_opengl_fill_and_stroke_draw;
	}
	else
	{
		*draw = _path_opengl_fill_or_stroke_draw;
	}

	return EINA_TRUE;
}
#endif

