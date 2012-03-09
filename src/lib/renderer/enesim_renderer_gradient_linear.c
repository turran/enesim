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
#include "Enesim.h"
#include "enesim_private.h"
#include "private/gradient.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_GRADIENT_LINEAR_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_GRADIENT_LINEAR_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_GRADIENT_LINEAR_MAGIC);\
	} while(0)

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
	EINA_MAGIC
	/* public properties */
	Enesim_Renderer_Gradient_Linear_State current;
	Enesim_Renderer_Gradient_Linear_State past;
	/* private */
	Eina_Bool changed : 1;
	/* generated at state setup */
	Eina_F16p16 fx0, fx1, fy0, fy1;
	Eina_F16p16 ayx, ayy;
	int length;
} Enesim_Renderer_Gradient_Linear;

static inline Enesim_Renderer_Gradient_Linear * _linear_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = enesim_renderer_gradient_data_get(r);
	ENESIM_RENDERER_GRADIENT_LINEAR_MAGIC_CHECK(thiz);

	return thiz;
}

static Eina_F16p16 _linear_distance(Enesim_Renderer_Gradient_Linear *thiz, Eina_F16p16 x,
		Eina_F16p16 y)
{
	Eina_F16p16 a, b;

	a = eina_f16p16_mul(thiz->ayx, (x - thiz->fx0 + 32768));
	b = eina_f16p16_mul(thiz->ayy, (y - thiz->fy0 + 32768));
	return eina_f16p16_sub(eina_f16p16_add(a, b), 32768);
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

	thiz = _linear_get(r);
	enesim_renderer_identity_setup(r, x, y, &xx, &yy);
	d = _linear_distance_internal(thiz, xx, yy);
	while (dst < end)
	{
		*dst++ = enesim_renderer_gradient_color_get(r, d);
		d += thiz->ayx;
	}
}
#endif

GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Linear, _linear_get, _linear_distance, restrict);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Linear, _linear_get, _linear_distance, repeat);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Linear, _linear_get, _linear_distance, pad);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Linear, _linear_get, _linear_distance, reflect);

GRADIENT_AFFINE(Enesim_Renderer_Gradient_Linear, _linear_get, _linear_distance, restrict);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Linear, _linear_get, _linear_distance, repeat);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Linear, _linear_get, _linear_distance, pad);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Linear, _linear_get, _linear_distance, reflect);

GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Linear, _linear_get, _linear_distance, restrict);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Linear, _linear_get, _linear_distance, repeat);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Linear, _linear_get, _linear_distance, pad);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Linear, _linear_get, _linear_distance, reflect);
/*----------------------------------------------------------------------------*
 *                The Enesim's gradient renderer interface                    *
 *----------------------------------------------------------------------------*/
static const char * _linear_name(Enesim_Renderer *r)
{
	return "gradient_linear";
}

static int _linear_length(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = _linear_get(r);
	return thiz->length;
}

static void _linear_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = _linear_get(r);
	thiz->changed = EINA_FALSE;
	thiz->past = thiz->current;
}

static Eina_Bool _linear_state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Gradient_State *gstate,
		Enesim_Surface *s,
		Enesim_Renderer_Gradient_Sw_Draw *draw, Enesim_Error **error)
{
	Enesim_Renderer_Gradient_Linear *thiz;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	double x0, x1, y0, y1;
	Eina_F16p16 xx0, xx1, yy0, yy1;
	Eina_F16p16 f;

	thiz = _linear_get(r);

	x0 = thiz->current.x0;
	x1 = thiz->current.x1;
	y0 = thiz->current.y0;
	y1 = thiz->current.y1;

	/* handle the geometry transformation */
	if (cs->geometry_transformation_type != ENESIM_MATRIX_IDENTITY)
	{
		const Enesim_Matrix *gm = &cs->geometry_transformation;
		enesim_matrix_point_transform(gm, x0, y0, &x0, &y0);
		enesim_matrix_point_transform(gm, x1, y1, &x1, &y1);
	}

	xx0 = eina_f16p16_double_from(x0);
	xx1 = eina_f16p16_double_from(x1);
	yy0 = eina_f16p16_double_from(y0);
	yy1 = eina_f16p16_double_from(y1);

	xx0 = xx1 - xx0;
	yy0 = yy1 - yy0;

	/* we need to use floats because of the limitation of 16.16 values */
	f = eina_f16p16_double_from(hypot(eina_f16p16_double_to(xx0), eina_f16p16_double_to(yy0)));
	f += 32768;
	thiz->ayx = ((int64_t)xx0 << 16) / f;
	thiz->ayy = ((int64_t)yy0 << 16) / f;
	/* TODO check that the difference between x0 - x1 and y0 - y1 is
	 * < tolerance
	 */
	thiz->length = eina_f16p16_int_to(f);
#if 0
	/* just override the identity case */
	if (cs->transformation_type == ENESIM_MATRIX_IDENTITY)
		*fill = _argb8888_pad_span_identity;
#endif
	*draw = _spans[gstate->mode][cs->transformation_type];

	return EINA_TRUE;
}

Eina_Bool _linear_has_changed(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES])
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = _linear_get(r);

	printf("linear changed\n");
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

static Enesim_Renderer_Gradient_Descriptor _linear_descriptor = {
	/* .length = 			*/ _linear_length,
	/* .name = 			*/ _linear_name,
	/* .sw_setup = 			*/ _linear_state_setup,
	/* .sw_cleanup = 		*/ _linear_state_cleanup,
	/* .free = 			*/ NULL,
	/* .boundings = 		*/ NULL,
	/* .destination_boundings = 	*/ NULL,
	/* .is_inside = 		*/ NULL,
	/* .has_changed = 		*/ _linear_has_changed,
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * FIXME
 * To be documented
 */
EAPI Enesim_Renderer * enesim_renderer_gradient_linear_new(void)
{
	Enesim_Renderer_Gradient_Linear *thiz;
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

	thiz = calloc(1, sizeof(Enesim_Renderer_Gradient_Linear));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_GRADIENT_LINEAR_MAGIC);
	r = enesim_renderer_gradient_new(&_linear_descriptor, thiz);
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

	thiz = _linear_get(r);

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

	thiz = _linear_get(r);

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

	thiz = _linear_get(r);
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

	thiz = _linear_get(r);
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

	thiz = _linear_get(r);
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

	thiz = _linear_get(r);
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

	thiz = _linear_get(r);
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

	thiz = _linear_get(r);
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

	thiz = _linear_get(r);
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

	thiz = _linear_get(r);
	if (y1)
		*y1 = thiz->current.y1;
}

