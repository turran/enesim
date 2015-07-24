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
#include "enesim_private.h"

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_path.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_circle.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_list_private.h"
#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_shape_path_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_RENDERER_CIRCLE(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Circle,	enesim_renderer_circle_descriptor_get())

typedef struct _Enesim_Renderer_Circle_State {
	double x;
	double y;
	double r;
} Enesim_Renderer_Circle_State;

typedef struct _Enesim_Renderer_Circle {
	Enesim_Renderer_Shape_Path parent;
	/* public properties */
	Enesim_Renderer_Circle_State current;
	Enesim_Renderer_Circle_State past;
	/* private */
	Eina_Bool changed : 1;
	Eina_Bool generated : 1;
} Enesim_Renderer_Circle;

typedef struct _Enesim_Renderer_Circle_Class {
	Enesim_Renderer_Shape_Path_Class parent;
} Enesim_Renderer_Circle_Class;

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
 *                            Shape path interface                            *
 *----------------------------------------------------------------------------*/
static Eina_Bool _circle_setup(Enesim_Renderer *r, Enesim_Path *path)
{
	Enesim_Renderer_Circle *thiz;

	thiz = ENESIM_RENDERER_CIRCLE(r);
	if (_circle_properties_have_changed(thiz) && !thiz->generated)
	{
		Enesim_Renderer_Shape_Draw_Mode draw_mode;
		double rad;
		double x, y;

		rad = thiz->current.r;
		draw_mode = enesim_renderer_shape_draw_mode_get(r);
		if (draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
		{
			Enesim_Renderer_Shape_Stroke_Location location;
			double sw;

			location = enesim_renderer_shape_stroke_location_get(r);
			sw = enesim_renderer_shape_stroke_weight_get(r);
			switch (location)
			{
				case ENESIM_RENDERER_SHAPE_STROKE_LOCATION_OUTSIDE:
				rad += sw / 2.0;
				break;

				case ENESIM_RENDERER_SHAPE_STROKE_LOCATION_INSIDE:
				rad -= sw / 2.0;
				break;

				case ENESIM_RENDERER_SHAPE_STROKE_LOCATION_CENTER:
				break;
			}
		}
		/* generate the four arcs */
		x = thiz->current.x;
		y = thiz->current.y;
		enesim_path_command_clear(path);
		enesim_path_move_to(path, x, y - rad);
		enesim_path_arc_to(path, rad, rad, 0, EINA_FALSE,
				EINA_TRUE, x + rad, y);
		enesim_path_arc_to(path, rad, rad, 0, EINA_FALSE,
				EINA_TRUE, x, y + rad);
		enesim_path_arc_to(path, rad, rad, 0, EINA_FALSE,
				EINA_TRUE, x - rad, y);
		enesim_path_arc_to(path, rad, rad, 0, EINA_FALSE,
				EINA_TRUE, x, y - rad);
		thiz->generated = EINA_TRUE;
	}
	return EINA_TRUE;
}

static void _circle_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Circle *thiz;

	thiz = ENESIM_RENDERER_CIRCLE(r);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

static Eina_Bool _circle_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Circle *thiz;
	Eina_Bool ret;

	thiz = ENESIM_RENDERER_CIRCLE(r);
	ret = _circle_properties_have_changed(thiz);
	return ret;
}
/*----------------------------------------------------------------------------*
 *                             Shape interface                                *
 *----------------------------------------------------------------------------*/
static void _circle_shape_features_get(Enesim_Renderer *r EINA_UNUSED,
		int *features)
{
	*features = ENESIM_RENDERER_SHAPE_FEATURE_FILL_RENDERER |
			ENESIM_RENDERER_SHAPE_FEATURE_STROKE_RENDERER |
			ENESIM_RENDERER_SHAPE_FEATURE_STROKE_LOCATION;
}

static Eina_Bool _circle_geometry_get(Enesim_Renderer *r,
		Enesim_Rectangle *geometry)
{
	Enesim_Renderer_Circle *thiz;

	thiz = ENESIM_RENDERER_CIRCLE(r);
	geometry->x = thiz->current.x - thiz->current.r;
	geometry->y = thiz->current.y - thiz->current.r;
	geometry->w = geometry->h = thiz->current.r * 2;
	return EINA_TRUE;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _circle_base_name_get(Enesim_Renderer *r EINA_UNUSED)
{
	return "circle";
}

/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_SHAPE_PATH_DESCRIPTOR,
		Enesim_Renderer_Circle, Enesim_Renderer_Circle_Class,
		enesim_renderer_circle);

static void _enesim_renderer_circle_class_init(void *k)
{
	Enesim_Renderer_Class *r_klass;
	Enesim_Renderer_Shape_Class *s_klass;
	Enesim_Renderer_Shape_Path_Class *klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _circle_base_name_get;

	s_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	s_klass->features_get = _circle_shape_features_get;
	s_klass->geometry_get = _circle_geometry_get;

	klass = ENESIM_RENDERER_SHAPE_PATH_CLASS(k);
	klass->has_changed = _circle_has_changed;
	klass->setup = _circle_setup;
	klass->cleanup = _circle_cleanup;
}

static void _enesim_renderer_circle_instance_init(void *o)
{
	/* to maintain compatibility */
	enesim_renderer_shape_stroke_location_set(ENESIM_RENDERER(o),
			ENESIM_RENDERER_SHAPE_STROKE_LOCATION_INSIDE);
}

static void _enesim_renderer_circle_instance_deinit(void *o EINA_UNUSED)
{
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Create a new circle renderer.
 *
 * @return A new circle renderer.
 *
 * This function returns a newly allocated circle renderer.
 */
EAPI Enesim_Renderer * enesim_renderer_circle_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_circle);
	return r;
}

/**
 * @brief Set the X coordinate of the center of a circle renderer.
 * @ender_prop{x}
 * @param[in] r The circle renderer.
 * @param[in] x The X coordinate.
 *
 * This function sets the X coordinate of the center of the circle
 * renderer @p r to the value @p x.
 */
EAPI void enesim_renderer_circle_x_set(Enesim_Renderer *r, double x)
{
	Enesim_Renderer_Circle *thiz;

	thiz = ENESIM_RENDERER_CIRCLE(r);
	thiz->current.x = x;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Gets the X coordinate of the center of a circle renderer.
 * @ender_prop{x}
 * @param[in] r The circle renderer.
 * @return The X coordinate.
 */
EAPI double enesim_renderer_circle_x_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Circle *thiz;

	thiz = ENESIM_RENDERER_CIRCLE(r);
	return thiz->current.x;
}

/**
 * @brief Set the Y coordinate of the center of a circle renderer.
 * @ender_prop{y}
 * @param[in] r The circle renderer.
 * @param[in] y The Y coordinate.
 *
 * This function sets the Y coordinate of the center of the circle
 * renderer @p r to the value @p y.
 */
EAPI void enesim_renderer_circle_y_set(Enesim_Renderer *r, double y)
{
	Enesim_Renderer_Circle *thiz;

	thiz = ENESIM_RENDERER_CIRCLE(r);
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Gets the Y coordinate of the center of a circle renderer.
 * @ender_prop{y}
 * @param[in] r The circle renderer.
 * @return The Y coordinate.
 */
EAPI double enesim_renderer_circle_y_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Circle *thiz;

	thiz = ENESIM_RENDERER_CIRCLE(r);
	return thiz->current.y;
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

	thiz = ENESIM_RENDERER_CIRCLE(r);
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
 * the circle renderer @p r in the pointers @p x and @p y. These pointers
 * can be @c NULL.
 */
EAPI void enesim_renderer_circle_center_get(Enesim_Renderer *r, double *x, double *y)
{
	Enesim_Renderer_Circle *thiz;

	thiz = ENESIM_RENDERER_CIRCLE(r);
	if (x) *x = thiz->current.x;
	if (y) *y = thiz->current.y;
}

/**
 * @brief Set the radius of a circle renderer.
 * @ender_prop{radius}
 * @param[in] r The circle renderer.
 * @param[in] radius The radius.
 *
 * This function sets the radius of the circle renderer @p r to the
 * value @p radius.
 */
EAPI void enesim_renderer_circle_radius_set(Enesim_Renderer *r, double radius)
{
	Enesim_Renderer_Circle *thiz;

	thiz = ENESIM_RENDERER_CIRCLE(r);
	thiz->current.r = radius;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Retrieve the radius of a circle renderer.
 * @ender_prop{radius}
 * @param[in] r The circle renderer.
 * @return The radius
 */
EAPI double enesim_renderer_circle_radius_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Circle *thiz;

	thiz = ENESIM_RENDERER_CIRCLE(r);
	return thiz->current.r;
}
