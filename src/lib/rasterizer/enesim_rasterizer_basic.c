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

#define INTERP_65536(a, c0, c1) \
	( ((((((c0 >> 16) & 0xff00) - ((c1 >> 16) & 0xff00)) * a) + \
	  (c1 & 0xff000000)) & 0xff000000) + \
	  ((((((c0 >> 16) & 0xff) - ((c1 >> 16) & 0xff)) * a) + \
	  (c1 & 0xff0000)) & 0xff0000) + \
	  ((((((c0 & 0xff00) - (c1 & 0xff00)) * a) >> 16) + \
	  (c1 & 0xff00)) & 0xff00) + \
	  ((((((c0 & 0xff) - (c1 & 0xff)) * a) >> 16) + \
	  (c1 & 0xff)) & 0xff) )

#define MUL_A_65536(a, c) \
	( ((((c >> 16) & 0xff00) * a) & 0xff000000) + \
	  ((((c >> 16) & 0xff) * a) & 0xff0000) + \
	  ((((c & 0xff00) * a) >> 16) & 0xff00) + \
	  ((((c & 0xff) * a) >> 16) & 0xff) )

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
	Enesim_F16p16_Vector *vectors;
	int nvectors;
	const Enesim_Figure *figure;
	Eina_Bool changed : 1;

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

typedef struct _Enesim_Rasterizer_Basic_Span {
	Enesim_F16p16_Edge *edges;
	int nedges;
	Eina_F16p16 lx;
	Eina_F16p16 rx;
} Enesim_Rasterizer_Basic_Span;

#define ENESIM_RASTERIZER_BASIC_SPAN_INIT(thiz, s)                            \
	s.edges = alloca(thiz->nvectors * sizeof(Enesim_F16p16_Edge));

static inline int _basic_setup_edges(Enesim_Rasterizer_Basic *thiz,
		Enesim_F16p16_Edge *edges, Eina_F16p16 xx, Eina_F16p16 yy,
		Eina_F16p16 *lx, Eina_F16p16 *rx)
{
	Enesim_F16p16_Edge *edge = edges;
	Enesim_F16p16_Vector *v = thiz->vectors;
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

#define SETUP_EDGES \
	edges = alloca(nvectors * sizeof(Enesim_F16p16_Edge)); \
	nedges = _basic_setup_edges(thiz, edges, xx, yy, &lx, &rx); \
 \
	if (!nedges) \
		goto get_out; \
 \
	rx = (rx >> 16) + 2; \
	if (rx < x) \
	{\
		memset(dst, 0, sizeof(unsigned int) * len); \
		return; \
	} \
	else \
	{ \
		rx -= x; \
		if (len > rx) \
		{ \
			/* rx is < 0 */ \
			len -= rx; \
			memset(dst + rx, 0, sizeof(unsigned int) * len); \
			e -= len; \
		} \
		else rx = len; \
	} \
 \
	lx = (lx >> 16) - 1 - x; \
repeat: \
	if (lx > (int)len) \
		lx = len; \
	if (lx > 0) \
	{ \
		if (first) \
			memset(d, 0, sizeof(unsigned int) * lx); \
 \
		xx += lx * axx; \
		d += lx; \
 \
		n = 0; \
		edge = edges; \
		while (n < nedges) \
		{ \
			edge->e += lx * edge->de; \
			edge++; \
			n++; \
		} \
	} \
	else lx = 0; \
	printf("first x: %d -> %d %d\n", x, lx, rx);


static inline void _basic_eval_edges_nz(Enesim_F16p16_Edge *edges,
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

#define EVAL_EDGES_NZ \
	_basic_eval_edges_nz(edges, nedges, xx, sww, &a, &count); \
 \
	if (!a) \
	{ \
		int nx = rx; \
 \
		edge = edges; \
		n = 0; \
		while (n < nedges) \
		{ \
			if ((x <= edge->lx) & (nx > edge->lx)) \
				nx = edge->lx; \
			edge->e -= edge->de; \
			edge++; \
			n++; \
		} \
		lx = nx - x; \
		if (lx < 1) \
			lx = 1; \
		outside = !count; \
		goto repeat; \
	}

#if 0
/* identity */
/* stroke and/or fill with possibly a fill renderer non-zero rule */
static void _stroke_fill_paint_nz(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = ENESIM_RASTERIZER_BASIC(r);
	Enesim_Rasterizer_Basic_State *state = &thiz->state;
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *fpaint;
	int stroke = 0;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	uint32_t *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	double ox, oy;
	int lx = INT_MAX / 2, rx = -lx;
	int first = 1, outside = 0;

	int axx = thiz->matrix.xx, axz = thiz->matrix.xz;
	int ayy = thiz->matrix.yy, ayz = thiz->matrix.yz;
	int xx = (axx * x) + (axx >> 1) + axz - 32768;
	int yy = (ayy * y) + (ayy >> 1) + ayz - 32768;

	ox = state->ox;
	oy = state->oy;
	xx -= eina_f16p16_double_from(ox);
	yy -= eina_f16p16_double_from(oy);

	if ((((yy >> 16) + 1) < (thiz->tyy >> 16)) ||
			((yy >> 16) > (1 + (thiz->byy >> 16))))
	{
get_out:
		memset(d, 0, sizeof(unsigned int) * len);
		return;
	}

	SETUP_EDGES

	printf("first\n");
	if (first)
	{
		first = 0;
		scolor = state->stroke.color;
		fcolor = state->fill.color;
		fpaint = state->fill.r;
		color = state->color;
		if (color != 0xffffffff)
		{
			scolor = enesim_color_mul4_sym(color, scolor);
			fcolor = enesim_color_mul4_sym(color, fcolor);
		}

		sw = state->stroke.weight;
		if (state->draw_mode == ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL)
		{
			sww = 65536;
			scolor = fcolor;
			stroke = 0;
			if (fpaint)
				enesim_renderer_sw_draw(fpaint, x + lx, y, rx - lx, dst + lx);
		}
		else if (state->draw_mode == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE_FILL)
		{
			stroke = 1;
			if (sw <= 0)
			{
				sww = 65536;
				stroke = 0;
			}
			else
				sww = sqrt(sw) * 65536;
			if (fpaint)
				enesim_renderer_sw_draw(fpaint, x + lx, y, rx - lx, dst + lx);
		}
		else  // if (state->draw_mode == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
		{
			if (sw <= 0)
			{
				memset(d, 0, sizeof(unsigned int) * len);
				return;
			}
			sww = sqrt(sw) * 65536;
			fcolor = 0;
			fpaint = NULL;
			stroke = 1;
		}
		rx += x;
	}
	else
	{
		int dx = lx;

		dst = d - lx;
		if (dx > (e - dst))
			dx = (e - dst);
		if (outside)
		{
			memset(dst, 0, sizeof(unsigned int) * dx);
		}
		else
		{
			uint32_t *ne = dst + dx;

			if (!fpaint)
			{
				while(dst < ne)
					*dst++ = fcolor;
			}
			else if (fcolor != 0xffffffff)
			{
				enesim_color_mul4_sym_sp_none_color_none(dst, ne - dst, fcolor);
				dst = ne;
			}
		}
	}

	x += lx;
	while (d < e)
	{
		unsigned int p0 = fcolor;
		int count = 0;
		int a = 0;

		EVAL_EDGES_NZ

		if (count)
		{
			if (fpaint)
			{
				p0 = *d;
				if (fcolor != 0xffffffff)
					p0 = enesim_color_mul4_sym(fcolor, p0);
			}

			if (stroke)
			{
				unsigned int q0 = p0;

				p0 = scolor;
				if (a < 65536)
					p0 = INTERP_65536(a, p0, q0);
			}
		}
		else
		{
			p0 = scolor;
			if (fpaint && !stroke)
			{
				p0 = *d;
				if (fcolor != 0xffffffff)
					p0 = enesim_color_mul4_sym(fcolor, p0);
			}
			if (a < 65536)
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		xx += axx;
		x++;
	}
}
#else

static inline void _fill_renderer_setup(Enesim_Rasterizer_Basic *thiz, int x, int y, int len, uint32_t *dst)
{
	enesim_renderer_sw_draw(thiz->fpaint, x, y, len, dst);
}

static inline void _fill_color_renderer_setup(Enesim_Rasterizer_Basic *thiz, int x, int y, int len, uint32_t *dst)
{
	if (thiz->fpaint)
		_fill_renderer_setup(thiz, x, y, len, dst);
}

static inline void _stroke_renderer_setup(Enesim_Rasterizer_Basic *thiz, int x, int y, int len, uint32_t *dst)
{
	enesim_renderer_sw_draw(thiz->spaint, x, y, len, dst);
}

static inline void _fill_color_setup(Enesim_Rasterizer_Basic *thiz, int x, int y, int len, uint32_t *dst)
{

}

static inline void _stroke_color_setup(Enesim_Rasterizer_Basic *thiz, int x, int y, int len, uint32_t *dst)
{

}

static inline void _fill_renderer_advance(Enesim_Rasterizer_Basic *thiz, uint32_t *dst, int len)
{
	enesim_color_mul4_sym_sp_none_color_none(dst, len, thiz->fcolor);
}

static inline void _fill_color_advance(Enesim_Rasterizer_Basic *thiz, uint32_t *dst, int len)
{
	enesim_color_fill_sp_none_color_none(dst, len, thiz->fcolor);
}

static inline void _fill_color_renderer_advance(Enesim_Rasterizer_Basic *thiz, uint32_t *dst, int len)
{
	if (!thiz->fpaint)
	{
		_fill_color_advance(thiz, dst, len);
	}
	else if (thiz->fcolor != 0xffffffff)
		_fill_renderer_advance(thiz, dst, len);
}

static inline uint32_t _fill_color_renderer_stroke_color_draw(Enesim_Rasterizer_Basic *thiz, uint32_t *d, int count, int a)
{
	uint32_t p0;

	if (count)
	{
		p0 = thiz->fcolor;
		if (thiz->fpaint)
		{
			p0 = *d;
			if (thiz->fcolor != 0xffffffff)
				p0 = enesim_color_mul4_sym(thiz->fcolor, p0);
		}

		if (thiz->stroke)
		{
			unsigned int q0 = p0;

			p0 = thiz->scolor;
			if (a < 65536)
				p0 = INTERP_65536(a, p0, q0);
		}
	}
	else
	{
		p0 = thiz->scolor;
		if (thiz->fpaint && !thiz->stroke)
		{
			p0 = *d;
			if (thiz->fcolor != 0xffffffff)
				p0 = enesim_color_mul4_sym(thiz->fcolor, p0);
		}
		if (a < 65536)
			p0 = MUL_A_65536(a, p0);
	}
	return p0;
}

static inline uint32_t _fill_color_stroke_color_draw(Enesim_Rasterizer_Basic *thiz, uint32_t *d, int count, int a)
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
				p0 = INTERP_65536(a, p0, q0);
		}
	}
	else
	{
		p0 = thiz->scolor;
		if (a < 65536)
		{
			p0 = MUL_A_65536(a, p0);
		}
	}

	return p0;

}

static inline uint32_t _fill_renderer_stroke_color_draw(Enesim_Rasterizer_Basic *thiz, uint32_t *d, int count, int a)
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
				p0 = INTERP_65536(a, p0, q0);
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
			p0 = MUL_A_65536(a, p0);
		}
	}

	return p0;
}


static inline uint32_t _fill_color_stroke_renderer_draw(Enesim_Rasterizer_Basic *thiz, uint32_t *d, int count, int a)
{
	uint32_t p0;

	p0 = *d;
	if (thiz->scolor != 0xffffffff)
		p0 = enesim_color_mul4_sym(thiz->scolor, p0);
	if (a < 65536)
	{
		if (count)
			p0 = INTERP_65536(a, p0, thiz->fcolor);
		else
			p0 = MUL_A_65536(a, p0);
	}
	return p0;
}

static void _stroke_fill_paint_nz(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = ENESIM_RASTERIZER_BASIC(r);
	Enesim_Rasterizer_Basic_State *state = &thiz->state;
	uint32_t *dst = ddata;
	uint32_t *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	double ox, oy;
	int lx = INT_MAX / 2, rx = -lx;
	Eina_Bool outside = EINA_TRUE;

	int xx = eina_f16p16_int_from(x);
	int yy = eina_f16p16_int_from(y);

	ox = state->ox;
	oy = state->oy;
	xx -= eina_f16p16_double_from(ox);
	yy -= eina_f16p16_double_from(oy);

	edges = alloca(nvectors * sizeof(Enesim_F16p16_Edge));
	nedges = _basic_setup_edges(thiz, edges, xx, yy, &lx, &rx);
	if (!_basic_clip(x, x + len, (lx >> 16) - 1, (rx >> 16) + 2, &lx, &rx))
	{
		memset(d, 0, sizeof(uint32_t) * len);
		return;
	}
	/* the most right coordinate found is smaller than the requested length to draw */
	if (rx < x + len)
	{
		int roff = rx - x;
		int nlen = (x + len) - rx;

		memset(d + roff, 0, sizeof(uint32_t) * nlen);
		len -= nlen;
		e -= nlen;
	}

	_fill_color_renderer_setup(thiz, lx, y, rx - lx, dst + (lx - x));
	/* lx will be an offset from now on */
	lx -= x;
repeat:
	if (lx > 0)
	{
		if (outside)
		{
			memset(d, 0, sizeof(uint32_t) * lx);
		}
		else
		{
			_fill_color_renderer_advance(thiz, d, lx);
		}

		/* advance the edges by lx */
		n = 0;
		edge = edges;
		while (n < nedges)
		{
			edge->e += lx * edge->de;
			edge++;
			n++;
		}

		/* advance our position by lx */
		xx += lx * EINA_F16P16_ONE;
		x += lx;
		d += lx;
	}

	while (d < e)
	{
		unsigned int p0;
		int count = 0;
		int a = 0;

		_basic_eval_edges_nz(edges, nedges, xx, thiz->sww, &a, &count);
		if (!a)
		{
			int nx = rx;

			edge = edges;
			n = 0;
			while (n < nedges)
			{
				if ((x <= edge->lx) & (nx > edge->lx))
					nx = edge->lx;
				edge->e -= edge->de;
				edge++;
				n++;
			}
			lx = nx - x;
			if (lx < 1)
				lx = 1;
			outside = !count;
			goto repeat;
		}

		p0 = _fill_color_renderer_stroke_color_draw(thiz, d, count, a);
		*d++ = p0;
		xx += EINA_F16P16_ONE;
		x++;
	}
}


#endif

/* stroke with a renderer and possibly fill with color non-zero rule */
static void _stroke_paint_fill_nz(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = ENESIM_RASTERIZER_BASIC(r);
	Enesim_Rasterizer_Basic_State *state = &thiz->state;
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *spaint;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	uint32_t *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	double ox, oy;
	int lx = INT_MAX / 2, rx = -lx;
	int first = 1, outside = 0;

	int axx = thiz->matrix.xx, axz = thiz->matrix.xz;
	int ayy = thiz->matrix.yy, ayz = thiz->matrix.yz;
	int xx = (axx * x) + (axx >> 1) + axz - 32768;
	int yy = (ayy * y) + (ayy >> 1) + ayz - 32768;

	ox = state->ox;
	oy = state->oy;
	xx -= eina_f16p16_double_from(ox);
	yy -= eina_f16p16_double_from(oy);

	if ((((yy >> 16) + 1) < (thiz->tyy >> 16)) ||
			((yy >> 16) > (1 + (thiz->byy >> 16))))
	{
get_out:
		memset(d, 0, sizeof(unsigned int) * len);
		return;
	}

	SETUP_EDGES

	if (first)
	{
		first = 0;
		scolor = state->stroke.color;
		spaint = state->stroke.r;
		fcolor = state->fill.color;

		color = state->color;
		if (color != 0xffffffff)
		{
			scolor = enesim_color_mul4_sym(color, scolor);
			fcolor = enesim_color_mul4_sym(color, fcolor);
		}

		sw = state->stroke.weight;
		sww = sqrt(sw) * 65536;

		if (state->draw_mode == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
			fcolor = 0;

		enesim_renderer_sw_draw(spaint, x + lx, y, rx - lx, dst + lx);
		rx += x;
	}
	else
	{
		int dx = lx;

		dst = d - lx;
		if (dx > (e - dst))
			dx = (e - dst);
		if (outside)
			memset(dst, 0, sizeof(unsigned int) * dx);
		else
		{
			uint32_t *ne = dst + dx;

			while (dst < ne)
				*dst++ = fcolor;
		}
	}

	x += lx;
	while (d < e)
	{
		unsigned int p0 = *d;
		int count = 0;
		int a = 0;

		EVAL_EDGES_NZ

		if (scolor != 0xffffffff)
			p0 = enesim_color_mul4_sym(scolor, p0);
		if (a < 65536)
		{
			if (count)
				p0 = INTERP_65536(a, p0, fcolor);
			else
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		xx += axx;
		x++;
	}
}

/* stroke and fill with renderers non-zero rule */
static void _stroke_paint_fill_paint_nz(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = ENESIM_RASTERIZER_BASIC(r);
	Enesim_Rasterizer_Basic_State *state = &thiz->state;
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *fpaint, *spaint;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	uint32_t *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	double ox, oy;
	int lx = INT_MAX / 2, rx = -lx;
	int first = 1, outside = 0;

	int axx = thiz->matrix.xx, axz = thiz->matrix.xz;
	int ayy = thiz->matrix.yy, ayz = thiz->matrix.yz;
	int xx = (axx * x) + (axx >> 1) + axz - 32768;
	int yy = (ayy * y) + (ayy >> 1) + ayz - 32768;

	unsigned int *sbuf, *s;

	ox = state->ox;
	oy = state->oy;
	xx -= eina_f16p16_double_from(ox);
	yy -= eina_f16p16_double_from(oy);

	if ((((yy >> 16) + 1) < (thiz->tyy >> 16)) ||
			((yy >> 16) > (1 + (thiz->byy >> 16))))
	{
get_out:
		memset(d, 0, sizeof(unsigned int) * len);
		return;
	}

	SETUP_EDGES

	if (first)
	{
		first = 0;
		scolor = state->stroke.color;
		spaint = state->stroke.r;
		fcolor = state->fill.color;
		fpaint = state->fill.r;

		color = state->color;
		if (color != 0xffffffff)
		{
			scolor = enesim_color_mul4_sym(color, scolor);
			fcolor = enesim_color_mul4_sym(color, fcolor);
		}

		sw = state->stroke.weight;
		sww = sqrt(sw) * 65536;

		enesim_renderer_sw_draw(fpaint, x + lx, y, rx - lx, dst + lx);

		sbuf = alloca((rx - lx) * sizeof(unsigned int));
		enesim_renderer_sw_draw(spaint, x + lx, y, rx - lx, sbuf);
		s = sbuf;
		rx += x;
	}
	else
	{
		int dx = lx;

		dst = d - lx;
		if (dx > (e - dst))
			dx = (e - dst);

		if (outside)
			memset(dst, 0, sizeof(unsigned int) * dx);
		else
		{
			enesim_color_mul4_sym_sp_none_color_none(dst, dx, fcolor);
			dst += dx;
		}
	}

	x += lx;
	while (d < e)
	{
		unsigned int p0 = *s;
		int count = 0;
		int a = 0;

		EVAL_EDGES_NZ

		if (scolor != 0xffffffff)
			p0 = enesim_color_mul4_sym(scolor, p0);
		if (a < 65536)
		{
			if (count)
			{
				unsigned int q0 = *d;

				if (fcolor != 0xffffffff)
					q0 = enesim_color_mul4_sym(fcolor, q0);
				p0 = INTERP_65536(a, p0, q0);
			}
			else
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		s++;
		xx += axx;
		x++;
	}
}


#define EVAL_EDGES_EO \
		edge = edges; \
		n = 0; \
		while (n < nedges) \
		{ \
			int ee = edge->e; \
 \
			if (edge->counted) \
			{ \
				np += (ee >= 0); \
				nn += (ee < 0); \
			} \
			if (ee < 0) \
				ee = -ee; \
 \
			if ((ee < sww) && ((xx + 0xffff) >= edge->xx0) & \
					(xx <= (0xffff + edge->xx1))) \
			{ \
				if (a < sww/4) \
					a = sww - ee; \
				else \
					a = (a + (sww - ee)) / 2; \
			} \
 \
			edge->e += edge->de; \
			edge++; \
			n++; \
		} \
 \
		if ((np + nn) % 4) \
			in = !(np % 2); \
		else \
			in = (np % 2); \
 \
		if (!a) \
		{ \
			int nx = rx; \
 \
			edge = edges; \
			n = 0; \
			while (n < nedges) \
			{ \
				if ((x <= edge->lx) & (nx > edge->lx)) \
					nx = edge->lx; \
				edge->e -= edge->de; \
				edge++; \
				n++; \
			} \
			lx = nx - x; \
			if (lx < 1) \
				lx = 1; \
			outside = !in; \
			goto repeat; \
		}

/* identity */
/* stroke and/or fill with possibly a fill renderer even-odd rule */
static void _stroke_fill_paint_eo(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = ENESIM_RASTERIZER_BASIC(r);
	Enesim_Rasterizer_Basic_State *state = &thiz->state;
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *fpaint;
	int stroke = 0;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	uint32_t *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	double ox, oy;
	int lx = INT_MAX / 2, rx = -lx;
	int first = 1, outside = 0;

	int axx = thiz->matrix.xx, axz = thiz->matrix.xz;
	int ayy = thiz->matrix.yy, ayz = thiz->matrix.yz;
	int xx = (axx * x) + (axx >> 1) + axz - 32768;
	int yy = (ayy * y) + (ayy >> 1) + ayz - 32768;

	ox = state->ox;
	oy = state->oy;
	xx -= eina_f16p16_double_from(ox);
	yy -= eina_f16p16_double_from(oy);

	if ((((yy >> 16) + 1) < (thiz->tyy >> 16)) ||
			((yy >> 16) > (1 + (thiz->byy >> 16))))
	{
get_out:
		memset(d, 0, sizeof(unsigned int) * len);
		return;
	}

	SETUP_EDGES

	if (first)
	{
		first = 0;
		scolor = state->stroke.color;
		fcolor = state->fill.color;
		fpaint = state->fill.r;

		color = state->color;
		if (color != 0xffffffff)
		{
			scolor = enesim_color_mul4_sym(color, scolor);
			fcolor = enesim_color_mul4_sym(color, fcolor);
		}

		sw = state->stroke.weight;
		if (state->draw_mode == ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL)
		{
			sww = 65536;
			scolor = fcolor;
			stroke = 0;
			if (fpaint)
				enesim_renderer_sw_draw(fpaint, x + lx, y, rx - lx, dst + lx);
		}
		else if (state->draw_mode == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE_FILL)
		{
			stroke = 1;
			if (sw <= 0)
			{
				sww = 65536;
				stroke = 0;
			}
			else
				sww = sqrt(sw) * 65536;
			if (fpaint)
				enesim_renderer_sw_draw(fpaint, x + lx, y, rx - lx, dst + lx);
		}
		else  // if (state->draw_mode == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
		{
			if (sw <= 0)
			{
				memset(d, 0, sizeof(unsigned int) * len);
				return;
			}
			sww = sqrt(sw) * 65536;
			fcolor = 0;
			fpaint = NULL;
			stroke = 1;
		}
		rx += x;
	}
	else
	{
		int dx = lx;

		dst = d - lx;
		if (dx > (e - dst))
			dx = (e - dst);
		if (outside)
			memset(dst, 0, sizeof(unsigned int) * dx);
		else
		{
			uint32_t *ne = dst + dx;

			if (!fpaint)
			{
				while(dst < ne)
					*dst++ = fcolor;
			}
			else if (fcolor != 0xffffffff)
			{
				enesim_color_mul4_sym_sp_none_color_none(dst, ne - dst, fcolor);
				dst = ne;
			}
		}
	}

	x += lx;
	while (d < e)
	{
		unsigned int p0 = 0;
		int in = 0;
		int np = 0, nn = 0;
		int a = 0;

		EVAL_EDGES_EO

		if (in)
		{
			p0 = fcolor;
			if (fpaint)
			{
				p0 = *d;
				if (fcolor != 0xffffffff)
					p0 = enesim_color_mul4_sym(fcolor, p0);
			}

			if (stroke && a)
			{
				unsigned int q0 = p0;

				p0 = scolor;
				if (a < 65536)
					p0 = INTERP_65536(a, p0, q0);
			}
		}
		else if (a)
		{
			p0 = scolor;
			if (fpaint && !stroke)
			{
				p0 = *d;
				if (fcolor != 0xffffffff)
					p0 = enesim_color_mul4_sym(fcolor, p0);
			}
			if (a < 65536)
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		xx += axx;
		x++;
	}
}

/* stroke with a renderer and possibly fill with color even-odd rule */
static void _stroke_paint_fill_eo(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = ENESIM_RASTERIZER_BASIC(r);
	Enesim_Rasterizer_Basic_State *state = &thiz->state;
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *spaint;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	uint32_t *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	double ox, oy;
	int lx = INT_MAX / 2, rx = -lx;
	int first = 1, outside = 0;

	int axx = thiz->matrix.xx, axz = thiz->matrix.xz;
	int ayy = thiz->matrix.yy, ayz = thiz->matrix.yz;
	int xx = (axx * x) + (axx >> 1) + axz - 32768;
	int yy = (ayy * y) + (ayy >> 1) + ayz - 32768;

	ox = state->ox;
	oy = state->oy;
	xx -= eina_f16p16_double_from(ox);
	yy -= eina_f16p16_double_from(oy);

	if ((((yy >> 16) + 1) < (thiz->tyy >> 16)) ||
			((yy >> 16) > (1 + (thiz->byy >> 16))))
	{
get_out:
		memset(d, 0, sizeof(unsigned int) * len);
		return;
	}

	SETUP_EDGES

	if (first)
	{
		first = 0;
		scolor = state->stroke.color;
		spaint = state->stroke.r;
		fcolor = state->fill.color;

		color = state->color;
		if (color != 0xffffffff)
		{
			scolor = enesim_color_mul4_sym(color, scolor);
			fcolor = enesim_color_mul4_sym(color, fcolor);
		}

		sw = state->stroke.weight;
		sww = sqrt(sw) * 65536;

		if (state->draw_mode == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
			fcolor = 0;

		enesim_renderer_sw_draw(spaint, x + lx, y, rx - lx, dst + lx);
		rx += x;
	}
	else
	{
		int dx = lx;

		dst = d - lx;
		if (dx > (e - dst))
			dx = (e - dst);
		if (outside)
			memset(dst, 0, sizeof(unsigned int) * dx);
		else
		{
			uint32_t *ne = dst + dx;

			while (dst < ne)
				*dst++ = fcolor;
		}
	}

	x += lx;
	while (d < e)
	{
		unsigned int p0 = *d;
		int in = 0;
		int np = 0, nn = 0;
		int a = 0;

		EVAL_EDGES_EO

		if (scolor != 0xffffffff)
			p0 = enesim_color_mul4_sym(scolor, p0);
		if (a < 65536)
		{
			if (in)
				p0 = INTERP_65536(a, p0, fcolor);
			else
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		xx += axx;
		x++;
	}
}

/* stroke and fill with renderers even-odd rule */
static void _stroke_paint_fill_paint_eo(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = ENESIM_RASTERIZER_BASIC(r);
	Enesim_Rasterizer_Basic_State *state = &thiz->state;
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *fpaint, *spaint;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	uint32_t *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	double ox, oy;
	int lx = INT_MAX / 2, rx = -lx;
	int first = 1, outside = 0;

	int axx = thiz->matrix.xx, axz = thiz->matrix.xz;
	int ayy = thiz->matrix.yy, ayz = thiz->matrix.yz;
	int xx = (axx * x) + (axx >> 1) + axz - 32768;
	int yy = (ayy * y) + (ayy >> 1) + ayz - 32768;

	unsigned int *sbuf, *s;

	ox = state->ox;
	oy = state->oy;
	xx -= eina_f16p16_double_from(ox);
	yy -= eina_f16p16_double_from(oy);

	if ((((yy >> 16) + 1) < (thiz->tyy >> 16)) ||
			((yy >> 16) > (1 + (thiz->byy >> 16))))
	{
get_out:
		memset(d, 0, sizeof(unsigned int) * len);
		return;
	}

	SETUP_EDGES

	if (first)
	{
		first = 0;
		scolor = state->stroke.color;
		spaint = state->stroke.r;
		fcolor = state->fill.color;
		fpaint = state->fill.r;

		color = state->color;
		if (color != 0xffffffff)
		{
			scolor = enesim_color_mul4_sym(color, scolor);
			fcolor = enesim_color_mul4_sym(color, fcolor);
		}

		sw = state->stroke.weight;
		sww = sqrt(sw) * 65536;

		enesim_renderer_sw_draw(fpaint, x + lx, y, rx - lx, dst + lx);

		sbuf = alloca((rx - lx) * sizeof(unsigned int));
		enesim_renderer_sw_draw(spaint, x + lx, y, rx - lx, sbuf);
		s = sbuf;
		rx += x;
	}
	else
	{
		int dx = lx;

		dst = d - lx;
		if (dx > (e - dst))
			dx = (e - dst);

		if (outside)
			memset(dst, 0, sizeof(unsigned int) * dx);
		else
		{
			enesim_color_mul4_sym_sp_none_color_none(dst, dx, fcolor);
			dst += dx;
		}
	}

	x += lx;
	while (d < e)
	{
		unsigned int p0 = *s;
		int in = 0;
		int np = 0, nn = 0;
		int a = 0;

		EVAL_EDGES_EO

		if (scolor != 0xffffffff)
			p0 = enesim_color_mul4_sym(scolor, p0);
		if (a < 65536)
		{
			if (in)
			{
				unsigned int q0 = *d;

				if (fcolor != 0xffffffff)
					q0 = enesim_color_mul4_sym(fcolor, q0);
				p0 = INTERP_65536(a, p0, q0);
			}
			else
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		s++;
		xx += axx;
		x++;
	}
}

/*----------------------------------------------------------------------------*
 *                           Rasterizer interface                             *
 *----------------------------------------------------------------------------*/
static void _basic_figure_set(Enesim_Renderer *r, const Enesim_Figure *figure)
{
	Enesim_Rasterizer_Basic *thiz;

	thiz = ENESIM_RASTERIZER_BASIC(r);
	thiz->figure = figure;
	thiz->changed = EINA_TRUE;
}
/*----------------------------------------------------------------------------*
 *                    The Enesim's rasterizer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _basic_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "basic";
}

static int _tysort(const void *l, const void *r)
{
	Enesim_F16p16_Vector *lv = (Enesim_F16p16_Vector *)l;
	Enesim_F16p16_Vector *rv = (Enesim_F16p16_Vector *)r;

	if (lv->yy0 <= rv->yy0)
		return -1;
	return 1;
}


static Eina_Bool _basic_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *draw, Enesim_Log **error)
{
	Enesim_Rasterizer_Basic *thiz;
	Enesim_Rasterizer_Basic_State *state;
	Enesim_Renderer_Shape_Draw_Mode draw_mode;
	Enesim_Renderer_Shape_Fill_Rule fill_rule;
	Enesim_Matrix matrix;

	thiz = ENESIM_RASTERIZER_BASIC(r);
	state = &thiz->state;
	if (!thiz->figure)
	{
		ENESIM_RENDERER_LOG(r, error, "No figure to rasterize");
		return EINA_FALSE;
	}

	draw_mode = enesim_renderer_shape_draw_mode_get(r);
	if ((state->draw_mode != draw_mode) &&
			((state->draw_mode == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE) ||
			(draw_mode == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)))
	{
		thiz->changed = EINA_TRUE;
	}

	if (thiz->changed)
	{
		Enesim_Polygon *p;
		Enesim_F16p16_Vector *vec;
		Eina_List *l1;
		int nvectors = 0;
		double sx = 1, sy = 1;
		double lx, rx, ty, by;

		if (thiz->vectors)
		{
			free(thiz->vectors);
			thiz->vectors = NULL;
		}

		EINA_LIST_FOREACH(thiz->figure->polygons, l1, p)
		{
			Enesim_Point *first_point;
			Enesim_Point *last_point;
			int pclosed = 0;
			int npts;

			npts = enesim_polygon_point_count(p);
			if ((npts < 2) ||
					((npts < 3) && (draw_mode != ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)))
			{
				ENESIM_RENDERER_LOG(r, error, "Not enough points %d", npts);
				return EINA_FALSE;
			}
			nvectors += npts;
			first_point = eina_list_data_get(p->points);
			last_point = eina_list_data_get(eina_list_last(p->points));
			{
				double x0, x1, y0, y1;
				double x01, y01;
				double len;

				x0 = ((int) (first_point->x * 256)) / 256.0;
				x1 = ((int) (last_point->x * 256)) / 256.0;
				y0 = ((int) (first_point->y * 256)) / 256.0;
				y1 = ((int) (last_point->y * 256)) / 256.0;
				//printf("%g %g -> %g %g\n", x0, y0, x1, y1);
				x01 = x1 - x0;
				y01 = y1 - y0;
				len = hypot(x01, y01);
				//printf("len = %g\n", len);
				if (len < (1 / 256.0))
				{
					//printf("last == first\n");
					nvectors--;
					pclosed = 1;
				}
			}

			if (!p->closed && (draw_mode == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE) && !pclosed)
				nvectors--;
		}

		thiz->vectors = calloc(nvectors, sizeof(Enesim_F16p16_Vector));
		if (!thiz->vectors)
		{
			return EINA_FALSE;
		}
		thiz->nvectors = nvectors;

		vec = thiz->vectors;

		/* generate the bounds to clip the span functions */
		if (!enesim_figure_bounds(thiz->figure, &lx, &ty, &rx, &by))
			return EINA_FALSE;
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
		/* in theory we should add 1 here, like the path renderer
		 * but the span functions handle that for us
		 */
		thiz->tyy = eina_f16p16_double_from(ty);
		thiz->byy = eina_f16p16_double_from(by);
		thiz->lxx = eina_f16p16_double_from(lx);
		thiz->rxx = eina_f16p16_double_from(rx);

		/* FIXME why this loop can't be done on the upper one? */
		EINA_LIST_FOREACH(thiz->figure->polygons, l1, p)
		{
			Enesim_Point *first_point;
			Enesim_Point *last_point;
			Enesim_Point *pt;
			Eina_List *l2;
			int n = 0;
			int nverts = enesim_polygon_point_count(p);
			int sopen = !p->closed;
			int pclosed = 0;

			if (sopen && (draw_mode != ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE))
				sopen = 0;

			first_point = eina_list_data_get(p->points);
			last_point = eina_list_data_get(eina_list_last(p->points));

			{
				double x0, x1, y0, y1;
				double x01, y01;
				double len;

				x0 = ((int) (first_point->x * 256)) / 256.0;
				x1 = ((int) (last_point->x * 256)) / 256.0;
				y0 = ((int) (first_point->y * 256)) / 256.0;
				y1 = ((int) (last_point->y * 256)) / 256.0;
				//printf("%g %g -> %g %g\n", x0, y0, x1, y1);
				x01 = x1 - x0;
				y01 = y1 - y0;
				len = hypot(x01, y01);
				//printf("len = %g\n", len);
				if (len < (1 / 256.0))
				{
					//printf("last == first\n");
					nverts--;
					pclosed = 1;
				}
			}

			if (sopen && !pclosed)
				nverts--;

			pt = first_point;
			l2 = eina_list_next(p->points);
			while (n < nverts)
			{
				Enesim_Point *npt;
				double x0, y0, x1, y1;
				double x01, y01;
				double len;

				npt = eina_list_data_get(l2);
				if ((n == (enesim_polygon_point_count(p) - 1)) && !sopen)
					npt = first_point;
				x0 = sx * (pt->x - lx) + lx;
				y0 = sy * (pt->y - ty) + ty;
				x1 = sx * (npt->x - lx) + lx;
				y1 = sy * (npt->y - ty) + ty;
				x0 = ((int) (x0 * 256)) / 256.0;
				x1 = ((int) (x1 * 256)) / 256.0;
				y0 = ((int) (y0 * 256)) / 256.0;
				y1 = ((int) (y1 * 256)) / 256.0;
				x01 = x1 - x0;
				y01 = y1 - y0;
				len = hypot(x01, y01);
#if 0
				if (len < (1 / 512.0))
				{
					/* FIXME what to do here?
					 * skip this point, pick the next? */
					ENESIM_RENDERER_LOG(r, error, "Length %g < %g for points %gx%g %gx%g", len, 1/512.0, x0, y0, x1, y1);
					return EINA_FALSE;
				}
#endif
				len *= 1 + (1 / 32.0);
				vec->a = -(y01 * 65536) / len;
				vec->b = (x01 * 65536) / len;
				vec->c = (65536 * ((y1 * x0) - (x1 * y0)))
						/ len;
				vec->xx0 = x0 * 65536;
				vec->yy0 = y0 * 65536;
				vec->xx1 = x1 * 65536;
				vec->yy1 = y1 * 65536;

				if ((vec->yy0 == vec->yy1) || (vec->xx0 == vec->xx1))
					vec->sgn = 0;
				else
				{
					vec->sgn = 1;
					if (vec->yy1 > vec->yy0)
					{
						if (vec->xx1 < vec->xx0)
							vec->sgn = -1;
					}
					else
					{
						 if (vec->xx1 > vec->xx0)
							vec->sgn = -1;
					}
				}

				if (vec->xx0 > vec->xx1)
				{
					int xx0 = vec->xx0;
					vec->xx0 = vec->xx1;
					vec->xx1 = xx0;
				}

				if (vec->yy0 > vec->yy1)
				{
					int yy0 = vec->yy0;
					vec->yy0 = vec->yy1;
					vec->yy1 = yy0;
				}

				pt = npt;
				vec++;
				n++;
				l2 = eina_list_next(l2);

			}
		}
		qsort(thiz->vectors, thiz->nvectors, sizeof(Enesim_F16p16_Vector), _tysort);
		thiz->changed = EINA_FALSE;
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
	/* stroke + fill */
	else
	{
		thiz->sww = sqrt(state->stroke.weight) * 65536;
		thiz->fcolor = 0;
		enesim_renderer_unref(thiz->fpaint);
		thiz->fpaint = NULL;
		thiz->stroke = EINA_TRUE;
	}

	if (fill_rule == ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO)
	{
		*draw = _stroke_fill_paint_nz;
		if ((state->stroke.weight > 0.0) && state->stroke.r && (draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE))
		{
			*draw = _stroke_paint_fill_nz;
			if (state->fill.r && (draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL))
				*draw = _stroke_paint_fill_paint_nz;
		}
	}
	else
	{
		*draw = _stroke_fill_paint_eo;
		if ((state->stroke.weight > 0.0) && state->stroke.r && (draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE))
		{
			*draw = _stroke_paint_fill_eo;
			if (state->fill.r && (draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL))
				*draw = _stroke_paint_fill_paint_eo;
		}
	}

	return EINA_TRUE;
}

static void _basic_sw_cleanup(Enesim_Renderer *r EINA_UNUSED, Enesim_Surface *s EINA_UNUSED)
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
	r_klass->sw_cleanup = _basic_sw_cleanup;
	r_klass->sw_hints_get = _basic_sw_hints;

	klass = ENESIM_RASTERIZER_CLASS(k);
	klass->figure_set = _basic_figure_set;
}

static void _enesim_rasterizer_basic_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_rasterizer_basic_instance_deinit(void *o)
{
	Enesim_Rasterizer_Basic *thiz;

	thiz = ENESIM_RASTERIZER_BASIC(o);
	if (thiz->vectors)
		free(thiz->vectors);
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

void enesim_rasterizer_basic_vectors_get(Enesim_Renderer *r, int *nvectors,
		Enesim_F16p16_Vector **vectors)
{
	Enesim_Rasterizer_Basic *thiz;

	thiz = ENESIM_RASTERIZER_BASIC(r);
	*nvectors = thiz->nvectors;
	*vectors = thiz->vectors;
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
