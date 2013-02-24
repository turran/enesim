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
#include "enesim_renderer_shape.h"

#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"

#include <cairo.h>
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Renderer_Path_Cairo
{
	cairo_t *cairo;
	cairo_surface_t *surface;
	Eina_Bool generated :1;
} Enesim_Renderer_Path_Cairo;

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _path_cairo_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "path_cairo";
}

static void _path_cairo_free(Enesim_Renderer *r)
{
}

static Eina_Bool _path_cairo_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *draw, Enesim_Error **error)
{
	/* iterate over the list of commands and generate the cairo commands */
	return EINA_TRUE;
}

static void _path_cairo_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
}

static void _path_cairo_flags(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_ARGB8888;
}

static void _path_cairo_sw_hints(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_COLORIZE;
}

static Eina_Bool _path_cairo_has_changed(Enesim_Renderer *r)
{
	return EINA_FALSE;
}

static void _path_cairo_feature_get(Enesim_Renderer *r EINA_UNUSED, Enesim_Shape_Feature *features)
{
	*features = ENESIM_SHAPE_FLAG_FILL_RENDERER | ENESIM_SHAPE_FLAG_STROKE_RENDERER;
}

static void _path_cairo_bounds(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
}

static void _path_cairo_destination_bounds(Enesim_Renderer *r,
		Eina_Rectangle *bounds)
{
}

static Enesim_Renderer_Shape_Descriptor _path_cairo_descriptor = {
	/* .version =			*/ ENESIM_RENDERER_API,
	/* .name =			*/ _path_cairo_name,
	/* .free =			*/ _path_cairo_free,
	/* .bounds =			*/ _path_cairo_bounds,
	/* .destination_bounds =	*/ _path_cairo_destination_bounds,
	/* .flags =			*/ _path_cairo_flags,
	/* .is_inside =			*/ NULL,
	/* .damage =			*/ NULL,
	/* .has_changed =		*/ _path_cairo_has_changed,
	/* .sw_hints_get =		*/ _path_cairo_sw_hints,
	/* .sw_setup =			*/ _path_cairo_sw_setup,
	/* .sw_cleanup =		*/ _path_cairo_sw_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
	/* .opengl_initialize =		*/ NULL,
	/* .opengl_setup =		*/ NULL,
	/* .opengl_cleanup =		*/ NULL,
	/* .feature_get =		*/ _path_cairo_feature_get
};

static Enesim_Renderer * _path_cairo_new(void)
{

}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_renderer_path_cairo_init(void)
{
#if 0
	enesim_renderer_path_descriptor_register(_path_cairo_new,
		ENESIM_PRIORITY_MARGINAL);
#endif
}

void enesim_renderer_path_cairo_shutdown(void)
{
#if 0
	enesim_renderer_path_descriptor_unregister(_path_cairo_new);
#endif
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
