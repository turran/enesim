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
#include "enesim_renderer_ellipse.h"
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
#define ENESIM_RENDERER_ELLIPSE(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Ellipse,					\
		enesim_renderer_ellipse_descriptor_get())

typedef struct _Enesim_Renderer_Ellipse_State {
	double x;
	double y;
	double rx;
	double ry;
} Enesim_Renderer_Ellipse_State;

typedef struct _Enesim_Renderer_Ellipse
{
	Enesim_Renderer_Shape_Path parent;
	/* properties */
	Enesim_Renderer_Ellipse_State current;
	Enesim_Renderer_Ellipse_State past;
	/* private */
	Eina_Bool changed : 1;
	Eina_Bool generated : 1;
} Enesim_Renderer_Ellipse;

typedef struct _Enesim_Renderer_Ellipse_Class {
	Enesim_Renderer_Shape_Path_Class parent;
} Enesim_Renderer_Ellipse_Class;

static Eina_Bool _ellipse_properties_have_changed(Enesim_Renderer_Ellipse *thiz)
{
	if (!thiz->changed) return EINA_FALSE;
	/* the rx */
	if (thiz->current.rx != thiz->past.rx)
		return EINA_TRUE;
	/* the ry */
	if (thiz->current.ry != thiz->past.ry)
		return EINA_TRUE;
	/* the x */
	if (thiz->current.x != thiz->past.x)
		return EINA_TRUE;
	/* the y */
	if (thiz->current.y != thiz->past.y)
		return EINA_TRUE;
	return EINA_FALSE;
}

static void _ellipse_get_real(Enesim_Renderer_Ellipse *thiz,
		Enesim_Renderer *r,
		double *x, double *y, double *rx, double *ry)
{
	Enesim_Renderer_Shape_Draw_Mode draw_mode;

	*rx = thiz->current.rx;
	*ry = thiz->current.ry;
	*x = thiz->current.x;
	*y = thiz->current.y;

	draw_mode = enesim_renderer_shape_draw_mode_get(r);
	if (draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
	{
		Enesim_Renderer_Shape_Stroke_Location location;
		double sw;

		sw = enesim_renderer_shape_stroke_weight_get(r);
		location = enesim_renderer_shape_stroke_location_get(r);
		switch (location)
		{
			case ENESIM_RENDERER_SHAPE_STROKE_LOCATION_OUTSIDE:
			*rx += sw / 2.0;
			*ry += sw / 2.0;
			break;

			case ENESIM_RENDERER_SHAPE_STROKE_LOCATION_INSIDE:
			*rx -= sw / 2.0;
			*ry -= sw / 2.0;
			break;

			case ENESIM_RENDERER_SHAPE_STROKE_LOCATION_CENTER:
			break;
		}
	}
}
/*----------------------------------------------------------------------------*
 *                            Shape path interface                            *
 *----------------------------------------------------------------------------*/
static Eina_Bool _ellipse_setup(Enesim_Renderer *r, Enesim_Path *path)
{
	Enesim_Renderer_Ellipse *thiz;
	double rx, ry;
	double x, y;

	thiz = ENESIM_RENDERER_ELLIPSE(r);
	_ellipse_get_real(thiz, r, &x, &y, &rx, &ry);
	if (!thiz || (thiz->current.rx <= 0) || (thiz->current.ry <= 0))
	{
		return EINA_FALSE;
	}

	if (_ellipse_properties_have_changed(thiz) && !thiz->generated)
	{
		enesim_path_command_clear(path);
		enesim_path_move_to(path, x, y - ry);
		enesim_path_arc_to(path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x + rx, y);
		enesim_path_arc_to(path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x, y + ry);
		enesim_path_arc_to(path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x - rx, y);
		enesim_path_arc_to(path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x, y - ry);
	}
	return EINA_TRUE;
}

static void _ellipse_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = ENESIM_RENDERER_ELLIPSE(r);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

static Eina_Bool _ellipse_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = ENESIM_RENDERER_ELLIPSE(r);
	return _ellipse_properties_have_changed(thiz);
}
/*----------------------------------------------------------------------------*
 *                             Shape interface                                *
 *----------------------------------------------------------------------------*/
static void _ellipse_shape_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Shape_Feature *features)
{
	*features = ENESIM_RENDERER_SHAPE_FEATURE_FILL_RENDERER |
			ENESIM_RENDERER_SHAPE_FEATURE_STROKE_RENDERER |
			ENESIM_RENDERER_SHAPE_FEATURE_STROKE_LOCATION;
}

static Eina_Bool _ellipse_geometry_get(Enesim_Renderer *r,
		Enesim_Rectangle *geometry)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = ENESIM_RENDERER_ELLIPSE(r);
	geometry->x = thiz->current.x - thiz->current.rx;
	geometry->y = thiz->current.y - thiz->current.ry;
	geometry->w = thiz->current.rx * 2;
	geometry->h = thiz->current.ry * 2;
	return EINA_TRUE;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _ellipse_base_name_get(Enesim_Renderer *r EINA_UNUSED)
{
	return "ellipse";
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_SHAPE_PATH_DESCRIPTOR,
		Enesim_Renderer_Ellipse, Enesim_Renderer_Ellipse_Class,
		enesim_renderer_ellipse);

static void _enesim_renderer_ellipse_class_init(void *k)
{
	Enesim_Renderer_Class *r_klass;
	Enesim_Renderer_Shape_Class *s_klass;
	Enesim_Renderer_Shape_Path_Class *klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _ellipse_base_name_get;

	s_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	s_klass->features_get = _ellipse_shape_features_get;
	s_klass->geometry_get = _ellipse_geometry_get;

	klass = ENESIM_RENDERER_SHAPE_PATH_CLASS(k);
	klass->has_changed = _ellipse_has_changed;
	klass->setup = _ellipse_setup;
	klass->cleanup = _ellipse_cleanup;
}

static void _enesim_renderer_ellipse_instance_init(void *o)
{
	/* to maintain compatibility */
	enesim_renderer_shape_stroke_location_set(ENESIM_RENDERER(o),
			ENESIM_RENDERER_SHAPE_STROKE_LOCATION_INSIDE);
}

static void _enesim_renderer_ellipse_instance_deinit(void *o EINA_UNUSED)
{
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Create a new ellipse renderer.
 *
 * @return A new ellipse renderer.
 *
 * This function returns a newly allocated ellipse renderer. On memory
 * l, this function returns @c NULL.
 */
EAPI Enesim_Renderer * enesim_renderer_ellipse_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_ellipse);
	return r;
}
/**
 * @brief Set the coordinates of the center of a ellipse renderer.
 *
 * @param[in] r The ellipse renderer.
 * @param[in] x The X coordinate of the center.
 * @param[in] y The Y coordinate of the center.
 *
 * This function sets the coordinates of the center of the ellipse
 * renderer @p r to the values @p x and @p y.
 */
EAPI void enesim_renderer_ellipse_center_set(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = ENESIM_RENDERER_ELLIPSE(r);
	if (!thiz) return;
	if ((thiz->current.x == x) && (thiz->current.y == y))
		return;
	thiz->current.x = x;
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}
/**
 * @brief Retrieve the coordinates of the center of a ellipse renderer.
 *
 * @param[in] r The ellipse renderer.
 * @param[out] x The X coordinate of the center.
 * @param[out] y The Y coordinate of the center.
 *
 * This function stores the coordinates value of the center of
 * the ellipse renderer @p r in the buffers @p x and @p y. These buffers
 * can be @c NULL.
 */
EAPI void enesim_renderer_ellipse_center_get(Enesim_Renderer *r, double *x, double *y)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = ENESIM_RENDERER_ELLIPSE(r);
	if (x) *x = thiz->current.x;
	if (y) *y = thiz->current.y;
}

/**
 * @brief Set the radii of a ellipse renderer.
 *
 * @param[in] r The ellipse renderer.
 * @param[in] radius_x The radius along the X axis.
 * @param[in] radius_y The radius along the Y axis.
 *
 * This function sets the radii of the ellipse renderer @p r to the
 * values @p radius_x along the X axis and @p radius_y along the Y axis.
 */
EAPI void enesim_renderer_ellipse_radii_set(Enesim_Renderer *r, double radius_x, double radius_y)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = ENESIM_RENDERER_ELLIPSE(r);
	if (!thiz) return;
	if ((thiz->current.rx == radius_x) && (thiz->current.ry == radius_y))
		return;
	thiz->current.rx = radius_x;
	thiz->current.ry = radius_y;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Retrieve the radii of a ellipse renderer.
 *
 * @param[in] r The ellipse renderer.
 * @param[out] radius_x The radius along the X axis.
 * @param[out] radius_y The radius along the Y axis.
 *
 * This function stores the radiis of the ellipse renderer @p r in the
 * buffers @p radius_x for the radius along the X axis and @p radius_y
 * for the radius along the Y axis. These buffers can be @c NULL.
 */
EAPI void enesim_renderer_ellipse_radii_get(Enesim_Renderer *r, double *radius_x, double *radius_y)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = ENESIM_RENDERER_ELLIPSE(r);
	if (radius_x) *radius_x = thiz->current.rx;
	if (radius_y) *radius_y = thiz->current.ry;
}

/**
 * @brief Set the X coordinate of the center of a ellipse renderer.
 *
 * @param[in] r The ellipse renderer.
 * @param[in] x The X coordinate.
 *
 * This function sets the X coordinate of the center of the ellipse
 * renderer @p r to the value @p x.
 */
EAPI void enesim_renderer_ellipse_x_set(Enesim_Renderer *r, double x)
{
	Enesim_Renderer_Ellipse *thiz;
	thiz = ENESIM_RENDERER_ELLIPSE(r);
	thiz->current.x = x;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Get the X coordinate of the center of a ellipse renderer.
 *
 * @param[in] r The ellipse renderer.
 * @param[in] x The X coordinate.
 */
EAPI double enesim_renderer_ellipse_x_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Ellipse *thiz;
	thiz = ENESIM_RENDERER_ELLIPSE(r);
	return thiz->current.x;
}

/**
 * @brief Set the Y coordinate of the center of a ellipse renderer.
 *
 * @param[in] r The ellipse renderer.
 * @param[in] y The Y coordinate.
 *
 * This function sets the Y coordinate of the center of the ellipse
 * renderer @p r to the value @p y.
 */
EAPI void enesim_renderer_ellipse_y_set(Enesim_Renderer *r, double y)
{
	Enesim_Renderer_Ellipse *thiz;
	thiz = ENESIM_RENDERER_ELLIPSE(r);
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Get the Y coordinate of the center of a ellipse renderer.
 *
 * @param[in] r The ellipse renderer.
 * @param[in] y The Y coordinate.
 */
EAPI double enesim_renderer_ellipse_y_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Ellipse *thiz;
	thiz = ENESIM_RENDERER_ELLIPSE(r);
	return thiz->current.y;
}

/**
 * @brief Set the radius along the X axis of a ellipse renderer.
 *
 * @param[in] r The ellipse renderer.
 * @param[in] rx The radius along the X axis.
 *
 * This function sets the radius along the X axis of the ellipse
 * renderer @p r to the value @p rx.
 */
EAPI void enesim_renderer_ellipse_x_radius_set(Enesim_Renderer *r, double rx)
{
	Enesim_Renderer_Ellipse *thiz;
	thiz = ENESIM_RENDERER_ELLIPSE(r);
	thiz->current.rx = rx;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Get the radius along the X axis of a ellipse renderer.
 *
 * @param[in] r The ellipse renderer.
 * @return The radius along the X axis.
 */
EAPI double enesim_renderer_ellipse_x_radius_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Ellipse *thiz;
	thiz = ENESIM_RENDERER_ELLIPSE(r);
	return thiz->current.rx;
}

/**
 * @brief Set the radius along the Y axis of a ellipse renderer.
 *
 * @param[in] r The ellipse renderer.
 * @param[in] ry The radius along the Y axis.
 *
 * This function sets the radius along the Y axis of the ellipse
 * renderer @p r to the value @p ry.
 */
EAPI void enesim_renderer_ellipse_y_radius_set(Enesim_Renderer *r, double ry)
{
	Enesim_Renderer_Ellipse *thiz;
	thiz = ENESIM_RENDERER_ELLIPSE(r);
	thiz->current.ry = ry;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Get the radius along the Y axis of a ellipse renderer.
 *
 * @param[in] r The ellipse renderer.
 * @return The radius along the Y axis.
 */
EAPI double enesim_renderer_ellipse_y_radius_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Ellipse *thiz;
	thiz = ENESIM_RENDERER_ELLIPSE(r);
	return thiz->current.ry;
}

