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
#include "Enesim.h"
#include "enesim_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_GRADIENT_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_GRADIENT_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_GRADIENT_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Gradient
{
	EINA_MAGIC
	/* properties */
	Enesim_Repeat_Mode mode;
	/* generated at state setup */
	uint32_t *src;
	int slen;
	Eina_List *stops;
	/* private */
	Eina_Bool changed : 1;
	Enesim_F16p16_Matrix matrix;
	Enesim_Renderer_Gradient_Descriptor *descriptor;
	void *data;
} Enesim_Renderer_Gradient;

typedef Enesim_Color (*Enesim_Renderer_Gradient_Color_Get)(Enesim_Renderer_Gradient *thiz, Eina_F16p16 distance);

typedef struct _Stop
{
	Enesim_Argb argb;
	double pos;
} Stop;

static inline Enesim_Renderer_Gradient * _gradient_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = enesim_renderer_data_get(r);
	ENESIM_RENDERER_GRADIENT_MAGIC_CHECK(thiz);

	return thiz;
}

static inline uint32_t _pad_color_get(Enesim_Renderer_Gradient *thiz, Eina_F16p16 p)
{
	int fp;
	uint32_t v;

	fp = eina_f16p16_int_to(p);
	if (fp < 0)
	{
		v = thiz->src[0];
	}
	else if (fp >= thiz->slen - 1)
	{
		v = thiz->src[thiz->slen - 1];
	}
	else
	{
		uint16_t a;

		a = eina_f16p16_fracc_get(p) >> 8;
		v = argb8888_interp_256(1 + a, thiz->src[fp + 1], thiz->src[fp]);
	}

	return v;
}

static inline uint32_t _restrict_color_get(Enesim_Renderer_Gradient *thiz, Eina_F16p16 p)
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
			v = argb8888_interp_256(1 + a, thiz->src[0], 0);
		}
	}
	else if (fp >= thiz->slen - 1)
	{
		Eina_F16p16 slen = eina_f16p16_int_from(thiz->slen - 1);

		v = 0;
		if (p - slen <= EINA_F16P16_ONE)
		{
			uint16_t a;

			a = eina_f16p16_fracc_get(p - slen) >> 8;
			v = argb8888_interp_256(1 + a, 0, thiz->src[thiz->slen - 1]);
		}
	}
	else
	{
		uint16_t a;

		a = eina_f16p16_fracc_get(p) >> 8;
		v = argb8888_interp_256(1 + a, thiz->src[fp + 1], thiz->src[fp]);
	}

	return v;
}

static inline uint32_t _reflect_color_get(Enesim_Renderer_Gradient *thiz, Eina_F16p16 p)
{
	return 0xff000000;
}

static inline uint32_t _repeat_color_get(Enesim_Renderer_Gradient *thiz, Eina_F16p16 p)
{
	int fp;
	int fp_next;
	uint32_t v;
	uint16_t a;

	fp = eina_f16p16_int_to(p);
	fp = fp % thiz->slen;
	fp_next = (fp + 1) % thiz->slen;

	a = eina_f16p16_fracc_get(p) >> 8;
	v = argb8888_interp_256(1 + a, thiz->src[fp_next], thiz->src[fp]);

	return v;
}

#define GRADIENT_PROJECTIVE(mode) \
static void _argb8888_##mode##_span_projective(Enesim_Renderer *r,	\
		int x,	int y, 	unsigned int len, void *ddata)		\
{									\
	Enesim_Renderer_Gradient *thiz;					\
	uint32_t *dst = ddata;						\
	uint32_t *end = dst + len;					\
	Eina_F16p16 xx, yy, zz;						\
									\
	thiz = _gradient_get(r);					\
	enesim_renderer_projective_setup(r, x, y, &thiz->matrix,	\
			&xx, &yy, &zz);					\
	while (dst < end)						\
	{								\
		Eina_F16p16 syy, sxx;					\
		Eina_F16p16 d;						\
									\
		syy = ((((int64_t)yy) << 16) / zz);			\
		sxx = ((((int64_t)xx) << 16) / zz);			\
									\
		d = thiz->descriptor->distance(r, sxx, syy);		\
		*dst++ = _##mode##_color_get(thiz, d);			\
		yy += thiz->matrix.yx;					\
		xx += thiz->matrix.xx;					\
		zz += thiz->matrix.zx;					\
	}								\
}

#define GRADIENT_IDENTITY(mode) \
static void _argb8888_##mode##_span_identity(Enesim_Renderer *r, int x,	\
		int y, 	unsigned int len, void *ddata)			\
{									\
	Enesim_Renderer_Gradient *thiz;					\
	uint32_t *dst = ddata;						\
	uint32_t *end = dst + len;					\
	Eina_F16p16 xx, yy;						\
									\
	thiz = _gradient_get(r);					\
	enesim_renderer_identity_setup(r, x, y, &xx, &yy);		\
	while (dst < end)						\
	{								\
		Eina_F16p16 d;						\
		d = thiz->descriptor->distance(r, xx, yy);		\
		*dst++ = _##mode##_color_get(thiz, d);			\
		xx += EINA_F16P16_ONE;					\
	}								\
	/* FIXME is there some mmx bug there? the interp_256 already calls this \
	 * but the float support is fucked up				\
	 */								\
}

#define GRADIENT_AFFINE(mode) \
static void _argb8888_##mode##_span_affine(Enesim_Renderer *r, int x,	\
		int y, 	unsigned int len, void *ddata)			\
{									\
	Enesim_Renderer_Gradient *thiz;					\
	uint32_t *dst = ddata;						\
	uint32_t *end = dst + len;					\
	Eina_F16p16 xx, yy;						\
									\
	thiz = _gradient_get(r);					\
	enesim_renderer_affine_setup(r, x, y, &thiz->matrix, &xx, &yy);	\
	while (dst < end)						\
	{								\
		Eina_F16p16 d;						\
									\
		d = thiz->descriptor->distance(r, xx, yy);		\
		*dst++ = _##mode##_color_get(thiz, d);			\
		yy += thiz->matrix.yx;					\
		xx += thiz->matrix.xx;					\
	}								\
}

GRADIENT_PROJECTIVE(restrict);
GRADIENT_PROJECTIVE(repeat);
GRADIENT_PROJECTIVE(pad);
GRADIENT_PROJECTIVE(reflect);

GRADIENT_IDENTITY(restrict);
GRADIENT_IDENTITY(repeat);
GRADIENT_IDENTITY(pad);
GRADIENT_IDENTITY(reflect);

GRADIENT_AFFINE(restrict);
GRADIENT_AFFINE(repeat);
GRADIENT_AFFINE(pad);
GRADIENT_AFFINE(reflect);

static Enesim_Renderer_Sw_Fill _spans[ENESIM_REPEAT_MODES][ENESIM_MATRIX_TYPES];
static Enesim_Renderer_Gradient_Color_Get _color_get[ENESIM_REPEAT_MODES];

static Eina_Bool _gradient_changed(Enesim_Renderer_Gradient *thiz)
{
	return thiz->changed;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static void _gradient_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	thiz->slen = 0;
	thiz->changed = EINA_FALSE;
	if (thiz->descriptor->sw_cleanup)
	{
		thiz->descriptor->sw_cleanup(r, s);
	}
}

static Eina_Bool _gradient_state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Gradient *thiz;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	Stop *curr, *next, *last;
	Eina_F16p16 xx, inc;
	Eina_List *tmp;
	int slen;
	int start;
	int end;
	int i;
	uint32_t *dst;

	thiz = _gradient_get(r);
	/* check that we have at least two stops */
	if (eina_list_count(thiz->stops) < 2)
	{
		ENESIM_RENDERER_ERROR(r, error, "Less than two stops");
		return EINA_FALSE;
	}
	/* setup the implementation */
	*fill = NULL;
	if (!thiz->descriptor->sw_setup(r, states, s, fill, error))
	{
		ENESIM_RENDERER_ERROR(r, error, "Gradient implementation failed");
		return EINA_FALSE;
	}
	if (!*fill)
	{
		enesim_matrix_f16p16_matrix_to(&cs->transformation,
				&thiz->matrix);
		*fill = _spans[thiz->mode][cs->transformation_type];
	}
	/* get the length */
	slen = thiz->descriptor->length(r);
	if (slen < 0)
	{
		ENESIM_RENDERER_ERROR(r, error, "Gradient length %d < 0", slen);
		return EINA_FALSE;
	}

	curr = eina_list_data_get(thiz->stops);
	tmp = eina_list_next(thiz->stops);
	next = eina_list_data_get(tmp);
	last = eina_list_data_get(eina_list_last(thiz->stops));
	/* TODO check that next->pos - curr->pos != 0 */
	/* Check that we dont divide by 0 */
	inc = eina_f16p16_double_from(1.0 / ((next->pos - curr->pos) * slen));
	xx = 0;

	start = curr->pos * slen;
	end = last->pos * slen;

	dst = thiz->src;
	if (!thiz->src || slen != thiz->slen)
	{
		thiz->slen = slen;
		if (thiz->src)
			free(thiz->src);
		thiz->src = dst = malloc(sizeof(uint32_t) * thiz->slen);
	}

	/* in case we dont start at 0.0 */
	for (i = 0; i < start; i++)
		*dst++ = 0x00ffffff;

	/* FIXME Im not sure if we increment xx by the 1 / ((next - curr) * len) value
	 * as it might not be too accurate
	 */
	for (i = start; i < end; i++)
	{
		uint16_t off;
		uint32_t p0;

		/* advance the curr and next */
		if (xx >= 65536)
		{
			tmp = eina_list_next(tmp);
			curr = next;
			next = eina_list_data_get(tmp);
			inc = eina_f16p16_double_from(1.0 / ((next->pos - curr->pos) * thiz->slen));
			xx = 0;
		}
		off = 1 + (eina_f16p16_fracc_get(xx) >> 8);
		p0 = argb8888_interp_256(off, next->argb, curr->argb);
		*dst++ = p0;
		xx += inc;
	}
	/* in case we dont end at 1.0 */
	for (i = end; i < thiz->slen; i++)
		*dst++ = 0x00ffffff;

	return EINA_TRUE;
}

static void _gradient_boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *boundings)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	if (thiz->mode == ENESIM_RESTRICT && thiz->descriptor->boundings)
	{
		thiz->descriptor->boundings(r, states, boundings);
	}
	else
	{
		boundings->x = INT_MIN / 2;
		boundings->y = INT_MIN / 2;
		boundings->w = INT_MAX;
		boundings->h = INT_MAX;
	}
}

static void _gradient_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	if (thiz->src)
		free(thiz->src);
	if (thiz->descriptor->free)
		thiz->descriptor->free(r);
	free(thiz);
}

static void _gradient_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_PROJECTIVE |
			//ENESIM_RENDERER_FLAG_GEOMETRY |
			ENESIM_RENDERER_FLAG_ARGB8888;
}

static const char * _gradient_name(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	if (thiz->descriptor->name)
		return thiz->descriptor->name(r);
	else
		return "gradient";
}

static Eina_Bool _gradient_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;
	Eina_Bool ret = EINA_TRUE;

	thiz = _gradient_get(r);
	ret = _gradient_changed(thiz);
	if (ret)
	{
		return ret;
	}
	if (thiz->descriptor->has_changed)
		ret = thiz->descriptor->has_changed(r);
	return ret;
}

static Enesim_Renderer_Descriptor _gradient_descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _gradient_name,
	/* .free = 			*/ _gradient_free,
	/* .boundings = 		*/ _gradient_boundings,
	/* .destination_boundings = 	*/ NULL,
	/* .flags = 			*/ _gradient_flags,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _gradient_has_changed,
	/* .sw_setup = 			*/ _gradient_state_setup,
	/* .sw_cleanup = 		*/ _gradient_state_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
	/* .opengl_setup =          	*/ NULL,
	/* .opengl_shader_setup = 	*/ NULL,
	/* .opengl_cleanup =        	*/ NULL
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_gradient_new(Enesim_Renderer_Gradient_Descriptor *gdescriptor, void *data)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Gradient *thiz;
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
		/* now the color get functions */
		_color_get[ENESIM_PAD] = _pad_color_get;
		_color_get[ENESIM_REFLECT] = _reflect_color_get;
		_color_get[ENESIM_RESTRICT] = _restrict_color_get;
		_color_get[ENESIM_REPEAT] = _repeat_color_get;
	}
	if (!gdescriptor->distance)
	{
		ERR("No suitable gradient distance function");
		return NULL;
	}
	if (!gdescriptor->length)
	{
		ERR("No suitable gradient length function");
		return NULL;
	}

	thiz = calloc(1, sizeof(Enesim_Renderer_Gradient));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_GRADIENT_MAGIC);

	thiz->data = data;
	thiz->descriptor = gdescriptor;
	/* set default properties */
	thiz->stops = NULL;

	r = enesim_renderer_new(&_gradient_descriptor, thiz);
	return r;
}

void * enesim_renderer_gradient_data_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	return thiz->data;
}

Enesim_Color enesim_renderer_gradient_color_get(Enesim_Renderer *r, Eina_F16p16 pos)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	return _color_get[thiz->mode](thiz, pos);
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_stop_add(Enesim_Renderer *r, Enesim_Renderer_Gradient_Stop *stop)
{
	Enesim_Renderer_Gradient *thiz;
	Stop *s;
	double pos;

	if (!stop) return;

	pos = stop->pos;
	if (pos < 0)
		pos = 0;
	else if (pos > 1)
		pos = 1;

	thiz = _gradient_get(r);
	s = malloc(sizeof(Stop));
	s->argb = stop->argb;
	s->pos = pos;
	/* if pos == 0.0 set to first */
	if (pos == 0.0)
	{
		thiz->stops = eina_list_prepend(thiz->stops, s);
	}
	/* if pos == 1.0 set to last */
	else if (pos == 1.0)
	{
		thiz->stops = eina_list_append(thiz->stops, s);
	}
	/* else iterate until pos > prev && pos < next */
	else
	{
		Eina_List *tmp;

		for (tmp = thiz->stops; tmp; tmp = eina_list_next(tmp))
		{
			Stop *p = eina_list_data_get(tmp);

			if (p->pos > s->pos)
				break;
		}
		thiz->stops = eina_list_append_relative_list(thiz->stops, s, tmp);
	}
	thiz->changed = EINA_TRUE;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_stop_clear(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient *thiz;
	Enesim_Renderer_Gradient_Stop *stop;
	Eina_List *l;
	Eina_List *l_next;

	thiz = _gradient_get(r);
	EINA_LIST_FOREACH_SAFE(thiz->stops, l, l_next, stop)
	{
		thiz->stops = eina_list_remove_list(thiz->stops, l);
		free(stop);
	}
	thiz->changed = EINA_TRUE;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_stop_set(Enesim_Renderer *r,
		Eina_List *list)
{
	Enesim_Renderer_Gradient *thiz;
	Enesim_Renderer_Gradient_Stop *stop;
	Eina_List *l;

	thiz = _gradient_get(r);
	EINA_LIST_FOREACH(list, l, stop)
	{
		enesim_renderer_gradient_stop_add(r, stop);
	}
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_mode_set(Enesim_Renderer *r,
		Enesim_Repeat_Mode mode)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	thiz->mode = mode;
	thiz->changed = EINA_TRUE;
}

/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_mode_get(Enesim_Renderer *r, Enesim_Repeat_Mode *mode)
{
	Enesim_Renderer_Gradient *thiz;

	thiz = _gradient_get(r);
	*mode = thiz->mode;
}
