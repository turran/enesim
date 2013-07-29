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
#include "enesim_renderer_shape.h"
#include "enesim_renderer_path.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "Enesim_OpenGL.h"
#include "nvpr_init.h"

#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_path_abstract_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Renderer_Path_Nvpr
{
} Enesim_Renderer_Path_Nvpr;

typedef struct _Enesim_Renderer_Path_Nvpr
{
	const Eina_List *commands;
	Eina_Bool changed;
	Eina_Bool generated;
} Enesim_Renderer_Path_Nvpr;

Enesim_Renderer_Path_Nvpr * _path_nvpr_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Nvpr *thiz;

	thiz = enesim_renderer_path_abstract_data_get(r);
	return thiz;
}

static void _path_nvpr_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED,
		const Eina_Rectangle *area, int w, int h)
{
#if 0
    /* Before rendering to a window with a stencil buffer, clear the stencil
    buffer to zero and the color buffer to black: */

    glClearStencil(0);
    glClearColor(0,0,0,0);
    glStencilMask(~0);
    glClear(GL_COLOR_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

    /* Use an orthographic path-to-clip-space transform to map the
    [0..500]x[0..400] range of the star's path coordinates to the [-1..1]
    clip space cube: */

    glMatrixLoadIdentityEXT(GL_PROJECTION);
    glMatrixLoadIdentityEXT(GL_MODELVIEW);
    glMatrixOrthoEXT(GL_MODELVIEW, 0, 500, 0, 400, -1, 1);

    if (filling) {

        /* Stencil the path: */

        glStencilFillPathNV(pathObj, GL_COUNT_UP_NV, 0x1F);

        /* The 0x1F mask means the counting uses modulo-32 arithmetic. In
        principle the star's path is simple enough (having a maximum winding
        number of 2) that modulo-4 arithmetic would be sufficient so the mask
        could be 0x3.  Or a mask of all 1's (~0) could be used to count with
        all available stencil bits.

        Now that the coverage of the star and the heart have been rasterized
        into the stencil buffer, cover the path with a non-zero fill style
        (indicated by the GL_NOTEQUAL stencil function with a zero reference
        value): */

        glEnable(GL_STENCIL_TEST);
        if (even_odd) {
            glStencilFunc(GL_NOTEQUAL, 0, 0x1);
        } else {
            glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
        }
        glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
        glColor3f(0,1,0); // green
        glCoverFillPathNV(pathObj, GL_BOUNDING_BOX_NV);

    }

    /* The result is a yellow star (with a filled center) to the left of
    a yellow heart.

    The GL_ZERO stencil operation ensures that any covered samples
    (meaning those with non-zero stencil values) are zero'ed when
    the path cover is rasterized. This allows subsequent paths to be
    rendered without clearing the stencil buffer again.

    A similar two-step rendering process can draw a white outline
    over the star and heart. */

     /* Now stencil the path's stroked coverage into the stencil buffer,
     setting the stencil to 0x1 for all stencil samples within the
     transformed path. */

    if (stroking) {

        glStencilStrokePathNV(pathObj, 0x1, ~0);

         /* Cover the path's stroked coverage (with a hull this time instead
         of a bounding box; the choice doesn't really matter here) while
         stencil testing that writes white to the color buffer and again
         zero the stencil buffer. */

        glColor3f(1,1,0); // yellow
        glCoverStrokePathNV(pathObj, GL_CONVEX_HULL_NV);

         /* In this example, constant color shading is used but the application
         can specify their own arbitrary shading and/or blending operations,
         whether with Cg compiled to fragment program assembly, GLSL, or
         fixed-function fragment processing.

         More complex path rendering is possible such as clipping one path to
         another arbitrary path.  This is because stencil testing (as well
         as depth testing, depth bound test, clip planes, and scissoring)
         can restrict path stenciling. */
    }
#endif
}

static void _path_nvpr_generate(Enesim_Renderer *r,
		Enesim_Renderer_Path_Nvpr *thiz)
{
#if 0
    static const GLubyte pathCommands[10] =
      { GL_MOVE_TO_NV, GL_LINE_TO_NV, GL_LINE_TO_NV, GL_LINE_TO_NV,
        GL_LINE_TO_NV, GL_CLOSE_PATH_NV,
        'M', 'C', 'C', 'Z' };  // character aliases
    static const GLshort pathCoords[12][2] =
      { {100, 180}, {40, 10}, {190, 120}, {10, 120}, {160, 10},
        {300,300}, {100,400}, {100,200}, {300,100},
        {500,200}, {500,400}, {300,300} };
    glPathCommandsNV(pathObj, 10, pathCommands, 24, GL_SHORT, pathCoords);
#endif
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _path_nvpr_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "path_nvpr";
}

static void _path_nvpr_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Nvpr *thiz;

	thiz = _path_nvpr_get(r);
	free(thiz);
}

static Eina_Bool _path_nvpr_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Path_Nvpr *thiz;

	thiz = _path_nvpr_get(r);

	*draw = _path_nvpr_opengl_draw;

	return EINA_TRUE;
}

static void _path_nvpr_gl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
}

static void _path_nvpr_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static Eina_Bool _path_nvpr_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Nvpr *thiz;

	thiz = _path_nvpr_get(r);
	return thiz->changed;
}

static void _path_nvpr_shape_features_get(Enesim_Renderer *r EINA_UNUSED, Enesim_Shape_Feature *features)
{
	*features = 0;
}

static void _path_nvpr_bounds(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Path_Nvpr *thiz;

	thiz = _path_nvpr_get(r);
}

static void _path_nvpr_destination_bounds(Enesim_Renderer *r,
		Eina_Rectangle *bounds)
{
}

static void _path_nvpr_commands_set(Enesim_Renderer *r, const Eina_List *commands)
{
	Enesim_Renderer_Path_Nvpr *thiz;

	thiz = _path_nvpr_get(r);
	thiz->commands = commands;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

static Enesim_Renderer_Path_Abstract_Descriptor _path_nvpr_descriptor = {
	/* .version =			*/ ENESIM_RENDERER_API,
	/* .name =			*/ _path_nvpr_name,
	/* .free =			*/ _path_nvpr_free,
	/* .bounds_get =			*/ _path_nvpr_bounds,
	/* .destination_bounds =	*/ _path_nvpr_destination_bounds,
	/* .features_get =		*/ _path_nvpr_features_get,
	/* .is_inside =			*/ NULL,
	/* .damage =			*/ NULL,
	/* .has_changed =		*/ _path_nvpr_has_changed,
	/* .sw_hints_get =		*/ NULL,
	/* .sw_setup =			*/ NULL,
	/* .sw_cleanup =		*/ NULL,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
	/* .opengl_initialize =		*/ NULL,
	/* .opengl_setup =		*/ _path_nvpr_opengl_setup,
	/* .opengl_cleanup =		*/ _path_nvpr_opengl_cleanup,
	/* .shape_features_get 		*/ _path_nvpr_shape_features_get,
	/* .commands_set = 		*/ _path_nvpr_commands_set,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_path_nvpr_new(void)
{
	Enesim_Renderer_Path_Nvpr *thiz;
	Enesim_Renderer *r;

	thiz = calloc(1, sizeof(Enesim_Renderer_Path_Nvpr));

	r = enesim_renderer_path_abstract_new(&_path_nvpr_descriptor, thiz);
	return r;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

