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
#include "enesim_renderer_path.h"
#include "enesim_renderer_circle.h"

#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
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
	/* the path renderer */
	Enesim_Renderer *path;
} Enesim_Renderer_Circle;

static inline Enesim_Renderer_Circle * _circle_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Circle *thiz;

	thiz = enesim_renderer_shape_data_get(r);
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

static void _circle_path_propagate(Enesim_Renderer_Circle *thiz,
		double r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES])
{
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	//const Enesim_Renderer_State *ps = states[ENESIM_STATE_PAST];
	const Enesim_Renderer_Shape_State *css = sstates[ENESIM_STATE_CURRENT];
	double x, y;

	if (!thiz->path)
		thiz->path = enesim_renderer_path_new();
	/* generate the four arcs */
	/* FIXME also check that the prev geometry and curr geometry transformations are diff */
	if (!_circle_properties_have_changed(thiz))
		goto pass;

	x = thiz->current.x;
	y = thiz->current.y;
	enesim_renderer_path_command_clear(thiz->path);
	enesim_renderer_path_move_to(thiz->path, x, y - r);
	enesim_renderer_path_arc_to(thiz->path, r, r, 0, EINA_FALSE, EINA_TRUE, x + r, y);
	enesim_renderer_path_arc_to(thiz->path, r, r, 0, EINA_FALSE, EINA_TRUE, x, y + r);
	enesim_renderer_path_arc_to(thiz->path, r, r, 0, EINA_FALSE, EINA_TRUE, x - r, y);
	enesim_renderer_path_arc_to(thiz->path, r, r, 0, EINA_FALSE, EINA_TRUE, x, y - r);

pass:
	/* pass all the properties to the path */
	enesim_renderer_color_set(thiz->path, cs->color);
	enesim_renderer_transformation_set(thiz->path, &cs->transformation);
	/* base properties */
	enesim_renderer_shape_fill_renderer_set(thiz->path, css->fill.r);
	enesim_renderer_shape_fill_color_set(thiz->path, css->fill.color);
	enesim_renderer_shape_stroke_renderer_set(thiz->path, css->stroke.r);
	enesim_renderer_shape_stroke_weight_set(thiz->path, css->stroke.weight);
	enesim_renderer_shape_stroke_color_set(thiz->path, css->stroke.color);
	enesim_renderer_shape_draw_mode_set(thiz->path, css->draw_mode);
}

static Eina_Bool _circle_path_setup(Enesim_Renderer_Circle *thiz,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Error **error)
{
	const Enesim_Renderer_Shape_State *css = sstates[ENESIM_STATE_CURRENT];
	double rad;

	rad = thiz->current.r;
	if (css->draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE)
	{
		switch (css->stroke.location)
		{
			case ENESIM_SHAPE_STROKE_OUTSIDE:
			rad += css->stroke.weight / 2.0;
			break;

			case ENESIM_SHAPE_STROKE_INSIDE:
			rad -= css->stroke.weight / 2.0;
			break;

			case ENESIM_SHAPE_STROKE_CENTER:
			break;
		}
	}

	_circle_path_propagate(thiz, rad, states, sstates);
	if (!enesim_renderer_setup(thiz->path, s, error))
	{
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

static void _circle_state_cleanup(Enesim_Renderer_Circle *thiz, Enesim_Renderer *r, Enesim_Surface *s)
{
	enesim_renderer_shape_cleanup(r, s);
	enesim_renderer_cleanup(thiz->path, s);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

#if BUILD_OPENGL
static void _circle_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		const Eina_Rectangle *area, int w, int h)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	enesim_renderer_opengl_draw(thiz->path, s, area, w, h);
}
#endif

/*----------------------------------------------------------------------------*
 *                               Span functions                               *
 *----------------------------------------------------------------------------*/
/* Use the internal path for drawing */
static void _circle_path_span(Enesim_Renderer *r,
		const Enesim_Renderer_State *state EINA_UNUSED,
		const Enesim_Renderer_Shape_State *sstate EINA_UNUSED,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	enesim_renderer_sw_draw(thiz->path, x, y, len, ddata);
}


/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _circle_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "circle";
}

/* FIXME we still need to decide what to do with the stroke
 * transformation
 */
static void _circle_bounds(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Circle *thiz;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	const Enesim_Renderer_Shape_State *css = sstates[ENESIM_STATE_CURRENT];
	double sw = 0;

	thiz = _circle_get(r);
	if (css->draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE)
		enesim_renderer_shape_stroke_weight_get(r, &sw);
	switch (css->stroke.location)
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
	rect->x = thiz->current.x - thiz->current.r - sw;
	rect->y = thiz->current.y - thiz->current.r - sw;
	rect->w = rect->h = (thiz->current.r + sw) * 2;

	/* apply the geometry transformation */
	if (cs->transformation_type != ENESIM_MATRIX_IDENTITY)
	{
		Enesim_Quad q;

		enesim_matrix_rectangle_transform(&cs->transformation, rect, &q);
		enesim_quad_rectangle_to(&q, rect);
	}
}

static void _circle_destination_bounds(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Eina_Rectangle *bounds)
{
	Enesim_Rectangle obounds;

	_circle_bounds(r, states, sstates, &obounds);
	bounds->x = floor(obounds.x);
	bounds->y = floor(obounds.y);
	bounds->w = ceil(obounds.x - bounds->x + obounds.w) + 1;
	bounds->h = ceil(obounds.y - bounds->y + obounds.h) + 1;
}

static Eina_Bool _circle_sw_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Shape_Sw_Draw *draw, Enesim_Error **error)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	if (!thiz || (thiz->current.r <= 0))
		return EINA_FALSE;

	if (!_circle_path_setup(thiz, states, sstates, s,error))
		return EINA_FALSE;
	*draw = _circle_path_span;
	return EINA_TRUE;
}

static void _circle_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	_circle_state_cleanup(thiz, r, s);
}

static void _circle_flags(Enesim_Renderer *r EINA_UNUSED, const Enesim_Renderer_State *state EINA_UNUSED,
		Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_AFFINE | ENESIM_RENDERER_FLAG_ARGB8888;
}

static void _circle_hints(Enesim_Renderer *r EINA_UNUSED, const Enesim_Renderer_State *state EINA_UNUSED,
		Enesim_Renderer_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_COLORIZE;
}

static Eina_Bool _circle_has_changed(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES] EINA_UNUSED)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	return _circle_properties_have_changed(thiz);
}

static void _circle_feature_get(Enesim_Renderer *r EINA_UNUSED, Enesim_Shape_Feature *features)
{
	*features = ENESIM_SHAPE_FLAG_FILL_RENDERER | ENESIM_SHAPE_FLAG_STROKE_RENDERER;
}

static void _circle_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	if (thiz->path)
		enesim_renderer_unref(thiz->path);
	free(thiz);
}

#if BUILD_OPENGL
static Eina_Bool _circle_opengl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Error **error)
{
	Enesim_Renderer_Circle *thiz;

 	thiz = _circle_get(r);
	if (!thiz || (thiz->current.r <= 0))
		return EINA_FALSE;
	if (!_circle_path_setup(thiz, states, sstates, s, error))
		return EINA_FALSE;
	*draw = _circle_opengl_draw;
	return EINA_TRUE;
}

static void _circle_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Circle *thiz;

 	thiz = _circle_get(r);
	_circle_state_cleanup(thiz, r, s);
}
#endif

static Enesim_Renderer_Shape_Descriptor _circle_descriptor = {
	/* .name = 			*/ _circle_name,
	/* .free = 			*/ _circle_free,
	/* .bounds = 		*/ _circle_bounds,
	/* .destination_bounds = 	*/ _circle_destination_bounds,
	/* .flags = 			*/ _circle_flags,
	/* .hints_get = 		*/ _circle_hints,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _circle_has_changed,
	/* .feature_get =		*/ _circle_feature_get,
	/* .sw_setup = 			*/ _circle_sw_setup,
	/* .sw_cleanup = 		*/ _circle_sw_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
#if BUILD_OPENGL
	/* .opengl_initialize =         */ NULL,
	/* .opengl_setup =          	*/ _circle_opengl_setup,
	/* .opengl_cleanup =        	*/ _circle_opengl_cleanup,
#else
	/* .opengl_initialize =         */ NULL,
	/* .opengl_setup =          	*/ NULL,
	/* .opengl_cleanup =        	*/ NULL
#endif
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
 * error, this function returns @c NULL.
 */
EAPI Enesim_Renderer * enesim_renderer_circle_new(void)
{
	Enesim_Renderer_Circle *thiz;
	Enesim_Renderer *r;

	thiz = calloc(1, sizeof(Enesim_Renderer_Circle));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_CIRCLE_MAGIC);
	r = enesim_renderer_shape_new(&_circle_descriptor, thiz);
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
