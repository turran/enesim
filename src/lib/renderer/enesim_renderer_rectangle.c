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
#include "enesim_renderer_rectangle.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#ifdef BUILD_OPENGL
#include "Enesim_OpenGL.h"
#endif

#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_shape_path_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_RECTANGLE(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Rectangle,					\
		enesim_renderer_rectangle_descriptor_get())

typedef struct _Enesim_Renderer_Rectangle_State
{
	double width;
	double height;
	double x;
	double y;
	struct {
		double rx, ry;
		Eina_Bool tl : 1;
		Eina_Bool tr : 1;
		Eina_Bool bl : 1;
		Eina_Bool br : 1;
	} corner;
} Enesim_Renderer_Rectangle_State;

typedef struct _Enesim_Renderer_Rectangle
{
	Enesim_Renderer_Shape_Path parent;
	/* public properties */
	Enesim_Renderer_Rectangle_State current;
	Enesim_Renderer_Rectangle_State past;
	/* internal state */
	Eina_Bool changed : 1;
	Eina_Bool generated : 1;
} Enesim_Renderer_Rectangle;

typedef struct _Enesim_Renderer_Rectangle_Class {
	Enesim_Renderer_Shape_Path_Class parent;
} Enesim_Renderer_Rectangle_Class;

static Eina_Bool _rectangle_properties_have_changed(Enesim_Renderer_Rectangle *thiz)
{
	if (!thiz->changed) return EINA_FALSE;

	/* the width */
	if (thiz->current.width != thiz->past.width)
		return EINA_TRUE;
	/* the height */
	if (thiz->current.height != thiz->past.height)
		return EINA_TRUE;
	/* the x */
	if (thiz->current.x != thiz->past.x)
		return EINA_TRUE;
	/* the y */
	if (thiz->current.y != thiz->past.y)
		return EINA_TRUE;
	/* the corners */
	if ((thiz->current.corner.tl != thiz->past.corner.tl) || (thiz->current.corner.tr != thiz->past.corner.tr) ||
	     (thiz->current.corner.bl != thiz->past.corner.bl) || (thiz->current.corner.br != thiz->past.corner.br))
		return EINA_TRUE;
	/* the corner radii */
	if (thiz->current.corner.rx != thiz->past.corner.rx)
		return EINA_TRUE;
	if (thiz->current.corner.ry != thiz->past.corner.ry)
		return EINA_TRUE;

	return EINA_FALSE;
}

static void _rectangle_path_propagate(Enesim_Renderer_Rectangle *thiz,
		Enesim_Renderer *path, 
		double x, double y, double w, double h, double rx, double ry)
{
	enesim_renderer_path_command_clear(path);
	if (thiz->current.corner.tl && (rx > 0.0) && (ry > 0.0))
	{
		enesim_renderer_path_move_to(path, x, y + ry);
		enesim_renderer_path_arc_to(path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x + rx, y);
	}
	else
	{
		enesim_renderer_path_move_to(path, x, y);
	}
	if (thiz->current.corner.tr && (rx > 0.0) && (ry > 0.0))
	{
		enesim_renderer_path_line_to(path, x + w - rx, y);
		enesim_renderer_path_arc_to(path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x + w, y + ry);
	}
	else
	{
		enesim_renderer_path_line_to(path, x + w, y);
	}
	if (thiz->current.corner.br && (rx > 0.0) && (ry > 0.0))
	{
		enesim_renderer_path_line_to(path, x + w, y + h - ry);
		enesim_renderer_path_arc_to(path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x + w - rx, y + h);
	}
	else
	{
		enesim_renderer_path_line_to(path, x + w, y + h);
	}
	if (thiz->current.corner.bl && (rx > 0.0) && (ry > 0.0))
	{
		enesim_renderer_path_line_to(path, x + rx, y + h);
		enesim_renderer_path_arc_to(path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x, y + h - ry);
	}
	else
	{
		enesim_renderer_path_line_to(path, x, y + h);
	}
	enesim_renderer_path_close(path, EINA_TRUE);
}

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _rectangle_base_name_get(Enesim_Renderer *r EINA_UNUSED)
{
	return "rectangle";
}

static Eina_Bool _rectangle_setup(Enesim_Renderer *r, Enesim_Renderer *path,
		Enesim_Log **l)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	if (_rectangle_properties_have_changed(thiz) && !thiz->generated)
	{
		Enesim_Shape_Draw_Mode draw_mode;
		double rx, ry;
		double x;
		double y;
		double w;
		double h;

		w = thiz->current.width;
		h = thiz->current.height;
		if ((w < 1) || (h < 1))
		{
			ENESIM_RENDERER_LOG(r, l, "Invalid size %g %g", w, h);
			return EINA_FALSE;
		}

		x = thiz->current.x;
		y = thiz->current.y;

		rx = thiz->current.corner.rx;
		if (rx > (w / 2.0))
			rx = w / 2.0;
		ry = thiz->current.corner.ry;
		if (ry > (h / 2.0))
			ry = h / 2.0;

		enesim_renderer_shape_draw_mode_get(r, &draw_mode);

		if (draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE)
		{
			Enesim_Shape_Stroke_Location location;
			double sw;

			enesim_renderer_shape_stroke_location_get(r, &location);
			enesim_renderer_shape_stroke_weight_get(r, &sw);
			switch (location)
			{
				case ENESIM_SHAPE_STROKE_OUTSIDE:
				x -= sw / 2.0;
				y -= sw / 2.0;
				w += sw;
				h += sw;
				rx += sw / 2.0;
				ry += sw / 2.0;
				break;

				case ENESIM_SHAPE_STROKE_INSIDE:
				x += sw / 2.0;
				y += sw / 2.0;
				w -= sw;
				h -= sw;
				rx -= sw / 2.0;
				ry -= sw / 2.0;
				break;

				default:
				break;
			}
		}
		/* generate the four arcs */
		_rectangle_path_propagate(thiz, path, x, y, w, h, rx, ry);
		thiz->generated = EINA_TRUE;
	}
	return EINA_TRUE;
}

static void _rectangle_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

static void _rectangle_shape_features_get(Enesim_Renderer *r EINA_UNUSED, Enesim_Shape_Feature *features)
{
	*features = ENESIM_SHAPE_FLAG_FILL_RENDERER | ENESIM_SHAPE_FLAG_STROKE_RENDERER | ENESIM_SHAPE_FLAG_STROKE_LOCATION;
}

static void _rectangle_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Rectangle *thiz;
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Matrix_Type type;
	double x, y, w, h;

	thiz = ENESIM_RENDERER_RECTANGLE(r);

	/* first scale */
	x = thiz->current.x;
	y = thiz->current.y;
	w = thiz->current.width;
	h = thiz->current.height;
	/* for the stroke location get the real width */
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);
	if (draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE)
	{
		Enesim_Shape_Stroke_Location location;
		double sw;

		enesim_renderer_shape_stroke_weight_get(r, &sw);
		enesim_renderer_shape_stroke_location_get(r, &location);
		switch (location)
		{
			case ENESIM_SHAPE_STROKE_CENTER:
			x -= sw / 2.0;
			y -= sw / 2.0;
			w += sw;
			h += sw;
			break;

			case ENESIM_SHAPE_STROKE_OUTSIDE:
			x -= sw;
			y -= sw;
			w += sw * 2.0;
			h += sw * 2.0;
			break;

			default:
			break;
		}
	}

	bounds->x = x;
	bounds->y = y;
	bounds->w = w;
	bounds->h = h;

	/* apply the geometry transformation */
	enesim_renderer_transformation_type_get(r, &type);
	if (type != ENESIM_MATRIX_IDENTITY)
	{
		Enesim_Matrix m;
		Enesim_Quad q;

		enesim_renderer_transformation_get(r, &m);
		enesim_matrix_rectangle_transform(&m, bounds, &q);
		enesim_quad_rectangle_to(&q, bounds);
	}
}

static Eina_Bool _rectangle_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	return _rectangle_properties_have_changed(thiz);
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_SHAPE_PATH_DESCRIPTOR,
		Enesim_Renderer_Rectangle, Enesim_Renderer_Rectangle_Class,
		enesim_renderer_rectangle);

static void _enesim_renderer_rectangle_class_init(void *k)
{
	Enesim_Renderer_Class *r_klass;
	Enesim_Renderer_Shape_Class *s_klass;
	Enesim_Renderer_Shape_Path_Class *klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _rectangle_base_name_get;
	r_klass->bounds_get = _rectangle_bounds_get;

	s_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	s_klass->features_get = _rectangle_shape_features_get;

	klass = ENESIM_RENDERER_SHAPE_PATH_CLASS(k);
	klass->has_changed = _rectangle_has_changed;
	klass->setup = _rectangle_setup;
	klass->cleanup = _rectangle_cleanup;
}

static void _enesim_renderer_rectangle_instance_init(void *o)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(o);
	/* to maintain compatibility */
	enesim_renderer_shape_stroke_location_set(ENESIM_RENDERER(o),
			ENESIM_SHAPE_STROKE_INSIDE);
}

static void _enesim_renderer_rectangle_instance_deinit(void *o EINA_UNUSED)
{
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Create a new rectangle renderer.
 *
 * @return A new rectangle renderer.
 *
 * This function returns a newly allocated rectangle renderer. On memory
 * l, this function returns @c NULL.
 */
EAPI Enesim_Renderer * enesim_renderer_rectangle_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_rectangle);
	return r;
}

/**
 * @brief Set the width of a rectangle renderer.
 *
 * @param[in] r The rectangle renderer.
 * @param[in] width The rectangle width.
 *
 * This function sets the width of the rectangle renderer @p r to the
 * value @p width.
 */
EAPI void enesim_renderer_rectangle_width_set(Enesim_Renderer *r, double width)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	thiz->current.width = width;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Retrieve the width of a rectangle renderer.
 *
 * @param[in] r The rectangle renderer.
 * @param[out] width The rectangle width.
 *
 * This function stores the width of the rectangle renderer @p r in
 * the buffer @p width.
 */
EAPI void enesim_renderer_rectangle_width_get(Enesim_Renderer *r, double *width)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	*width = thiz->current.width;
}

/**
 * @brief Set the height of a rectangle renderer.
 *
 * @param[in] r The rectangle renderer.
 * @param[in] height The rectangle height.
 *
 * This function sets the height of the rectangle renderer @p r to the
 * value @p height.
 */
EAPI void enesim_renderer_rectangle_height_set(Enesim_Renderer *r, double height)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	thiz->current.height = height;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Retrieve the height of a rectangle renderer.
 *
 * @param[in] r The rectangle renderer.
 * @param[out] height The rectangle height.
 *
 * This function stores the height of the rectangle renderer @p r in
 * the buffer @p height.
 */
EAPI void enesim_renderer_rectangle_height_get(Enesim_Renderer *r, double *height)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	*height = thiz->current.height;
}

/**
 * @brief Set the top left X coordinate of a rectangle renderer.
 *
 * @param[in] r The rectangle renderer.
 * @param[in] x The top left X coordinate.
 *
 * This function sets the top left X coordinate of the rectangle
 * renderer @p r to the value @p x.
 */
EAPI void enesim_renderer_rectangle_x_set(Enesim_Renderer *r, double x)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	thiz->current.x = x;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Retrieve the top left X coordinate of a rectangle renderer.
 *
 * @param[in] r The rectangle renderer.
 * @param[out] x The top left X coordinate.
 *
 * This function stores the top left X coordinate of the rectangle
 * renderer @p r in the buffer @p x.
 */
EAPI void enesim_renderer_rectangle_x_get(Enesim_Renderer *r, double *x)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	*x = thiz->current.x;
}

/**
 * @brief Set the top left Y coordinate of a rectangle renderer.
 *
 * @param[in] r The rectangle renderer.
 * @param[in] y The top left Y coordinate.
 *
 * This function sets the top left Y coordinate of the rectangle
 * renderer @p r to the value @p y.
 */
EAPI void enesim_renderer_rectangle_y_set(Enesim_Renderer *r, double y)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Retrieve the top left Y coordinate of a rectangle renderer.
 *
 * @param[in] r The rectangle renderer.
 * @param[out] y The top left Y coordinate.
 *
 * This function stores the top left Y coordinate of the rectangle
 * renderer @p r in the buffer @p y.
 */
EAPI void enesim_renderer_rectangle_y_get(Enesim_Renderer *r, double *y)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	*y = thiz->current.y;
}

/**
 * @brief Set the top left coordinates of a rectangle renderer.
 *
 * @param[in] r The rectangle renderer.
 * @param[in] x The top left X coordinate.
 * @param[in] y The top left Y coordinate.
 *
 * This function sets the top left coordinates of the rectangle
 * renderer @p r to the values @p x and @p y.
 */
EAPI void enesim_renderer_rectangle_position_set(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Rectangle *thiz;
	thiz = ENESIM_RENDERER_RECTANGLE(r);
	thiz->current.x = x;
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Retrieve the top left coordinates of a rectangle renderer.
 *
 * @param[in] r The rectangle renderer.
 * @param[out] x The top left X coordinate.
 * @param[out] y The top left Y coordinate.
 *
 * This function stores the top left coordinates value of the
 * rectangle renderer @p r in the buffers @p x and @p y. These buffers
 * can be @c NULL.
 */
EAPI void enesim_renderer_rectangle_position_get(Enesim_Renderer *r, double *x, double *y)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	if (x) *x = thiz->current.x;
	if (y) *y = thiz->current.y;
}

/**
 * @brief Set the size of a rectangle renderer.
 *
 * @param[in] r The rectangle renderer.
 * @param[in] width The width.
 * @param[in] height The height.
 *
 * This function sets the size of the rectangle renderer @p r to the
 * values @p width and @p height.
 */
EAPI void enesim_renderer_rectangle_size_set(Enesim_Renderer *r, double width, double height)
{
	Enesim_Renderer_Rectangle *thiz;
	thiz = ENESIM_RENDERER_RECTANGLE(r);
	thiz->current.width = width;
	thiz->current.height = height;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Retrieve the size of a rectangle renderer.
 *
 * @param[in] r The rectangle renderer.
 * @param[out] width The width.
 * @param[out] height The height.
 *
 * This function stores the size of the rectangle renderer @p r in the
 * buffers @p width and @p height. These buffers can be @c NULL.
 */
EAPI void enesim_renderer_rectangle_size_get(Enesim_Renderer *r, double *width, double *height)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	if (width) *width = thiz->current.width;
	if (height) *height = thiz->current.height;
}


EAPI void enesim_renderer_rectangle_corner_radius_x_set(Enesim_Renderer *r, double rx)
{
	Enesim_Renderer_Rectangle *thiz;
	thiz = ENESIM_RENDERER_RECTANGLE(r);
	if (!thiz) return;
	if (rx < 0)
		rx = 0;
	if (thiz->current.corner.rx == rx)
		return;
	thiz->current.corner.rx = rx;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

EAPI void enesim_renderer_rectangle_corner_radius_y_set(Enesim_Renderer *r, double ry)
{
	Enesim_Renderer_Rectangle *thiz;
	thiz = ENESIM_RENDERER_RECTANGLE(r);
	if (!thiz) return;
	if (ry < 0)
		ry = 0;
	if (thiz->current.corner.ry == ry)
		return;
	thiz->current.corner.ry = ry;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

EAPI void enesim_renderer_rectangle_corner_radius_x_get(Enesim_Renderer *r, double *rx)
{
	Enesim_Renderer_Rectangle *thiz;
	thiz = ENESIM_RENDERER_RECTANGLE(r);
	if (!thiz) return;
	if (rx)
		*rx = thiz->current.corner.rx;
}

EAPI void enesim_renderer_rectangle_corner_radius_y_get(Enesim_Renderer *r, double *ry)
{
	Enesim_Renderer_Rectangle *thiz;
	thiz = ENESIM_RENDERER_RECTANGLE(r);
	if (!thiz) return;
	if (ry)
		*ry = thiz->current.corner.ry;
}

/**
 * @brief Set the radii of the corners of a rectangle renderer.
 *
 * @param[in] r The rectangle renderer.
 * @param[in] rx The corners x-radius.
 * @param[in] ry The corners y-radius.
 *
 * This function sets the radii of the corners of the rectangle
 * renderer @p r to the values @p rx, @p ry.
 */
EAPI void enesim_renderer_rectangle_corner_radii_set(Enesim_Renderer *r, double rx, double ry)
{
	Enesim_Renderer_Rectangle *thiz;
	thiz = ENESIM_RENDERER_RECTANGLE(r);
	if (!thiz) return;
	if ((rx < 0) || (ry < 0))
	{ rx = 0;  ry = 0; }
	if ((thiz->current.corner.rx == rx) && (thiz->current.corner.ry == ry))
		return;
	thiz->current.corner.rx = rx;
	thiz->current.corner.ry = ry;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

EAPI void enesim_renderer_rectangle_corner_radii_get(Enesim_Renderer *r, double *rx, double *ry)
{
	Enesim_Renderer_Rectangle *thiz;
	thiz = ENESIM_RENDERER_RECTANGLE(r);
	if (!thiz) return;
	if (rx)
		*rx = thiz->current.corner.rx;
	if (ry)
		*ry = thiz->current.corner.ry;
}

/**
 * @brief Set whether the top left corner of a rectangle renderer is rounded.
 *
 * @param[in] r The rectangle renderer.
 * @param[in] rounded Whether the top left corner is rounded.
 *
 * This function sets the top left corner of the rectangle renderer
 * @p r to rounded if @p rounded is EINA_TRUE, not rounded if it is
 * EINA_FALSE.
 */
EAPI void enesim_renderer_rectangle_top_left_corner_set(Enesim_Renderer *r, Eina_Bool rounded)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	thiz->current.corner.tl = rounded;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

EAPI void enesim_renderer_rectangle_top_left_corner_get(Enesim_Renderer *r, Eina_Bool *rounded)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	if (rounded)
		*rounded = thiz->current.corner.tl;
}

/**
 * @brief Set whether the top right corner of a rectangle renderer is rounded.
 *
 * @param[in] r The rectangle renderer.
 * @param[in] rounded Whether the top right corner is rounded.
 *
 * This function sets the top right corner of the rectangle renderer
 * @p r to rounded if @p rounded is EINA_TRUE, not rounded if it is
 * EINA_FALSE.
 */
EAPI void enesim_renderer_rectangle_top_right_corner_set(Enesim_Renderer *r, Eina_Bool rounded)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	thiz->current.corner.tr = rounded;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

EAPI void enesim_renderer_rectangle_top_right_corner_get(Enesim_Renderer *r, Eina_Bool *rounded)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	if (rounded)
		*rounded = thiz->current.corner.tr;
}


/**
 * @brief Set whether the bottom left corner of a rectangle renderer is rounded.
 *
 * @param[in] r The rectangle renderer.
 * @param[in] rounded Whether the bottom left corner is rounded.
 *
 * This function sets the bottom left corner of the rectangle renderer
 * @p r to rounded if @p rounded is EINA_TRUE, not rounded if it is
 * EINA_FALSE.
 */
EAPI void enesim_renderer_rectangle_bottom_left_corner_set(Enesim_Renderer *r, Eina_Bool rounded)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	thiz->current.corner.bl = rounded;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

EAPI void enesim_renderer_rectangle_bottom_left_corner_get(Enesim_Renderer *r, Eina_Bool *rounded)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	if (rounded)
		*rounded = thiz->current.corner.bl;
}

/**
 * @brief Set whether the bottom right corner of a rectangle renderer is rounded.
 *
 * @param[in] r The rectangle renderer.
 * @param[in] rounded Whether the bottom right corner is rounded.
 *
 * This function sets the bottom right corner of the rectangle renderer
 * @p r to rounded if @p rounded is EINA_TRUE, not rounded if it is
 * EINA_FALSE.
 */
EAPI void enesim_renderer_rectangle_bottom_right_corner_set(Enesim_Renderer *r, Eina_Bool rounded)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	thiz->current.corner.br = rounded;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

EAPI void enesim_renderer_rectangle_bottom_right_corner_get(Enesim_Renderer *r, Eina_Bool *rounded)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	if (rounded)
		*rounded = thiz->current.corner.br;
}

/**
 * @brief Set whether the corners of a rectangle renderer are rounded.
 *
 * @param[in] r The rectangle renderer.
 * @param[in] tl Whether the top left corner is rounded.
 * @param[in] tr Whether the top right corner is rounded.
 * @param[in] bl Whether the bottom left corner is rounded.
 * @param[in] br Whether the bottom right corner is rounded.
 *
 * This function sets the corners of the rectangle renderer
 * @p r to rounded if @p rounded is EINA_TRUE, not rounded if it is
 * EINA_FALSE. @p tl, @p tr, @p bl, @p br are respectively for top
 * left, top right, bottom left and bottom right corners.
 */
EAPI void enesim_renderer_rectangle_corners_set(Enesim_Renderer *r,
		Eina_Bool tl, Eina_Bool tr, Eina_Bool bl, Eina_Bool br)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	if (!thiz) return;

	if ((thiz->current.corner.tl == tl) && (thiz->current.corner.tr == tr) &&
	     (thiz->current.corner.bl == bl) && (thiz->current.corner.br == br))
		return;
	thiz->current.corner.tl = tl;  thiz->current.corner.tr = tr;
	thiz->current.corner.bl = bl;  thiz->current.corner.br = br;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

EAPI void enesim_renderer_rectangle_corners_get(Enesim_Renderer *r,
		Eina_Bool *tl, Eina_Bool *tr, Eina_Bool *bl, Eina_Bool *br)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = ENESIM_RENDERER_RECTANGLE(r);
	if (!thiz) return;

	if (tl)
		*tl = thiz->current.corner.tl;
	if (tr)
		*tr = thiz->current.corner.tr;
	if (bl)
		*bl = thiz->current.corner.bl;
	if (br)
		*br = thiz->current.corner.br;
}
