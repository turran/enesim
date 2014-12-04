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
/*----------------------------------------------------------------------------*
 *                                 Sampling                                   *
 *----------------------------------------------------------------------------*/
static inline void _kiia_even_odd_sample(
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE *mask, int x, int m)
{
	mask[x] ^= m;
}

static inline ENESIM_RENDERER_PATH_KIIA_MASK_TYPE _kiia_even_odd_get_mask(
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE *mask, int i,
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE cm, int *cacc EINA_UNUSED)
{
	cm ^= mask[i];
	/* reset the mask */
	mask[i] = 0;
	return cm;
}

static inline void _kiia_non_zero_sample(int *mask, int x, int m)
{
	mask[x] += m;
}

static inline ENESIM_RENDERER_PATH_KIIA_MASK_TYPE _kiia_non_zero_get_mask(
		int *mask, int i, int cm, int *cacc)
{
	int abs_cacc;
	int m;

	m = mask[i];
	/* reset the mask */
	mask[i] = 0;
	/* accumulate the winding */
	*cacc += m;
	abs_cacc = abs(*cacc);
	if (abs_cacc < ENESIM_RENDERER_PATH_KIIA_SAMPLES)
	{
		if (abs_cacc == 0)
			cm = 0;
		else
			cm = abs_cacc;
	}
	else
	{
		cm = ENESIM_RENDERER_PATH_KIIA_MASK_MAX;
	}
	return cm;
}

/*----------------------------------------------------------------------------*
 *                                 Evaluation                                 *
 *----------------------------------------------------------------------------*/
static inline void _kiia_even_odd_figure_evalute(Enesim_Renderer *r,
		Enesim_Renderer_Path_Kiia_Figure *f,
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE *mask, int *lx, int *rx,
		int y)
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
			_kiia_even_odd_sample(mask, mx, m);

			cx += edge->slope;
			pattern++;
			m <<= 1;
		}
	}
	*lx = mlx;
	*rx = mrx;
}

static inline void _kiia_non_zero_figure_evalute(Enesim_Renderer *r,
		Enesim_Renderer_Path_Kiia_Figure *f,
		int *mask, int *lx, int *rx, int y)
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
		}
		else
		{
			Eina_F16p16 inc;

			inc = eina_f16p16_mul(yyy0 - edge->yy0,
					eina_f16p16_int_from(
					ENESIM_RENDERER_PATH_KIIA_SAMPLES));
			cx = edge->mx + eina_f16p16_mul(inc, edge->slope);
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
			_kiia_non_zero_sample(mask, mx, edge->sgn);

			cx += edge->slope;
			pattern++;
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

static inline Eina_Bool _kiia_figure_color_color_fill(
		Enesim_Renderer_Path_Kiia_Figure *f,
		Enesim_Renderer_Path_Kiia_Figure *s,
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE cm,
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE scm,
		uint32_t *src, uint32_t *ssrc EINA_UNUSED,
		uint32_t *p0)
{
	if (scm == ENESIM_RENDERER_PATH_KIIA_MASK_MAX)
	{
		*p0 = s->color;
	}
	else if (scm == 0)
	{
		if (!_kiia_figure_color_fill(f, cm, src, p0))
			return EINA_FALSE;
	}
	else
	{
		uint16_t coverage;

		coverage = ENESIM_RENDERER_PATH_KIIA_GET_ALPHA(scm);
		if (cm == ENESIM_RENDERER_PATH_KIIA_MASK_MAX)
		{
			*p0 = enesim_color_interp_256(coverage, s->color, f->color);
		}
		else if (cm == 0)
		{
			*p0 = enesim_color_mul_256(coverage, s->color);
		}
		else
		{
			uint32_t q0;
			uint16_t fcoverage;

			fcoverage = ENESIM_RENDERER_PATH_KIIA_GET_ALPHA(cm);
			q0 = enesim_color_mul_256(fcoverage, f->color);
			*p0 = enesim_color_interp_256(coverage, s->color, q0);
		}
	}
	return EINA_TRUE;
}

static inline Eina_Bool _kiia_figure_renderer_color_fill(
		Enesim_Renderer_Path_Kiia_Figure *f,
		Enesim_Renderer_Path_Kiia_Figure *s,
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE cm,
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE scm,
		uint32_t *src, uint32_t *ssrc EINA_UNUSED,
		uint32_t *p0)
{
	if (scm == ENESIM_RENDERER_PATH_KIIA_MASK_MAX)
	{
		*p0 = s->color;
	}
	else if (scm == 0)
	{
		if (!_kiia_figure_renderer_fill(f, cm, src, p0))
			return EINA_FALSE;
	}
	else
	{
		uint16_t coverage;

		coverage = ENESIM_RENDERER_PATH_KIIA_GET_ALPHA(scm);
		if (cm == ENESIM_RENDERER_PATH_KIIA_MASK_MAX)
		{
			uint32_t q0;

			if (f->color != 0xffffffff)
			{
				q0 = enesim_color_mul4_sym(*src, f->color);
			}
			else
			{
				q0 = *src;
			}
			*p0 = enesim_color_interp_256(coverage, s->color, q0);
		}
		else if (cm == 0)
		{
			*p0 = enesim_color_mul_256(coverage, s->color);
		}
		else
		{
			uint32_t q0;
			uint16_t fcoverage;

			fcoverage = ENESIM_RENDERER_PATH_KIIA_GET_ALPHA(cm);
			q0 = *src;
			if (f->color != 0xffffffff)
			{
				q0 = enesim_color_mul4_sym(*src, f->color);
			}
			q0 = enesim_color_mul_256(fcoverage, q0);
			*p0 = enesim_color_interp_256(coverage, s->color, q0);
		}
	}
	return EINA_TRUE;
}

static inline Eina_Bool _kiia_figure_color_renderer_fill(
		Enesim_Renderer_Path_Kiia_Figure *f,
		Enesim_Renderer_Path_Kiia_Figure *s,
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE cm,
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE scm,
		uint32_t *src, uint32_t *ssrc,
		uint32_t *p0)
{
	if (scm == ENESIM_RENDERER_PATH_KIIA_MASK_MAX)
	{
		uint32_t q0;

		q0 = *ssrc;
		if (s->color != 0xffffffff)
			q0 = enesim_color_mul4_sym(q0, s->color);
		*p0 = q0;
	}
	else if (scm == 0)
	{
		if (!_kiia_figure_color_fill(f, cm, src, p0))
			return EINA_FALSE;
	}
	else
	{
		uint16_t coverage;

		coverage = ENESIM_RENDERER_PATH_KIIA_GET_ALPHA(scm);
		if (cm == ENESIM_RENDERER_PATH_KIIA_MASK_MAX)
		{
			uint32_t q0;

			if (s->color != 0xffffffff)
			{
				q0 = enesim_color_mul4_sym(*ssrc, s->color);
			}
			else
			{
				q0 = *ssrc;
			}
			*p0 = enesim_color_interp_256(coverage, q0, f->color);
		}
		else if (cm == 0)
		{
			uint32_t q0;

			if (s->color != 0xffffffff)
			{
				q0 = enesim_color_mul4_sym(*ssrc, s->color);
			}
			else
			{
				q0 = *ssrc;
			}
			*p0 = enesim_color_mul_256(coverage, q0);
		}
		else
		{
			uint32_t q0;
			uint32_t q1;
			uint16_t fcoverage;

			fcoverage = ENESIM_RENDERER_PATH_KIIA_GET_ALPHA(cm);
			if (s->color != 0xffffffff)
			{
				q0 = enesim_color_mul4_sym(*ssrc, s->color);
			}
			else
			{
				q0 = *ssrc;
			}

			q1 = enesim_color_mul_256(fcoverage, f->color);
			*p0 = enesim_color_interp_256(coverage, q0, q1);
		}
	}
	return EINA_TRUE;
}

static inline Eina_Bool _kiia_figure_renderer_renderer_fill(
		Enesim_Renderer_Path_Kiia_Figure *f,
		Enesim_Renderer_Path_Kiia_Figure *s,
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE cm,
		ENESIM_RENDERER_PATH_KIIA_MASK_TYPE scm,
		uint32_t *src, uint32_t *ssrc,
		uint32_t *p0)
{
	if (scm == ENESIM_RENDERER_PATH_KIIA_MASK_MAX)
	{
		uint32_t q0;

		q0 = *ssrc;
		if (s->color != 0xffffffff)
			q0 = enesim_color_mul4_sym(q0, s->color);
		*p0 = q0;
	}
	else if (scm == 0)
	{
		if (!_kiia_figure_renderer_fill(f, cm, src, p0))
			return EINA_FALSE;
	}
	else
	{
		uint16_t coverage;

		coverage = ENESIM_RENDERER_PATH_KIIA_GET_ALPHA(scm);
		if (cm == ENESIM_RENDERER_PATH_KIIA_MASK_MAX)
		{
			uint32_t q0, q1;

			if (s->color != 0xffffffff)
			{
				q0 = enesim_color_mul4_sym(*ssrc, s->color);
			}
			else
			{
				q0 = *ssrc;
			}
			if (f->color != 0xffffffff)
			{
				q1 = enesim_color_mul4_sym(*src, f->color);
			}
			else
			{
				q1 = *src;
			}
			*p0 = enesim_color_interp_256(coverage, q0, q1);
		}
		else if (cm == 0)
		{
			uint32_t q0;

			if (s->color != 0xffffffff)
			{
				q0 = enesim_color_mul4_sym(*ssrc, s->color);
			}
			else
			{
				q0 = *ssrc;
			}
			*p0 = enesim_color_mul_256(coverage, q0);
		}
		else
		{
			uint32_t q0;
			uint32_t q1;
			uint16_t fcoverage;

			fcoverage = ENESIM_RENDERER_PATH_KIIA_GET_ALPHA(cm);
			if (s->color != 0xffffffff)
			{
				q0 = enesim_color_mul4_sym(*ssrc, s->color);
			}
			else
			{
				q0 = *ssrc;
			}

			q1 = *src;
			if (f->color != 0xffffffff)
			{
				q1 = enesim_color_mul4_sym(*src, f->color);
			}

			q1 = enesim_color_mul_256(fcoverage, q1);
			*p0 = enesim_color_interp_256(coverage, q0, q1);
		}
	}
	return EINA_TRUE;
}

/* nsamples = 8, 16, 32
 * fill_mode = non_zero, even_odd
 * fill = color, renderer
 */
#define ENESIM_RENDERER_PATH_KIIA_SPAN_SIMPLE(nsamples, fill_mode, fill)	\
void enesim_renderer_path_kiia_##nsamples##_##fill_mode##_##fill##_simple(	\
		Enesim_Renderer *r, int x, int y, int len, void *ddata)		\
{										\
	Enesim_Renderer_Path_Kiia *thiz;					\
	Enesim_Renderer_Path_Kiia_Worker *w;					\
	ENESIM_RENDERER_PATH_KIIA_MASK_TYPE cm = 0;				\
	Enesim_Renderer_Path_Kiia_Figure *f;					\
	ENESIM_RENDERER_PATH_KIIA_MASK_TYPE *mask;				\
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
										\
	/* evaluate the edges at y */						\
	_kiia_##fill_mode##_figure_evalute(r, f, mask, &mlx, &mrx, y);		\
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
		if (adv > len)							\
			adv = len;						\
		memset(dst, 0, adv * sizeof(uint32_t));				\
		len -= adv;							\
		dst += adv;							\
		lx = mlx;							\
		i = mlx;							\
		x += adv; 							\
	}									\
	/* advance the mask until we reach the requested x */			\
	/* also clear the mask in the process */				\
	else									\
	{									\
		for (i = mlx; i < lx; i++)					\
		{								\
			cm = _kiia_##fill_mode##_get_mask(mask, i, cm, 		\
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
		cm = _kiia_##fill_mode##_get_mask(mask, i, cm, &cwinding);	\
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
			mask[i] = 0;						\
		}								\
	}									\
	/* update the latest y coordinate of the worker */			\
	w->y = y;								\
}

/* nsamples = 8, 16, 32
 * fill_mode = non_zero, even_odd
 * ft = color, renderer
 * st = color, renderer
 */
#define ENESIM_RENDERER_PATH_KIIA_SPAN_FULL(nsamples, fill_mode, ft, st)	\
void 										\
enesim_renderer_path_kiia_##nsamples##_##fill_mode##_##ft##_##st##_full(	\
		Enesim_Renderer *r, int x, int y, int len, void *ddata)		\
{										\
	Enesim_Renderer_Path_Kiia *thiz;					\
	Enesim_Renderer_Path_Kiia_Worker *w;					\
	ENESIM_RENDERER_PATH_KIIA_MASK_TYPE cm = 0;				\
	int ocm = 0;								\
	int cwinding = 0, ocwinding = 0;					\
	uint32_t *dst = ddata;							\
	uint32_t *end, *rend = dst + len;					\
	uint32_t *odst = NULL;							\
	int lx, mlx, omlx;							\
	int rx, mrx, omrx;							\
	int i;									\
										\
	thiz = ENESIM_RENDERER_PATH_KIIA(r);					\
	/* pick the worker at y coordinate */					\
	w = &thiz->workers[y % thiz->nworkers];					\
										\
	/* evaluate the edges at y */						\
	_kiia_##fill_mode##_figure_evalute(r, &thiz->fill, w->mask, &mlx, &mrx, \
			y);							\
	_kiia_non_zero_figure_evalute(r, &thiz->stroke, w->omask, &omlx, &omrx, \
			y);							\
	/* pick the larger */							\
	if (omlx < mlx)								\
		mlx = omlx;							\
	if (omrx > mrx)								\
		mrx = omrx;							\
	/* does not intersect with anything */					\
	if (mlx == INT_MAX)							\
	{									\
		memset(dst, 0, len * sizeof(uint32_t));				\
		return;								\
	}									\
	/* allocate the tmp buffer */ 						\
	if (thiz->stroke.ren)							\
		odst = alloca(sizeof(uint32_t) * len);				\
										\
	/* clip on the left side [x.. left] */					\
	lx = x - thiz->lx;							\
	if (lx < mlx)								\
	{									\
		int adv;							\
										\
		adv = mlx - lx;							\
		if (adv > len)							\
			adv = len;						\
		memset(dst, 0, adv * sizeof(uint32_t));				\
		len -= adv;							\
		dst += adv;							\
		odst += adv;							\
		lx = mlx;							\
		i = mlx;							\
		x += adv; 							\
	}									\
	/* advance the mask until we reach the requested x */			\
	/* also clear the mask in the process */				\
	else									\
	{									\
		for (i = mlx; i < lx; i++)					\
		{								\
			cm = _kiia_##fill_mode##_get_mask(w->mask, i, cm, 	\
					&cwinding);				\
			ocm = _kiia_non_zero_get_mask(w->omask, i, ocm,		\
					&ocwinding);				\
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
	_kiia_figure_##ft##_setup(&thiz->fill, x, y, len, dst);			\
	/* do the setup, i.e draw the stroke renderer */			\
	_kiia_figure_##st##_setup(&thiz->stroke, x, y, len, odst);		\
	/* iterate over the mask and fill */					\
	while (dst < end)							\
	{									\
		uint32_t p0;							\
										\
		cm = _kiia_##fill_mode##_get_mask(w->mask, i, cm, &cwinding);	\
		ocm = _kiia_non_zero_get_mask(w->omask, i, ocm, &ocwinding);	\
		if (!_kiia_figure_##ft##_##st##_fill(&thiz->fill, &thiz->stroke,\
				cm, ocm, dst, odst, &p0))			\
			goto next;						\
		*dst = p0;							\
next:										\
		odst++;								\
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
			ENESIM_RENDERER_PATH_KIIA_MASK_TYPE *tmp;		\
			tmp = w->mask;						\
			tmp[i] = 0;						\
			tmp = w->omask;						\
			tmp[i] = 0;						\
		}								\
	}									\
	/* update the latest y coordinate of the worker */			\
	w->y = y;								\
}

#define ENESIM_RENDERER_PATH_KIIA_WORKER_SETUP(nsamples, fill_mode)		\
void enesim_renderer_path_kiia_##nsamples##_##fill_mode##_worker_setup(		\
		Enesim_Renderer *r, int y, int len)				\
{										\
	Enesim_Renderer_Path_Kiia *thiz;					\
	int i;									\
	thiz = ENESIM_RENDERER_PATH_KIIA(r);					\
	/* setup the workers */							\
	for (i = 0; i < thiz->nworkers; i++)					\
	{									\
		thiz->workers[i].y = y;						\
		/* +1 because of the pattern offset */				\
		thiz->workers[i].mask = calloc(len + 1, 			\
				sizeof(ENESIM_RENDERER_PATH_KIIA_MASK_TYPE));	\
		thiz->workers[i].omask = calloc(len + 1,			\
				sizeof(int));					\
	}									\
}
