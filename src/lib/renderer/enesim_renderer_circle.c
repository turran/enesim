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
#include "enesim_renderer_circle.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_shape_path_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_CIRCLE_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_CIRCLE_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_CIRCLE_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Circle_State {
	double x;
	double y;
	double r;
} Enesim_Renderer_Circle_State;

typedef struct _Enesim_Renderer_Circle {
	EINA_MAGIC
	/* public properties */
	Enesim_Renderer_Circle_State current;
	Enesim_Renderer_Circle_State past;
	/* private */
	Eina_Bool changed : 1;
	Eina_Bool generated : 1;
} Enesim_Renderer_Circle;

static inline Enesim_Renderer_Circle * _circle_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Circle *thiz;

	thiz = enesim_renderer_shape_path_data_get(r);
	ENESIM_RENDERER_CIRCLE_MAGIC_CHECK(thiz);

	return thiz;
}

static Eina_Bool _circle_properties_have_changed(Enesim_Renderer_Circle *thiz)
{
	if (!thiz->changed) return EINA_FALSE;
	/* the radius */
	if (thiz->current.r != thiz->past.r)
		return EINA_TRUE;
	/* the x */
	if (thiz->current.x != thiz->past.x)
		return EINA_TRUE;
	/* the y */
	if (thiz->current.y != thiz->past.y)
		return EINA_TRUE;
	return EINA_FALSE;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _circle_base_name_get(Enesim_Renderer *r EINA_UNUSED)
{
	return "circle";
}

static Eina_Bool _circle_setup(Enesim_Renderer *r, Enesim_Renderer *path,
		Enesim_Log **l)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	if (_circle_properties_have_changed(thiz) && !thiz->generated)
	{
		Enesim_Shape_Draw_Mode draw_mode;
		double rad;
		double x, y;

		rad = thiz->current.r;
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
				rad += sw / 2.0;
				break;

				case ENESIM_SHAPE_STROKE_INSIDE:
				rad -= sw / 2.0;
				break;

				case ENESIM_SHAPE_STROKE_CENTER:
				break;
			}
		}
		/* generate the four arcs */
		x = thiz->current.x;
		y = thiz->current.y;
		enesim_renderer_path_command_clear(path);
		enesim_renderer_path_move_to(path, x, y - rad);
		enesim_renderer_path_arc_to(path, rad, rad, 0, EINA_FALSE,
				EINA_TRUE, x + rad, y);
		enesim_renderer_path_arc_to(path, rad, rad, 0, EINA_FALSE,
				EINA_TRUE, x, y + rad);
		enesim_renderer_path_arc_to(path, rad, rad, 0, EINA_FALSE,
				EINA_TRUE, x - rad, y);
		enesim_renderer_path_arc_to(path, rad, rad, 0, EINA_FALSE,
				EINA_TRUE, x, y - rad);
		thiz->generated = EINA_TRUE;
	}
	return EINA_TRUE;
}

static void _circle_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

/* FIXME we still need to decide what to do with the stroke
 * transformation
 */
static void _circle_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Circle *thiz;
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Matrix_Type type;
	double sw = 0;

	thiz = _circle_get(r);
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);
	if (draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE)
	{
		Enesim_Shape_Stroke_Location location;
		enesim_renderer_shape_stroke_weight_get(r, &sw);
		enesim_renderer_shape_stroke_location_get(r, &location);
		switch (location)
		{
			case ENESIM_SHAPE_STROKE_CENTER:
			sw /= 2.0;
			break;

			case ENESIM_SHAPE_STROKE_INSIDE:
			sw = 0.0;
			break;

			case ENESIM_SHAPE_STROKE_OUTSIDE:
			break;
		}
	}
	rect->x = thiz->current.x - thiz->current.r - sw;
	rect->y = thiz->current.y - thiz->current.r - sw;
	rect->w = rect->h = (thiz->current.r + sw) * 2;

	/* apply the geometry transformation */
	enesim_renderer_transformation_type_get(r, &type);
	if (type != ENESIM_MATRIX_IDENTITY)
	{
		Enesim_Matrix m;
		Enesim_Quad q;

		enesim_renderer_transformation_get(r, &m);
		enesim_matrix_rectangle_transform(&m, rect, &q);
		enesim_quad_rectangle_to(&q, rect);
	}
}

static Eina_Bool _circle_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	return _circle_properties_have_changed(thiz);
}

static void _circle_shape_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Shape_Feature *features)
{
	*features = ENESIM_SHAPE_FLAG_FILL_RENDERER |
			ENESIM_SHAPE_FLAG_STROKE_RENDERER |
			ENESIM_SHAPE_FLAG_STROKE_LOCATION;
}

static void _circle_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	free(thiz);
}

static Enesim_Renderer_Shape_Path_Descriptor _circle_descriptor = {
	/* .base_name_get = 		*/ _circle_base_name_get,
	/* .free = 			*/ _circle_free,
	/* .has_changed = 		*/ _circle_has_changed,
	/* .shape_features_get =	*/ _circle_shape_features_get,
	/* .bounds_get = 			*/ _circle_bounds_get,
	/* .setup = 			*/ _circle_setup,
	/* .cleanup = 			*/ _circle_cleanup,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Create a new circle renderer.
 *
 * @return A new circle renderer.
 *
 * This function returns a newly allocated circle renderer. On memory
 * l, this function returns @c NULL.
 */
EAPI Enesim_Renderer * enesim_renderer_circle_new(void)
{
	Enesim_Renderer_Circle *thiz;
	Enesim_Renderer *r;

	thiz = calloc(1, sizeof(Enesim_Renderer_Circle));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_CIRCLE_MAGIC);
	r = enesim_renderer_shape_path_new(&_circle_descriptor, thiz);
	/* to maintain compatibility */
	enesim_renderer_shape_stroke_location_set(r, ENESIM_SHAPE_STROKE_INSIDE);
	return r;
}

/**
 * @brief Set the X coordinate of the center of a circle renderer.
 *
 * @param[in] r The circle renderer.
 * @param[in] x The X coordinate.
 *
 * This function sets the X coordinate of the center of the circle
 * renderer @p r to the value @p x.
 */
EAPI void enesim_renderer_circle_x_set(Enesim_Renderer *r, double x)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	thiz->current.x = x;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Set the Y coordinate of the center of a circle renderer.
 *
 * @param[in] r The circle renderer.
 * @param[in] y The Y coordinate.
 *
 * This function sets the Y coordinate of the center of the circle
 * renderer @p r to the value @p y.
 */
EAPI void enesim_renderer_circle_y_set(Enesim_Renderer *r, double y)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Set the coordinates of the center of a circle renderer.
 *
 * @param[in] r The circle renderer.
 * @param[in] x The X coordinate of the center.
 * @param[in] y The Y coordinate of the center.
 *
 * This function sets the coordinates of the center of the circle
 * renderer @p r to the values @p x and @p y.
 */
EAPI void enesim_renderer_circle_center_set(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	thiz->current.x = x;
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Retrieve the coordinates of the center of a circle renderer.
 *
 * @param[in] r The circle renderer.
 * @param[out] x The X coordinate of the center.
 * @param[out] y The Y coordinate of the center.
 *
 * This function stores the coordinates value of the center of
 * the circle renderer @p r in the buffers @p x and @p y. These buffers
 * can be @c NULL.
 */
EAPI void enesim_renderer_circle_center_get(Enesim_Renderer *r, double *x, double *y)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	if (x) *x = thiz->current.x;
	if (y) *y = thiz->current.y;
}

/**
 * @brief Set the radius of a circle renderer.
 *
 * @param[in] r The circle renderer.
 * @param[in] radius The radius.
 *
 * This function sets the radius of the circle renderer @p r to the
 * value @p radius.
 */
EAPI void enesim_renderer_circle_radius_set(Enesim_Renderer *r, double radius)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	if (radius < 1)
		radius = 1;
	thiz->current.r = radius;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Retrieve the radius of a circle renderer.
 *
 * @param[in] r The circle renderer.
 * @param[out] radius The radius.
 *
 * This function stores the radius of the circle renderer @p r in the
 * buffer @p radius.
 */
EAPI void enesim_renderer_circle_radius_get(Enesim_Renderer *r, double *radius)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	*radius = thiz->current.r;
}
