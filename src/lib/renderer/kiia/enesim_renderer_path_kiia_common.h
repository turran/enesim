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
#include "enesim_renderer_path_kiia_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static inline void _kiia_even_odd_sample(
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE *mask, int x, int m)
{
	mask[x] ^= m;
}

static inline ENESIM_RENDERER_PATH_KIIA_MASK_TYPE _kiia_even_odd_get_mask(
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE *mask, int i,
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE cm,
		int *winding EINA_UNUSED, int *cwinding EINA_UNUSED)
{
	cm ^= mask[i];
	/* reset the mask */
	mask[i] = 0;
	return cm;
}

static inline void _kiia_non_zero_sample(
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE *mask, int x, int m,
		int *windings, int winding)
{
	mask[x] ^= m;
	windings[x] += winding;
}

static inline ENESIM_RENDERER_PATH_KIIA_MASK_TYPE _kiia_non_zero_get_mask(
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE *mask, int i,
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE cm,
		int *winding, int *cwinding)
{
	int m;
	int w;

	m = mask[i];
	/* reset the mask */
	mask[i] = 0;
	w = winding[i];
	/* reset the winding */
	winding[i] = 0;
	*cwinding += w;
	if (abs(*cwinding) < ENESIM_RENDERER_PATH_KIIA_SAMPLES)
	{
		if (*cwinding == 0)
			cm = 0;
		else
			cm = cm ^ m;
	}
	else
	{
		cm = ENESIM_RENDERER_PATH_KIIA_MASK_MAX;
	}
	return cm;
}

static inline void _kiia_figure_evalute(Enesim_Renderer *r,
		Enesim_Renderer_Path_Kiia_Figure *f,
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE *mask, int *winding,
		int *lx, int *rx, int y)
{
	Enesim_Renderer_Path_Kiia *thiz;
	Eina_F16p16 yy0, yy1;
	Eina_F16p16 sinc;
	int mlx = INT_MAX;
	int mrx = -INT_MAX;
	int i;

	thiz = ENESIM_RENDERER_PATH_KIIA(r);
	yy0 = eina_f16p16_int_from(y);
	yy1 = eina_f16p16_int_from(y + 1);
	sinc = thiz->inc;
	/* intersect with each edge */
	for (i = 0; i < f->nedges; i++)
	{
		Enesim_Renderer_Path_Kiia_Edge *edge = &f->edges[i];
		Eina_F16p16 *pattern;
		Eina_F16p16 yyy0, yyy1;
		Eina_F16p16 cx;
		int sample;
		int m;

		/* up the span */
		if (yy0 >= edge->yy1)
			continue;
		/* down the span, just skip processing, the edges are ordered in y */
		if (yy1 < edge->yy0)
			break;

		/* make sure not overflow */
		yyy1 = yy1;
		if (yyy1 > edge->yy1)
			yyy1 = edge->yy1;

		yyy0 = yy0;
		if (yy0 <= edge->yy0)
		{
			yyy0 = edge->yy0;
			cx = edge->mx;
			/* 16.16 << nsamples */
			sample = (eina_f16p16_fracc_get(yyy0) >> (16 - ENESIM_RENDERER_PATH_KIIA_SHIFT));
			m = 1 << sample;
		}
		else
		{
			Eina_F16p16 inc;

			inc = eina_f16p16_mul(yyy0 - edge->yy0,
					eina_f16p16_int_from(
					ENESIM_RENDERER_PATH_KIIA_SAMPLES));
			cx = edge->mx + eina_f16p16_mul(inc, edge->slope);
			sample = 0;
			m = 1;
		}

		/* get the pattern to use */
		pattern = thiz->pattern;
		/* finally sample */
		for (; yyy0 < yyy1; yyy0 += ENESIM_RENDERER_PATH_KIIA_INC)
		{
			int mx;

			mx = eina_f16p16_int_to(cx - thiz->llx + *pattern);
			/* keep track of the start, end of intersections */
			if (mx < mlx)
				mlx = mx;
			if (mx > mrx)
				mrx = mx;
			/* FIXME For now we evaluate always on non-zero (i.e with winding) */
			_kiia_non_zero_sample(mask, mx, m, winding, edge->sgn);

			cx += edge->slope;
			pattern++;
			m <<= 1;
		}
	}
	*lx = mlx;
	*rx = mrx;
}

static inline void _kiia_figure_renderer_setup(Enesim_Renderer_Path_Kiia_Figure *f,
		int x, int y, int len, uint32_t *dst)
{
	enesim_renderer_sw_draw(f->ren, x, y, len, dst);
}

static inline Eina_Bool _kiia_figure_renderer_fill(
		Enesim_Renderer_Path_Kiia_Figure *f,
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE cm,
		uint32_t *src, uint32_t *p0)
{
	if (cm == ENESIM_RENDERER_PATH_KIIA_MASK_MAX)
	{
		if (f->color != 0xffffffff)
		{
			*p0 = enesim_color_mul4_sym(*src, f->color);
		}
		else
		{
			/* don't draw */
			return EINA_FALSE;
		}
	}
	else if (cm == 0)
	{
		*p0 = 0;
	}
	else
	{
		uint16_t coverage;
		uint32_t q0 = *src;

		coverage = ENESIM_RENDERER_PATH_KIIA_GET_ALPHA(cm);

		if (f->color != 0xffffffff)
			q0 = enesim_color_mul4_sym(f->color, q0);
		*p0 = enesim_color_mul_256(coverage, q0);
	}
	return EINA_TRUE;
}

static inline Eina_Bool _kiia_figure_color_fill(
		Enesim_Renderer_Path_Kiia_Figure *f,
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE cm,
		uint32_t *src EINA_UNUSED, uint32_t *p0)
{
	if (cm == ENESIM_RENDERER_PATH_KIIA_MASK_MAX)
	{
		*p0 = f->color;
	}
	else if (cm == 0)
	{
		*p0 = 0;
	}
	else
	{
		uint16_t coverage;

		coverage = ENESIM_RENDERER_PATH_KIIA_GET_ALPHA(cm);
		*p0 = enesim_color_mul_256(coverage, f->color);
	}
	return EINA_TRUE;
}


static inline void _kiia_figure_color_setup(
		Enesim_Renderer_Path_Kiia_Figure *f EINA_UNUSED,
		int x EINA_UNUSED, int y EINA_UNUSED, int len EINA_UNUSED,
		uint32_t *dst EINA_UNUSED)
{
}

/* nsamples = 8, 16, 32
 * fill_mode = non_zero, even_odd
 * fill = color, renderer
 */
#define ENESIM_RENDERER_PATH_KIIA_SPAN_SIMPLE(nsamples, fill_mode, fill)	\
void										\
enesim_renderer_path_kiia_##nsamples##_##fill_mode##_##fill##_simple(		\
		Enesim_Renderer *r, int x, int y, int len, void *ddata)		\
{										\
	Enesim_Renderer_Path_Kiia *thiz;					\
	Enesim_Renderer_Path_Kiia_Worker *w;					\
	ENESIM_RENDERER_PATH_KIIA_MASK_TYPE cm = 0;				\
	Enesim_Renderer_Path_Kiia_Figure *f;					\
	ENESIM_RENDERER_PATH_KIIA_MASK_TYPE *mask;				\
	int *winding;								\
	int cwinding = 0;							\
	uint32_t *dst = ddata;							\
	uint32_t *end, *rend = dst + len;					\
	int lx, mlx;								\
	int rx, mrx;								\
	int i;									\
										\
	thiz = ENESIM_RENDERER_PATH_KIIA(r);					\
	/* pick the worker at y coordinate */					\
	w = &thiz->workers[y % thiz->nworkers];					\
	/* set our own local vars */						\
	f = thiz->current;							\
	mask = w->mask;								\
	winding = w->winding;							\
										\
	/* evaluate the edges at y */						\
	_kiia_figure_evalute(r, f, mask, winding, &mlx, &mrx, y);		\
	/* does not intersect with anything */					\
	if (mlx == INT_MAX)							\
	{									\
		memset(dst, 0, len * sizeof(uint32_t));				\
		return;								\
	}									\
										\
	/* clip on the left side [x.. left] */					\
	lx = x - thiz->lx;							\
	if (lx < mlx)								\
	{									\
		int adv;							\
										\
		adv = mlx - lx;							\
		memset(dst, 0, adv * sizeof(uint32_t));				\
		len -= adv;							\
		dst += adv;							\
		lx = mlx;							\
		i = mlx;							\
	}									\
	/* advance the mask until we reach the requested x */			\
	/* also clear the mask in the process */				\
	else									\
	{									\
		for (i = mlx; i < lx; i++)					\
		{								\
			cm = _kiia_##fill_mode##_get_mask(mask, i, cm, winding, \
					&cwinding);				\
		}								\
	}									\
	/* clip on the right side [right ... x + len] */			\
	rx = lx + len;								\
	/* given that we always jump to the next pixel because of the offset	
	 * pattern, start counting on the next pixel */				\
	mrx += 1;								\
	if (rx > mrx)								\
	{									\
		int adv;							\
										\
		adv = rx - mrx;							\
		len -= adv;							\
		rx = mrx;							\
	}									\
										\
	/* time to draw */							\
 	end = dst + len;							\
	/* do the setup, i.e draw the fill renderer */				\
	_kiia_figure_##fill##_setup(f, x, y, len, dst);				\
	/* iterate over the mask and fill */					\
	while (dst < end)							\
	{									\
		uint32_t p0;							\
										\
		cm = _kiia_even_odd_get_mask(mask, i, cm, winding, &cwinding);	\
		if (!_kiia_figure_##fill##_fill(f, cm, dst, &p0))		\
			goto next;						\
		*dst = p0;							\
next:										\
		dst++;								\
		i++;								\
	}									\
	/* finally memset on dst at the end to keep the correct order on the
	 * dst access
	 */									\
	if (dst < rend)								\
	{									\
		memset(dst, 0, (rend - dst) * sizeof(uint32_t));		\
	}									\
	/* set to zero the rest of the bits of the mask */			\
	else									\
	{									\
		for (i = rx; i < mrx; i++)					\
		{								\
			/* FIXME For now we always clear both arrays, even if
			 * one might not be used */				\
			mask[i] = 0;						\
			winding[i] = 0;						\
		}								\
	}									\
	/* update the latest y coordinate of the worker */			\
	w->y = y;								\
}
