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
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_gradient_linear.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_renderer_private.h"
#include "enesim_coord_private.h"
#include "enesim_renderer_gradient_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_renderer_gradient

#define ENESIM_RENDERER_GRADIENT_LINEAR(o) ENESIM_OBJECT_INSTANCE_CHECK(o,	\
		Enesim_Renderer_Gradient_Linear,				\
		enesim_renderer_gradient_linear_descriptor_get())

static Enesim_Renderer_Gradient_Sw_Draw _spans[ENESIM_REPEAT_MODES][ENESIM_MATRIX_TYPES];

typedef struct _Enesim_Renderer_Gradient_Linear_State
{
	double x0;
	double x1;
	double y0;
	double y1;
} Enesim_Renderer_Gradient_Linear_State;

typedef struct _Enesim_Renderer_Gradient_Linear
{
	Enesim_Renderer_Gradient parent;
	/* public properties */
	Enesim_Renderer_Gradient_Linear_State current;
	Enesim_Renderer_Gradient_Linear_State past;
	/* private */
	Eina_Bool changed : 1;
	/* generated at state setup */
	Eina_F16p16 xx, yy;
	Eina_F16p16 scale;
	Eina_F16p16 ayx, ayy;
	int length;
} Enesim_Renderer_Gradient_Linear;

typedef struct _Enesim_Renderer_Gradient_Linear_Class {
	Enesim_Renderer_Gradient_Class parent;
} Enesim_Renderer_Gradient_Linear_Class;

/* get the input on origin coordinates and return the distance on destination
 * coordinates
 * FIXME the next iteration will use its own functions for drawing so we will
 * not need to do a transformation twice
 */
static Eina_F16p16 _linear_distance(Enesim_Renderer_Gradient_Linear *thiz, Eina_F16p16 x,
		Eina_F16p16 y)
{
	Eina_F16p16 a, b;
	Eina_F16p16 d;

	/* input is on origin coordinates */
	x = x - thiz->xx;
	y = y - thiz->yy;
	a = eina_f16p16_mul(thiz->ayx, x);
	b = eina_f16p16_mul(thiz->ayy, y);
	d = eina_f16p16_add(a, b);
	/* d is on origin coordinates */
	d = eina_f16p16_mul(d, thiz->scale);

	return d;
}

#if 0
/* This will be enabled later */
static void _argb8888_pad_span_identity(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Gradient_Linear *thiz;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	Eina_F16p16 xx, yy;
	Eina_F16p16 d;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	enesim_coord_identity_setup(r, x, y, &xx, &yy);
	d = _linear_distance_internal(thiz, xx, yy);
	while (dst < end)
	{
		*dst++ = enesim_renderer_gradient_color_get(r, d);
		d += thiz->ayx;
	}
}
#endif

GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, restrict);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, repeat);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, pad);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, reflect);

GRADIENT_AFFINE(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, restrict);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, repeat);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, pad);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, reflect);

GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, restrict);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, repeat);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, pad);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, reflect);
/*----------------------------------------------------------------------------*
 *                The Enesim's gradient renderer interface                    *
 *----------------------------------------------------------------------------*/
static const char * _linear_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "gradient_linear";
}

static int _linear_length(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	return thiz->length;
}

static void _linear_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	thiz->changed = EINA_FALSE;
	thiz->past = thiz->current;
}

static Eina_Bool _linear_state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_Gradient_State *gstate,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_Gradient_Sw_Draw *draw, Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Gradient_Linear *thiz;
	Enesim_Matrix om;
	Enesim_Matrix m;
	Enesim_Matrix_Type type;
	double xx0, yy0;
	double x0, x1, y0, y1;
	double orig_len;
	double dst_len;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);

	x0 = thiz->current.x0;
	x1 = thiz->current.x1;
	y0 = thiz->current.y0;
	y1 = thiz->current.y1;

	enesim_matrix_identity(&m);
	enesim_renderer_transformation_get(r, &om);
	type = enesim_matrix_type_get(&om);

	/* we need to translate by the x0 and y0 */
	thiz->yy = eina_extra_f16p16_double_from(y0);
	thiz->xx = eina_extra_f16p16_double_from(x0);

	/* calculate the increment on x and y */
	xx0 = x1 - x0;
	yy0 = y1 - y0;
	orig_len = hypot(xx0, yy0);
	thiz->ayx = eina_extra_f16p16_double_from(xx0 / orig_len);
	thiz->ayy = eina_extra_f16p16_double_from(yy0 / orig_len);

	/* handle the geometry transformation */
	if (type != ENESIM_MATRIX_IDENTITY)
	{
		enesim_matrix_point_transform(&om, x0, y0, &x0, &y0);
		enesim_matrix_point_transform(&om, x1, y1, &x1, &y1);
		enesim_matrix_inverse(&om, &m);

		dst_len = hypot(x1 - x0, y1 - y0);
	}
	else
	{
		dst_len = orig_len;
	}
	enesim_renderer_transformation_set(r, &m);

	/* TODO check that the difference between x0 - x1 and y0 - y1 is < tolerance */
	thiz->length = ceil(dst_len);
	/* the scale factor, this is useful to know the original length and transformed
	 * length scale
	 */
	thiz->scale = eina_extra_f16p16_double_from(dst_len / orig_len);
#if 0
	/* just override the identity case */
	if (type == ENESIM_MATRIX_IDENTITY)
		*fill = _argb8888_pad_span_identity;
#endif
	*draw = _spans[gstate->mode][type];

	return EINA_TRUE;
}

static Eina_Bool _linear_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);

	if (!thiz->changed)
		return EINA_FALSE;

	if (thiz->current.x0 != thiz->past.x0)
	{
		return EINA_TRUE;
	}
	if (thiz->current.x1 != thiz->past.x1)
	{
		return EINA_TRUE;
	}
	if (thiz->current.y0 != thiz->past.y0)
	{
		return EINA_TRUE;
	}
	if (thiz->current.y1 != thiz->past.y1)
	{
		return EINA_TRUE;
	}
	return EINA_FALSE;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_GRADIENT_DESCRIPTOR,
		Enesim_Renderer_Gradient_Linear,
		Enesim_Renderer_Gradient_Linear_Class,
		enesim_renderer_gradient_linear);

static void _enesim_renderer_gradient_linear_class_init(void *k)
{
	Enesim_Renderer_Class *r_klass;
	Enesim_Renderer_Gradient_Class *klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _linear_name;

	klass = ENESIM_RENDERER_GRADIENT_CLASS(k);
	klass->length = _linear_length;
	klass->sw_setup = _linear_state_setup;
	klass->has_changed = _linear_has_changed;
	klass->sw_cleanup = _linear_state_cleanup;

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

static void _enesim_renderer_gradient_linear_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_renderer_gradient_linear_instance_deinit(void *o EINA_UNUSED)
{
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * FIXME
 * To be documented
 */
EAPI Enesim_Renderer * enesim_renderer_gradient_linear_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_gradient_linear);
	return r;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_pos_set(Enesim_Renderer *r, double x0,
		double y0, double x1, double y1)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);

	thiz->current.x0 = x0;
	thiz->current.x1 = x1;
	thiz->current.y0 = y0;
	thiz->current.y1 = y1;
	thiz->changed = EINA_TRUE;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_pos_get(Enesim_Renderer *r, double *x0,
		double *y0, double *x1, double *y1)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);

	if (x0)
		*x0 = thiz->current.x0;
	if (y0)
		*y0 = thiz->current.y0;
	if (x1)
		*x1 = thiz->current.x1;
	if (y1)
		*y1 = thiz->current.y1;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_x0_set(Enesim_Renderer *r, double x0)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	thiz->current.x0 = x0;
	thiz->changed = EINA_TRUE;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_x0_get(Enesim_Renderer *r, double *x0)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	if (x0)
		*x0 = thiz->current.x0;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_y0_set(Enesim_Renderer *r, double y0)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	thiz->current.y0 = y0;
	thiz->changed = EINA_TRUE;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_y0_get(Enesim_Renderer *r, double *y0)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	if (y0)
		*y0 = thiz->current.y0;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_x1_set(Enesim_Renderer *r, double x1)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	thiz->current.x1 = x1;
	thiz->changed = EINA_TRUE;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_x1_get(Enesim_Renderer *r, double *x1)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	if (x1)
		*x1 = thiz->current.x1;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_y1_set(Enesim_Renderer *r, double y1)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	thiz->current.y1 = y1;
	thiz->changed = EINA_TRUE;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_y1_get(Enesim_Renderer *r, double *y1)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	if (y1)
		*y1 = thiz->current.y1;
}

