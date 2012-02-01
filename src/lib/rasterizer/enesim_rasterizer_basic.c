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
/* TODO merge this with the bifigure */
#define MUL4_SYM(x, y) \
 ( ((((((x) >> 16) & 0xff00) * (((y) >> 16) & 0xff00)) + 0xff0000) & 0xff000000) + \
   ((((((x) >> 8) & 0xff00) * (((y) >> 16) & 0xff)) + 0xff00) & 0xff0000) + \
   ((((((x) & 0xff00) * ((y) & 0xff00)) + 0xff00) >> 16) & 0xff00) + \
   (((((x) & 0xff) * ((y) & 0xff)) + 0xff) >> 8) )

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

#define ENESIM_RASTERIZER_BASIC_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RASTERIZER_BASIC_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RASTERIZER_BASIC_MAGIC);\
	} while(0)

typedef struct _Enesim_Rasterizer_Basic
{
	EINA_MAGIC
	/* private */
	Enesim_F16p16_Vector *vectors;
	int nvectors;
	const Enesim_Figure *figure;
	Eina_Bool changed : 1;

	/* FIXME this are the boundings calculated at the setup
	 * we either generate them at the Enesim_Polygon level
	 * or something like that
	 */
	int lxx, rxx, tyy, byy;
	Enesim_F16p16_Matrix matrix;
	int draw_mode; // a temp workaround for dealing with 'closed'
} Enesim_Rasterizer_Basic;

static inline Enesim_Rasterizer_Basic * _basic_get(Enesim_Renderer *r)
{
	Enesim_Rasterizer_Basic *thiz;

	thiz = enesim_rasterizer_data_get(r);
	ENESIM_RASTERIZER_BASIC_MAGIC_CHECK(thiz);

	return thiz;
}

#define SETUP_EDGES \
	edges = alloca(nvectors * sizeof(Enesim_F16p16_Edge)); \
	edge = edges; \
	while (n < nvectors) \
	{ \
		if (yy + 0xffff < v->yy0) \
			break; \
		if (((yy + 0xffff) >= v->yy0) & (yy <= (v->yy1 + 0xffff))) \
		{ \
			edge->xx0 = v->xx0; \
			edge->xx1 = v->xx1; \
			edge->yy0 = v->yy0; \
			edge->yy1 = v->yy1; \
			edge->de = v->a; \
			edge->e = ((v->a * (long long int) xx) >> 16) + \
					((v->b * (long long int) yy) >> 16) + \
					v->c; \
			if (lx > v->xx0)  lx = v->xx0; \
			if (rx < v->xx1)  rx = v->xx1; \
			edge++; \
			nedges++; \
		} \
		n++; \
		v++; \
	} \
	if (!nedges) \
		goto get_out; \
 \
	lx = (lx >> 16) - 1 - x; \
	if (lx > 0) \
	{ \
		memset(dst, 0, sizeof(unsigned int) * lx); \
		xx += lx * 65536; \
		d += lx; \
		n = 0;  edge = edges; \
		while (n < nedges) \
		{ \
			edge->e += lx * edge->de; \
			edge++; \
			n++; \
		} \
	} \
	else lx = 0; \
 \
	rx = (rx >> 16) + 2 - x; \
	if (len > rx) \
	{ \
		len -= rx; \
		memset(dst + rx, 0, sizeof(unsigned int) * len); \
		e -= len; \
	} \
	else rx = len;

#define EVAL_EDGES_NZ \
		n = 0; \
		edge = edges; \
		while (n < nedges) \
		{ \
			int ee = edge->e; \
 \
			if ((yy >= edge->yy0) & (yy < edge->yy1)) \
				count += (ee >= 0) - (ee <0); \
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
		}


/* identity */
/* stroke and/or fill with possibly a fill renderer non-zero rule */
static void _stroke_fill_paint_nz(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = _basic_get(r);
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *fpaint;
	int stroke = 0;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	double ox, oy;
	int lx = INT_MAX / 2, rx = -lx;

	int xx = x << 16;
	int yy = y << 16;

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

	scolor = sstate->stroke.color;
	fcolor = sstate->fill.color;
	fpaint = sstate->fill.r;

	color = state->color;
	if (color != 0xffffffff)
	{
		scolor = argb8888_mul4_sym(color, scolor);
		fcolor = argb8888_mul4_sym(color, fcolor);
	}

	sw = sstate->stroke.weight;
	if (sstate->draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL)
	{
		sww = 65536;
		scolor = fcolor;
		stroke = 0;
		if (fpaint)
		{
			enesim_renderer_sw_draw(fpaint, x + lx, y, rx - lx, dst + lx);
		}
	}
	if (sstate->draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL)
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
		{
			enesim_renderer_sw_draw(fpaint, x + lx, y, rx - lx, dst + lx);
		}
	}
	if (sstate->draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)
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

	while (d < e)
	{
		unsigned int p0 = 0;
		int count = 0;
		int a = 0;

		EVAL_EDGES_NZ

		if (count)
		{
			p0 = fcolor;
			if (fpaint)
			{
				p0 = *d;
				if (fcolor != 0xffffffff)
					p0 = MUL4_SYM(fcolor, p0);
			}

			if (stroke && a)
			{
				unsigned int q0 = p0;

				p0 = scolor;
				if (a < 65536)
					p0 = INTERP_65536(a, p0, q0);
			}
		}
		if (a && !count)
		{
			p0 = scolor;
			if (fpaint && !stroke)
			{
				p0 = *d;
				if (fcolor != 0xffffffff)
					p0 = MUL4_SYM(fcolor, p0);
			}
			if (a < 65536)
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		xx += 65536;
	}
}

/* stroke with a renderer and possibly fill with color non-zero rule */
static void _stroke_paint_fill_nz(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = _basic_get(r);
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *spaint;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	double ox, oy;
	int lx = INT_MAX / 2, rx = -lx;

	int xx = x << 16;
	int yy = y << 16;

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

	scolor = sstate->stroke.color;
	spaint = sstate->stroke.r;
	fcolor = sstate->fill.color;

	color = state->color;
	if (color != 0xffffffff)
	{
		scolor = argb8888_mul4_sym(color, scolor);
		fcolor = argb8888_mul4_sym(color, fcolor);
	}

	sw = sstate->stroke.weight;
	sww = sqrt(sw) * 65536;

	if (sstate->draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)
		fcolor = 0;

	enesim_renderer_sw_draw(spaint, x + lx, y, rx - lx, dst + lx);

	while (d < e)
	{
		unsigned int p0 = 0;
		int count = 0;
		int a = 0;

		EVAL_EDGES_NZ

		if (count)
		{
			p0 = fcolor;
			if (a)
			{
				unsigned int q0 = p0;

				p0 = *d;
				if (scolor != 0xffffffff)
					p0 = MUL4_SYM(scolor, p0);

				if (a < 65536)
					p0 = INTERP_65536(a, p0, q0);
			}
		}
		if (a && !count)
		{
			p0 = *d;
			if (scolor != 0xffffffff)
				p0 = MUL4_SYM(scolor, p0);

			if (a < 65536)
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		xx += 65536;
	}
}

/* stroke and fill with renderers non-zero rule */
static void _stroke_paint_fill_paint_nz(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = _basic_get(r);
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *fpaint, *spaint;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	double ox, oy;
	int lx = INT_MAX / 2, rx = -lx;

	int xx = x << 16;
	int yy = y << 16;

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

	scolor = sstate->stroke.color;
	spaint = sstate->stroke.r;
	fcolor = sstate->fill.color;
	fpaint = sstate->fill.r;

	color = state->color;
	if (color != 0xffffffff)
	{
		scolor = argb8888_mul4_sym(color, scolor);
		fcolor = argb8888_mul4_sym(color, fcolor);
	}

	sw = sstate->stroke.weight;
	sww = sqrt(sw) * 65536;

	enesim_renderer_sw_draw(fpaint, x + lx, y, rx - lx, dst + lx);

	sbuf = alloca((rx - lx) * sizeof(unsigned int));
	enesim_renderer_sw_draw(spaint, x + lx, y, rx - lx, sbuf);
	s = sbuf;

	while (d < e)
	{
		unsigned int p0 = 0;
		int count = 0;
		int a = 0;

		EVAL_EDGES_NZ

		if (count)
		{
			p0 = *d;
			if (fcolor != 0xffffffff)
				p0 = MUL4_SYM(fcolor, p0);
			if (a)
			{
				unsigned int q0 = p0;


				p0 = *s;
				if (scolor != 0xffffffff)
					p0 = MUL4_SYM(scolor, p0);

				if (a < 65536)
					p0 = INTERP_65536(a, p0, q0);
			}
		}
		if (a && !count)
		{
			p0 = *s;
			if (scolor != 0xffffffff)
				p0 = MUL4_SYM(scolor, p0);

			if (a < 65536)
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		s++;
		xx += 65536;
	}
}

#define SETUP_EDGES_AFFINE \
	y0 = yy >> 16; \
	y1 = yy + ((len - 1) * ayx); \
	y1 = y1 >> 16; \
	if (y1 < y0) \
	{ \
		y0 = y1; \
		y1 = yy >> 16; \
	} \
	edges = alloca(nvectors * sizeof(Enesim_F16p16_Edge)); \
	edge = edges; \
	while (n < nvectors) \
	{ \
		if ((y0 <= (v->yy1 >> 16)) && (y1 >= (v->yy0 >> 16))) \
		{ \
			edge->xx0 = v->xx0; \
			edge->xx1 = v->xx1; \
			edge->yy0 = v->yy0; \
			edge->yy1 = v->yy1; \
			edge->de = ((v->a * (long long int) axx) >> 16) + \
					((v->b * (long long int) ayx) >> 16); \
			edge->e = ((v->a * (long long int) xx) >> 16) + \
					((v->b * (long long int) yy) >> 16) + \
					v->c; \
			edge++; \
			nedges++; \
		} \
		n++; \
		v++; \
	} \
	if (!nedges) \
		goto get_out;

#define EVAL_EDGES_AFFINE_NZ \
		n = 0; \
		edge = edges; \
		while (n < nedges) \
		{ \
			int ee = edge->e; \
 \
			if (((yy + 0xffff) >= edge->yy0) && \
					(yy <= (edge->yy1 + 0xffff))) \
			{ \
				if ((yy >= edge->yy0) && (yy < edge->yy1)) \
					count += (ee >= 0) - (ee <0); \
				if (ee < 0) \
					ee = -ee; \
				if ((ee < sww) && \
						((xx + 0xffff) >= edge->xx0) && \
						(xx <= (0xffff + edge->xx1))) \
				{ \
					if (a < sww/4) \
						a = sww - ee; \
					else \
						a = (a + (sww - ee)) / 2; \
				} \
			} \
 \
			edge->e += edge->de; \
			edge++; \
			n++; \
		}

/* affine general */
/* stroke and/or fill with possibly a fill renderer non-zero rule */
static void _stroke_fill_paint_affine_nz(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = _basic_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *fpaint;
	int stroke = 0;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	int y0, y1;
	double ox, oy;

	int axx = thiz->matrix.xx, axy = thiz->matrix.xy, axz =
			thiz->matrix.xz;
	int ayx = thiz->matrix.yx, ayy = thiz->matrix.yy, ayz =
			thiz->matrix.yz;
	int xx = (axx * x) + (axx >> 1) + (axy * y) + (axy >> 1) + axz - 32768;
	int yy = (ayx * x) + (ayx >> 1) + (ayy * y) + (ayy >> 1) + ayz - 32768;

	ox = state->ox;
	oy = state->oy;
	xx -= eina_f16p16_double_from(ox);
	yy -= eina_f16p16_double_from(oy);

	if (((ayx <= 0) && ((yy >> 16) + 1 < (thiz->tyy >> 16))) || ((ayx >= 0)
			&& ((yy >> 16) > 1 + (thiz->byy >> 16))))
	{
get_out:
		memset(d, 0, sizeof(unsigned int) * len);
		return;
	}

	SETUP_EDGES_AFFINE

	scolor = sstate->stroke.color;
	fcolor = sstate->fill.color;
	fpaint = sstate->fill.r;

	color = state->color;
	if (color != 0xffffffff)
	{
		scolor = argb8888_mul4_sym(color, scolor);
		fcolor = argb8888_mul4_sym(color, fcolor);
	}

	sw = sstate->stroke.weight;
	draw_mode = sstate->draw_mode;
	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL)
	{
		sww = 65536;
		scolor = fcolor;
		stroke = 0;
		if (fpaint)
		{
			enesim_renderer_sw_draw(fpaint, x, y, len, dst);
		}
	}
	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL)
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
		{
			enesim_renderer_sw_draw(fpaint, x, y, len, dst);
		}
	}
	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)
	{
		if (sw <= 0)
		{
			memset(d, 0, sizeof (unsigned int) * len);
			return;
		}

		sww = sqrt(sw) * 65536;
		fcolor = 0;
		fpaint = NULL;
		stroke = 1;
	}

	while (d < e)
	{
		unsigned int p0 = 0;
		int count = 0;
		int a = 0;

		EVAL_EDGES_AFFINE_NZ
	
		if (count)
		{
			p0 = fcolor;
			if (fpaint)
			{
				p0 = *d;
				if (fcolor != 0xffffffff)
					p0 = MUL4_SYM(fcolor, p0);
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
					p0 = MUL4_SYM(fcolor, p0);
			}
			if (a < 65536)
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		yy += ayx;
		xx += axx;
	}
}

/* stroke with a renderer and possibly fill with color non-zero rule */
static void _stroke_paint_fill_affine_nz(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = _basic_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *spaint;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	int y0, y1;
	double ox, oy;

	int axx = thiz->matrix.xx, axy = thiz->matrix.xy, axz =
			thiz->matrix.xz;
	int ayx = thiz->matrix.yx, ayy = thiz->matrix.yy, ayz =
			thiz->matrix.yz;
	int xx = (axx * x) + (axx >> 1) + (axy * y) + (axy >> 1) + axz - 32768;
	int yy = (ayx * x) + (ayx >> 1) + (ayy * y) + (ayy >> 1) + ayz - 32768;

	ox = state->ox;
	oy = state->oy;
	xx -= eina_f16p16_double_from(ox);
	yy -= eina_f16p16_double_from(oy);

	if (((ayx <= 0) && ((yy >> 16) + 1 < (thiz->tyy >> 16))) || ((ayx >= 0)
			&& ((yy >> 16) > 1 + (thiz->byy >> 16))))
	{
get_out:
		memset(d, 0, sizeof(unsigned int) * len);
		return;
	}

	SETUP_EDGES_AFFINE

	scolor = sstate->stroke.color;
	spaint = sstate->stroke.r;
	fcolor = sstate->fill.color;

	color = state->color;
	if (color != 0xffffffff)
	{
		scolor = argb8888_mul4_sym(color, scolor);
		fcolor = argb8888_mul4_sym(color, fcolor);
	}

	sw = sstate->stroke.weight;
	sww = sqrt(sw) * 65536;

	draw_mode = sstate->draw_mode;
	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)
		fcolor = 0;

	enesim_renderer_sw_draw(spaint, x, y, len, dst);

	while (d < e)
	{
		unsigned int p0 = 0;
		int count = 0;
		int a = 0;

		EVAL_EDGES_AFFINE_NZ
	
		if (count)
		{
			p0 = fcolor;
			if (a)
			{
				unsigned int q0 = p0;

				p0 = *d;
				if (scolor != 0xffffffff)
					p0 = MUL4_SYM(scolor, p0);

				if (a < 65536)
					p0 = INTERP_65536(a, p0, q0);
			}
		}
		else if (a)
		{
			p0 = *d;
			if (scolor != 0xffffffff)
				p0 = MUL4_SYM(scolor, p0);

			if (a < 65536)
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		yy += ayx;
		xx += axx;
	}
}

/* stroke and fill with renderers non-zero rule */
static void _stroke_paint_fill_paint_affine_nz(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = _basic_get(r);
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *fpaint, *spaint;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	int y0, y1;
	double ox, oy;

	int axx = thiz->matrix.xx, axy = thiz->matrix.xy, axz =
			thiz->matrix.xz;
	int ayx = thiz->matrix.yx, ayy = thiz->matrix.yy, ayz =
			thiz->matrix.yz;
	int xx = (axx * x) + (axx >> 1) + (axy * y) + (axy >> 1) + axz - 32768;
	int yy = (ayx * x) + (ayx >> 1) + (ayy * y) + (ayy >> 1) + ayz - 32768;
	unsigned int *sbuf, *s;

	ox = state->ox;
	oy = state->oy;
	xx -= eina_f16p16_double_from(ox);
	yy -= eina_f16p16_double_from(oy);

	if (((ayx <= 0) && ((yy >> 16) + 1 < (thiz->tyy >> 16))) || ((ayx >= 0)
			&& ((yy >> 16) > 1 + (thiz->byy >> 16))))
	{
get_out:
		memset(d, 0, sizeof(unsigned int) * len);
		return;
	}

	SETUP_EDGES_AFFINE

	scolor = sstate->stroke.color;
	spaint = sstate->stroke.r;
	fcolor = sstate->fill.color;
	fpaint = sstate->fill.r;

	color = state->color;
	if (color != 0xffffffff)
	{
		scolor = argb8888_mul4_sym(color, scolor);
		fcolor = argb8888_mul4_sym(color, fcolor);
	}

	sw = sstate->stroke.weight;
	sww = sqrt(sw) * 65536;

	enesim_renderer_sw_draw(fpaint, x, y, len, dst);
	sbuf = alloca(len * sizeof(unsigned int));
	enesim_renderer_sw_draw(spaint, x, y, len, sbuf);
	s = sbuf;

	while (d < e)
	{
		unsigned int p0 = 0;
		int count = 0;
		int a = 0;

		EVAL_EDGES_AFFINE_NZ

		if (count)
		{
			p0 = *d;
			if (fcolor != 0xffffffff)
				p0 = MUL4_SYM(fcolor, p0);
			if (a)
			{
				unsigned int q0 = p0;

				p0 = *s;
				if (scolor != 0xffffffff)
					p0 = MUL4_SYM(scolor, p0);

				if (a < 65536)
					p0 = INTERP_65536(a, p0, q0);
			}
		}
		else if (a)
		{
			p0 = *s;
			if (scolor != 0xffffffff)
				p0 = MUL4_SYM(scolor, p0);

			if (a < 65536)
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		s++;
		yy += ayx;
		xx += axx;
	}
}

#define EVAL_EDGES_EO \
		n = 0; \
		edge = edges; \
		while (n < nedges) \
		{ \
			int ee = edge->e; \
 \
			if ((yy >= edge->yy0) & (yy < edge->yy1)) \
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
		{ \
			if (!(np % 2)) \
				in = 1; \
		} \
		else \
		{ \
			if (np % 2) \
				in = 1; \
		}

/* identity */
/* stroke and/or fill with possibly a fill renderer even-odd rule */
static void _stroke_fill_paint_eo(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = _basic_get(r);
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *fpaint;
	int stroke = 0;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	double ox, oy;
	int lx = INT_MAX / 2, rx = -lx;

	int xx = x << 16;
	int yy = y << 16;

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

	scolor = sstate->stroke.color;
	fcolor = sstate->fill.color;
	fpaint = sstate->fill.r;

	color = state->color;
	if (color != 0xffffffff)
	{
		scolor = argb8888_mul4_sym(color, scolor);
		fcolor = argb8888_mul4_sym(color, fcolor);
	}

	sw = sstate->stroke.weight;
	if (sstate->draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL)
	{
		sww = 65536;
		scolor = fcolor;
		stroke = 0;
		if (fpaint)
		{
			enesim_renderer_sw_draw(fpaint, x + lx, y, rx - lx, dst + lx);
		}
	}
	if (sstate->draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL)
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
		{
			enesim_renderer_sw_draw(fpaint, x + lx, y, rx - lx, dst + lx);
		}
	}
	if (sstate->draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)
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
					p0 = MUL4_SYM(fcolor, p0);
			}

			if (stroke && a)
			{
				unsigned int q0 = p0;

				p0 = scolor;
				if (a < 65536)
					p0 = INTERP_65536(a, p0, q0);
			}
		}
		if (a && !in)
		{
			p0 = scolor;
			if (fpaint && !stroke)
			{
				p0 = *d;
				if (fcolor != 0xffffffff)
					p0 = MUL4_SYM(fcolor, p0);
			}
			if (a < 65536)
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		xx += 65536;
	}
}

/* stroke with a renderer and possibly fill with color even-odd rule */
static void _stroke_paint_fill_eo(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = _basic_get(r);
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *spaint;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	double ox, oy;
	int lx = INT_MAX / 2, rx = -lx;

	int xx = x << 16;
	int yy = y << 16;

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

	scolor = sstate->stroke.color;
	spaint = sstate->stroke.r;
	fcolor = sstate->fill.color;

	color = state->color;
	if (color != 0xffffffff)
	{
		scolor = argb8888_mul4_sym(color, scolor);
		fcolor = argb8888_mul4_sym(color, fcolor);
	}

	sw = sstate->stroke.weight;
	sww = sqrt(sw) * 65536;

	if (sstate->draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)
		fcolor = 0;

	enesim_renderer_sw_draw(spaint, x + lx, y, rx - lx, dst + lx);

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
			if (a)
			{
				unsigned int q0 = p0;

				p0 = *d;
				if (scolor != 0xffffffff)
					p0 = MUL4_SYM(scolor, p0);

				if (a < 65536)
					p0 = INTERP_65536(a, p0, q0);
			}
		}
		if (a && !in)
		{
			p0 = *d;
			if (scolor != 0xffffffff)
				p0 = MUL4_SYM(scolor, p0);

			if (a < 65536)
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		xx += 65536;
	}
}

/* stroke and fill with renderers even-odd rule */
static void _stroke_paint_fill_paint_eo(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = _basic_get(r);
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *fpaint, *spaint;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	double ox, oy;
	int lx = INT_MAX / 2, rx = -lx;

	int xx = x << 16;
	int yy = y << 16;

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

	scolor = sstate->stroke.color;
	spaint = sstate->stroke.r;
	fcolor = sstate->fill.color;
	fpaint = sstate->fill.r;

	color = state->color;
	if (color != 0xffffffff)
	{
		scolor = argb8888_mul4_sym(color, scolor);
		fcolor = argb8888_mul4_sym(color, fcolor);
	}

	sw = sstate->stroke.weight;
	sww = sqrt(sw) * 65536;

	enesim_renderer_sw_draw(fpaint, x + lx, y, rx - lx, dst + lx);

	sbuf = alloca((rx - lx) * sizeof(unsigned int));
	enesim_renderer_sw_draw(spaint, x + lx, y, rx - lx, sbuf);
	s = sbuf;

	while (d < e)
	{
		unsigned int p0 = 0;
		int in = 0;
		int np = 0, nn = 0;
		int a = 0;

		EVAL_EDGES_EO

		if (in)
		{
			p0 = *d;
			if (fcolor != 0xffffffff)
				p0 = MUL4_SYM(fcolor, p0);
			if (a)
			{
				unsigned int q0 = p0;


				p0 = *s;
				if (scolor != 0xffffffff)
					p0 = MUL4_SYM(scolor, p0);

				if (a < 65536)
					p0 = INTERP_65536(a, p0, q0);
			}
		}
		if (a && !in)
		{
			p0 = *s;
			if (scolor != 0xffffffff)
				p0 = MUL4_SYM(scolor, p0);

			if (a < 65536)
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		s++;
		xx += 65536;
	}
}

#define EVAL_EDGES_AFFINE_EO \
		n = 0; \
		edge = edges; \
		while (n < nedges) \
		{ \
			int ee = edge->e; \
 \
			if (((yy + 0xffff) >= edge->yy0) & \
					(yy <= (edge->yy1 + 0xffff))) \
			{ \
				if ((yy >= edge->yy0) & (yy < edge->yy1)) \
				{ \
					np += (ee >= 0); \
					nn += (ee < 0); \
				} \
				if (ee < 0) \
					ee = -ee; \
				if ((ee < sww) && \
						((xx + 0xffff) >= edge->xx0) & \
						(xx <= (0xffff + edge->xx1))) \
				{ \
					if (a < sww/4) \
						a = sww - ee; \
					else \
						a = (a + (sww - ee)) / 2; \
				} \
			} \
 \
			edge->e += edge->de; \
			edge++; \
			n++; \
		} \
 \
		if ((np + nn) % 4) \
		{ \
			if (!(np % 2)) \
				in = 1; \
		} \
		else \
		{ \
			if (np % 2) \
				in = 1; \
		}

/* affine general */
/* stroke and/or fill with possibly a fill renderer even-odd rule */
static void _stroke_fill_paint_affine_eo(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = _basic_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *fpaint;
	int stroke = 0;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	int y0, y1;
	double ox, oy;

	int axx = thiz->matrix.xx, axy = thiz->matrix.xy, axz =
			thiz->matrix.xz;
	int ayx = thiz->matrix.yx, ayy = thiz->matrix.yy, ayz =
			thiz->matrix.yz;
	int xx = (axx * x) + (axx >> 1) + (axy * y) + (axy >> 1) + axz - 32768;
	int yy = (ayx * x) + (ayx >> 1) + (ayy * y) + (ayy >> 1) + ayz - 32768;

	ox = state->ox;
	oy = state->oy;
	xx -= eina_f16p16_double_from(ox);
	yy -= eina_f16p16_double_from(oy);

	if (((ayx <= 0) && ((yy >> 16) + 1 < (thiz->tyy >> 16))) || ((ayx >= 0)
			&& ((yy >> 16) > 1 + (thiz->byy >> 16))))
	{
get_out:
		memset(d, 0, sizeof(unsigned int) * len);
		return;
	}

	SETUP_EDGES_AFFINE

	scolor = sstate->stroke.color;
	fcolor = sstate->fill.color;
	fpaint = sstate->fill.r;

	color = state->color;
	if (color != 0xffffffff)
	{
		scolor = argb8888_mul4_sym(color, scolor);
		fcolor = argb8888_mul4_sym(color, fcolor);
	}

	sw = sstate->stroke.weight;
	draw_mode = sstate->draw_mode;
	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL)
	{
		sww = 65536;
		scolor = fcolor;
		stroke = 0;
		if (fpaint)
		{
			enesim_renderer_sw_draw(fpaint, x, y, len, dst);
		}
	}
	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL)
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
		{
			enesim_renderer_sw_draw(fpaint, x, y, len, dst);
		}
	}
	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)
	{
		if (sw <= 0)
		{
			memset(d, 0, sizeof (unsigned int) * len);
			return;
		}

		sww = sqrt(sw) * 65536;
		fcolor = 0;
		fpaint = NULL;
		stroke = 1;
	}

	while (d < e)
	{
		unsigned int p0 = 0;
		int in = 0;
		int np = 0, nn = 0;
		int a = 0;

		EVAL_EDGES_AFFINE_EO
	
		if (in)
		{
			p0 = fcolor;
			if (fpaint)
			{
				p0 = *d;
				if (fcolor != 0xffffffff)
					p0 = MUL4_SYM(fcolor, p0);
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
					p0 = MUL4_SYM(fcolor, p0);
			}
			if (a < 65536)
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		yy += ayx;
		xx += axx;
	}
}

/* stroke with a renderer and possibly fill with color even-odd rule */
static void _stroke_paint_fill_affine_eo(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = _basic_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *spaint;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	int y0, y1;
	double ox, oy;

	int axx = thiz->matrix.xx, axy = thiz->matrix.xy, axz =
			thiz->matrix.xz;
	int ayx = thiz->matrix.yx, ayy = thiz->matrix.yy, ayz =
			thiz->matrix.yz;
	int xx = (axx * x) + (axx >> 1) + (axy * y) + (axy >> 1) + axz - 32768;
	int yy = (ayx * x) + (ayx >> 1) + (ayy * y) + (ayy >> 1) + ayz - 32768;

	ox = state->ox;
	oy = state->oy;
	xx -= eina_f16p16_double_from(ox);
	yy -= eina_f16p16_double_from(oy);

	if (((ayx <= 0) && ((yy >> 16) + 1 < (thiz->tyy >> 16))) || ((ayx >= 0)
			&& ((yy >> 16) > 1 + (thiz->byy >> 16))))
	{
get_out:
		memset(d, 0, sizeof(unsigned int) * len);
		return;
	}

	SETUP_EDGES_AFFINE

	scolor = sstate->stroke.color;
	spaint = sstate->stroke.r;
	fcolor = sstate->fill.color;

	color = state->color;
	if (color != 0xffffffff)
	{
		scolor = argb8888_mul4_sym(color, scolor);
		fcolor = argb8888_mul4_sym(color, fcolor);
	}

	sw = sstate->stroke.weight;
	sww = sqrt(sw) * 65536;

	draw_mode = sstate->draw_mode;
	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)
		fcolor = 0;

	enesim_renderer_sw_draw(spaint, x, y, len, dst);

	while (d < e)
	{
		unsigned int p0 = 0;
		int in = 0;
		int np = 0, nn = 0;
		int a = 0;

		EVAL_EDGES_AFFINE_EO
	
		if (in)
		{
			p0 = fcolor;
			if (a)
			{
				unsigned int q0 = p0;

				p0 = *d;
				if (scolor != 0xffffffff)
					p0 = MUL4_SYM(scolor, p0);

				if (a < 65536)
					p0 = INTERP_65536(a, p0, q0);
			}
		}
		else if (a)
		{
			p0 = *d;
			if (scolor != 0xffffffff)
				p0 = MUL4_SYM(scolor, p0);

			if (a < 65536)
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		yy += ayx;
		xx += axx;
	}
}

/* stroke and fill with renderers even-odd rule */
static void _stroke_paint_fill_paint_affine_eo(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Rasterizer_Basic *thiz = _basic_get(r);
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *fpaint, *spaint;
	double sw;
	int sww;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Enesim_F16p16_Edge *edges, *edge;
	Enesim_F16p16_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	int y0, y1;
	double ox, oy;

	int axx = thiz->matrix.xx, axy = thiz->matrix.xy, axz =
			thiz->matrix.xz;
	int ayx = thiz->matrix.yx, ayy = thiz->matrix.yy, ayz =
			thiz->matrix.yz;
	int xx = (axx * x) + (axx >> 1) + (axy * y) + (axy >> 1) + axz - 32768;
	int yy = (ayx * x) + (ayx >> 1) + (ayy * y) + (ayy >> 1) + ayz - 32768;
	unsigned int *sbuf, *s;

	ox = state->ox;
	oy = state->oy;
	xx -= eina_f16p16_double_from(ox);
	yy -= eina_f16p16_double_from(oy);

	if (((ayx <= 0) && ((yy >> 16) + 1 < (thiz->tyy >> 16))) || ((ayx >= 0)
			&& ((yy >> 16) > 1 + (thiz->byy >> 16))))
	{
get_out:
		memset(d, 0, sizeof(unsigned int) * len);
		return;
	}

	SETUP_EDGES_AFFINE

	scolor = sstate->stroke.color;
	spaint = sstate->stroke.r;
	fcolor = sstate->fill.color;
	fpaint = sstate->fill.r;

	color = state->color;
	if (color != 0xffffffff)
	{
		scolor = argb8888_mul4_sym(color, scolor);
		fcolor = argb8888_mul4_sym(color, fcolor);
	}

	sw = sstate->stroke.weight;
	sww = sqrt(sw) * 65536;

	enesim_renderer_sw_draw(fpaint, x, y, len, dst);
	sbuf = alloca(len * sizeof(unsigned int));
	enesim_renderer_sw_draw(spaint, x, y, len, sbuf);
	s = sbuf;

	while (d < e)
	{
		unsigned int p0 = 0;
		int in = 0;
		int np = 0, nn = 0;
		int a = 0;

		EVAL_EDGES_AFFINE_EO

		if (in)
		{
			p0 = *d;
			if (fcolor != 0xffffffff)
				p0 = MUL4_SYM(fcolor, p0);
			if (a)
			{
				unsigned int q0 = p0;

				p0 = *s;
				if (scolor != 0xffffffff)
					p0 = MUL4_SYM(scolor, p0);

				if (a < 65536)
					p0 = INTERP_65536(a, p0, q0);
			}
		}
		else if (a)
		{
			p0 = *s;
			if (scolor != 0xffffffff)
				p0 = MUL4_SYM(scolor, p0);

			if (a < 65536)
				p0 = MUL_A_65536(a, p0);
		}

		*d++ = p0;
		s++;
		yy += ayx;
		xx += axx;
	}
}

/*----------------------------------------------------------------------------*
 *                    The Enesim's rasterizer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _basic_name(Enesim_Renderer *r)
{
	return "basic";
}

static void _basic_free(Enesim_Renderer *r)
{
	Enesim_Rasterizer_Basic *thiz;

	thiz = _basic_get(r);
	if (thiz->vectors)
		free(thiz->vectors);
	free(thiz);
}

static void _basic_figure_set(Enesim_Renderer *r, const Enesim_Figure *figure)
{
	Enesim_Rasterizer_Basic *thiz;

	thiz = _basic_get(r);
	thiz->figure = figure;
	thiz->changed = EINA_TRUE;
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
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Shape_Sw_Draw *draw,
		Enesim_Error **error)
{
	Enesim_Rasterizer_Basic *thiz;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	const Enesim_Renderer_Shape_State *css = sstates[ENESIM_STATE_CURRENT];
	const Enesim_Renderer_Shape_State *pss = sstates[ENESIM_STATE_PAST];
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Shape_Fill_Rule rule;
	Enesim_Renderer *spaint;
	double sw;

	thiz = _basic_get(r);
	if (!thiz->figure)
	{
		ENESIM_RENDERER_ERROR(r, error, "No figure to rasterize");
		return EINA_FALSE;
	}

	draw_mode = css->draw_mode;
	if ((pss->draw_mode != draw_mode) &&
        		((thiz->draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE) ||
			(draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)))
		thiz->changed = 1;

	if (thiz->changed)
	{
		Enesim_Polygon *p;
		Enesim_F16p16_Vector *vec;
		Eina_List *l1;
		int nvectors = 0;

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
					((npts < 3) && (draw_mode != ENESIM_SHAPE_DRAW_MODE_STROKE)))
			{
				ENESIM_RENDERER_ERROR(r, error, "Not enough points %d", npts);
				return EINA_FALSE;
			}
			nvectors += npts;
			first_point = eina_list_data_get(p->points);
			last_point = eina_list_data_get(eina_list_last(p->points));
			{
				double x0, x1,y0, y1;
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

			if (!p->closed && (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE) && !pclosed)
				nvectors--;
		}

		thiz->vectors = calloc(nvectors, sizeof(Enesim_F16p16_Vector));
		if (!thiz->vectors)
		{
			return EINA_FALSE;
		}
		thiz->nvectors = nvectors;

		vec = thiz->vectors;
		thiz->lxx = INT_MAX / 2;
		thiz->rxx = -thiz->lxx;
		thiz->tyy = thiz->lxx;
		thiz->byy = -thiz->tyy;

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

			if (sopen && (draw_mode != ENESIM_SHAPE_DRAW_MODE_STROKE))
				sopen = 0;

			first_point = eina_list_data_get(p->points);
			last_point = eina_list_data_get(eina_list_last(p->points));

			{
				double x0, x1,y0, y1;
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
				x0 = pt->x;
				y0 = pt->y;
				x1 = npt->x;
				y1 = npt->y;
				x0 = ((int) (x0 * 256)) / 256.0;
				x1 = ((int) (x1 * 256)) / 256.0;
				y0 = ((int) (y0 * 256)) / 256.0;
				y1 = ((int) (y1 * 256)) / 256.0;
				x01 = x1 - x0;
				y01 = y1 - y0;
				len = hypot(x01, y01);
				if (len < (1 / 512.0))
				{
					/* FIXME what to do here?
					 * skip this point, pick the next? */
					ENESIM_RENDERER_ERROR(r, error, "Length %g < %g for points %gx%g %gx%g", len, 1/512.0, x0, y0, x1, y1);
					return EINA_FALSE;
				}
				len *= 1 + (1 / 32.0);
				vec->a = -(y01 * 65536) / len;
				vec->b = (x01 * 65536) / len;
				vec->c = (65536 * ((y1 * x0) - (x1 * y0)))
						/ len;
				vec->xx0 = x0 * 65536;
				vec->yy0 = y0 * 65536;
				vec->xx1 = x1 * 65536;
				vec->yy1 = y1 * 65536;

				if (vec->yy0 < thiz->tyy)
					thiz->tyy = vec->yy0;
				if (vec->yy0 > thiz->byy)
					thiz->byy = vec->yy0;

				if (vec->xx0 < thiz->lxx)
					thiz->lxx = vec->xx0;
				if (vec->xx0 > thiz->rxx)
					thiz->rxx = vec->xx0;

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

	enesim_matrix_f16p16_matrix_to(&cs->transformation,
			&thiz->matrix);

	sw = css->stroke.weight;
	spaint = css->stroke.r;
	rule = css->fill.rule;

	if (rule == ENESIM_SHAPE_FILL_RULE_NON_ZERO)
	{
		*draw = _stroke_fill_paint_affine_nz;
		if (cs->transformation_type == ENESIM_MATRIX_IDENTITY)
			*draw = _stroke_fill_paint_nz;
		if ((sw > 0.0) && spaint && (draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE))
		{
			Enesim_Renderer *fpaint;

			*draw = _stroke_paint_fill_affine_nz;
			if (cs->transformation_type == ENESIM_MATRIX_IDENTITY)
				*draw = _stroke_paint_fill_nz;

			enesim_renderer_shape_fill_renderer_get(r, &fpaint);
			if (fpaint && (draw_mode & ENESIM_SHAPE_DRAW_MODE_FILL))
			{
				*draw = _stroke_paint_fill_paint_affine_nz;
				if (cs->transformation_type == ENESIM_MATRIX_IDENTITY)
					*draw = _stroke_paint_fill_paint_nz;
			}
		}
	}
	else
	{
		*draw = _stroke_fill_paint_affine_eo;
		if (cs->transformation_type == ENESIM_MATRIX_IDENTITY)
			*draw = _stroke_fill_paint_eo;
		if ((sw > 0.0) && spaint && (draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE))
		{
			Enesim_Renderer *fpaint;

			*draw = _stroke_paint_fill_affine_eo;
			if (cs->transformation_type == ENESIM_MATRIX_IDENTITY)
				*draw = _stroke_paint_fill_eo;

			enesim_renderer_shape_fill_renderer_get(r, &fpaint);
			if (fpaint && (draw_mode & ENESIM_SHAPE_DRAW_MODE_FILL))
			{
				*draw = _stroke_paint_fill_paint_affine_eo;
				if (cs->transformation_type == ENESIM_MATRIX_IDENTITY)
					*draw = _stroke_paint_fill_paint_eo;
			}
		}
	}

	return EINA_TRUE;
}

static void _basic_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
}

static Enesim_Rasterizer_Descriptor _descriptor = {
	/* .name = 		*/ _basic_name,
	/* .free = 		*/ _basic_free,
	/* .figure_set =	*/ _basic_figure_set,
	/* .sw_setup = 		*/ _basic_sw_setup,
	/* .sw_cleanup = 	*/ _basic_sw_cleanup,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_rasterizer_basic_new(void)
{
	Enesim_Rasterizer_Basic *thiz;
	Enesim_Renderer *r;

	thiz = calloc(1, sizeof(Enesim_Rasterizer_Basic));
	r = enesim_rasterizer_new(&_descriptor, thiz);
	EINA_MAGIC_SET(thiz, ENESIM_RASTERIZER_BASIC_MAGIC);

	return r;
}

void enesim_rasterizer_basic_vectors_get(Enesim_Renderer *r, int *nvectors,
		Enesim_F16p16_Vector **vectors)
{
	Enesim_Rasterizer_Basic *thiz;

	thiz = _basic_get(r);
	*nvectors = thiz->nvectors;
	*vectors = thiz->vectors;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
