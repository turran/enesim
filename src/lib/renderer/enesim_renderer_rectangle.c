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
#include "enesim_eina.h"
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
#include "enesim_renderer_rectangle.h"

#ifdef BUILD_OPENGL
#include "Enesim_OpenGL.h"
#endif

#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_RECTANGLE_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_RECTANGLE_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_RECTANGLE_MAGIC);\
	} while(0)

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
	EINA_MAGIC
	/* public properties */
	Enesim_Renderer_Rectangle_State current;
	Enesim_Renderer_Rectangle_State past;
	/* internal state */
	Eina_Bool changed : 1;
	Eina_Bool use_path : 1;
	/* for the case we use the path renderer */
	Enesim_Renderer *path;
} Enesim_Renderer_Rectangle;


static inline Enesim_Renderer_Rectangle * _rectangle_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	ENESIM_RENDERER_RECTANGLE_MAGIC_CHECK(thiz);

	return thiz;
}

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


static Eina_Bool _rectangle_use_path(Enesim_Matrix_Type geometry_type)
{
	/* Also need to check if coords are int aligned, no corners, etc */
	if (geometry_type != ENESIM_MATRIX_IDENTITY)
		return EINA_TRUE;
	return EINA_FALSE;
}

static void _rectangle_path_propagate(Enesim_Renderer_Rectangle *thiz,
		double x, double y, double w, double h, double rx, double ry,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES])
{
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	const Enesim_Renderer_Shape_State *css = sstates[ENESIM_STATE_CURRENT];
	int count = 0;

	if (!thiz->path)
		thiz->path = enesim_renderer_path_new();
	/* FIXME the best approach would be to know *what* has changed
	 * because we only need to generate the vertices for x,y,w,h
	 * change
	 */
	/* FIXME or prev->geometry_transformation_type == IDENTITY && curr->geometry_transformation_type != IDENTITY */
	if (!_rectangle_properties_have_changed(thiz))
		goto pass;

	enesim_renderer_path_command_clear(thiz->path);

	if (thiz->current.corner.tl && (rx > 0.0) && (ry > 0.0))
	{
		enesim_renderer_path_move_to(thiz->path, x, y + ry);
		enesim_renderer_path_arc_to(thiz->path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x + rx, y);
		count++;
	}
	else
	{
		enesim_renderer_path_move_to(thiz->path, x, y);
		count++;
	}
	if (thiz->current.corner.tr && (rx > 0.0) && (ry > 0.0))
	{
		enesim_renderer_path_line_to(thiz->path, x + w - rx, y);
		enesim_renderer_path_arc_to(thiz->path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x + w, y + ry);
		count++;
	}
	else
	{
		enesim_renderer_path_line_to(thiz->path, x + w, y);
		count++;
	}
	if (thiz->current.corner.br && (rx > 0.0) && (ry > 0.0))
	{
		enesim_renderer_path_line_to(thiz->path, x + w, y + h - ry);
		enesim_renderer_path_arc_to(thiz->path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x + w - rx, y + h);
		count++;
	}
	else
	{
		enesim_renderer_path_line_to(thiz->path, x + w, y + h);
		count++;
	}
	if (thiz->current.corner.bl && (rx > 0.0) && (ry > 0.0))
	{
		enesim_renderer_path_line_to(thiz->path, x + rx, y + h);
		enesim_renderer_path_arc_to(thiz->path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x, y + h - ry);
		count++;
	}
	else
	{
		enesim_renderer_path_line_to(thiz->path, x, y + h);
		count++;
	}
	enesim_renderer_path_close(thiz->path, EINA_TRUE);

pass:
	/* pass all the properties to the path */
	enesim_renderer_color_set(thiz->path, cs->color);
	enesim_renderer_origin_set(thiz->path, cs->ox, cs->oy);
	enesim_renderer_geometry_transformation_set(thiz->path, &cs->geometry_transformation);
	/* base properties */
	enesim_renderer_shape_fill_renderer_set(thiz->path, css->fill.r);
	enesim_renderer_shape_fill_color_set(thiz->path, css->fill.color);
	enesim_renderer_shape_stroke_renderer_set(thiz->path, css->stroke.r);
	enesim_renderer_shape_stroke_weight_set(thiz->path, css->stroke.weight);
	enesim_renderer_shape_stroke_color_set(thiz->path, css->stroke.color);
	enesim_renderer_shape_draw_mode_set(thiz->path, css->draw_mode);
}

static Eina_Bool _rectangle_path_setup(Enesim_Renderer_Rectangle *thiz,
		double x, double y, double w, double h, double rx, double ry,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Error **error)
{
	const Enesim_Renderer_Shape_State *css = sstates[ENESIM_STATE_CURRENT];
	double sw;

	sw = css->stroke.weight;
	/* in case of a stroked rectangle we need to convert the location of the stroke to center */
	if (css->draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE)
	{
		switch (css->stroke.location)
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
	_rectangle_path_propagate(thiz, x, y, w, h, rx, ry, states, sstates);
	if (!enesim_renderer_setup(thiz->path, s, error))
	{
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

static void _rectangle_state_cleanup(Enesim_Renderer_Rectangle *thiz,
		Enesim_Renderer *r, Enesim_Surface *s)
{
	enesim_renderer_shape_cleanup(r, s);
	/* check if we should use the path approach */
	if (thiz->path)
	{
		enesim_renderer_cleanup(thiz->path, s);
	}
	thiz->past = thiz->current;
	thiz->use_path = EINA_FALSE;
	thiz->changed = EINA_FALSE;
}

#if BUILD_OPENGL
static void _rectangle_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		const Eina_Rectangle *area, int w, int h)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	enesim_renderer_opengl_draw(thiz->path, s, area, w, h);
}
#endif


/*----------------------------------------------------------------------------*
 *                               Span functions                               *
 *----------------------------------------------------------------------------*/
/* Use the internal path for drawing */
static void _rectangle_path_span(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	enesim_renderer_sw_draw(thiz->path, x, y, len, ddata);
}

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _rectangle_name(Enesim_Renderer *r)
{
	return "rectangle";
}

static inline Eina_Bool _rectangle_generate_geometry(Enesim_Renderer_Rectangle *thiz,
		Enesim_Renderer *r,
		const Enesim_Renderer_State *cs,
		double *dx, double *dy, double *dw, double *dh, double *drx, double *dry,
		Enesim_Error **error)
{
	double rx, ry;
	double x;
	double y;
	double w;
	double h;

	w = thiz->current.width * cs->sx;
	h = thiz->current.height * cs->sy;
	if ((w < 1) || (h < 1))
	{
		ENESIM_RENDERER_ERROR(r, error, "Invalid size %g %g", w, h);
		return EINA_FALSE;
	}

	x = thiz->current.x * cs->sx;
	y = thiz->current.y * cs->sy;

	rx = thiz->current.corner.rx;
	if (rx > (w / 2.0))
		rx = w / 2.0;
	ry = thiz->current.corner.ry;
	if (ry > (h / 2.0))
		ry = h / 2.0;

	*dx = x;
	*dy = y;
	*dw = w;
	*dh = h;
	*drx = rx;
	*dry = ry;
	return EINA_TRUE;
}

static Eina_Bool _rectangle_sw_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Shape_Sw_Draw *draw, Enesim_Error **error)
{
	Enesim_Renderer_Rectangle *thiz;
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Renderer *spaint;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	const Enesim_Renderer_Shape_State *css = sstates[ENESIM_STATE_CURRENT];
	double rx, ry;
	double x;
	double y;
	double w;
	double h;

	thiz = _rectangle_get(r);
	if (!thiz)
	{
		return EINA_FALSE;
	}

	if (!_rectangle_generate_geometry(thiz, r, cs, &x, &y, &w, &h, &rx, &ry, error))
		return EINA_FALSE;

	_rectangle_path_setup(thiz, x, y, w, h, rx, ry, states, sstates, s, error);
	*draw = _rectangle_path_span;

#if 0
	/* TODO: check if we should use the path approach or a simple rect */
	thiz->use_path = _rectangle_use_path(cs->geometry_transformation_type);
	if (thiz->use_path)
	{
		_rectangle_path_setup(thiz, x, y, w, h, rx, ry, states, sstates, s, error);
		*draw = _rectangle_path_span;

		return EINA_TRUE;
	}
	else
	{

	}
#endif
	return EINA_TRUE;
}

static void _rectangle_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	_rectangle_state_cleanup(thiz, r, s);
}

static void _rectangle_flags(Enesim_Renderer *r, const Enesim_Renderer_State *state,
		Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_SCALE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_GEOMETRY |
			ENESIM_RENDERER_FLAG_ARGB8888;
}

static void _rectangle_hints(Enesim_Renderer *r, const Enesim_Renderer_State *state,
		Enesim_Renderer_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_COLORIZE;
}

static void _rectangle_feature_get(Enesim_Renderer *r, Enesim_Shape_Feature *features)
{
	*features = ENESIM_SHAPE_FLAG_FILL_RENDERER | ENESIM_SHAPE_FLAG_STROKE_RENDERER;
}

static void _rectangle_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	if (thiz->path)
		enesim_renderer_unref(thiz->path);
	free(thiz);
}

static void _rectangle_boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *boundings)
{
	Enesim_Renderer_Rectangle *thiz;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	const Enesim_Renderer_Shape_State *css = sstates[ENESIM_STATE_CURRENT];
	double x, y, w, h;

	thiz = _rectangle_get(r);

	/* first scale */
	x = thiz->current.x * cs->sx;
	y = thiz->current.y * cs->sy;
	w = thiz->current.width * cs->sx;
	h = thiz->current.height * cs->sy;
	/* for the stroke location get the real width */
	if (css->draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE)
	{
		double sw = css->stroke.weight;
		switch (css->stroke.location)
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

	boundings->x = x;
	boundings->y = y;
	boundings->w = w;
	boundings->h = h;

	/* translate by the origin */
	boundings->x += cs->ox;
	boundings->y += cs->oy;
	/* apply the geometry transformation */
	if (cs->geometry_transformation_type != ENESIM_MATRIX_IDENTITY)
	{
		Enesim_Quad q;

		enesim_matrix_rectangle_transform(&cs->geometry_transformation, boundings, &q);
		enesim_quad_rectangle_to(&q, boundings);
	}
}

static void _rectangle_destination_boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Eina_Rectangle *boundings)
{
	Enesim_Rectangle oboundings;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];

	_rectangle_boundings(r, states, sstates, &oboundings);
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

static Eina_Bool _rectangle_has_changed(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES])
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	return _rectangle_properties_have_changed(thiz);
}

#if BUILD_OPENGL
/* TODO instead of trying to do our own geometry shader it might be better
 * to use the path renderer and only create a shader there, this will simplify the code
 * given that at the end we'll generate vertices too
 */
static Eina_Bool _rectangle_opengl_initialize(Enesim_Renderer *r,
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***programs)
{
#if 0
	*num_shaders = 2;
	*shaders = shader = calloc(*num_shaders, sizeof(Enesim_Renderer_OpenGL_Shader));
	shader->type = ENESIM_SHADER_GEOMETRY;
	shader->name = "rectangle";
		//"#version 150\n"
	shader->source = 
		"#version 120\n"
		"#extension GL_EXT_geometry_shader4 : enable\n"
	#include "enesim_renderer_rectangle.glsl"

	/* FIXME for now we should use the background renderer for the color */
	/* if we have a renderer set use that one but render into another texture
	 * if it has some geometric shader
	 */
	shader++;
	shader->type = ENESIM_SHADER_FRAGMENT;
	shader->name = "stripes";
	shader->source = 
	#include "enesim_renderer_stripes.glsl"
	;
#endif
	return EINA_TRUE;
}

static Eina_Bool _rectangle_opengl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Error **error)
{
	Enesim_Renderer_Rectangle *thiz;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	double rx, ry;
	double x;
	double y;
	double w;
	double h;

 	thiz = _rectangle_get(r);

	thiz->use_path = EINA_TRUE;
	if (!_rectangle_generate_geometry(thiz, r, cs, &x, &y, &w, &h, &rx, &ry, error))
		return EINA_FALSE;
	_rectangle_path_setup(thiz, x, y, w, h, rx, ry, states, sstates, s, error);
	*draw = _rectangle_opengl_draw;
	return EINA_TRUE;
}

static void _rectangle_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Rectangle *thiz;

 	thiz = _rectangle_get(r);
	_rectangle_state_cleanup(thiz, r, s);
}
#endif

static Enesim_Renderer_Shape_Descriptor _rectangle_descriptor = {
	/* .name = 			*/ _rectangle_name,
	/* .free = 			*/ _rectangle_free,
	/* .boundings = 		*/ _rectangle_boundings,
	/* .destination_boundings = 	*/ _rectangle_destination_boundings,
	/* .flags = 			*/ _rectangle_flags,
	/* .hints_get = 			*/ _rectangle_hints,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _rectangle_has_changed,
	/* .feature_get =		*/ _rectangle_feature_get,
	/* .sw_setup = 			*/ _rectangle_sw_setup,
	/* .sw_cleanup = 		*/ _rectangle_sw_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
#if BUILD_OPENGL
	/* .opengl_initialize =         */ _rectangle_opengl_initialize,
	/* .opengl_setup =          	*/ _rectangle_opengl_setup,
	/* .opengl_cleanup =        	*/ _rectangle_opengl_cleanup,
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
 * @brief Create a new rectangle renderer.
 *
 * @return A new rectangle renderer.
 *
 * This function returns a newly allocated rectangle renderer. On memory
 * error, this function returns @c NULL.
 */
EAPI Enesim_Renderer * enesim_renderer_rectangle_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Rectangle *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Rectangle));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_RECTANGLE_MAGIC);
	r = enesim_renderer_shape_new(&_rectangle_descriptor, thiz);
	/* to maintain compatibility */
	enesim_renderer_shape_stroke_location_set(r, ENESIM_SHAPE_STROKE_INSIDE);
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

	thiz = _rectangle_get(r);
	thiz->current.width = width;
	thiz->changed = EINA_TRUE;
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

	thiz = _rectangle_get(r);
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

	thiz = _rectangle_get(r);
	thiz->current.height = height;
	thiz->changed = EINA_TRUE;
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

	thiz = _rectangle_get(r);
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

	thiz = _rectangle_get(r);
	thiz->current.x = x;
	thiz->changed = EINA_TRUE;
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

	thiz = _rectangle_get(r);
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

	thiz = _rectangle_get(r);
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
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

	thiz = _rectangle_get(r);
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
	thiz = _rectangle_get(r);
	thiz->current.x = x;
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
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

	thiz = _rectangle_get(r);
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
	thiz = _rectangle_get(r);
	thiz->current.width = width;
	thiz->current.height = height;
	thiz->changed = EINA_TRUE;
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

	thiz = _rectangle_get(r);
	if (width) *width = thiz->current.width;
	if (height) *height = thiz->current.height;
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
	thiz = _rectangle_get(r);
	if (!thiz) return;
	if ((rx < 0) || (ry < 0))
	{ rx = 0;  ry = 0; }
	if ((thiz->current.corner.rx == rx) && (thiz->current.corner.ry == ry))
		return;
	thiz->current.corner.rx = rx;
	thiz->current.corner.ry = ry;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_rectangle_corner_radii_get(Enesim_Renderer *r, double *rx, double *ry)
{
	Enesim_Renderer_Rectangle *thiz;
	thiz = _rectangle_get(r);
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

	thiz = _rectangle_get(r);
	thiz->current.corner.tl = rounded;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_rectangle_top_left_corner_get(Enesim_Renderer *r, Eina_Bool *rounded)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
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

	thiz = _rectangle_get(r);
	thiz->current.corner.tr = rounded;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_rectangle_top_right_corner_get(Enesim_Renderer *r, Eina_Bool *rounded)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
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

	thiz = _rectangle_get(r);
	thiz->current.corner.bl = rounded;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_rectangle_bottom_left_corner_get(Enesim_Renderer *r, Eina_Bool *rounded)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
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

	thiz = _rectangle_get(r);
	thiz->current.corner.br = rounded;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_rectangle_bottom_right_corner_get(Enesim_Renderer *r, Eina_Bool *rounded)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
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

	thiz = _rectangle_get(r);
	if (!thiz) return;

	if ((thiz->current.corner.tl == tl) && (thiz->current.corner.tr == tr) &&
	     (thiz->current.corner.bl == bl) && (thiz->current.corner.br == br))
		return;
	thiz->current.corner.tl = tl;  thiz->current.corner.tr = tr;
	thiz->current.corner.bl = bl;  thiz->current.corner.br = br;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_rectangle_corners_get(Enesim_Renderer *r,
		Eina_Bool *tl, Eina_Bool *tr, Eina_Bool *bl, Eina_Bool *br)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
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
