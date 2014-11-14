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

#include <math.h>

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_color_private.h"
#include "enesim_color_fill_private.h"
#include "enesim_color_mul4_sym_private.h"
#include "enesim_list_private.h"
#include "enesim_vector_private.h"
#include "enesim_renderer_private.h"
#include "enesim_rasterizer_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_RASTERIZER_BASIC(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Rasterizer_Basic,					\
		enesim_rasterizer_basic_descriptor_get())

/* State generated at the state_setup process
 * the colors are already multiplied by the renderer color
 */
/* TODO ok, this needs to change easily:
 * 1. dont put draw specific data on the type put all the draw
 *    on its own structure. that own structure should be filled
 *    on setup() and cleaned on cleanup()
 * 2. Move the draw algorithm into its own draw function, not the
 *    renderer one, but a specific one that receives the struct
 *    above.
 * The above might sound absurd, but once the different algorithms
 * are isolated, not only is simple to work with them but also
 * is simple to port them to other backends, like opengl with
 * its own types. But for now on the first round of the refactoring
 * we keep the current way :-/
 */
typedef struct _Enesim_Rasterizer_Basic_State
{
	double ox;
	double oy;

	struct {
		Enesim_Renderer *r;
		Enesim_Color color;
	} fill;

	struct {
		Enesim_Renderer *r;
		Enesim_Color color;
		double weight;
	} stroke;

	Enesim_Renderer_Shape_Draw_Mode draw_mode;
	Enesim_Color color;
} Enesim_Rasterizer_Basic_State;

typedef struct _Enesim_Rasterizer_Basic
{
	Enesim_Rasterizer parent;
	/* private */
	const Enesim_F16p16_Vector *vectors;
	int nvectors;

	Enesim_Rasterizer_Basic_State state;
	/* FIXME this are the bounds calculated at the setup
	 * we either generate them at the Enesim_Polygon level
	 * or something like that
	 */
	int lxx, rxx, tyy, byy;
	Enesim_Matrix_F16p16 matrix;
	/* TODO the above must be cleaned up */
	Enesim_Renderer *spaint;
	Enesim_Renderer *fpaint;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Eina_F16p16 sww;
	Eina_Bool stroke;
} Enesim_Rasterizer_Basic;

typedef struct _Enesim_Rasterizer_Basic_Class {
	Enesim_Rasterizer_Class parent;
} Enesim_Rasterizer_Basic_Class;

static inline int _basic_setup_edges(Enesim_Rasterizer_Basic *thiz,
		Enesim_F16p16_Edge *edges, Eina_F16p16 xx, Eina_F16p16 yy,
		Eina_F16p16 *lx, Eina_F16p16 *rx)
{
	Enesim_F16p16_Edge *edge = edges;
	const Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors;
	int nedges = 0;
	int n = 0;

	*lx = INT_MAX / 2;
	*rx = -(*lx);
	while (n < nvectors)
	{
		if (yy + 0xffff < v->yy0)
			break;
		if (((yy + 0xffff) >= v->yy0) & (yy <= (v->yy1 + 0xffff)))
		{
			edge->xx0 = v->xx0;
			edge->xx1 = v->xx1;
			edge->yy0 = v->yy0;
			edge->yy1 = v->yy1;
			edge->de = (v->a * (long long int) thiz->matrix.xx) >> 16;
			edge->e = ((v->a * (long long int) xx) >> 16) +
					((v->b * (long long int) yy) >> 16) +
					v->c;
			edge->counted = ((yy >= edge->yy0) & (yy < edge->yy1));
			if (v->sgn)
			{
				int dxx = (v->xx1 - v->xx0);
				double dd = dxx / (double)(v->yy1 - v->yy0);
				int lxxc, lyyc = yy - 0xffff;
				int rxxc, ryyc = yy + 0xffff;

				if (v->sgn < 0)
				{
					lyyc = yy + 0xffff;
					ryyc = yy - 0xffff;
				}

				lxxc = (lyyc - v->yy0) * dd;
				rxxc = (ryyc - v->yy0) * dd;

				if (v->sgn < 0)
				{
					lxxc = dxx - lxxc;
					rxxc = dxx - rxxc;
				}

				lxxc += v->xx0;
				rxxc += v->xx0;

				if (lxxc < v->xx0)
					lxxc = v->xx0;
				if (rxxc > v->xx1)
					rxxc = v->xx1;

				if (*lx > lxxc)
					*lx = lxxc;
				if (*rx < rxxc)
					*rx = rxxc;
				edge->lx = (lxxc >> 16);
			}
			else
			{
				if (*lx > v->xx0)
					*lx = v->xx0;
				if (*rx < v->xx1)
					*rx = v->xx1;
				edge->lx = (v->xx0 >> 16);
			}
			edge++;
			nedges++;
		}
		n++;
		v++;
	}
	return nedges;
}

static inline Eina_Bool _basic_clip(int l1, int r1, int l2, int r2, int *rl, int *rr)
{
	if (r2 < l1)
	{
		return EINA_FALSE;
	}
	else
	{
		if (r2 > r1)
		{
			r2 = r1;
		}
	}

	if (l2 > r1)
	{
		return EINA_FALSE;
	}
	else
	{
		if (l2 < l1)
		{
			l2 = l1;
		}
	}
	*rl = l2;
	*rr = r2;
	return EINA_TRUE;
}

/*----------------------------------------------------------------------------*
 *                            Evaluate functions                              *
 *----------------------------------------------------------------------------*/
static inline void _basic_edges_nz_evaluate(Enesim_F16p16_Edge *edges,
		int nedges, Eina_F16p16 xx, int sww, int *a, int *count)
{
	Enesim_F16p16_Edge *edge = edges;
	int n = 0;

	/* initialize the output parameters */
	*a = 0;
	*count = 0;

	/* start evaluating the edges */
	edge = edges;
	n = 0;
	while (n < nedges)
	{
		int ee = edge->e;

		/* alternative */
		/* count += (ee >> 31) | ((~ee >> 31) & 1); */
		if (edge->counted)
			*count += (ee >= 0) - (ee < 0);
		if (ee < 0)
			ee = -ee;

		if ((ee < sww) && ((xx + 0xffff) >= edge->xx0) &
				(xx <= (0xffff + edge->xx1)))
		{
			if (*a < sww/4)
			{
				*a = sww - ee;
			}
			else
			{
				*a = (*a + (sww - ee)) / 2;
			}
		}

		edge->e += edge->de;
		edge++;
		n++;
	}
}

static inline void _basic_edges_eo_evaluate(Enesim_F16p16_Edge *edges,
		int nedges, Eina_F16p16 xx, int sww, int *a, int *in)
{
	Enesim_F16p16_Edge *edge = edges;
	int n = 0;
	int np = 0;
	int nn = 0;

	/* initialize the output parameters */
	*a = 0;
	*in = 0;

	/* start evaluating the edges */
	edge = edges;
	while (n < nedges)
	{
		int ee = edge->e;

		if (edge->counted)
		{
			np += (ee >= 0);
			nn += (ee < 0);
		}
		if (ee < 0)
			ee = -ee;

		if ((ee < sww) && ((xx + 0xffff) >= edge->xx0) &
				(xx <= (0xffff + edge->xx1)))
		{
			if (*a < sww/4)
				*a = sww - ee;
			else
				*a = (*a + (sww - ee)) / 2;
		}

		edge->e += edge->de;
		edge++;
		n++;
	}

	if ((np + nn) % 4)
		*in = !(np % 2);
	else
		*in = (np % 2);
}

/*----------------------------------------------------------------------------*
 *                           Fill/Stroke variants                             *
 *----------------------------------------------------------------------------*/
static inline void _basic_fill_color_stroke_color_setup(
			Enesim_Rasterizer_Basic *thiz EINA_UNUSED,
			int x EINA_UNUSED, int y EINA_UNUSED, int len EINA_UNUSED,
			uint32_t *dst EINA_UNUSED)
{

}

static inline void _basic_fill_color_stroke_renderer_setup(
			Enesim_Rasterizer_Basic *thiz, int x, int y, int len,
			uint32_t *dst)
{
	enesim_renderer_sw_draw(thiz->spaint, x, y, len, dst);
}

static inline void _basic_fill_renderer_stroke_color_setup(
			Enesim_Rasterizer_Basic *thiz, int x, int y, int len,
			uint32_t *dst)
{
	enesim_renderer_sw_draw(thiz->fpaint, x, y, len, dst);
}


static inline void _basic_fill_renderer_stroke_renderer_setup(
			Enesim_Rasterizer_Basic *thiz, int x, int y, int len,
			uint32_t *dst, uint32_t *sdst)
{
	enesim_renderer_sw_draw(thiz->fpaint, x, y, len, dst);
	enesim_renderer_sw_draw(thiz->spaint, x, y, len, sdst);
}

static inline void _stroke_renderer_setup(Enesim_Rasterizer_Basic *thiz,
		int x, int y, int len, uint32_t *dst)
{
	enesim_renderer_sw_draw(thiz->spaint, x, y, len, dst);
}

static inline void _basic_fill_renderer_advance(Enesim_Rasterizer_Basic *thiz,
		uint32_t *dst, int len)
{
	enesim_color_mul4_sym_sp_none_color_none(dst, len, thiz->fcolor);
}

static inline void _basic_fill_color_advance(Enesim_Rasterizer_Basic *thiz,
		uint32_t *dst, int len)
{
	enesim_color_fill_sp_none_color_none(dst, len, thiz->fcolor);
}

static inline uint32_t _basic_fill_color_stroke_color_draw(
		Enesim_Rasterizer_Basic *thiz, uint32_t *d EINA_UNUSED,
		int count, int a)
{
	uint32_t p0;

	if (count)
	{
		p0 = thiz->fcolor;
		if (thiz->stroke)
		{
			uint32_t q0 = p0;

			p0 = thiz->scolor;
			if (a < 65536)
				p0 = enesim_color_interp_65536(a, p0, q0);
		}
	}
	else
	{
		p0 = thiz->scolor;
		if (a < 65536)
		{
			p0 = enesim_color_mul_65536(a, p0);
		}
	}

	return p0;

}

static inline uint32_t _basic_fill_renderer_stroke_color_draw(
		Enesim_Rasterizer_Basic *thiz, uint32_t *d, int count, int a)
{
	uint32_t p0;

	if (count)
	{
		p0 = *d;
		if (thiz->fcolor != 0xffffffff)
			p0 = enesim_color_mul4_sym(thiz->fcolor, p0);

		if (thiz->stroke)
		{
			uint32_t q0 = p0;

			p0 = thiz->scolor;
			if (a < 65536)
				p0 = enesim_color_interp_65536(a, p0, q0);
		}
	}
	else
	{
		p0 = thiz->scolor;
		if (!thiz->stroke)
		{
			p0 = *d;
			if (thiz->fcolor != 0xffffffff)
				p0 = enesim_color_mul4_sym(thiz->fcolor, p0);
		}
		if (a < 65536)
		{
			p0 = enesim_color_mul_65536(a, p0);
		}
	}

	return p0;
}


static inline uint32_t _basic_fill_color_stroke_renderer_draw(
		Enesim_Rasterizer_Basic *thiz, uint32_t *d, int count, int a)
{
	uint32_t p0;

	p0 = *d;
	if (thiz->scolor != 0xffffffff)
		p0 = enesim_color_mul4_sym(thiz->scolor, p0);
	if (a < 65536)
	{
		if (count)
			p0 = enesim_color_interp_65536(a, p0, thiz->fcolor);
		else
			p0 = enesim_color_mul_65536(a, p0);
	}
	return p0;
}

static inline uint32_t _basic_fill_renderer_stroke_renderer_draw(
		Enesim_Rasterizer_Basic *thiz, uint32_t *d, uint32_t *s,
		int count, int a)
{
	uint32_t p0;

	p0 = *s;
	if (thiz->scolor != 0xffffffff)
		p0 = enesim_color_mul4_sym(thiz->scolor, p0);
	if (a < 65536)
	{
		if (count)
		{
			uint32_t q0 = *d;

			if (thiz->fcolor != 0xffffffff)
				q0 = enesim_color_mul4_sym(thiz->fcolor, q0);
			p0 = enesim_color_interp_65536(a, p0, q0);
		}
		else
			p0 = enesim_color_mul_65536(a, p0);
	}
	return p0;
}

#define BASIC_SIMPLE(ftype, stype, evaluate)					\
static void _basic_span_fill_##ftype##_stroke_##stype##_##evaluate(		\
		Enesim_Renderer *r, int x, int y, int len, void *ddata)		\
{										\
	Enesim_Rasterizer_Basic *thiz = ENESIM_RASTERIZER_BASIC(r);		\
	Enesim_Rasterizer_Basic_State *state = &thiz->state;			\
	uint32_t *dst = ddata;							\
	uint32_t *d = dst, *e = d + len;					\
	Enesim_F16p16_Edge *edges, *edge;					\
	int nvectors = thiz->nvectors, n = 0, nedges = 0;			\
	double ox, oy;								\
	int lx = INT_MAX / 2, rx = -lx;						\
	Eina_Bool outside = EINA_TRUE;						\
										\
	int xx = eina_f16p16_int_from(x);					\
	int yy = eina_f16p16_int_from(y);					\
										\
	ox = state->ox;								\
	oy = state->oy;								\
	xx -= eina_f16p16_double_from(ox);					\
	yy -= eina_f16p16_double_from(oy);					\
										\
	edges = alloca(nvectors * sizeof(Enesim_F16p16_Edge));			\
	nedges = _basic_setup_edges(thiz, edges, xx, yy, &lx, &rx);		\
	if (!_basic_clip(x, x + len, (lx >> 16) - 1, (rx >> 16) + 2, &lx, &rx))	\
	{									\
		memset(d, 0, sizeof(uint32_t) * len);				\
		return;								\
	}									\
	/* the most right coordinate is smaller than the requested length */ 	\
	if (rx < x + len)							\
	{									\
		int roff = rx - x;						\
		int nlen = (x + len) - rx;					\
										\
		memset(d + roff, 0, sizeof(uint32_t) * nlen);			\
		len -= nlen;							\
		e -= nlen;							\
	}									\
										\
	_basic_fill_##ftype##_stroke_##stype##_setup(thiz, lx, y, rx - lx,	\
			 dst + (lx - x));					\
	/* lx will be an offset from now on */					\
	lx -= x;								\
repeat:										\
	if (lx > 0)								\
	{									\
		if (outside)							\
		{								\
			memset(d, 0, sizeof(uint32_t) * lx);			\
		}								\
		else								\
		{								\
			_basic_fill_##ftype##_advance(thiz, d, lx);		\
		}								\
										\
		/* advance the edges by lx */					\
		n = 0;								\
		edge = edges;							\
		while (n < nedges)						\
		{								\
			edge->e += lx * edge->de;				\
			edge++;							\
			n++;							\
		}								\
										\
		/* advance our position by lx */				\
		xx += lx * EINA_F16P16_ONE;					\
		x += lx;							\
		d += lx;							\
	}									\
										\
	while (d < e)								\
	{									\
		uint32_t p0;							\
		int count = 0;							\
		int a = 0;							\
										\
		_basic_edges_##evaluate##_evaluate(edges, nedges, xx, 		\
				thiz->sww, &a, &count);				\
		if (!a)								\
		{								\
			int nx = rx;						\
										\
			edge = edges;						\
			n = 0;							\
			while (n < nedges)					\
			{							\
				if ((x <= edge->lx) & (nx > edge->lx))		\
					nx = edge->lx;				\
				edge->e -= edge->de;				\
				edge++;						\
				n++;						\
			}							\
			lx = nx - x;						\
			if (lx < 1)						\
				lx = 1;						\
			outside = !count;					\
			goto repeat;						\
		}								\
										\
		p0 = _basic_fill_##ftype##_stroke_##stype##_draw(thiz, d, 	\
				count, a);					\
		*d++ = p0;							\
		xx += EINA_F16P16_ONE;						\
		x++;								\
	}									\
}

#define BASIC_FULL(evaluate)							\
static void _basic_span_fill_renderer_stroke_renderer_##evaluate(		\
		Enesim_Renderer *r, int x, int y, int len, void *ddata)		\
{										\
	Enesim_Rasterizer_Basic *thiz = ENESIM_RASTERIZER_BASIC(r);		\
	Enesim_Rasterizer_Basic_State *state = &thiz->state;			\
	uint32_t *dst = ddata;							\
	uint32_t *d = dst, *e = d + len;					\
	uint32_t *s;								\
	Enesim_F16p16_Edge *edges, *edge;					\
	int nvectors = thiz->nvectors, n = 0, nedges = 0;			\
	double ox, oy;								\
	int lx = INT_MAX / 2, rx = -lx;						\
	Eina_Bool outside = EINA_TRUE;						\
										\
	int xx = eina_f16p16_int_from(x);					\
	int yy = eina_f16p16_int_from(y);					\
										\
	ox = state->ox;								\
	oy = state->oy;								\
	xx -= eina_f16p16_double_from(ox);					\
	yy -= eina_f16p16_double_from(oy);					\
										\
	edges = alloca(nvectors * sizeof(Enesim_F16p16_Edge));			\
	nedges = _basic_setup_edges(thiz, edges, xx, yy, &lx, &rx);		\
	if (!_basic_clip(x, x + len, (lx >> 16) - 1, (rx >> 16) + 2, &lx, &rx))	\
	{									\
		memset(d, 0, sizeof(uint32_t) * len);				\
		return;								\
	}									\
	/* the most right coordinate is smaller than the requested length */ 	\
	if (rx < x + len)							\
	{									\
		int roff = rx - x;						\
		int nlen = (x + len) - rx;					\
										\
		memset(d + roff, 0, sizeof(uint32_t) * nlen);			\
		len -= nlen;							\
		e -= nlen;							\
	}									\
										\
	s = alloca(len * sizeof(uint32_t));					\
	_basic_fill_renderer_stroke_renderer_setup(thiz, lx, y, rx - lx,	\
			 dst + (lx - x), s + (lx - x));				\
	/* lx will be an offset from now on */					\
	lx -= x;								\
repeat:										\
	if (lx > 0)								\
	{									\
		if (outside)							\
		{								\
			memset(d, 0, sizeof(uint32_t) * lx);			\
		}								\
		else								\
		{								\
			_basic_fill_renderer_advance(thiz, d, lx);		\
		}								\
										\
		/* advance the edges by lx */					\
		n = 0;								\
		edge = edges;							\
		while (n < nedges)						\
		{								\
			edge->e += lx * edge->de;				\
			edge++;							\
			n++;							\
		}								\
										\
		/* advance our position by lx */				\
		xx += lx * EINA_F16P16_ONE;					\
		x += lx;							\
		d += lx;							\
		s += lx;							\
	}									\
										\
	while (d < e)								\
	{									\
		uint32_t p0;							\
		int count = 0;							\
		int a = 0;							\
										\
		_basic_edges_##evaluate##_evaluate(edges, nedges, xx, 		\
				thiz->sww, &a, &count);				\
		if (!a)								\
		{								\
			int nx = rx;						\
										\
			edge = edges;						\
			n = 0;							\
			while (n < nedges)					\
			{							\
				if ((x <= edge->lx) & (nx > edge->lx))		\
					nx = edge->lx;				\
				edge->e -= edge->de;				\
				edge++;						\
				n++;						\
			}							\
			lx = nx - x;						\
			if (lx < 1)						\
				lx = 1;						\
			outside = !count;					\
			goto repeat;						\
		}								\
										\
		p0 = _basic_fill_renderer_stroke_renderer_draw(thiz, d, s, 	\
				count, a);					\
		*d++ = p0;							\
		xx += EINA_F16P16_ONE;						\
		x++;								\
		s++;								\
	}									\
}

BASIC_SIMPLE(color, color, nz);
BASIC_SIMPLE(color, renderer, nz);
BASIC_SIMPLE(renderer, color, nz);
BASIC_FULL(nz);
BASIC_SIMPLE(color, color, eo);
BASIC_SIMPLE(color, renderer, eo);
BASIC_SIMPLE(renderer, color, eo);
BASIC_FULL(eo);

/* [fill_rule][fill color|renderer][stroke color|renderer] */
static Enesim_Renderer_Sw_Fill _spans[ENESIM_RENDERER_SHAPE_FILL_RULES][2][2];
/*----------------------------------------------------------------------------*
 *                           Rasterizer interface                             *
 *----------------------------------------------------------------------------*/
static void _basic_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Rasterizer_Basic *thiz;
	Enesim_Rasterizer_Basic_State *state;

	thiz = ENESIM_RASTERIZER_BASIC(r);
	if (thiz->fpaint)
	{
		enesim_renderer_unref(thiz->fpaint);
		thiz->fpaint = NULL;
	}
	if (thiz->spaint)
	{
		enesim_renderer_unref(thiz->spaint);
		thiz->spaint = NULL;
	}

	state = &thiz->state;
	if (state->fill.r)
		enesim_renderer_unref(state->fill.r);
	if (state->stroke.r)
		enesim_renderer_unref(state->stroke.r);
}

/*----------------------------------------------------------------------------*
 *                    The Enesim's rasterizer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _basic_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "basic";
}

static Eina_Bool _basic_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *draw, Enesim_Log **error)
{
	Enesim_Rasterizer *rr;
	Enesim_Rasterizer_Basic *thiz;
	Enesim_Rasterizer_Basic_State *state;
	Enesim_Renderer_Shape_Draw_Mode draw_mode;
	Enesim_Renderer_Shape_Fill_Rule fill_rule;
	Enesim_Matrix matrix;
	Eina_Bool changed;

	rr = ENESIM_RASTERIZER(r);
	if (!rr->figure)
	{
		ENESIM_RENDERER_LOG(r, error, "No figure to rasterize");
		return EINA_FALSE;
	}

	thiz = ENESIM_RASTERIZER_BASIC(r);
	state = &thiz->state;
	/* in case the draw mode has changed */
	draw_mode = enesim_renderer_shape_draw_mode_get(r);
	if ((state->draw_mode != draw_mode) &&
			((state->draw_mode == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE) ||
			(draw_mode == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)))
	{
		changed = EINA_TRUE;
	}

	if (changed || enesim_rasterizer_has_changed(r))
	{
#if 0
		double sx = 1, sy = 1;
#endif
		double lx, rx, ty, by;

		thiz->vectors = enesim_rasterizer_figure_vectors_get(r, &thiz->nvectors);
		if (!thiz->vectors)
			return EINA_FALSE;
		/* generate the bounds to clip the span functions */
		if (!enesim_figure_bounds(rr->figure, &lx, &ty, &rx, &by))
			return EINA_FALSE;
#if 0
		if ((draw_mode == ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL) &&
			 (lx != rx) && (ty != by))
		{
			sx = (rx - lx - 1) / (rx - lx);
			if (sx < (1 / 16.0))
				sx = 1 / 16.0;
			sy = (by - ty - 1) / (by - ty);
			if (sy < (1 / 16.0))
				sy = 1 / 16.0;
		}
#endif
		/* in theory we should add 1 here, like the path renderer
		 * but the span functions handle that for us
		 */
		thiz->tyy = eina_f16p16_double_from(ty);
		thiz->byy = eina_f16p16_double_from(by);
		thiz->lxx = eina_f16p16_double_from(lx);
		thiz->rxx = eina_f16p16_double_from(rx);
	}

	enesim_renderer_transformation_get(r, &matrix);
	enesim_matrix_matrix_f16p16_to(&matrix,
			&thiz->matrix);

	state->color = enesim_renderer_color_get(r);
	state->stroke.color = enesim_renderer_shape_stroke_color_get(r);
	state->stroke.weight = enesim_renderer_shape_stroke_weight_get(r);
	state->stroke.r = enesim_renderer_shape_stroke_renderer_get(r);
	state->fill.color = enesim_renderer_shape_fill_color_get(r);
	state->fill.r = enesim_renderer_shape_fill_renderer_get(r);
	fill_rule = enesim_renderer_shape_fill_rule_get(r);
	state->draw_mode = draw_mode;

	/* setup the colors */
	thiz->scolor = state->stroke.color;
	thiz->fcolor = state->fill.color;
	if (state->color != 0xffffffff)
	{
		thiz->scolor = enesim_color_mul4_sym(state->color, thiz->scolor);
		thiz->fcolor = enesim_color_mul4_sym(state->color, thiz->fcolor);
	}
	/* setup the renderers to use */
	thiz->fpaint = enesim_renderer_ref(state->fill.r);
	thiz->spaint = enesim_renderer_ref(state->stroke.r);

	if (state->draw_mode == ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL)
	{
		thiz->scolor = thiz->fcolor;
		thiz->stroke = EINA_FALSE;
		thiz->sww = 65536;
	}
	else if (state->draw_mode == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE_FILL)
	{
		thiz->stroke = EINA_TRUE;
		if (state->stroke.weight <= 0)
		{
			thiz->sww = 65536;
			thiz->stroke = EINA_FALSE;
		}
		else
		{
			thiz->sww = sqrt(state->stroke.weight) * 65536;
		}
	}
	/* stroke only */
	else
	{
		thiz->sww = sqrt(state->stroke.weight) * 65536;
		thiz->fcolor = 0;
		enesim_renderer_unref(thiz->fpaint);
		thiz->fpaint = NULL;
		thiz->stroke = EINA_TRUE;
	}

	*draw = _spans[fill_rule][thiz->fpaint ? 1 : 0][thiz->spaint ? 1 : 0];
	return EINA_TRUE;
}

static void _basic_sw_hints(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Rop rop EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_SW_HINT_COLORIZE;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RASTERIZER_DESCRIPTOR,
		Enesim_Rasterizer_Basic, Enesim_Rasterizer_Basic_Class,
		enesim_rasterizer_basic);

static void _enesim_rasterizer_basic_class_init(void *k)
{
	Enesim_Renderer_Class *r_klass;
	Enesim_Rasterizer_Class *klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _basic_name;
	r_klass->sw_setup = _basic_sw_setup;
	r_klass->sw_hints_get = _basic_sw_hints;

	klass = ENESIM_RASTERIZER_CLASS(k);
	klass->sw_cleanup = _basic_sw_cleanup;

	_spans[ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][0][0] = _basic_span_fill_color_stroke_color_nz;
	_spans[ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][1][0] = _basic_span_fill_renderer_stroke_color_nz;
	_spans[ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][0][1] = _basic_span_fill_color_stroke_renderer_nz;
	_spans[ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO][1][1] = _basic_span_fill_renderer_stroke_renderer_nz;
	_spans[ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][0][0] = _basic_span_fill_color_stroke_color_eo;
	_spans[ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][1][0] = _basic_span_fill_renderer_stroke_color_eo;
	_spans[ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][0][1] = _basic_span_fill_color_stroke_renderer_eo;
	_spans[ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD][1][1] = _basic_span_fill_renderer_stroke_renderer_eo;
}

static void _enesim_rasterizer_basic_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_rasterizer_basic_instance_deinit(void *o EINA_UNUSED)
{
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_rasterizer_basic_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_rasterizer_basic);
	return r;
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
