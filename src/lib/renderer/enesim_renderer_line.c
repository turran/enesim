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
#include "enesim_renderer_line.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_shape_path_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_LINE(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Line,					\
		enesim_renderer_line_descriptor_get())

typedef struct _Enesim_Renderer_Line_State
{
	double x0;
	double y0;
	double x1;
	double y1;
} Enesim_Renderer_Line_State;

typedef struct _Enesim_Renderer_Line
{
	Enesim_Renderer_Shape_Path parent;
	/* properties */
	Enesim_Renderer_Line_State current;
	Enesim_Renderer_Line_State past;
	Eina_Bool changed;
	Eina_Bool generated;
} Enesim_Renderer_Line;

typedef struct _Enesim_Renderer_Line_Class {
	Enesim_Renderer_Shape_Path_Class parent;
} Enesim_Renderer_Line_Class;

static Eina_Bool _line_changed(Enesim_Renderer_Line *thiz)
{
	if (!thiz->changed)
		return EINA_FALSE;

	/* x0 */
	if (thiz->current.x0 != thiz->past.x0)
		return EINA_TRUE;
	/* y0 */
	if (thiz->current.y0 != thiz->past.y0)
		return EINA_TRUE;
	/* x1 */
	if (thiz->current.x1 != thiz->past.x1)
		return EINA_TRUE;
	/* y1 */
	if (thiz->current.y1 != thiz->past.y1)
		return EINA_TRUE;

	return EINA_FALSE;
}

/*----------------------------------------------------------------------------*
 *                            Shape path interface                            *
 *----------------------------------------------------------------------------*/
static Eina_Bool _line_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Line *thiz;
	Eina_Bool ret;

	thiz = ENESIM_RENDERER_LINE(r);
	ret = _line_changed(thiz);
	return ret;
}

static Eina_Bool _line_setup(Enesim_Renderer *r, Enesim_Path *path)
{
	Enesim_Renderer_Line *thiz;

	thiz = ENESIM_RENDERER_LINE(r);
	if (_line_changed(thiz) && !thiz->generated)
	{
		enesim_path_command_clear(path);
		enesim_path_move_to(path, thiz->current.x0, thiz->current.y0);
		enesim_path_line_to(path, thiz->current.x1, thiz->current.y1);
		thiz->generated = EINA_TRUE;
	}
	return EINA_TRUE;
}

static void _line_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Line *thiz;

	thiz = ENESIM_RENDERER_LINE(r);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}
/*----------------------------------------------------------------------------*
 *                             Shape interface                                *
 *----------------------------------------------------------------------------*/
static void _line_shape_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Shape_Feature *features)
{
	*features = ENESIM_RENDERER_SHAPE_FEATURE_STROKE_RENDERER;
}

static Eina_Bool _line_geometry_get(Enesim_Renderer *r,
		Enesim_Rectangle *geometry)
{
	Enesim_Renderer_Line *thiz;

	thiz = ENESIM_RENDERER_LINE(r);
	enesim_rectangle_coords_from(geometry,
			thiz->current.x0 < thiz->current.x1 ? thiz->current.x0 : thiz->current.x1,
			thiz->current.y0 < thiz->current.y1 ? thiz->current.y0 : thiz->current.y1,
			fabs(thiz->current.x0 - thiz->current.x1),
			fabs(thiz->current.y0 - thiz->current.y1));
	return EINA_TRUE;	
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _line_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "line";
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_SHAPE_PATH_DESCRIPTOR,
		Enesim_Renderer_Line, Enesim_Renderer_Line_Class,
		enesim_renderer_line);

static void _enesim_renderer_line_class_init(void *k)
{
	Enesim_Renderer_Class *r_klass;
	Enesim_Renderer_Shape_Class *s_klass;
	Enesim_Renderer_Shape_Path_Class *klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _line_name;

	s_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	s_klass->features_get = _line_shape_features_get;
	s_klass->geometry_get = _line_geometry_get;

	klass = ENESIM_RENDERER_SHAPE_PATH_CLASS(k);
	klass->has_changed = _line_has_changed;
	klass->setup = _line_setup;
	klass->cleanup = _line_cleanup;
}

static void _enesim_renderer_line_instance_init(void *o)
{
	Enesim_Renderer *r;

	r = ENESIM_RENDERER(o);
	/* the draw mode should be always a stroke only */
	enesim_renderer_shape_draw_mode_set(r, ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE);
}

static void _enesim_renderer_line_instance_deinit(void *o EINA_UNUSED)
{
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Create a new line renderer.
 *
 * @return A new line renderer.
 *
 * This function returns a newly allocated line renderer. On memory
 * l, this function returns @c NULL.
 */
EAPI Enesim_Renderer * enesim_renderer_line_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_line);
	return r;
}

/**
 * @brief Set the X coordinate of the first point of a line renderer.
 *
 * @param[in] r The line renderer.
 * @param[in] x0 The X coordinate.
 *
 * This function sets the X coordinate of the first point of the line
 * renderer @p r to the value @p x0.
 */
EAPI void enesim_renderer_line_x0_set(Enesim_Renderer *r, double x0)
{
	Enesim_Renderer_Line *thiz;

	thiz = ENESIM_RENDERER_LINE(r);
	thiz->current.x0 = x0;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Retrieve the X coordinate of the first point of a line renderer.
 *
 * @param[in] r The line renderer.
 * @param[out] x0 The X coordinate.
 *
 * This function stores the X coordinate value of the first point of
 * the line renderer @p r in the buffer @p x0.
 */
EAPI void enesim_renderer_line_x0_get(Enesim_Renderer *r, double *x0)
{
	Enesim_Renderer_Line *thiz;

	thiz = ENESIM_RENDERER_LINE(r);
	*x0 = thiz->current.x0;
}

/**
 * @brief Set the Y coordinate of the first point of a line renderer.
 *
 * @param[in] r The line renderer.
 * @param[in] y The Y coordinate.
 *
 * This function sets the Y coordinate of the first point of the line
 * renderer @p r to the value @p y.
 */
EAPI void enesim_renderer_line_y0_set(Enesim_Renderer *r, double y)
{
	Enesim_Renderer_Line *thiz;

	thiz = ENESIM_RENDERER_LINE(r);
	thiz->current.y0 = y;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Retrieve the Y coordinate of the first point of a line renderer.
 *
 * @param[in] r The line renderer.
 * @param[out] y The Y coordinate.
 *
 * This function stores the Y coordinate value of the first point of
 * the line renderer @p r in the buffer @p y.
 */
EAPI void enesim_renderer_line_y0_get(Enesim_Renderer *r, double *y)
{
	Enesim_Renderer_Line *thiz;

	thiz = ENESIM_RENDERER_LINE(r);
	*y = thiz->current.y0;
}

/**
 * @brief Set the X coordinate of the second point of a line renderer.
 *
 * @param[in] r The line renderer.
 * @param[in] x The X coordinate.
 *
 * This function sets the X coordinate of the second point of the line
 * renderer @p r to the value @p x.
 */
EAPI void enesim_renderer_line_x1_set(Enesim_Renderer *r, double x)
{
	Enesim_Renderer_Line *thiz;

	thiz = ENESIM_RENDERER_LINE(r);
	thiz->current.x1 = x;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Retrieve the X coordinate of the second point of a line renderer.
 *
 * @param[in] r The line renderer.
 * @param[out] x The X coordinate.
 *
 * This function stores the X coordinate value of the second point of
 * the line renderer @p r in the buffer @p x.
 */
EAPI void enesim_renderer_line_x1_get(Enesim_Renderer *r, double *x)
{
	Enesim_Renderer_Line *thiz;

	thiz = ENESIM_RENDERER_LINE(r);
	*x = thiz->current.x1;
}

/**
 * @brief Set the Y coordinate of the second point of a line renderer.
 *
 * @param[in] r The line renderer.
 * @param[in] y The Y coordinate.
 *
 * This function sets the Y coordinate of the second point of the line
 * renderer @p r to the value @p y.
 */
EAPI void enesim_renderer_line_y1_set(Enesim_Renderer *r, double y)
{
	Enesim_Renderer_Line *thiz;

	thiz = ENESIM_RENDERER_LINE(r);
	thiz->current.y1 = y;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Retrieve the Y coordinate of the second point of a line renderer.
 *
 * @param[in] r The line renderer.
 * @param[out] y The Y coordinate.
 *
 * This function stores the Y coordinate value of the second point of
 * the line renderer @p r in the buffer @p y.
 */
EAPI void enesim_renderer_line_y1_get(Enesim_Renderer *r, double *y)
{
	Enesim_Renderer_Line *thiz;

	thiz = ENESIM_RENDERER_LINE(r);
	*y = thiz->current.y1;
}

/**
 * @brief Set the coordinates of a line renderer.
 *
 * @param[in] r The line renderer.
 * @param[in] x0 The X coordinate of the first point.
 * @param[in] y0 The Y coordinate of the first point.
 * @param[in] x1 The X coordinate of the second point.
 * @param[in] y1 The Y coordinate of the second point.
 *
 * This function sets the coordinates of the points of the line
 * renderer @p r to the values @p x0, @p y0, @p x1 and @p y1.
 */
EAPI void enesim_renderer_line_coords_set(Enesim_Renderer *r, double x0, double y0, double x1, double y1)
{
	Enesim_Renderer_Line *thiz;

	thiz = ENESIM_RENDERER_LINE(r);
	thiz->current.x0 = x0;
	thiz->current.y0 = y0;
	thiz->current.x1 = x1;
	thiz->current.y1 = y1;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * @brief Retrieve the coordinates of a line renderer.
 *
 * @param[in] r The line renderer.
 * @param[out] x0 The X coordinate of the first point.
 * @param[out] y0 The Y coordinate of the first point.
 * @param[out] x1 The X coordinate of the second point.
 * @param[out] y1 The Y coordinate of the second point.
 *
 * This function stores the coordinates value of the points of
 * the line renderer @p r in the buffers @p x0, @p y0, @p x1 and @p
 * y1. These buffers can be @c NULL.
 */
EAPI void enesim_renderer_line_coords_get(Enesim_Renderer *r, double *x0, double *y0, double *x1, double *y1)
{
	Enesim_Renderer_Line *thiz;

	thiz = ENESIM_RENDERER_LINE(r);
	if (x0) *x0 = thiz->current.x0;
	if (y0) *y0 = thiz->current.y0;
	if (x1) *x1 = thiz->current.x1;
	if (y1) *y1 = thiz->current.y1;
}
