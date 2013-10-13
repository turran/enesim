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
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_gradient_radial.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_renderer_private.h"
#include "enesim_coord_private.h"
#include "enesim_renderer_gradient_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_renderer_gradient_radial

#define ENESIM_RENDERER_GRADIENT_RADIAL(o) ENESIM_OBJECT_INSTANCE_CHECK(o,	\
		Enesim_Renderer_Gradient_Radial,				\
		enesim_renderer_gradient_radial_descriptor_get())

static Enesim_Renderer_Gradient_Sw_Draw _spans[ENESIM_REPEAT_MODES][ENESIM_MATRIX_TYPES];

typedef struct _Enesim_Renderer_Gradient_Radial
{
	Enesim_Renderer_Gradient parent;
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

typedef struct _Enesim_Renderer_Gradient_Radial_Class {
	Enesim_Renderer_Gradient_Class parent;
} Enesim_Renderer_Gradient_Radial_Class;

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

GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, restrict);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, repeat);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, pad);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, reflect);

GRADIENT_AFFINE(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, restrict);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, repeat);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, pad);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, reflect);

GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, restrict);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, repeat);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, pad);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, reflect);
/*----------------------------------------------------------------------------*
 *                The Enesim's gradient renderer interface                    *
 *----------------------------------------------------------------------------*/
static int _radial_length(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	return thiz->glen;
}

static const char * _radial_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "gradient_radial";
}

static void _radial_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Gradient_Radial *thiz;
	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	thiz->changed = EINA_FALSE;
}

static Eina_Bool _radial_sw_setup(Enesim_Renderer *r,
		const Enesim_Renderer_Gradient_State *gstate,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_Gradient_Sw_Draw *draw, Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Gradient_Radial *thiz;
	Enesim_Matrix om;
	Enesim_Matrix m;
	Enesim_Matrix_Type type;
	double cx, cy;
	double fx, fy;
	double rad, scale, small = (1 / 8192.0);
	int glen;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
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

static void _radial_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);

	bounds->x = thiz->center.x - fabs(thiz->radius);
	bounds->y = thiz->center.y - fabs(thiz->radius);
	bounds->w = fabs(thiz->radius) * 2;
	bounds->h = fabs(thiz->radius) * 2;
}

static Eina_Bool _radial_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	if (!thiz->changed)
		return EINA_FALSE;
	/* TODO check if we have really changed */
	return EINA_TRUE;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_GRADIENT_DESCRIPTOR,
		Enesim_Renderer_Gradient_Radial,
		Enesim_Renderer_Gradient_Radial_Class,
		enesim_renderer_gradient_radial);

static void _enesim_renderer_gradient_radial_class_init(void *k)
{
	Enesim_Renderer_Class *r_klass;
	Enesim_Renderer_Gradient_Class *klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _radial_name;

	klass = ENESIM_RENDERER_GRADIENT_CLASS(k);
	klass->length = _radial_length;
	klass->sw_setup = _radial_sw_setup;
	klass->has_changed = _radial_has_changed;
	klass->sw_cleanup = _radial_sw_cleanup;
	klass->bounds_get = _radial_bounds_get;

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

static void _enesim_renderer_gradient_radial_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_renderer_gradient_radial_instance_deinit(void *o EINA_UNUSED)
{
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates radial gradient renderer
 * @return The new renderer
 */
EAPI Enesim_Renderer * enesim_renderer_gradient_radial_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_gradient_radial);
	return r;
}

/**
 * Set the center of a radial gradient renderer
 * @param[in] r The gradient renderer to set the center on
 * @param[in] center_x The X coordinate of the center
 * @param[in] center_y The Y coordinate of the center
 */
EAPI void enesim_renderer_gradient_radial_center_set(Enesim_Renderer *r,
		double center_x, double center_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	thiz->center.x = center_x;
	thiz->center.y = center_y;
	thiz->changed = EINA_TRUE;
}

/**
 * Get the center of a radial gradient renderer
 * @param[in] r The gradient renderer to get the center from
 * @param[out] center_x The pointer to store the X coordinate center
 * @param[out] center_y The pointer to store the Y coordinate center
 */
EAPI void enesim_renderer_gradient_radial_center_get(Enesim_Renderer *r,
		double *center_x, double *center_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	if (center_x)
		*center_x = thiz->center.x;
	if (center_y)
		*center_y = thiz->center.y;
}

EAPI void enesim_renderer_gradient_radial_center_x_set(Enesim_Renderer *r, double center_x)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	thiz->center.x = center_x;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_gradient_radial_center_y_set(Enesim_Renderer *r, double center_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	thiz->center.y = center_y;
	thiz->changed = EINA_TRUE;
}

EAPI double enesim_renderer_gradient_radial_center_x_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	return thiz->center.x;
}

EAPI double enesim_renderer_gradient_radial_center_y_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	return thiz->center.y;
}

EAPI void enesim_renderer_gradient_radial_focus_set(Enesim_Renderer *r, double focus_x, double focus_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	thiz->focus.x = focus_x;
	thiz->focus.y = focus_y;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_gradient_radial_focus_get(Enesim_Renderer *r, 
							double *focus_x, double *focus_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	if (focus_x)
		*focus_x = thiz->focus.x;
	if (focus_y)
		*focus_y = thiz->focus.y;
}
EAPI void enesim_renderer_gradient_radial_focus_x_set(Enesim_Renderer *r, double focus_x)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	thiz->focus.x = focus_x;
	thiz->changed = EINA_TRUE;
}
EAPI void enesim_renderer_gradient_radial_focus_y_set(Enesim_Renderer *r, double focus_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	thiz->focus.y = focus_y;
	thiz->changed = EINA_TRUE;
}
EAPI double enesim_renderer_gradient_radial_focus_x_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	return thiz->focus.x;
}
EAPI double enesim_renderer_gradient_radial_focus_y_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	return thiz->focus.y;
}

EAPI void enesim_renderer_gradient_radial_radius_set(Enesim_Renderer *r, double radius)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	thiz->radius = radius;
	thiz->changed = EINA_TRUE;
}
EAPI double enesim_renderer_gradient_radial_radius_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	return thiz->radius;
}
