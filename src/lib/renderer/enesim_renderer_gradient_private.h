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
#ifndef GRADIENT_H_
#define GRADIENT_H_

#include "libargb.h"
/* helper functions for different spread modes */
static inline uint32_t enesim_renderer_gradient_pad_color_get(Enesim_Color *src, size_t len, Eina_F16p16 p)
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

static inline uint32_t enesim_renderer_gradient_restrict_color_get(Enesim_Color *src, size_t len, Eina_F16p16 p)
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

static inline uint32_t enesim_renderer_gradient_reflect_color_get(Enesim_Color *src, size_t len, Eina_F16p16 p)
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

static inline uint32_t enesim_renderer_gradient_repeat_color_get(Enesim_Color *src, size_t len, Eina_F16p16 p)
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
		const Enesim_Renderer_Gradient_Sw_Draw_Data *data,	\
		int x, int y, unsigned int len, void *ddata)		\
{									\
	type *thiz;							\
	const Enesim_Renderer_Gradient_Sw_State 			\
			*gstate = data->sw_state;			\
	const Enesim_Renderer_State 					\
			*state = data->state;				\
	uint32_t *dst = ddata;						\
	uint32_t *end = dst + len;					\
	Eina_F16p16 xx, yy, zz;						\
									\
	thiz = type_get(r);						\
	enesim_coord_projective_setup(&xx, &yy, &zz, x, y, state->ox,	\
			state->oy, &gstate->matrix);			\
	while (dst < end)						\
	{								\
		Eina_F16p16 syy, sxx;					\
		Eina_F16p16 d;						\
									\
		syy = ((((int64_t)yy) << 16) / zz);			\
		sxx = ((((int64_t)xx) << 16) / zz);			\
									\
		d = distance(thiz, sxx, syy);				\
		*dst++ = enesim_renderer_gradient_##mode##_color_get(	\
				gstate->src,				\
				gstate->len, d);			\
		yy += gstate->matrix.yx;				\
		xx += gstate->matrix.xx;				\
		zz += gstate->matrix.zx;				\
	}								\
}

#define GRADIENT_IDENTITY(type, type_get, distance, mode) \
static void _argb8888_##mode##_span_identity(Enesim_Renderer *r,	\
		const Enesim_Renderer_Gradient_Sw_Draw_Data *data,	\
		int x, int y, unsigned int len, void *ddata)		\
{									\
	type *thiz;							\
	const Enesim_Renderer_Gradient_Sw_State 			\
			*gstate = data->sw_state;			\
	const Enesim_Renderer_State 					\
			*state = data->state;				\
	uint32_t *dst = ddata;						\
	uint32_t *end = dst + len;					\
	Eina_F16p16 xx, yy;						\
									\
	thiz = type_get(r);						\
	enesim_coord_identity_setup(&xx, &yy, x, y, state->ox,		\
			state->oy);					\
	while (dst < end)						\
	{								\
		Eina_F16p16 d;						\
		d = distance(thiz, xx, yy);				\
		*dst++ = enesim_renderer_gradient_##mode##_color_get(	\
				gstate->src,				\
				gstate->len, d);			\
		xx += EINA_F16P16_ONE;					\
	}								\
}

#define GRADIENT_AFFINE(type, type_get, distance, mode) \
static void _argb8888_##mode##_span_affine(Enesim_Renderer *r,		\
		const Enesim_Renderer_Gradient_Sw_Draw_Data *data,	\
 		int x, int y, unsigned int len, void *ddata)		\
{									\
	type *thiz;							\
	const Enesim_Renderer_Gradient_Sw_State 			\
			*gstate = data->sw_state;			\
	const Enesim_Renderer_State 					\
			*state = data->state;				\
	uint32_t *dst = ddata;						\
	uint32_t *end = dst + len;					\
	Eina_F16p16 xx, yy;						\
									\
	thiz = type_get(r);						\
	enesim_coord_affine_setup(&xx, &yy, x, y, state->ox, state->oy, \
			&gstate->matrix);				\
	while (dst < end)						\
	{								\
		Eina_F16p16 d;						\
									\
		d = distance(thiz, xx, yy);				\
		*dst++ = enesim_renderer_gradient_##mode##_color_get(	\
				gstate->src,				\
				gstate->len, d);			\
		yy += gstate->matrix.yx;				\
		xx += gstate->matrix.xx;				\
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
	size_t len;
	Enesim_F16p16_Matrix matrix;
} Enesim_Renderer_Gradient_Sw_State;

typedef struct _Enesim_Renderer_Gradient_Sw_Draw_Data
{
	const Enesim_Renderer_State *state;
	const Enesim_Renderer_Gradient_State *gstate;
	const Enesim_Renderer_Gradient_Sw_State *sw_state;
} Enesim_Renderer_Gradient_Sw_Draw_Data;

typedef void (*Enesim_Renderer_Gradient_Sw_Draw)(Enesim_Renderer *r,
		const Enesim_Renderer_Gradient_Sw_Draw_Data *gdata,
		int x, int y,
		unsigned int len,
		void *data);

typedef int (*Enesim_Renderer_Gradient_Length)(Enesim_Renderer *r);
typedef Eina_Bool (*Enesim_Renderer_Gradient_Sw_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Gradient_State *gstate,
		Enesim_Surface *s,
		Enesim_Renderer_Gradient_Sw_Draw *draw,
		Enesim_Error **error);

typedef struct _Enesim_Renderer_Gradient_Descriptor
{
	/* gradient based functions */
	Enesim_Renderer_Gradient_Length length;
	Enesim_Renderer_Gradient_Sw_Setup sw_setup;
	/* common renderer functions */
	Enesim_Renderer_Base_Name_Get_Cb name;
	Enesim_Renderer_Delete free;
	Enesim_Renderer_Bounds_Get_Cb bounds;
	Enesim_Renderer_Destination_Bounds_Get_Cb destination_bounds;
	Enesim_Renderer_Is_Inside_Cb is_inside;
	Enesim_Renderer_Has_Changed_Cb has_changed;
	/* software based functions */
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	/* opengl based functions */
	Enesim_Renderer_OpenGL_Initialize opengl_initialize;
	Enesim_Renderer_OpenGL_Setup opengl_setup;
} Enesim_Renderer_Gradient_Descriptor;

Enesim_Renderer * enesim_renderer_gradient_new(Enesim_Renderer_Gradient_Descriptor *gdescriptor, void *data);
void * enesim_renderer_gradient_data_get(Enesim_Renderer *r);
int enesim_renderer_gradient_natural_length_get(Enesim_Renderer *r);

#endif
