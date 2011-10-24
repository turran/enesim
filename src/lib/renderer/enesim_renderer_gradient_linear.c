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
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Renderer_Gradient_Linear
{
	/* public properties */
	double x0, x1, y0, y1;
	/* generated at state setup */
	Eina_F16p16 fx0, fx1, fy0, fy1;
	Eina_F16p16 ayx, ayy;
	int length;
} Enesim_Renderer_Gradient_Linear;

static inline Enesim_Renderer_Gradient_Linear * _linear_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = enesim_renderer_gradient_data_get(r);
	return thiz;
}

static Eina_F16p16 _linear_distance_internal(Enesim_Renderer_Gradient_Linear *thiz, Eina_F16p16 x,
		Eina_F16p16 y)
{
	Eina_F16p16 a, b;

	a = eina_f16p16_mul(thiz->ayx, (x - thiz->fx0 + 32768));
	b = eina_f16p16_mul(thiz->ayy, (y - thiz->fy0 + 32768));
	return eina_f16p16_sub(eina_f16p16_add(a, b), 32768);
}

static void _argb8888_pad_span_identity(Enesim_Renderer *r, int x, int y,
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
/*----------------------------------------------------------------------------*
 *            The Enesim's gradient renderer interface                        *
 *----------------------------------------------------------------------------*/
static const char * _linear_name(Enesim_Renderer *r)
{
	return "linear";
}

static int _linear_length(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = _linear_get(r);
	return thiz->length;
}

static Eina_F16p16 _linear_distance(Enesim_Renderer *r, Eina_F16p16 x,
		Eina_F16p16 y)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = _linear_get(r);
	return _linear_distance_internal(thiz, x, y);
}

static void _linear_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{

}

static Eina_Bool _linear_state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Gradient_Linear *thiz;
	Eina_F16p16 x0, x1, y0, y1;
	Eina_F16p16 f;

	thiz = _linear_get(r);
#define MATRIX 0
	x0 = eina_f16p16_double_from(thiz->x0);
	x1 = eina_f16p16_double_from(thiz->x1);
	y0 = eina_f16p16_double_from(thiz->y0);
	y1 = eina_f16p16_double_from(thiz->y1);
#if MATRIX
	f = eina_f16p16_mul(r->matrix.values.xx, r->matrix.values.yy) -
			eina_f16p16_mul(r->matrix.values.xy, r->matrix.values.yx);
	/* TODO check that (xx * yy) - (xy * yx) < epsilon */
	f = ((int64_t)f << 16) / 65536;
	/* apply the transformation on each point */
	thiz->fx0 = eina_f16p16_mul(r->matrix.values.yy, x0) -
			eina_f16p16_mul(eina_f16p16_mul(r->matrix.values.xy, y0), f) -
			r->matrix.values.xz;
	thiz->fx1 = eina_f16p16_mul(r->matrix.values.yy, x1) -
			eina_f16p16_mul(eina_f16p16_mul(r->matrix.values.xy, y1), f) -
			r->matrix.values.xz;
	thiz->fy0 = eina_f16p16_mul(r->matrix.values.yx, x0) -
			eina_f16p16_mul(eina_f16p16_mul(r->matrix.values.xx, y0), f) -
			r->matrix.values.yz;
	thiz->fy1 = eina_f16p16_mul(r->matrix.values.yx, x1) -
			eina_f16p16_mul(eina_f16p16_mul(r->matrix.values.xx, y1), f) -
			r->matrix.values.yz;
	/* get the length of the transformed points */
	x0 = thiz->fx1 - thiz->fx0;
	y0 = thiz->fy1 - thiz->fy0;
#else
	x0 = x1 - x0;
	y0 = y1 - y0;
#endif

	/* we need to use floats because of the limitation of 16.16 values */
	f = eina_f16p16_double_from(hypot(eina_f16p16_double_to(x0), eina_f16p16_double_to(y0)));
	f += 32768;
	thiz->ayx = ((int64_t)x0 << 16) / f;
	thiz->ayy = ((int64_t)y0 << 16) / f;
	/* TODO check that the difference between x0 - x1 and y0 - y1 is
	 * < tolerance
	 */
	thiz->length = eina_f16p16_int_to(f);
	/* just override the identity case */
	if (state->transformation_type == ENESIM_MATRIX_IDENTITY)
		*fill = _argb8888_pad_span_identity;

	return EINA_TRUE;
}

static Enesim_Renderer_Gradient_Descriptor _linear_descriptor = {
	/* .distance =   */ _linear_distance,
	/* .length =     */ _linear_length,
	/* .name =       */ _linear_name,
	/* .sw_setup =   */ _linear_state_setup,
	/* .sw_cleanup = */ _linear_state_cleanup,
	/* .free =       */ NULL,
	/* .boundings =  */ NULL,
	/* .is_inside =  */ 0
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

	thiz = calloc(1, sizeof(Enesim_Renderer_Gradient_Linear));
	if (!thiz) return NULL;
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

	thiz->x0 = x0;
	thiz->x1 = x1;
	thiz->y0 = y0;
	thiz->y1 = y1;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_x0_set(Enesim_Renderer *r, double x0)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = _linear_get(r);
	thiz->x0 = x0;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_y0_set(Enesim_Renderer *r, double y0)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = _linear_get(r);
	thiz->y0 = y0;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_x1_set(Enesim_Renderer *r, double x1)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = _linear_get(r);
	thiz->x1 = x1;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_y1_set(Enesim_Renderer *r, double y1)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = _linear_get(r);
	thiz->y1 = y1;
}

