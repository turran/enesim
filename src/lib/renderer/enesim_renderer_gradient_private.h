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
#ifndef GRADIENT_H_
#define GRADIENT_H_

#include "libargb.h"

#define ENESIM_RENDERER_GRADIENT_DESCRIPTOR enesim_renderer_gradient_descriptor_get()
#define ENESIM_RENDERER_GRADIENT_CLASS(k) ENESIM_OBJECT_CLASS_CHECK(k,		\
		Enesim_Renderer_Gradient_Class,					\
		ENESIM_RENDERER_GRADIENT_DESCRIPTOR)
#define ENESIM_RENDERER_GRADIENT_CLASS_GET(o) ENESIM_RENDERER_GRADIENT_CLASS(	\
		ENESIM_OBJECT_INSTANCE_CLASS(o));
#define ENESIM_RENDERER_GRADIENT(o) ENESIM_OBJECT_INSTANCE_CHECK(o, 		\
		Enesim_Renderer_Gradient, ENESIM_RENDERER_GRADIENT_DESCRIPTOR)

/* helper functions for different spread modes */
static inline uint32_t enesim_renderer_gradient_pad_color_get(Enesim_Color *src, int len, Eina_F16p16 p)
{
	int fp;
	uint32_t v;

	fp = eina_f16p16_int_to(p);
	if (fp < 0)
	{
		v = src[0];
	}
	else if (fp >= len - 1)
	{
		v = src[len - 1];
	}
	else
	{
		uint16_t a;

		a = eina_f16p16_fracc_get(p) >> 8;
		v = argb8888_interp_256(1 + a, src[fp + 1], src[fp]);
	}

	return v;
}

static inline uint32_t enesim_renderer_gradient_restrict_color_get(Enesim_Color *src, int len, Eina_F16p16 p)
{
	int fp;
	uint32_t v;

	fp = eina_f16p16_int_to(p);
	if (fp < 0)
	{
		v = 0;
		if (p >= -EINA_F16P16_ONE)
		{
			uint16_t a;

			a = eina_f16p16_fracc_get(p) >> 8;
			v = argb8888_interp_256(1 + a, src[0], 0);
		}
	}
	else if (fp >= len - 1)
	{
		Eina_F16p16 slen = eina_f16p16_int_from(len - 1);

		v = 0;
		if (p - slen <= EINA_F16P16_ONE)
		{
			uint16_t a;

			a = eina_f16p16_fracc_get(p - slen) >> 8;
			v = argb8888_interp_256(1 + a, 0, src[len - 1]);
		}
	}
	else
	{
		uint16_t a;

		a = eina_f16p16_fracc_get(p) >> 8;
		v = argb8888_interp_256(1 + a, src[fp + 1], src[fp]);
	}

	return v;
}

static inline uint32_t enesim_renderer_gradient_reflect_color_get(Enesim_Color *src, int len, Eina_F16p16 p)
{
	int fp;
	int fp_next;
	uint32_t v;
	uint16_t a;

	fp = eina_f16p16_int_to(p);
	fp = fp % (2 * len);
	if (fp < 0) fp += 2 * len;
	if (fp >= len) fp = (2 * len) - fp - 1;
	fp_next = (fp < (len - 1) ? (fp + 1) : len - 1);

	a = eina_f16p16_fracc_get(p) >> 8;
	v = argb8888_interp_256(1 + a, src[fp_next], src[fp]);

	return v;
}

static inline uint32_t enesim_renderer_gradient_repeat_color_get(Enesim_Color *src, int len, Eina_F16p16 p)
{
	int fp;
	int fp_next;
	uint32_t v;
	uint16_t a;

	fp = eina_f16p16_int_to(p);
	if ((fp > len - 1) || (fp < 0))
	{
		fp = fp % len;
		if (fp < 0)
			fp += len;
	}
	fp_next = (fp < (len - 1) ? fp + 1 : 0);

	a = eina_f16p16_fracc_get(p) >> 8;
	v = argb8888_interp_256(1 + a, src[fp_next], src[fp]);

	return v;
}

/* helper macros to draw gradients */
#define GRADIENT_PROJECTIVE(type, type_get, distance, mode) \
static void _argb8888_##mode##_span_projective(Enesim_Renderer *r,	\
		int x, int y, int len, void *ddata)			\
{									\
	type *thiz;							\
	Enesim_Renderer_Gradient *g;					\
	Eina_F16p16 xx, yy, zz;						\
	uint32_t *dst = ddata;						\
	uint32_t *end = dst + len;					\
	double ox, oy;							\
									\
	thiz = type_get(r);						\
									\
	g = ENESIM_RENDERER_GRADIENT(r);				\
	if (g->sw.do_mask)						\
		enesim_renderer_sw_draw(g->sw.mask, x, y, len, dst);	\
	ox = r->state.current.ox;					\
	oy = r->state.current.oy;					\
	enesim_coord_projective_setup(&xx, &yy, &zz, x, y, ox, oy,	\
			&thiz->sw.matrix);				\
	while (dst < end)						\
	{								\
		Eina_F16p16 syy, sxx;					\
		Eina_F16p16 d;						\
		uint32_t p0;						\
		int ma = 255;						\
									\
		if (g->sw.do_mask)					\
		{							\
			ma = (*dst) >> 24;				\
			if (!ma)					\
				goto next;				\
		}							\
		syy = ((((int64_t)yy) << 16) / zz);			\
		sxx = ((((int64_t)xx) << 16) / zz);			\
									\
		d = distance(thiz, sxx, syy);				\
		p0 = enesim_renderer_gradient_##mode##_color_get(	\
				g->sw.src, g->sw.len, d);		\
		if (ma < 255)						\
			p0 = argb8888_mul_sym(ma, p0);			\
		*dst = p0;						\
next:									\
		dst++;							\
		yy += thiz->sw.matrix.yx;				\
		xx += thiz->sw.matrix.xx;				\
		zz += thiz->sw.matrix.zx;				\
	}								\
}

#define GRADIENT_IDENTITY(type, type_get, distance, mode) \
static void _argb8888_##mode##_span_identity(Enesim_Renderer *r,	\
		int x, int y, int len, void *ddata)			\
{									\
	type *thiz;							\
	Enesim_Renderer_Gradient *g;					\
	Eina_F16p16 xx, yy;						\
	uint32_t *dst = ddata;						\
	uint32_t *end = dst + len;					\
	double ox, oy;							\
									\
	thiz = type_get(r);						\
									\
	g = ENESIM_RENDERER_GRADIENT(r);				\
	if (g->sw.do_mask)						\
		enesim_renderer_sw_draw(g->sw.mask, x, y, len, dst);	\
	ox = r->state.current.ox;					\
	oy = r->state.current.oy;					\
	enesim_coord_identity_setup(&xx, &yy, x, y, ox, oy);		\
	while (dst < end)						\
	{								\
		Eina_F16p16 d;						\
		uint32_t p0;						\
		int ma = 255;						\
									\
		if (g->sw.do_mask)					\
		{							\
			ma = (*dst) >> 24;				\
			if (!ma)					\
				goto next;				\
		}							\
		d = distance(thiz, xx, yy);				\
		p0 = enesim_renderer_gradient_##mode##_color_get(	\
				g->sw.src, g->sw.len, d);		\
		if (ma < 255)						\
			p0 = argb8888_mul_sym(ma, p0);			\
		*dst = p0;						\
next:									\
		dst++;							\
		xx += EINA_F16P16_ONE;					\
	}								\
}

#define GRADIENT_AFFINE(type, type_get, distance, mode) \
static void _argb8888_##mode##_span_affine(Enesim_Renderer *r,		\
 		int x, int y, int len, void *ddata)			\
{									\
	type *thiz;							\
	Enesim_Renderer_Gradient *g;					\
	Eina_F16p16 xx, yy;						\
	uint32_t *dst = ddata;						\
	uint32_t *end = dst + len;					\
	double ox, oy;							\
									\
	thiz = type_get(r);						\
									\
	g = ENESIM_RENDERER_GRADIENT(r);				\
	if (g->sw.do_mask)						\
		enesim_renderer_sw_draw(g->sw.mask, x, y, len, dst);	\
	ox = r->state.current.ox;					\
	oy = r->state.current.oy;					\
	enesim_coord_affine_setup(&xx, &yy, x, y, ox, oy, 		\
			&thiz->sw.matrix);				\
	while (dst < end)						\
	{								\
		Eina_F16p16 d;						\
		uint32_t p0;						\
		int ma = 255;						\
									\
		if (g->sw.do_mask)					\
		{							\
			ma = (*dst) >> 24;				\
			if (!ma)					\
				goto next;				\
		}							\
									\
		d = distance(thiz, xx, yy);				\
		p0 = enesim_renderer_gradient_##mode##_color_get(	\
				g->sw.src, g->sw.len, d);		\
		if (ma < 255)						\
			p0 = argb8888_mul_sym(ma, p0);			\
		*dst = p0;						\
next:									\
		dst++;							\
		yy += thiz->sw.matrix.yx;				\
		xx += thiz->sw.matrix.xx;				\
	}								\
}

/* common gradient renderer functions */
typedef struct _Enesim_Renderer_Gradient_State
{
	Enesim_Repeat_Mode mode;
	Eina_List *stops;
} Enesim_Renderer_Gradient_State;

typedef struct _Enesim_Renderer_Gradient_Sw_State
{
	Enesim_Color *src;
	int len;
	Enesim_Renderer *mask;
	Eina_Bool do_mask;
} Enesim_Renderer_Gradient_Sw_State;

typedef struct _Enesim_Renderer_Gradient
{
	Enesim_Renderer parent;
	/* properties */
	Enesim_Renderer_Gradient_State state;
	/* generated at state setup */
	Enesim_Renderer_Gradient_Sw_State sw;
#if BUILD_OPENGL
	struct {
		int len;
		GLuint gen_stops;
	} gl;
#endif
	/* private */
	Enesim_Repeat_Mode past_mode;
	Eina_Bool changed : 1;
	Eina_Bool stops_changed : 1;
} Enesim_Renderer_Gradient;

typedef int (*Enesim_Renderer_Gradient_Length)(Enesim_Renderer *r);

typedef struct _Enesim_Renderer_Gradient_Class
{
	Enesim_Renderer_Class parent;

	Enesim_Renderer_Bounds_Get_Cb bounds_get;
	Enesim_Renderer_Has_Changed_Cb has_changed;
	/* software based functions */
	Enesim_Renderer_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	/* opengl based functions */
	Enesim_Renderer_OpenGL_Setup opengl_setup;
	Enesim_Renderer_OpenGL_Cleanup opengl_cleanup;
	/* gradient based functions */
	Enesim_Renderer_Gradient_Length length;
} Enesim_Renderer_Gradient_Class;

Enesim_Object_Descriptor * enesim_renderer_gradient_descriptor_get(void);
int enesim_renderer_gradient_natural_length_get(Enesim_Renderer *r);

#endif
