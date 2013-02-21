/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2010 Jorge Luis Zapata
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
#include "enesim_renderer_gradient_radial.h"

#include "enesim_renderer_private.h"
#include "enesim_coord_private.h"
#include "enesim_renderer_gradient_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_renderer_gradient_radial

#define ENESIM_RENDERER_GRADIENT_RADIAL_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_GRADIENT_RADIAL_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_GRADIENT_RADIAL_MAGIC);\
	} while(0)

static Enesim_Renderer_Gradient_Sw_Draw _spans[ENESIM_REPEAT_MODES][ENESIM_MATRIX_TYPES];

typedef struct _Enesim_Renderer_Gradient_Radial
{
	EINA_MAGIC
	/* properties */
	struct {
		double x, y;
	} center, focus;
	double radius;
	/* state generated */
	double r, zf;
	double fx, fy;
	double scale;
	int glen;
	Eina_Bool simple : 1;
	Eina_Bool changed : 1;
} Enesim_Renderer_Gradient_Radial;

static inline Enesim_Renderer_Gradient_Radial * _radial_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = enesim_renderer_gradient_data_get(r);
	ENESIM_RENDERER_GRADIENT_RADIAL_MAGIC_CHECK(thiz);

	return thiz;
}

static inline Eina_F16p16 _radial_distance(Enesim_Renderer_Gradient_Radial *thiz,
		Eina_F16p16 x, Eina_F16p16 y)
{
	double r = thiz->r, fx = thiz->fx, fy = thiz->fy;
	Eina_F16p16 ret;
	double a, b;
	double d1, d2;

	if (thiz->simple)
	{
		a = x - 65536 * thiz->center.x;  b = y - 65536 * thiz->center.y;
		ret = thiz->scale * sqrt(a*a + b*b);
		return ret;
	}

	a = thiz->scale * (eina_f16p16_double_to(x) - (fx + thiz->center.x));
	b = thiz->scale * (eina_f16p16_double_to(y) - (fy + thiz->center.y));

	d1 = (a * fy) - (b * fx);
	d2 = fabs(((r * r) * ((a * a) + (b * b))) - (d1 * d1));
	r = ((a * fx) + (b * fy) + sqrt(d2)) * thiz->zf;

	ret = eina_f16p16_double_from(r);
	return ret;
}

GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Radial, _radial_get, _radial_distance, restrict);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Radial, _radial_get, _radial_distance, repeat);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Radial, _radial_get, _radial_distance, pad);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Radial, _radial_get, _radial_distance, reflect);

GRADIENT_AFFINE(Enesim_Renderer_Gradient_Radial, _radial_get, _radial_distance, restrict);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Radial, _radial_get, _radial_distance, repeat);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Radial, _radial_get, _radial_distance, pad);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Radial, _radial_get, _radial_distance, reflect);

GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Radial, _radial_get, _radial_distance, restrict);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Radial, _radial_get, _radial_distance, repeat);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Radial, _radial_get, _radial_distance, pad);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Radial, _radial_get, _radial_distance, reflect);
/*----------------------------------------------------------------------------*
 *                The Enesim's gradient renderer interface                    *
 *----------------------------------------------------------------------------*/
static int _radial_length(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	return thiz->glen;
}

static const char * _radial_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "gradient_radial";
}

static void _state_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Gradient_Radial *thiz;
	thiz = _radial_get(r);
	thiz->changed = EINA_FALSE;
}

static Eina_Bool _state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_Gradient_State *gstate,
		Enesim_Surface *s EINA_UNUSED,
		Enesim_Renderer_Gradient_Sw_Draw *draw, Enesim_Error **error EINA_UNUSED)
{
	Enesim_Renderer_Gradient_Radial *thiz;
	Enesim_Matrix om;
	Enesim_Matrix m;
	Enesim_Matrix_Type type;
	double cx, cy;
	double fx, fy;
	double rad, scale, small = (1 / 8192.0);
	int glen;

	thiz = _radial_get(r);
	cx = thiz->center.x;
	cy = thiz->center.y;
	rad = fabs(thiz->radius);

	if (rad < small)
		return EINA_FALSE;
	thiz->r = rad;

	scale = 1;
	glen = ceil(rad) + 1;

	enesim_matrix_identity(&m);
	enesim_renderer_transformation_get(r, &om);
	type = enesim_matrix_type_get(&om);
	if (type != ENESIM_MATRIX_IDENTITY)
	{
		double mx, my;

		mx = hypot(om.xx, om.yx);
		my = hypot(om.xy, om.yy);
		scale = hypot(mx, my) / sqrt(2);
		glen = ceil(rad * scale) + 1;

		enesim_matrix_inverse(&om, &m);
	}
	enesim_renderer_transformation_set(r, &m);
	DBG("Using a transformation matrix of %" ENESIM_MATRIX_FORMAT,
			ENESIM_MATRIX_ARGS (&m));

	if (glen < 4)
	{
		scale = 3 / rad;
		glen = 4;
	}

	thiz->scale = scale;
	thiz->glen = glen;

	fx = thiz->focus.x;
	fy = thiz->focus.y;
	scale = hypot(fx - cx, fy - cy);
	if (scale + small >= rad)
	{
		double t = rad / (scale + small);

		fx = cx + t * (fx - cx);
		fy = cy + t * (fy - cy);
	}

	fx -= cx;  fy -= cy;
	thiz->fx = fx;  thiz->fy = fy;
	thiz->zf = rad / (rad*rad - (fx*fx + fy*fy));

	thiz->simple = 0;
	if ((fabs(fx) < small) && (fabs(fy) < small))
		thiz->simple = 1;

	*draw = _spans[gstate->mode][type];
	return EINA_TRUE;
}

static void _radial_bounds(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);

	bounds->x = thiz->center.x - fabs(thiz->radius);
	bounds->y = thiz->center.y - fabs(thiz->radius);
	bounds->w = fabs(thiz->radius) * 2;
	bounds->h = fabs(thiz->radius) * 2;
}

static Eina_Bool _radial_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	if (!thiz->changed)
		return EINA_FALSE;
	/* TODO check if we have really changed */
	return EINA_TRUE;
}

static Enesim_Renderer_Gradient_Descriptor _radial_descriptor = {
	/* .length = 			*/ _radial_length,
	/* .sw_setup = 			*/ _state_setup,
	/* .name = 			*/ _radial_name,
	/* .free = 			*/ NULL,
	/* .bounds = 		*/ _radial_bounds,
	/* .destination_bounds = 	*/ NULL,
	/* .is_inside = 		*/ NULL,
	/* .has_changed = 		*/ _radial_has_changed,
	/* .sw_cleanup = 		*/ _state_cleanup,
	/* .opengl_initialize = 	*/ NULL,
	/* .opengl_setup = 		*/ NULL,
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * FIXME
 * To be documented
 */
EAPI Enesim_Renderer * enesim_renderer_gradient_radial_new(void)
{
	Enesim_Renderer_Gradient_Radial *thiz;
	Enesim_Renderer *r;
	static Eina_Bool spans_initialized = EINA_FALSE;

	if (!spans_initialized)
	{
		spans_initialized = EINA_TRUE;
		/* first the span functions */
		_spans[ENESIM_REPEAT][ENESIM_MATRIX_IDENTITY] = _argb8888_repeat_span_identity;
		_spans[ENESIM_REPEAT][ENESIM_MATRIX_AFFINE] = _argb8888_repeat_span_affine;
		_spans[ENESIM_REPEAT][ENESIM_MATRIX_PROJECTIVE] = _argb8888_repeat_span_projective;
		_spans[ENESIM_REFLECT][ENESIM_MATRIX_IDENTITY] = _argb8888_reflect_span_identity;
		_spans[ENESIM_REFLECT][ENESIM_MATRIX_AFFINE] = _argb8888_reflect_span_affine;
		_spans[ENESIM_REFLECT][ENESIM_MATRIX_PROJECTIVE] = _argb8888_reflect_span_projective;
		_spans[ENESIM_RESTRICT][ENESIM_MATRIX_IDENTITY] = _argb8888_restrict_span_identity;
		_spans[ENESIM_RESTRICT][ENESIM_MATRIX_AFFINE] = _argb8888_restrict_span_affine;
		_spans[ENESIM_RESTRICT][ENESIM_MATRIX_PROJECTIVE] = _argb8888_restrict_span_projective;
		_spans[ENESIM_PAD][ENESIM_MATRIX_IDENTITY] = _argb8888_pad_span_identity;
		_spans[ENESIM_PAD][ENESIM_MATRIX_AFFINE] = _argb8888_pad_span_affine;
		_spans[ENESIM_PAD][ENESIM_MATRIX_PROJECTIVE] = _argb8888_pad_span_projective;
	}

	thiz = calloc(1, sizeof(Enesim_Renderer_Gradient_Radial));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_GRADIENT_RADIAL_MAGIC);
	r = enesim_renderer_gradient_new(&_radial_descriptor, thiz);

	return r;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_center_set(Enesim_Renderer *r, double center_x, double center_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	thiz->center.x = center_x;
	thiz->center.y = center_y;
	thiz->changed = EINA_TRUE;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_center_get(Enesim_Renderer *r, 
							double *center_x, double *center_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	if (center_x)
		*center_x = thiz->center.x;
	if (center_y)
		*center_y = thiz->center.y;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_center_x_set(Enesim_Renderer *r, double center_x)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	thiz->center.x = center_x;
	thiz->changed = EINA_TRUE;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_center_y_set(Enesim_Renderer *r, double center_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	thiz->center.y = center_y;
	thiz->changed = EINA_TRUE;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_center_x_get(Enesim_Renderer *r, double *center_x)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	if (center_x)
		*center_x = thiz->center.x;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_center_y_get(Enesim_Renderer *r, double *center_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	if (center_y)
		*center_y = thiz->center.y;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_focus_set(Enesim_Renderer *r, double focus_x, double focus_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	thiz->focus.x = focus_x;
	thiz->focus.y = focus_y;
	thiz->changed = EINA_TRUE;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_focus_get(Enesim_Renderer *r, 
							double *focus_x, double *focus_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	if (focus_x)
		*focus_x = thiz->focus.x;
	if (focus_y)
		*focus_y = thiz->focus.y;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_focus_x_set(Enesim_Renderer *r, double focus_x)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	thiz->focus.x = focus_x;
	thiz->changed = EINA_TRUE;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_focus_y_set(Enesim_Renderer *r, double focus_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	thiz->focus.y = focus_y;
	thiz->changed = EINA_TRUE;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_focus_x_get(Enesim_Renderer *r, double *focus_x)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	if (focus_x)
		*focus_x = thiz->focus.x;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_focus_y_get(Enesim_Renderer *r, double *focus_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	if (focus_y)
		*focus_y = thiz->focus.y;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_radius_set(Enesim_Renderer *r, double radius)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	thiz->radius = radius;
	thiz->changed = EINA_TRUE;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_radius_get(Enesim_Renderer *r, double *radius)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	if (radius)
		*radius = thiz->radius;
}
