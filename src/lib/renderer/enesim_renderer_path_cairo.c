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
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_path.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_path_private.h"
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
#define ENESIM_RENDERER_PATH_CAIRO(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Path_Cairo,					\
		enesim_renderer_path_cairo_descriptor_get())

typedef struct _Enesim_Renderer_Path_Cairo
{
	Enesim_Renderer_Path_Abstract parent;
	Enesim_Path_Normalizer *normalizer;

	const Eina_List *commands;
	cairo_t *cairo_rec;
	cairo_t *cairo_draw;
	cairo_surface_t *recording;
	cairo_surface_t *surface;
	unsigned char *data;
	int stride;
	Enesim_Rectangle bounds;
	Eina_Rectangle destination_bounds;
	Eina_Bool changed;
	Eina_Bool generated;
	Eina_Bool drawing;
} Enesim_Renderer_Path_Cairo;

typedef struct _Enesim_Renderer_Path_Cairo_Class
{
	Enesim_Renderer_Path_Abstract_Class parent;
} Enesim_Renderer_Path_Cairo_Class;

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
	cairo_t *cairo;

	if (thiz->drawing)
		cairo = thiz->cairo_draw;
	else
		cairo = thiz->cairo_rec;

	sstate = enesim_renderer_shape_state_get(rend);
	rstate = enesim_renderer_state_get(rend);

	/* set the matrix */
	matrix.xx = rstate->current.transformation.xx;
	matrix.xy = rstate->current.transformation.xy;
	matrix.yx = rstate->current.transformation.yx;
	matrix.yy = rstate->current.transformation.yy;
	matrix.x0 = rstate->current.transformation.xz;
	matrix.y0 = rstate->current.transformation.zy;
	cairo_set_matrix(cairo, &matrix);
	/* we need to translate by the bounds, to align to the temporary
	 * surface
	 */
	if (thiz->drawing)
	{
		cairo_translate(cairo, -thiz->bounds.x, -thiz->bounds.y);
	}
	cairo_new_path(cairo);
	EINA_LIST_FOREACH(thiz->commands, l, cmd)
	{
		enesim_path_normalizer_normalize(thiz->normalizer, cmd);
	}

	if (sstate->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_FILL)
	{
		double a, r, g, b;

		_path_cairo_colors_get(sstate->current.fill.color, &a, &r, &g, &b);
		cairo_set_source_rgba(cairo, r, g, b, a);
		if (sstate->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE)
			cairo_fill_preserve(cairo);
		else
			cairo_fill(cairo);
	}

	if (sstate->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE)
	{
		double a, r, g, b;

		_path_cairo_colors_get(sstate->current.stroke.color, &a, &r, &g, &b);
		cairo_set_source_rgba(cairo, r, g, b, a);
		cairo_set_line_width(cairo, sstate->current.stroke.weight);
		cairo_stroke(cairo);
	}
	thiz->generated = EINA_TRUE;
}

static void _path_cairo_draw(Enesim_Renderer *r, int x, int y, unsigned int len,
		void *ddata)
{
	Enesim_Renderer_Path_Cairo *thiz;
	unsigned char *src;

	thiz = ENESIM_RENDERER_PATH_CAIRO(r);
	/* translate to our own origin */
	x -= thiz->destination_bounds.x;
	y -= thiz->destination_bounds.y;
	//printf("drawing at %d,%d -> %d\n", x, y, len);
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
	cairo_t *cairo;
	double x, y;

	if (thiz->drawing)
		cairo = thiz->cairo_draw;
	else
		cairo = thiz->cairo_rec;
	enesim_path_command_move_to_values_to(move_to, &x, &y);
	//printf("M %g %g", x, y);
	cairo_move_to(cairo, x, y);
}

static void _path_cairo_line_to(Enesim_Path_Command_Line_To *line_to,
		void *data)
{
	Enesim_Renderer_Path_Cairo *thiz = data;
	cairo_t *cairo;
	double x, y;

	if (thiz->drawing)
		cairo = thiz->cairo_draw;
	else
		cairo = thiz->cairo_rec;
	enesim_path_command_line_to_values_to(line_to, &x, &y);
	//printf(" L %g %g", x, y);
	cairo_line_to(cairo, x, y);
}

static void _path_cairo_cubic_to(Enesim_Path_Command_Cubic_To *cubic_to,
		void *data)
{
	Enesim_Renderer_Path_Cairo *thiz = data;
	cairo_t *cairo;
	double x, y, ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1;

	if (thiz->drawing)
		cairo = thiz->cairo_draw;
	else
		cairo = thiz->cairo_rec;
	enesim_path_command_cubic_to_values_to(cubic_to, &x, &y, &ctrl_x0, &ctrl_y0, &ctrl_x1, &ctrl_y1);
	//printf(" C %g %g %g %g %g %g", ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1, x, y);
	cairo_curve_to(cairo, ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1, x, y);
}

static void _path_cairo_close(Enesim_Path_Command_Close *close,
		void *data)
{
	Enesim_Renderer_Path_Cairo *thiz = data;
	cairo_t *cairo;

	if (thiz->drawing)
		cairo = thiz->cairo_draw;
	else
		cairo = thiz->cairo_rec;
	//printf(" Z\n");
	cairo_close_path(cairo);
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

	thiz = ENESIM_RENDERER_PATH_CAIRO(r);

	if (thiz->changed && !thiz->generated)
	{
		thiz->drawing = EINA_FALSE;
		_path_cairo_generate(r, thiz);
		cairo_recording_surface_ink_extents(thiz->recording,
				&thiz->bounds.x, &thiz->bounds.y,
				&thiz->bounds.w, &thiz->bounds.h);
		enesim_rectangle_normalize(&thiz->bounds, &thiz->destination_bounds);
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
		//printf("surface of %d %d\n", rwidth, rheight);
		thiz->cairo_draw = cairo_create(thiz->surface);
	}

	if (!thiz->surface)
	{
		ENESIM_RENDERER_LOG(r, l, "No surface created for size %d %d", rwidth, rheight);
		return EINA_FALSE;
	}

	thiz->data = cairo_image_surface_get_data(thiz->surface);
	thiz->stride = cairo_image_surface_get_stride(thiz->surface);

	/* finally draw */
	thiz->drawing = EINA_TRUE;
	_path_cairo_generate(r, thiz);

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

	thiz = ENESIM_RENDERER_PATH_CAIRO(r);
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

	thiz = ENESIM_RENDERER_PATH_CAIRO(r);
	if (thiz->changed && !thiz->generated)
	{
		thiz->drawing = EINA_FALSE;
		_path_cairo_generate(r, thiz);
		cairo_recording_surface_ink_extents(thiz->recording,
				&thiz->bounds.x, &thiz->bounds.y,
				&thiz->bounds.w, &thiz->bounds.h);
		enesim_rectangle_normalize(&thiz->bounds, &thiz->destination_bounds);
	}
	//printf("bounds of %" ENESIM_RECTANGLE_FORMAT "\n", ENESIM_RECTANGLE_ARGS(&thiz->bounds));
	*bounds = thiz->bounds;
}

static void _path_cairo_commands_set(Enesim_Renderer *r, const Eina_List *commands)
{
	Enesim_Renderer_Path_Cairo *thiz;

	thiz = ENESIM_RENDERER_PATH_CAIRO(r);
	thiz->commands = commands;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_PATH_ABSTRACT_DESCRIPTOR,
		Enesim_Renderer_Path_Cairo, Enesim_Renderer_Path_Cairo_Class,
		enesim_renderer_path_cairo);

static void _enesim_renderer_path_cairo_class_init(void *k)
{
	Enesim_Renderer_Path_Abstract_Class *klass;
	Enesim_Renderer_Shape_Class *s_klass;
	Enesim_Renderer_Class *r_klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _path_cairo_name;
	r_klass->bounds_get = _path_cairo_bounds_get;
	r_klass->features_get = _path_cairo_features_get;
	r_klass->has_changed = _path_cairo_has_changed;
	r_klass->sw_hints_get = _path_cairo_sw_hints;

	s_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	s_klass->features_get = _path_cairo_shape_features_get;
	s_klass->sw_setup = _path_cairo_sw_setup;
	s_klass->sw_cleanup = _path_cairo_sw_cleanup;

	klass = ENESIM_RENDERER_PATH_ABSTRACT_CLASS(k);
	klass->commands_set = _path_cairo_commands_set;
}

static void _enesim_renderer_path_cairo_instance_init(void *o)
{
	Enesim_Renderer_Path_Cairo *thiz;

	thiz = ENESIM_RENDERER_PATH_CAIRO(o);
	thiz->recording = cairo_recording_surface_create(CAIRO_CONTENT_COLOR_ALPHA, NULL);
	thiz->cairo_rec = cairo_create(thiz->recording);
	thiz->normalizer = enesim_path_normalizer_path_new(&_normalizer_descriptor, thiz);
}

static void _enesim_renderer_path_cairo_instance_deinit(void *o)
{
	Enesim_Renderer_Path_Cairo *thiz;

	thiz = ENESIM_RENDERER_PATH_CAIRO(o);
	cairo_surface_destroy(thiz->recording);
	if (thiz->surface)
		cairo_surface_destroy(thiz->surface);
	cairo_destroy(thiz->cairo_rec);
	if (thiz->cairo_draw)
		cairo_destroy(thiz->cairo_draw);
	enesim_path_normalizer_free(thiz->normalizer);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_path_cairo_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_path_cairo);
	return r;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
