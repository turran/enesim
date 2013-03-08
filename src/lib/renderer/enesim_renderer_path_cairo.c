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
#include "enesim_path.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_path.h"

#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_path_abstract_private.h"
#include "enesim_path_normalizer_private.h"

#include <cairo.h>
/* TODO
 * - Add cairo versions of every command
 * - Check what cairo version handles correctly the recording surface
 * -
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Renderer_Path_Cairo
{
	Enesim_Path_Normalizer *normalizer;

	const Eina_List *commands;
	cairo_t *cairo;
	cairo_surface_t *recording;
	cairo_surface_t *surface;
	unsigned char *data;
	int stride;
	Enesim_Rectangle bounds;
	Eina_Bool changed;
	Eina_Bool generated;
} Enesim_Renderer_Path_Cairo;

Enesim_Renderer_Path_Cairo * _path_cairo_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Cairo *thiz;

	thiz = enesim_renderer_path_abstract_data_get(r);
	return thiz;
}

static void _path_cairo_colors_get(Enesim_Color color,
		double *a, double *r, double *g, double *b)
{
	*a = argb8888_alpha_get(color) / 255.0;
	*r = argb8888_red_get(color) / 255.0;
	*g = argb8888_green_get(color) / 255.0;
	*b = argb8888_blue_get(color) / 255.0;
}

static void _path_cairo_generate(Enesim_Renderer *rend,
		Enesim_Renderer_Path_Cairo *thiz)
{
	Enesim_Path_Command *cmd;
	const Enesim_Renderer_Shape_State *sstate;
	const Enesim_Renderer_State *rstate;
	const Eina_List *l;
	cairo_matrix_t matrix;

	sstate = enesim_renderer_shape_state_get(rend);
	rstate = enesim_renderer_state_get(rend);

	cairo_new_path(thiz->cairo);
	EINA_LIST_FOREACH(thiz->commands, l, cmd)
	{
		enesim_path_normalizer_normalize(thiz->normalizer, cmd);
	}
	/* set the matrix */
	matrix.xx = rstate->current.transformation.xx;
	matrix.xy = rstate->current.transformation.xy;
	matrix.yx = rstate->current.transformation.yx;
	matrix.yy = rstate->current.transformation.yy;
	matrix.x0 = rstate->current.transformation.xz;
	matrix.y0 = rstate->current.transformation.zy;
	cairo_set_matrix(thiz->cairo, &matrix);

	if (sstate->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_FILL)
	{
		double a, r, g, b;

		_path_cairo_colors_get(sstate->current.fill.color, &a, &r, &g, &b);
		cairo_set_source_rgba(thiz->cairo, r, g, b, a);
		if (sstate->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE)
			cairo_fill_preserve(thiz->cairo);
		else
			cairo_fill(thiz->cairo);
	}

	if (sstate->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE)
	{
		double a, r, g, b;

		_path_cairo_colors_get(sstate->current.stroke.color, &a, &r, &g, &b);
		cairo_set_source_rgba(thiz->cairo, r, g, b, a);
		cairo_set_line_width (thiz->cairo, sstate->current.stroke.weight);
		cairo_stroke(thiz->cairo);
	}
	thiz->generated = EINA_TRUE;
}

static void _path_cairo_draw(Enesim_Renderer *r, int x, int y, unsigned int len,
		void *ddata)
{
	Enesim_Renderer_Path_Cairo *thiz;
	unsigned char *src;

	thiz = _path_cairo_get(r);
	/* just copy the pixels from the cairo rendered surface to the destination */
	src = (unsigned char *)argb8888_at((uint32_t *)thiz->data, thiz->stride, x, y);
	memcpy(ddata, src, len * 4);
}
/*----------------------------------------------------------------------------*
 *                       The path normalizer interface                        *
 *----------------------------------------------------------------------------*/
static void _path_cairo_move_to(Enesim_Path_Command_Move_To *move_to,
		void *data)
{
	Enesim_Renderer_Path_Cairo *thiz = data;
	double x, y;

	enesim_path_command_move_to_values_to(move_to, &x, &y);
	cairo_move_to(thiz->cairo, x, y);
}

static void _path_cairo_line_to(Enesim_Path_Command_Line_To *line_to,
		void *data)
{
	Enesim_Renderer_Path_Cairo *thiz = data;
	double x, y;

	enesim_path_command_line_to_values_to(line_to, &x, &y);
	cairo_line_to(thiz->cairo, x, y);
}

static void _path_cairo_cubic_to(Enesim_Path_Command_Cubic_To *cubic_to,
		void *data)
{
	Enesim_Renderer_Path_Cairo *thiz = data;
	double x, y, ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1;

	enesim_path_command_cubic_to_values_to(cubic_to, &x, &y, &ctrl_x0, &ctrl_y0, &ctrl_x1, &ctrl_y1);
	cairo_curve_to(thiz->cairo, x, y, ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1);
}

static void _path_cairo_close(Enesim_Path_Command_Close *close,
		void *data)
{
	Enesim_Renderer_Path_Cairo *thiz = data;
	cairo_close_path(thiz->cairo);
}

static Enesim_Path_Normalizer_Path_Descriptor _normalizer_descriptor = {
	/* .move_to = 	*/ _path_cairo_move_to,
	/* .line_to = 	*/ _path_cairo_line_to,
	/* .cubic_to = 	*/ _path_cairo_cubic_to,
	/* .close = 	*/ _path_cairo_close,
};
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _path_cairo_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "path_cairo";
}

static void _path_cairo_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Cairo *thiz;

	thiz = _path_cairo_get(r);
	cairo_surface_destroy(thiz->recording);
	if (thiz->surface)
		cairo_surface_destroy(thiz->surface);
	cairo_destroy(thiz->cairo);
	free(thiz);
}

static Eina_Bool _path_cairo_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *draw, Enesim_Log **l)
{
	Enesim_Renderer_Path_Cairo *thiz;
	cairo_t *cr;
	int width = 0;
	int height = 0;
	int rwidth;
	int rheight;

	thiz = _path_cairo_get(r);

	if (thiz->changed && !thiz->generated)
	{
		_path_cairo_generate(r, thiz);
		cairo_recording_surface_ink_extents(thiz->recording,
				&thiz->bounds.x, &thiz->bounds.y,
				&thiz->bounds.w, &thiz->bounds.h);
	}

	if (thiz->surface)
	{
		width = cairo_image_surface_get_width(thiz->surface);
		height = cairo_image_surface_get_height(thiz->surface);
	}

	rwidth = ceil(thiz->bounds.w);
	rheight = ceil(thiz->bounds.h);

	if (width != rwidth || height != rheight)
	{
		if (thiz->surface)
			cairo_surface_destroy(thiz->surface);
		thiz->surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, rwidth, rheight);
	}

	if (!thiz->surface)
	{
		ENESIM_RENDERER_LOG(r, l, "No surface created for size %d %d", rwidth, rheight);
		return EINA_FALSE;
	}

	thiz->data = cairo_image_surface_get_data(thiz->surface);
	thiz->stride = cairo_image_surface_get_stride(thiz->surface);

	/* finally draw */
	cr = cairo_create(thiz->surface);
#if 0
	cairo_set_source_surface(cr, thiz->recording, 0, 0);
	cairo_paint(cr);
	cairo_destroy(cr);
#else
	thiz->cairo = cr;
	_path_cairo_generate(r, thiz);
#endif

	*draw = _path_cairo_draw;

	return EINA_TRUE;
}

static void _path_cairo_sw_cleanup(Enesim_Renderer *r EINA_UNUSED, Enesim_Surface *s EINA_UNUSED)
{
}

static void _path_cairo_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static void _path_cairo_sw_hints(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_COLORIZE;
}

static Eina_Bool _path_cairo_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Cairo *thiz;

	thiz = _path_cairo_get(r);
	return thiz->changed;
}

static void _path_cairo_shape_features_get(Enesim_Renderer *r EINA_UNUSED, Enesim_Shape_Feature *features)
{
	*features = 0;
}

static void _path_cairo_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Path_Cairo *thiz;

	thiz = _path_cairo_get(r);
	if (thiz->changed && !thiz->generated)
	{
		_path_cairo_generate(r, thiz);
		cairo_recording_surface_ink_extents(thiz->recording,
				&thiz->bounds.x, &thiz->bounds.y,
				&thiz->bounds.w, &thiz->bounds.h);
	}
	*bounds = thiz->bounds;
}

static void _path_cairo_commands_set(Enesim_Renderer *r, const Eina_List *commands)
{
	Enesim_Renderer_Path_Cairo *thiz;

	thiz = _path_cairo_get(r);
	thiz->commands = commands;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

static Enesim_Renderer_Path_Abstract_Descriptor _path_cairo_descriptor = {
	/* .version =			*/ ENESIM_RENDERER_API,
	/* .name =			*/ _path_cairo_name,
	/* .free =			*/ _path_cairo_free,
	/* .bounds_get =		*/ _path_cairo_bounds_get,
	/* .features_get =		*/ _path_cairo_features_get,
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
	/* .shape_features_get =	*/ _path_cairo_shape_features_get,
	/* .commands_set = 		*/ _path_cairo_commands_set,
};

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_path_cairo_new(void)
{
	Enesim_Renderer_Path_Cairo *thiz;
	Enesim_Renderer *r;

	thiz = calloc(1, sizeof(Enesim_Renderer_Path_Cairo));
	thiz->recording = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, NULL);
	thiz->cairo = cairo_create(thiz->recording);
	thiz->normalizer = enesim_path_normalizer_path_new(&_normalizer_descriptor, thiz);

	r = enesim_renderer_path_abstract_new(&_path_cairo_descriptor, thiz);
	return r;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
