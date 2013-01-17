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
#include "enesim_renderer_ellipse.h"

#include "private/renderer.h"
#include "private/shape.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_ELLIPSE_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_ELLIPSE_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_ELLIPSE_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Ellipse_State {
	double x;
	double y;
	double rx;
	double ry;
} Enesim_Renderer_Ellipse_State;

typedef struct _Enesim_Renderer_Ellipse
{
	EINA_MAGIC
	/* properties */
	Enesim_Renderer_Ellipse_State current;
	Enesim_Renderer_Ellipse_State past;
	/* private */
	Eina_Bool changed : 1;
	/* the path renderer */
	Enesim_Renderer *path;
} Enesim_Renderer_Ellipse;

static inline Enesim_Renderer_Ellipse * _ellipse_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	ENESIM_RENDERER_ELLIPSE_MAGIC_CHECK(thiz);

	return thiz;
}

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

static void _ellipse_path_propagate(Enesim_Renderer_Ellipse *thiz,
		double rx, double ry,
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
	if (!_ellipse_properties_have_changed(thiz))
		goto pass;

	x = thiz->current.x;
	y = thiz->current.y;
	enesim_renderer_path_command_clear(thiz->path);
	enesim_renderer_path_move_to(thiz->path, x, y - ry);
	enesim_renderer_path_arc_to(thiz->path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x + rx, y);
	enesim_renderer_path_arc_to(thiz->path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x, y + ry);
	enesim_renderer_path_arc_to(thiz->path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x - rx, y);
	enesim_renderer_path_arc_to(thiz->path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x, y - ry);

pass:
	enesim_renderer_color_set(thiz->path, cs->color);
	enesim_renderer_origin_set(thiz->path, cs->ox, cs->oy);
	enesim_renderer_geometry_transformation_set(thiz->path, &cs->geometry_transformation);

	enesim_renderer_shape_fill_renderer_set(thiz->path, css->fill.r);
	enesim_renderer_shape_fill_color_set(thiz->path, css->fill.color);
	enesim_renderer_shape_stroke_renderer_set(thiz->path, css->stroke.r);
	enesim_renderer_shape_stroke_weight_set(thiz->path, css->stroke.weight);
	enesim_renderer_shape_stroke_color_set(thiz->path, css->stroke.color);
	enesim_renderer_shape_draw_mode_set(thiz->path, css->draw_mode);
}

static Eina_Bool _ellipse_path_setup(Enesim_Renderer_Ellipse *thiz,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Error **error)
{
	const Enesim_Renderer_Shape_State *css = sstates[ENESIM_STATE_CURRENT];
	double rx, ry;

	rx = thiz->current.rx;
	ry = thiz->current.ry;
	if (css->draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE)
	{
		switch (css->stroke.location)
		{
			case ENESIM_SHAPE_STROKE_OUTSIDE:
			rx += css->stroke.weight / 2.0;
			ry += css->stroke.weight / 2.0;
			break;

			case ENESIM_SHAPE_STROKE_INSIDE:
			rx -= css->stroke.weight / 2.0;
			ry -= css->stroke.weight / 2.0;
			break;

			case ENESIM_SHAPE_STROKE_CENTER:
			break;
		}
	}
	_ellipse_path_propagate(thiz, rx, ry, states, sstates);
	if (!enesim_renderer_setup(thiz->path, s, error))
	{
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

static void _ellipse_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = _ellipse_get(r);
	enesim_renderer_shape_cleanup(r, s);
	enesim_renderer_cleanup(thiz->path, s);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

#if BUILD_OPENGL
static void _ellipse_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		const Eina_Rectangle *area, int w, int h)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = _ellipse_get(r);
	enesim_renderer_opengl_draw(thiz->path, s, area, w, h);
}
#endif
/*----------------------------------------------------------------------------*
 *                               Span functions                               *
 *----------------------------------------------------------------------------*/
/* Use the internal path for drawing */
static void _ellipse_path_span(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = _ellipse_get(r);
	enesim_renderer_sw_draw(thiz->path, x, y, len, ddata);
}


/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _ellipse_name(Enesim_Renderer *r)
{
	return "ellipse";
}

static Eina_Bool _ellipse_sw_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Shape_Sw_Draw *draw, Enesim_Error **error)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = _ellipse_get(r);
	if (!thiz || (thiz->current.rx <= 0) || (thiz->current.ry <= 0))
		return EINA_FALSE;

	if (!_ellipse_path_setup(thiz, states, sstates, s, error))
		return EINA_FALSE;
	*draw = _ellipse_path_span;
	return EINA_TRUE;
}

static void _ellipse_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	_ellipse_state_cleanup(r, s);
}

static void _ellipse_boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Ellipse *thiz;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	const Enesim_Renderer_Shape_State *css = sstates[ENESIM_STATE_CURRENT];
	double sw = 0;

	thiz = _ellipse_get(r);
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
	rect->x = thiz->current.x - thiz->current.rx;
	rect->y = thiz->current.y - thiz->current.ry;
	rect->w = (thiz->current.rx + sw) * 2;
	rect->h = (thiz->current.ry + sw) * 2;
	/* translate by the origin */
	rect->x += cs->ox;
	rect->y += cs->oy;
	/* apply the geometry transformation */
	if (cs->geometry_transformation_type != ENESIM_MATRIX_IDENTITY)
	{
		Enesim_Quad q;

		enesim_matrix_rectangle_transform(&cs->geometry_transformation, rect, &q);
		enesim_quad_rectangle_to(&q, rect);
	}
}

static void _ellipse_destination_boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Eina_Rectangle *boundings)
{
	Enesim_Rectangle oboundings;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];

	_ellipse_boundings(r, states, sstates, &oboundings);
	/* apply the inverse matrix */
	if (cs->transformation_type != ENESIM_MATRIX_IDENTITY)
	{
		Enesim_Quad q;
		Enesim_Matrix m;

		enesim_matrix_inverse(&cs->transformation, &m);
		enesim_matrix_rectangle_transform(&m, &oboundings, &q);
		enesim_quad_rectangle_to(&q, &oboundings);
		/* fix the antialias scaling */
		boundings->x -= m.xx;
		boundings->y -= m.yy;
		boundings->w += m.xx;
		boundings->h += m.yy;
	}
	boundings->x = floor(oboundings.x);
	boundings->y = floor(oboundings.y);
	boundings->w = ceil(oboundings.x - boundings->x + oboundings.w) + 1;
	boundings->h = ceil(oboundings.y - boundings->y + oboundings.h) + 1;
}

static Eina_Bool _ellipse_has_changed(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES])
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = _ellipse_get(r);
	return _ellipse_properties_have_changed(thiz);
}

static void _free(Enesim_Renderer *r)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = _ellipse_get(r);
	if (thiz->path)
		enesim_renderer_unref(thiz->path);
	free(thiz);
}

static void _ellipse_flags(Enesim_Renderer *r, const Enesim_Renderer_State *state,
		Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_ARGB8888 |
			ENESIM_RENDERER_FLAG_GEOMETRY;
}

static void _ellipse_hints(Enesim_Renderer *r, const Enesim_Renderer_State *state,
		Enesim_Renderer_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_COLORIZE;
}

static void _ellipse_feature_get(Enesim_Renderer *r, Enesim_Shape_Feature *features)
{
	*features = ENESIM_SHAPE_FLAG_FILL_RENDERER | ENESIM_SHAPE_FLAG_STROKE_RENDERER;
}

#if BUILD_OPENGL
static Eina_Bool _ellipse_opengl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Error **error)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = _ellipse_get(r);
	if (!thiz || (thiz->current.rx <= 0) || (thiz->current.ry <= 0))
		return EINA_FALSE;
	if (!_ellipse_path_setup(thiz, states, sstates, s, error))
		return EINA_FALSE;
	*draw = _ellipse_opengl_draw;
	return EINA_TRUE;
}

static void _ellipse_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	_ellipse_state_cleanup(r, s);
}
#endif


static Enesim_Renderer_Shape_Descriptor _ellipse_descriptor = {
	/* .name = 			*/ _ellipse_name,
	/* .free = 			*/ _free,
	/* .boundings =  		*/ _ellipse_boundings,
	/* .destination_boundings = 	*/ _ellipse_destination_boundings,
	/* .flags = 			*/ _ellipse_flags,
	/* .hints_get = 		*/ _ellipse_hints,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _ellipse_has_changed,
	/* .feature_get =		*/ _ellipse_feature_get,
	/* .sw_setup = 			*/ _ellipse_sw_setup,
	/* .sw_cleanup = 		*/ _ellipse_sw_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
#if BUILD_OPENGL
	/* .opengl_initialize =         */ NULL,
	/* .opengl_setup =          	*/ _ellipse_opengl_setup,
	/* .opengl_cleanup =        	*/ _ellipse_opengl_cleanup,
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
 * @brief Create a new ellipse renderer.
 *
 * @return A new ellipse renderer.
 *
 * This function returns a newly allocated ellipse renderer. On memory
 * error, this function returns @c NULL.
 */
EAPI Enesim_Renderer * enesim_renderer_ellipse_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Ellipse *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Ellipse));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_ELLIPSE_MAGIC);
	r = enesim_renderer_shape_new(&_ellipse_descriptor, thiz);
	/* to maintain compatibility */
	enesim_renderer_shape_stroke_location_set(r, ENESIM_SHAPE_STROKE_INSIDE);
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

	thiz = _ellipse_get(r);
	if (!thiz) return;
	if ((thiz->current.x == x) && (thiz->current.y == y))
		return;
	thiz->current.x = x;
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
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

	thiz = _ellipse_get(r);
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

	thiz = _ellipse_get(r);
	if (!thiz) return;
	if ((thiz->current.rx == radius_x) && (thiz->current.ry == radius_y))
		return;
	thiz->current.rx = radius_x;
	thiz->current.ry = radius_y;
	thiz->changed = EINA_TRUE;
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

	thiz = _ellipse_get(r);
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
	thiz = _ellipse_get(r);
	thiz->current.x = x;
	thiz->changed = EINA_TRUE;
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
	thiz = _ellipse_get(r);
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
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
	thiz = _ellipse_get(r);
	thiz->current.rx = rx;
	thiz->changed = EINA_TRUE;
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
	thiz = _ellipse_get(r);
	thiz->current.ry = ry;
	thiz->changed = EINA_TRUE;
}
