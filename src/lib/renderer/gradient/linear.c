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
	double x0, x1, y0, y1;
	Eina_F16p16 fx0, fx1, fy0, fy1;
	Eina_F16p16 ayx, ayy;
} Enesim_Renderer_Gradient_Linear;

static inline Enesim_Renderer_Gradient_Linear * _linear_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = enesim_renderer_gradient_data_get(r);
	return thiz;
}

static inline Eina_F16p16 _linear_distance(Enesim_Renderer_Gradient_Linear *thiz, Eina_F16p16 x,
		Eina_F16p16 y)
{
	Eina_F16p16 a, b;

	a = eina_f16p16_mul(thiz->ayx, (x - thiz->fx0 + 32768));
	b = eina_f16p16_mul(thiz->ayy, (y - thiz->fy0 + 32768));
	return eina_f16p16_sub(eina_f16p16_add(a, b), 32768);
}

static inline uint32_t _linear_pad(Enesim_Renderer *r, Eina_F16p16 p)
{
	int fp;
	uint32_t v;
	uint32_t *data;
	int data_length;

	enesim_renderer_gradient_pixels_get(r, &data, &data_length);
	fp = eina_f16p16_int_to(p);
	if (fp < 0)
	{
		v = data[0];
	}
	else if (fp >= data_length - 1)
	{
		v = data[data_length - 1];
	}
	else
	{
		uint16_t a;

		a = eina_f16p16_fracc_get(p) >> 8;
		v = argb8888_interp_256(1 + a, data[fp + 1], data[fp]);
	}

	return v;
}

static void _argb8888_pad_span_affine(Enesim_Renderer *r, int x, int y,
		unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Gradient_Linear *thiz;
	uint32_t *end = dst + len;
	Eina_F16p16 xx, yy;

	thiz = _linear_get(r);
	renderer_affine_setup(r, x, y, &xx, &yy);
	while (dst < end)
	{
		Eina_F16p16 d;
		uint32_t p0;

		d = _linear_distance(thiz, xx, yy);
		*dst++ = _linear_pad(r, d);
		yy += r->matrix.values.yx;
		xx += r->matrix.values.xx;
	}
}

static void _argb8888_pad_span_projective(Enesim_Renderer *r, int x, int y,
		unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Gradient_Linear *thiz;
	uint32_t *end = dst + len;
	Eina_F16p16 xx, yy, zz;

	thiz = _linear_get(r);
	renderer_projective_setup(r, x, y, &xx, &yy, &zz);
	while (dst < end)
	{
		Eina_F16p16 syy, sxx;
		Eina_F16p16 d;
		uint32_t p0;

		syy = ((((int64_t)yy) << 16) / zz);
		sxx = ((((int64_t)xx) << 16) / zz);

		d = _linear_distance(thiz, sxx, syy);
		*dst++ = _linear_pad(r, d);
		yy += r->matrix.values.yx;
		xx += r->matrix.values.xx;
		zz += r->matrix.values.zx;
	}
}

static void _argb8888_pad_span_identity(Enesim_Renderer *r, int x, int y,
		unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Gradient_Linear *thiz;
	uint32_t *end = dst + len;
	Eina_F16p16 xx, yy;
	Eina_F16p16 d;

	thiz = _linear_get(r);
	renderer_identity_setup(r, x, y, &xx, &yy);
	d = _linear_distance(thiz, xx, yy);
	while (dst < end)
	{
		*dst++ = _linear_pad(r, d);
		d += thiz->ayx;
	}
	/* FIXME is there some mmx bug there? the interp_256 already calls this
	 * but the float support is fucked up
	 */
#ifdef EFL_HAVE_MMX
	_mm_empty();
#endif
}

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static void _state_cleanup(Enesim_Renderer *r)
{

}

static Eina_Bool _state_setup(Enesim_Renderer *r, Enesim_Renderer_Sw_Fill *fill)
{
	Enesim_Renderer_Gradient_Linear *thiz;
	Eina_F16p16 x0, x1, y0, y1;
	Eina_F16p16 f;

	thiz = _linear_get(r);
#define MATRIX 0
	x0 = eina_f16p16_float_from(thiz->x0);
	x1 = eina_f16p16_float_from(thiz->x1);
	y0 = eina_f16p16_float_from(thiz->y0);
	y1 = eina_f16p16_float_from(thiz->y1);
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
	f = eina_f16p16_float_from(hypot(eina_f16p16_float_to(x0), eina_f16p16_float_to(y0)));
	f += 32768;
	thiz->ayx = ((int64_t)x0 << 16) / f;
	thiz->ayy = ((int64_t)y0 << 16) / f;
	/* TODO check that the difference between x0 - x1 and y0 - y1 is
	 * < tolerance
	 */
	enesim_renderer_gradient_state_setup(r, eina_f16p16_int_to(f));
	switch (r->matrix.type)
	{
		case ENESIM_MATRIX_IDENTITY:
		*fill = _argb8888_pad_span_identity;
		break;

		case ENESIM_MATRIX_AFFINE:
		*fill = _argb8888_pad_span_affine;
		break;

		case ENESIM_MATRIX_PROJECTIVE:
		*fill = _argb8888_pad_span_projective;
		break;
	}

	return EINA_TRUE;
}

static Enesim_Renderer_Descriptor _linear_descriptor = {
	/* .sw_setup =   */ _state_setup,
	/* .sw_cleanup = */ _state_cleanup,
	/* .free =       */ NULL,
	/* .boundings =  */ NULL,
	/* .flags =      */ NULL,
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

