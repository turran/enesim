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
typedef struct _Linear
{
	Enesim_Renderer_Gradient base;
	double x0, x1, y0, y1;
	Eina_F16p16 fx0, fx1, fy0, fy1;
	Eina_F16p16 ayx, ayy;
} Linear;

static inline Eina_F16p16 _linear_distance(Linear *l, Eina_F16p16 x,
		Eina_F16p16 y)
{
	Eina_F16p16 a, b;

	a = eina_f16p16_mul(l->ayx, (x - l->fx0 + 32768));
	b = eina_f16p16_mul(l->ayy, (y - l->fy0 + 32768));
	return eina_f16p16_sub(eina_f16p16_add(a, b), 32768);
}

static inline uint32_t _linear_pad(Enesim_Renderer_Gradient *g, Eina_F16p16 p)
{
	int fp;
	uint32_t v;

	fp = eina_f16p16_int_to(p);
	if (fp < 0)
	{
		v = g->src[0];
	}
	else if (fp >= g->slen - 1)
	{
		v = g->src[g->slen - 1];
	}
	else
	{
		uint16_t a;

		a = eina_f16p16_fracc_get(p) >> 8;
		v = argb8888_interp_256(1 + a, g->src[fp + 1], g->src[fp]);
	}

	return v;
}

static void _argb8888_pad_span_affine(Enesim_Renderer *r, int x, int y,
		unsigned int len, uint32_t *dst)
{
	Linear *l = (Linear *)r;
	Enesim_Renderer_Gradient *g = (Enesim_Renderer_Gradient *)l;
	uint32_t *end = dst + len;
	Eina_F16p16 xx, yy;

	renderer_affine_setup(r, x, y, &xx, &yy);
	while (dst < end)
	{
		Eina_F16p16 d;
		uint32_t p0;

		d = _linear_distance(l, xx, yy);
		*dst++ = _linear_pad(g, d);
		yy += r->matrix.values.yx;
		xx += r->matrix.values.xx;
	}
}

static void _argb8888_pad_span_projective(Enesim_Renderer *r, int x, int y,
		unsigned int len, uint32_t *dst)
{
	Linear *l = (Linear *)r;
	Enesim_Renderer_Gradient *g = (Enesim_Renderer_Gradient *)l;
	uint32_t *end = dst + len;
	Eina_F16p16 xx, yy, zz;

	renderer_projective_setup(r, x, y, &xx, &yy, &zz);
	while (dst < end)
	{
		Eina_F16p16 syy, sxx;
		Eina_F16p16 d;
		uint32_t p0;

		syy = ((((int64_t)yy) << 16) / zz);
		sxx = ((((int64_t)xx) << 16) / zz);

		d = _linear_distance(l, sxx, syy);
		*dst++ = _linear_pad(g, d);
		yy += r->matrix.values.yx;
		xx += r->matrix.values.xx;
		zz += r->matrix.values.zx;
	}
}

static void _argb8888_pad_span_identity(Enesim_Renderer *r, int x, int y,
		unsigned int len, uint32_t *dst)
{
	Linear *l = (Linear *)r;
	Enesim_Renderer_Gradient *g = (Enesim_Renderer_Gradient *)l;
	uint32_t *end = dst + len;
	Eina_F16p16 xx, yy;
	Eina_F16p16 d;

	renderer_identity_setup(r, x, y, &xx, &yy);
	d = _linear_distance(l, xx, yy);
	while (dst < end)
	{
		*dst++ = _linear_pad(g, d);
		d += l->ayx;
	}
	/* FIXME is there some mmx bug there? the interp_256 already calls this
	 * but the float support is fucked up
	 */
#ifdef EFL_HAVE_MMX
	_mm_empty();
#endif
}

static void _state_cleanup(Enesim_Renderer *r)
{

}

static Eina_Bool _state_setup(Enesim_Renderer *r, Enesim_Renderer_Sw_Fill *fill)
{
	Linear *l = (Linear *)r;
	Eina_F16p16 x0, x1, y0, y1;
	Eina_F16p16 f;

#define MATRIX 0
	x0 = eina_f16p16_float_from(l->x0);
	x1 = eina_f16p16_float_from(l->x1);
	y0 = eina_f16p16_float_from(l->y0);
	y1 = eina_f16p16_float_from(l->y1);
#if MATRIX
	f = eina_f16p16_mul(r->matrix.values.xx, r->matrix.values.yy) -
			eina_f16p16_mul(r->matrix.values.xy, r->matrix.values.yx);
	/* TODO check that (xx * yy) - (xy * yx) < epsilon */
	f = ((int64_t)f << 16) / 65536;
	/* apply the transformation on each point */
	l->fx0 = eina_f16p16_mul(r->matrix.values.yy, x0) -
			eina_f16p16_mul(eina_f16p16_mul(r->matrix.values.xy, y0), f) -
			r->matrix.values.xz;
	l->fx1 = eina_f16p16_mul(r->matrix.values.yy, x1) -
			eina_f16p16_mul(eina_f16p16_mul(r->matrix.values.xy, y1), f) -
			r->matrix.values.xz;
	l->fy0 = eina_f16p16_mul(r->matrix.values.yx, x0) -
			eina_f16p16_mul(eina_f16p16_mul(r->matrix.values.xx, y0), f) -
			r->matrix.values.yz;
	l->fy1 = eina_f16p16_mul(r->matrix.values.yx, x1) -
			eina_f16p16_mul(eina_f16p16_mul(r->matrix.values.xx, y1), f) -
			r->matrix.values.yz;
	/* get the length of the transformed points */
	x0 = l->fx1 - l->fx0;
	y0 = l->fy1 - l->fy0;
#else
	x0 = x1 - x0;
	y0 = y1 - y0;
#endif

	/* we need to use floats because of the limitation of 16.16 values */
	f = eina_f16p16_float_from(hypot(eina_f16p16_float_to(x0), eina_f16p16_float_to(y0)));
	f += 32768;
	l->ayx = ((int64_t)x0 << 16) / f;
	l->ayy = ((int64_t)y0 << 16) / f;
	/* TODO check that the difference between x0 - x1 and y0 - y1 is
	 * < tolerance
	 */
	enesim_renderer_gradient_state_setup(r, eina_f16p16_int_to(f));
	//*fill = _argb8888_pad_span_identity;
	*fill = _argb8888_pad_span_affine;
	*fill = _argb8888_pad_span_projective;

	return EINA_TRUE;
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
	Linear *l;
	Enesim_Renderer *r;

	l = calloc(1, sizeof(Linear));

	r = (Enesim_Renderer *)l;
	enesim_renderer_gradient_init(r);
	r->sw_setup = _state_setup;
	r->sw_cleanup = _state_cleanup;

	return r;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_pos_set(Enesim_Renderer *r, double x0,
		double y0, double x1, double y1)
{
	Linear *l = (Linear *)r;

	l->x0 = x0;
	l->x1 = x1;
	l->y0 = y0;
	l->y1 = y1;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_x0_set(Enesim_Renderer *r, double x0)
{
	Linear *l = (Linear *)r;

	l->x0 = x0;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_y0_set(Enesim_Renderer *r, double y0)
{
	Linear *l = (Linear *)r;

	l->y0 = y0;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_x1_set(Enesim_Renderer *r, double x1)
{
	Linear *l = (Linear *)r;

	l->x1 = x1;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_linear_y1_set(Enesim_Renderer *r, double y1)
{
	Linear *l = (Linear *)r;

	l->y1 = y1;
}

