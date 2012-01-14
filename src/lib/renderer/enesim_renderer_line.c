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
#define ENESIM_RENDERER_LINE_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_LINE_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_LINE_MAGIC);\
	} while(0)

/* FIXME use the ones from libargb */
#define MUL_A_256(a, c) \
 ( (((((c) >> 8) & 0x00ff00ff) * (a)) & 0xff00ff00) + \
   (((((c) & 0x00ff00ff) * (a)) >> 8) & 0x00ff00ff) )

#define MUL4_SYM(x, y) \
 ( ((((((x) >> 16) & 0xff00) * (((y) >> 16) & 0xff00)) + 0xff0000) & 0xff000000) + \
   ((((((x) >> 8) & 0xff00) * (((y) >> 16) & 0xff)) + 0xff00) & 0xff0000) + \
   ((((((x) & 0xff00) * ((y) & 0xff00)) + 0xff00) >> 16) & 0xff00) + \
   (((((x) & 0xff) * ((y) & 0xff)) + 0xff) >> 8) )


typedef struct _Enesim_Renderer_Line_State
{
	double x0;
	double y0;
	double x1;
	double y1;
} Enesim_Renderer_Line_State;

typedef struct _Enesim_Renderer_Line
{
	EINA_MAGIC
	/* properties */
	Enesim_Renderer_Line_State current;
	/* private */
	Enesim_Renderer_Line_State past;
	Eina_Bool changed : 1;
	Enesim_F16p16_Matrix matrix;

	Enesim_F16p16_Line line;
	Enesim_F16p16_Line np;
	Enesim_F16p16_Line nm;

	Eina_F16p16 rr;
	Eina_F16p16 lxx, rxx, tyy, byy;
} Enesim_Renderer_Line;

static inline Enesim_Renderer_Line * _line_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Line *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	ENESIM_RENDERER_LINE_MAGIC_CHECK(thiz);

	return thiz;
}

static void _line_setup(Enesim_F16p16_Line *l, Enesim_Point *p0, double vx, double vy, double len)
{
	Enesim_Line line;

	enesim_line_direction_from(&line, p0, vx, vy);
	l->a = eina_f16p16_double_from(line.a /= len);
	l->b = eina_f16p16_double_from(line.b /= len);
	l->c = eina_f16p16_double_from(line.c /= len);
}

static uint32_t _butt_line(Eina_F16p16 rr, Eina_F16p16 e01, Eina_F16p16 e01_np, Eina_F16p16 e01_nm,
		Enesim_Renderer *srend, Enesim_Color scolor, uint32_t *d)
{
	uint32_t p0 = 0;

	if ((abs(e01) <= rr) && (e01_np >= 0) && (e01_nm >= 0))
	{
		int a = 256;

		p0 = scolor;
		if (srend)
		{
			p0 = *d;
			if (scolor != 0xffffffff)
				p0 = MUL4_SYM(p0, scolor);
		}

		/* anti-alias the edges */
		if (((rr - e01) >> 16) == 0)
			a = 1 + (((rr - e01) & 0xffff) >> 8);
		if (((e01 + rr) >> 16) == 0)
			a = (a * (1 + ((e01 + rr) & 0xffff))) >> 16;
		if ((e01_np >> 16) == 0)
			a = (a * (1 + (e01_np & 0xffff))) >> 16;
		if ((e01_nm >> 16) == 0)
			a = ((a * (1 + (e01_nm & 0xffff))) >> 16);

		if (a < 256)
			p0 = MUL_A_256(a, p0);
	}

	return p0;
}

static uint32_t _square_line(Eina_F16p16 rr, Eina_F16p16 e01, Eina_F16p16 e01_np, Eina_F16p16 e01_nm,
		Enesim_Renderer *srend, Enesim_Color scolor, uint32_t *d)
{
	uint32_t p0 = 0;

	if ((abs(e01) <= rr) && (e01_np >= -rr) && (e01_nm >= -rr))
	{
		int a = 256;

		p0 = scolor;
		if (srend)
		{
			p0 = *d;
			if (scolor != 0xffffffff)
				p0 = MUL4_SYM(p0, scolor);
		}

		/* anti-alias the edges */
		if (((rr - e01) >> 16) == 0)
			a = 1 + (((rr - e01) & 0xffff) >> 8);
		if (((e01 + rr) >> 16) == 0)
			a = (a * (1 + ((e01 + rr) & 0xffff))) >> 16;
		if (((e01_np + rr) >> 16) == 0)
			a = (a * (1 + ((e01_np + rr) & 0xffff))) >> 16;
		if (((e01_nm + rr) >> 16) == 0)
			a = ((a * (1 + ((e01_nm + rr) & 0xffff))) >> 16);

		if (a < 256)
			p0 = MUL_A_256(a, p0);
	}

	return p0;
}

static uint32_t _round_line(Eina_F16p16 rr, Eina_F16p16 e01, Eina_F16p16 e01_np, Eina_F16p16 e01_nm,
		Enesim_Renderer *srend, Enesim_Color scolor, uint32_t *d)
{
	Eina_F16p16 yy;
	uint32_t p0 = 0;

	yy = abs(e01);
	if ((yy <= rr) && (e01_np >= -rr) && (e01_nm >= -rr))
	{
		int a = 256;
		Eina_F16p16 r;

		p0 = scolor;
		if (srend)
		{
			p0 = *d;
			if (scolor != 0xffffffff)
				p0 = MUL4_SYM(p0, scolor);
		}

		/* anti-alias the edges */
		if (((rr - yy) >> 16) == 0)
			a = 1 + (((rr - yy) & 0xffff) >> 8);

		if (e01_np <= 0)
		{
			a = 0;
			r = hypot(e01_np, yy);

			if (r <= rr)
			{
				a = 255;
				if (((rr -r) >> 16) == 0)
				{
					a = ((rr - r) & 0xffff) >> 8;
				}
			}
		}
		if (e01_nm <= 0)
		{
			a = 0;
			r = hypot(e01_nm, yy);

			if (r <= rr)
			{
				a = 255;
				if (((rr -r) >> 16) == 0)
				{
					a = ((rr - r) & 0xffff) >> 8;
				}
			}
		}

		if (a < 256)
			p0 = MUL_A_256(a, p0);
	}

	return p0;
}

#define LINE(cap, capfunction) \
static void _span_##cap(Enesim_Renderer *r, int x, int y,			\
		unsigned int len, void *ddata)					\
{										\
	Enesim_Renderer_Line *thiz;						\
	Enesim_Renderer *srend = NULL;						\
	Enesim_Color scolor;							\
	Eina_F16p16 rr;								\
	/* FIXME use eina_f16p16 */						\
	long long int axx, axy, axz;						\
	long long int ayx, ayy, ayz;						\
	Eina_F16p16 d01;							\
	Eina_F16p16 d01_np;							\
	Eina_F16p16 d01_nm;							\
	Eina_F16p16 e01;							\
	Eina_F16p16 e01_np;							\
	Eina_F16p16 e01_nm;							\
	/* FIXME use eina_f16p16 */						\
	long long int xx;							\
	long long int yy;							\
	uint32_t *dst = ddata;							\
	uint32_t *d = dst;							\
	uint32_t *e = d + len;							\
										\
	thiz = _line_get(r);							\
										\
	rr = thiz->rr;								\
	axx = thiz->matrix.xx, axy = thiz->matrix.xy, axz = thiz->matrix.xz;	\
	ayx = thiz->matrix.yx, ayy = thiz->matrix.yy, ayz = thiz->matrix.yz;	\
										\
	d01 = eina_f16p16_mul(thiz->line.a, axx) +				\
			eina_f16p16_mul(thiz->line.b, ayx);			\
	d01_np = eina_f16p16_mul(thiz->np.a, axx) +				\
			eina_f16p16_mul(thiz->np.b, ayx);			\
	d01_nm = eina_f16p16_mul(thiz->nm.a, axx) +				\
			eina_f16p16_mul(thiz->nm.b, ayx);			\
										\
	xx = (axx * x) + (axx >> 1) + (axy * y) + (axy >> 1) + axz - 32768;	\
	yy = (ayx * x) + (ayx >> 1) + (ayy * y) + (ayy >> 1) + ayz - 32768;	\
										\
	e01 = enesim_f16p16_line_affine_setup(&thiz->line, xx, yy);		\
	e01_np = enesim_f16p16_line_affine_setup(&thiz->np, xx, yy);		\
	e01_nm = enesim_f16p16_line_affine_setup(&thiz->nm, xx, yy);		\
										\
	enesim_renderer_shape_stroke_color_get(r, &scolor);			\
	enesim_renderer_shape_stroke_renderer_get(r, &srend);			\
										\
	if (srend)								\
	{									\
			Enesim_Renderer_Sw_Data *sdata;				\
										\
			sdata = enesim_renderer_backend_data_get(srend,		\
					ENESIM_BACKEND_SOFTWARE);		\
			sdata->fill(srend, x, y, len, ddata);			\
	}									\
										\
	while (d < e)								\
	{									\
		uint32_t p0 = 0;						\
										\
		p0 = capfunction(rr, e01, e01_np, e01_nm, srend, scolor, d);	\
										\
		*d++ = p0;							\
		e01 += d01;							\
		e01_np += d01_np;						\
		e01_nm += d01_nm;						\
	}									\
}

LINE(butt, _butt_line);
LINE(round, _round_line);
LINE(square, _square_line);

static Enesim_Renderer_Sw_Fill _spans[ENESIM_SHAPE_STROKE_CAPS];
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _line_name(Enesim_Renderer *r)
{
	return "line";
}

static Eina_Bool _line_state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Line *thiz;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	const Enesim_Renderer_Shape_State *css = sstates[ENESIM_STATE_CURRENT];
	Enesim_Point p0, p1;
	double vx, vy;
	double x0, x1, y0, y1;
	double x01, y01;
	double len;
	double stroke;

	thiz = _line_get(r);

	x0 = thiz->current.x0;
	x1 = thiz->current.x1;
	y0 = thiz->current.y0;
	y1 = thiz->current.y1;

	if (cs->geometry_transformation_type != ENESIM_MATRIX_IDENTITY)
	{
		enesim_matrix_point_transform(&cs->geometry_transformation, x0, y0, &x0, &y0);
		enesim_matrix_point_transform(&cs->geometry_transformation, x1, y1, &x1, &y1);
	}

	if (y1 < y0)
	{
		thiz->byy = eina_f16p16_double_from(y0);
		thiz->tyy = eina_f16p16_double_from(y1);
	}
	else
	{
		thiz->byy = eina_f16p16_double_from(y1);
		thiz->tyy = eina_f16p16_double_from(y0);
	}

	if (x1 < x0)
	{
		thiz->rxx = eina_f16p16_double_from(x0);
		thiz->lxx = eina_f16p16_double_from(x1);
	}
	else
	{
		thiz->rxx = eina_f16p16_double_from(x1);
		thiz->lxx = eina_f16p16_double_from(x0);
	}

	x01 = x1 - x0;
	y01 = y1 - y0;

	if ((len = hypot(x01, y01)) < 1)
		return EINA_FALSE;

	vx = x01;
	vy = y01;
	p0.x = x0;
	p0.y = y0;
	p1.x = x1;
	p1.y = y1;

	/* normalize to line length so that aa works well */
	/* the original line */
	_line_setup(&thiz->line, &p0, vx, vy, len);
	/* the perpendicular line on the initial point */
	_line_setup(&thiz->np, &p0, -vy, vx, len);
	/* the perpendicular line on the last point */
	_line_setup(&thiz->nm, &p1, vy, -vx, len);

	if (!enesim_renderer_shape_setup(r, states, s, error))
	{
		ENESIM_RENDERER_ERROR(r, error, "Shape cannot setup");
		return EINA_FALSE;
	}

	enesim_renderer_shape_stroke_weight_get(r, &stroke);
	thiz->rr = EINA_F16P16_HALF * (stroke + 1);
	if (thiz->rr < EINA_F16P16_HALF) thiz->rr = EINA_F16P16_HALF;

	enesim_matrix_f16p16_matrix_to(&cs->transformation,
			&thiz->matrix);

	*fill = _spans[css->stroke.cap];

	return EINA_TRUE;
}

static void _line_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);

	enesim_renderer_shape_cleanup(r, s);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

static void _line_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_ARGB8888 |
			ENESIM_RENDERER_FLAG_GEOMETRY |
			ENESIM_SHAPE_FLAG_STROKE_RENDERER;
}

static void _line_free(Enesim_Renderer *r)
{
}

static void _line_boundings(Enesim_Renderer *r, Enesim_Rectangle *boundings)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);

	boundings->x = thiz->current.x0;
	boundings->y = thiz->current.y0;
	boundings->w = fabs(thiz->current.x1 - thiz->current.x0);
	boundings->h = fabs(thiz->current.y1 - thiz->current.y0);
}

static Eina_Bool _line_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);

	if (!thiz->changed) return EINA_FALSE;

	/* x0 */
	if (thiz->current.x0 != thiz->past.x0)
		return EINA_TRUE;
	/* y0 */
	if (thiz->current.y0 != thiz->past.y0)
		return EINA_TRUE;
	/* x1 */
	if (thiz->current.x1 != thiz->past.x1)
		return EINA_TRUE;
	/* y1 */
	if (thiz->current.y1 != thiz->past.y1)
		return EINA_TRUE;

	return EINA_FALSE;
}


static Enesim_Renderer_Shape_Descriptor _line_descriptor = {
	/* .name = 			*/ _line_name,
	/* .free = 			*/ _line_free,
	/* .boundings = 		*/ NULL,// _line_boundings,
	/* .destination_transform = 	*/ NULL,
	/* .flags = 			*/ _line_flags,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _line_has_changed,
	/* .sw_setup = 			*/ _line_state_setup,
	/* .sw_cleanup = 		*/ _line_state_cleanup,
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
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Renderer * enesim_renderer_line_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Line *thiz;
	static Eina_Bool spans_initialized = EINA_FALSE;

	if (!spans_initialized)
	{
		spans_initialized = EINA_TRUE;
		_spans[ENESIM_CAP_BUTT] = _span_butt;
		_spans[ENESIM_CAP_ROUND] = _span_round;
		_spans[ENESIM_CAP_SQUARE] = _span_square;
	}

	thiz = calloc(1, sizeof(Enesim_Renderer_Line));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_LINE_MAGIC);
	r = enesim_renderer_shape_new(&_line_descriptor, thiz);
	return r;
}

EAPI void enesim_renderer_line_x0_set(Enesim_Renderer *r, double x0)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);
	thiz->current.x0 = x0;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_line_x0_get(Enesim_Renderer *r, double *x0)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);
	*x0 = thiz->current.x0;
}


EAPI void enesim_renderer_line_y0_set(Enesim_Renderer *r, double y0)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);
	thiz->current.y0 = y0;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_line_y0_get(Enesim_Renderer *r, double *y0)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);
	*y0 = thiz->current.y0;
}

EAPI void enesim_renderer_line_x1_set(Enesim_Renderer *r, double x1)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);
	thiz->current.x1 = x1;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_line_x1_get(Enesim_Renderer *r, double *x1)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);
	*x1 = thiz->current.x1;
}

EAPI void enesim_renderer_line_y1_set(Enesim_Renderer *r, double y1)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);
	thiz->current.y1 = y1;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_line_y1_get(Enesim_Renderer *r, double *y1)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);
	*y1 = thiz->current.y1;
}
