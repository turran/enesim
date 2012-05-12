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
#include "private/shape.h"
#include "libargb.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_CIRCLE_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_CIRCLE_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_CIRCLE_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Circle_State {
	double x;
	double y;
	double r;
} Enesim_Renderer_Circle_State;

typedef struct _Enesim_Renderer_Circle_Sw
{
	Eina_F16p16 rr0;
	Eina_F16p16 rr1; /* rr0 + 1 */
	Eina_F16p16 rr2; /* rr1 * sqrt2 */
} Enesim_Renderer_Circle_Sw;

typedef struct _Enesim_Renderer_Circle_Sw_State
{
	Enesim_Renderer_Circle_Sw outter;
	Enesim_Renderer_Circle_Sw inner;
	Eina_F16p16 xx0, yy0;
	Enesim_F16p16_Matrix matrix;
	Eina_Bool do_inner :1;
} Enesim_Renderer_Circle_Sw_State;

typedef struct _Enesim_Renderer_Circle {
	EINA_MAGIC
	/* public properties */
	Enesim_Renderer_Circle_State current;
	Enesim_Renderer_Circle_State past;
	/* private */
	Eina_Bool changed : 1;
	Eina_Bool use_path : 1;
	/* for the case we use the path renderer */
	Enesim_Renderer *path;
	Enesim_Renderer_Circle_Sw_State sw_state;
} Enesim_Renderer_Circle;

static inline Enesim_Renderer_Circle * _circle_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Circle *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	ENESIM_RENDERER_CIRCLE_MAGIC_CHECK(thiz);

	return thiz;
}

static Eina_Bool _circle_use_path(Enesim_Matrix_Type geometry_type)
{
	if (geometry_type != ENESIM_MATRIX_IDENTITY)
		return EINA_TRUE;
	return EINA_FALSE;
}

static void _circle_path_setup(Enesim_Renderer_Circle *thiz,
		double x, double y, double r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES])
{
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	//const Enesim_Renderer_State *ps = states[ENESIM_STATE_PAST];
	const Enesim_Renderer_Shape_State *css = sstates[ENESIM_STATE_CURRENT];

	if (!thiz->path)
		thiz->path = enesim_renderer_path_new();
	/* generate the four arcs */
	/* FIXME also check that the prev geometry and curr geometry transformations are diff */
	if (thiz->changed)
	{
		enesim_renderer_path_command_clear(thiz->path);
		enesim_renderer_path_move_to(thiz->path, x, y -r);
		enesim_renderer_path_arc_to(thiz->path, r, r, 0, EINA_FALSE, EINA_TRUE, x + r, y);
		enesim_renderer_path_arc_to(thiz->path, r, r, 0, EINA_FALSE, EINA_TRUE, x, y + r);
		enesim_renderer_path_arc_to(thiz->path, r, r, 0, EINA_FALSE, EINA_TRUE, x - r, y);
		enesim_renderer_path_arc_to(thiz->path, r, r, 0, EINA_FALSE, EINA_TRUE, x, y - r);
	}

	/* pass all the properties to the path */
	enesim_renderer_color_set(thiz->path, cs->color);
	enesim_renderer_origin_set(thiz->path, cs->ox, cs->oy);
	enesim_renderer_geometry_transformation_set(thiz->path, &cs->geometry_transformation);
	/* base properties */
	enesim_renderer_shape_fill_renderer_set(thiz->path, css->fill.r);
	enesim_renderer_shape_fill_color_set(thiz->path, css->fill.color);
	enesim_renderer_shape_stroke_renderer_set(thiz->path, css->stroke.r);
	enesim_renderer_shape_stroke_weight_set(thiz->path, css->stroke.weight);
	enesim_renderer_shape_stroke_color_set(thiz->path, css->stroke.color);
	enesim_renderer_shape_draw_mode_set(thiz->path, css->draw_mode);
}

static inline Enesim_Color _circle_sample(Eina_F16p16 xx, Eina_F16p16 yy,
		Enesim_Renderer_Circle_Sw *swc,
		Enesim_Color ocolor, Enesim_Color icolor)
{
	Eina_F16p16 rr0 = swc->rr0;
	Eina_F16p16 rr1 = swc->rr1;
	Eina_F16p16 rr2 = swc->rr2;
	Enesim_Color p0;
	uint16_t a = 256;

	p0 = icolor;
	if (abs(xx) + abs(yy) >= rr0)
	{
		a = 0;
		if (abs(xx) + abs(yy) <= rr2)
		{
			int rr = hypot(xx, yy);

			if (rr < rr1)
			{
				a = 256;
				if (rr > rr0)
					a -= ((rr - rr0) >> 8);
			}
		}
	}
	if (a < 256)
		p0 = argb8888_interp_256(a, icolor, ocolor);

	return p0;
}
/*----------------------------------------------------------------------------*
 *                               Span functions                               *
 *----------------------------------------------------------------------------*/
/* Use the internal path for drawing */
static void _circle_path_span(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	enesim_renderer_sw_draw(thiz->path, x, y, len, ddata);
}
/*
 these span draw functions need to be optimized further
 eg. special cases are needed when transform == identity.
 */

/* stroke and/or fill with possibly a fill renderer */
static void _stroke_fill_paint_affine(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y,
 		unsigned int len, void *ddata)
{
	Enesim_Renderer_Circle *thiz = _circle_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Renderer *fpaint;
	int axx = thiz->sw_state.matrix.xx;
	int ayx = thiz->sw_state.matrix.yx;
	int do_inner = thiz->sw_state.do_inner;
	Enesim_Color color;
	unsigned int ocolor;
	unsigned int icolor;
	int rr0 = thiz->sw_state.outter.rr0, rr1 = thiz->sw_state.outter.rr1;
	int irr0 = thiz->sw_state.inner.rr0, irr1 = thiz->sw_state.inner.rr1;
	int xx0 = thiz->sw_state.xx0, yy0 = thiz->sw_state.yy0;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	int xx, yy;
	int fill_only = 0;

	enesim_renderer_shape_stroke_color_get(r, &ocolor);
	enesim_renderer_shape_fill_color_get(r, &icolor);
 	enesim_renderer_shape_fill_renderer_get(r, &fpaint);
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);

	enesim_renderer_color_get(r, &color);
	if (color != 0xffffffff)
	{
		ocolor = argb8888_mul4_sym(color, ocolor);
		icolor = argb8888_mul4_sym(color, icolor);
	}

	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)
	{
		if (rr0 == irr0)  // if stroke weight 0 or too small radii
		{
			memset(dst, 0, len * sizeof (unsigned int));
			return;
		}
		icolor = 0;
		fpaint = NULL;
	}

	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL)
	{
		ocolor = icolor;
		fill_only = 1;
		do_inner = 0;
		if (fpaint)
		{
			enesim_renderer_sw_draw(fpaint, x, y, len, dst);
		}
	}
	if ((draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL) && do_inner
			&& fpaint)
	{
		enesim_renderer_sw_draw(fpaint, x, y, len, dst);
	}

        enesim_renderer_affine_setup(r, x, y, &thiz->sw_state.matrix, &xx, &yy);
	xx -= xx0;
	yy -= yy0;

	while (d < e)
	{
		Eina_F16p16 absxx = abs(xx);
		Eina_F16p16 absyy = abs(yy);
		unsigned int q0 = 0;

		if ((absxx <= rr1) && (absyy <= rr1))
		{
			unsigned int op0 = ocolor, p0;

			if (fill_only && fpaint)
				op0 = argb8888_mul4_sym(*d, op0);

			p0 = _circle_sample(xx, yy, &thiz->sw_state.outter, 0, op0);
			op0 = p0;
			if (do_inner && (absxx <= irr1) && (absyy <= irr1))
			{
				p0 = icolor;
				if (fpaint)
				{
					p0 = *d;
					if (icolor != 0xffffffff)
						p0 = argb8888_mul4_sym(icolor, p0);
				}
				p0 = _circle_sample(xx, yy, &thiz->sw_state.inner, op0, p0);
			}
			q0 = p0;
		}
		*d++ = q0;
		xx += axx;
		yy += ayx;
	}
}

/* stroke with a renderer and possibly fill with color */
static void _stroke_paint_fill_affine(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Circle *thiz = _circle_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Renderer *spaint;
	int axx = thiz->sw_state.matrix.xx;
	int ayx = thiz->sw_state.matrix.yx;
	int do_inner = thiz->sw_state.do_inner;
	Enesim_Color color;
	unsigned int ocolor;
	unsigned int icolor;
	int rr1 = thiz->sw_state.outter.rr1;
	int irr1 = thiz->sw_state.inner.rr1;
	int xx0 = thiz->sw_state.xx0, yy0 = thiz->sw_state.yy0;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	int xx, yy;

	enesim_renderer_shape_stroke_color_get(r, &ocolor);
	enesim_renderer_shape_stroke_renderer_get(r, &spaint);
	enesim_renderer_shape_fill_color_get(r, &icolor);
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);

	enesim_renderer_color_get(r, &color);
	if (color != 0xffffffff)
	{
		ocolor = argb8888_mul4_sym(color, ocolor);
		icolor = argb8888_mul4_sym(color, icolor);
	}

	enesim_renderer_sw_draw(spaint, x, y, len, dst);

	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)
		icolor = 0;

        enesim_renderer_affine_setup(r, x, y, &thiz->sw_state.matrix, &xx, &yy);
	xx -= xx0;
	yy -= yy0;

	while (d < e)
	{
		Eina_F16p16 absxx = abs(xx);
		Eina_F16p16 absyy = abs(yy);
		unsigned int q0 = 0;

		if ((absxx <= rr1) && (absyy <= rr1))
		{
			unsigned int op0 = *d, p0;

			if (ocolor != 0xffffffff)
				op0 = argb8888_mul4_sym(ocolor, op0);

			p0 = _circle_sample(xx, yy, &thiz->sw_state.outter, 0, op0);
			op0 = p0;
			if (do_inner && (absxx <= irr1) && (absyy <= irr1))
			{
				p0 = _circle_sample(xx, yy, &thiz->sw_state.inner, op0, icolor);
			}
			q0 = p0;
		}
		*d++ = q0;
		xx += axx;
		yy += ayx;
	}
}

/* stroke and fill with renderers */
static void _stroke_paint_fill_paint_affine(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Circle *thiz = _circle_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Renderer *fpaint, *spaint;
	int axx = thiz->sw_state.matrix.xx;
	int ayx = thiz->sw_state.matrix.yx;
	unsigned int ocolor;
	unsigned int icolor;
	int rr1 = thiz->sw_state.outter.rr1;
	int irr1 = thiz->sw_state.inner.rr1;
	int xx0 = thiz->sw_state.xx0, yy0 = thiz->sw_state.yy0;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	unsigned int *sbuf, *s;
	int xx, yy;

	enesim_renderer_shape_stroke_color_get(r, &ocolor);
	enesim_renderer_shape_stroke_renderer_get(r, &spaint);
	enesim_renderer_shape_fill_color_get(r, &icolor);
	enesim_renderer_shape_fill_renderer_get(r, &fpaint);
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);

	enesim_renderer_sw_draw(fpaint, x, y, len, dst);
	sbuf = alloca(len * sizeof(unsigned int));
	enesim_renderer_sw_draw(spaint, x, y, len, sbuf);
	s = sbuf;

        enesim_renderer_affine_setup(r, x, y, &thiz->sw_state.matrix, &xx, &yy);
	xx -= xx0;
	yy -= yy0;

	while (d < e)
	{
		Eina_F16p16 absxx = abs(xx);
		Eina_F16p16 absyy = abs(yy);
		unsigned int q0 = 0;

		if ((absxx <= rr1) && (absyy <= rr1))
		{
			unsigned int op0 = *s, p0;

			p0 = _circle_sample(xx, yy, &thiz->sw_state.outter, 0, op0);
			op0 = p0;

			if ((absxx <= irr1) && (absyy <= irr1))
			{
				p0 = *d;
				if (icolor != 0xffffffff)
					p0 = argb8888_mul4_sym(icolor, p0);
				p0 = _circle_sample(xx, yy, &thiz->sw_state.inner, op0, p0);
			}
			q0 = p0;
		}
		*d++ = q0;
		s++;
		xx += axx;
		yy += ayx;
	}
}

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _circle_name(Enesim_Renderer *r)
{
	return "circle";
}

/* FIXME we still need to decide what to do with the stroke
 * transformation
 */
static void _circle_boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Circle *thiz;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	const Enesim_Renderer_Shape_State *css = sstates[ENESIM_STATE_CURRENT];
	double sw = 0;

	thiz = _circle_get(r);
	if (css->draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE)
		enesim_renderer_shape_stroke_weight_get(r, &sw);
	switch (css->stroke.location)
	{
		case ENESIM_SHAPE_STROKE_CENTER:
		sw /= 2.0;
		break;

		case ENESIM_SHAPE_STROKE_INSIDE:
		sw = 0.0;
		break;

		case ENESIM_SHAPE_STROKE_OUTSIDE:
		break;
	}
	rect->x = thiz->current.x - thiz->current.r - sw;
	rect->y = thiz->current.y - thiz->current.r - sw;
	rect->w = rect->h = (thiz->current.r + sw) * 2;

	/* translate by the origin */
	rect->x += cs->ox;
	rect->y += cs->oy;
	/* apply the geometry transformation */
	if (cs->geometry_transformation_type != ENESIM_MATRIX_IDENTITY)
	{
		Enesim_Quad q;

		enesim_matrix_rectangle_transform(&cs->geometry_transformation, rect, &q);
		enesim_quad_rectangle_to(&q, rect);
	}
}

static void _circle_destination_boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Eina_Rectangle *boundings)
{
	Enesim_Rectangle oboundings;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];

	_circle_boundings(r, states, sstates, &oboundings);
	/* apply the inverse matrix */
	if (cs->transformation_type != ENESIM_MATRIX_IDENTITY)
	{
		Enesim_Quad q;
		Enesim_Matrix m;

		enesim_matrix_inverse(&cs->transformation, &m);
		enesim_matrix_rectangle_transform(&m, &oboundings, &q);
		enesim_quad_rectangle_to(&q, &oboundings);
		/* fix the antialias scaling */
		oboundings.x -= m.xx;
		oboundings.y -= m.yy;
		oboundings.w += m.xx;
		oboundings.h += m.yy;
	}
	boundings->x = floor(oboundings.x);
	boundings->y = floor(oboundings.y);
	boundings->w = ceil(oboundings.x - boundings->x + oboundings.w);
	boundings->h = ceil(oboundings.y - boundings->y + oboundings.h);
}

static Eina_Bool _circle_sw_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Shape_Sw_Draw *draw, Enesim_Error **error)
{
	Enesim_Renderer_Circle *thiz;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	const Enesim_Renderer_Shape_State *css = sstates[ENESIM_STATE_CURRENT];

	thiz = _circle_get(r);
	if (!thiz || (thiz->current.r < 1))
		return EINA_FALSE;

	thiz->use_path = _circle_use_path(cs->geometry_transformation_type);
	if (thiz->use_path)
	{
		double rad;

		rad = thiz->current.r;
		if (css->draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE)
		{
			switch (css->stroke.location)
			{
				case ENESIM_SHAPE_STROKE_OUTSIDE:
				rad += css->stroke.weight / 2.0;
				break;

				case ENESIM_SHAPE_STROKE_INSIDE:
				rad -= css->stroke.weight / 2.0;
				break;

				case ENESIM_SHAPE_STROKE_CENTER:
				break;
			}
		}

		_circle_path_setup(thiz, thiz->current.x, thiz->current.y, rad, states, sstates);
		if (!enesim_renderer_setup(thiz->path, s, error))
		{
			return EINA_FALSE;
		}
		*draw = _circle_path_span;

		return EINA_TRUE;
	}
	else
	{
		Enesim_Shape_Draw_Mode draw_mode;
		Enesim_Renderer *spaint;
		double rad;
		double sw;

		thiz->sw_state.do_inner = 1;

		sw = css->stroke.weight;
		rad = thiz->current.r;
		if (css->draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE)
		{
			/* handle the different stroke locations */
			switch (css->stroke.location)
			{
				case ENESIM_SHAPE_STROKE_CENTER:
				rad += sw / 2.0;
				break;

				case ENESIM_SHAPE_STROKE_INSIDE:
				if (sw >= (thiz->current.r - 1))
				{
					sw = 0;
					thiz->sw_state.do_inner = 0;
				}
				break;

				case ENESIM_SHAPE_STROKE_OUTSIDE:
				rad += sw;
				break;
			}
		}

		thiz->sw_state.outter.rr0 = 65536 * (rad - 1);
		thiz->sw_state.outter.rr1 = thiz->sw_state.outter.rr0 + 65536;
		thiz->sw_state.outter.rr2 = thiz->sw_state.outter.rr1 * M_SQRT2;
		thiz->sw_state.xx0 = 65536 * (thiz->current.x - 0.5);
		thiz->sw_state.yy0 = 65536 * (thiz->current.y - 0.5);

		rad = rad - 1 - sw;
		if (rad < 0.0039)
			rad = 0;

		thiz->sw_state.inner.rr0 = rad * 65536;
		thiz->sw_state.inner.rr1 = thiz->sw_state.inner.rr0 + 65536;
		thiz->sw_state.inner.rr2 = thiz->sw_state.inner.rr1 * M_SQRT2;

		if (!enesim_renderer_shape_setup(r, states, s, error))
			return EINA_FALSE;

		enesim_matrix_f16p16_matrix_to(&cs->transformation,
				&thiz->sw_state.matrix);

		enesim_renderer_shape_draw_mode_get(r, &draw_mode);
		enesim_renderer_shape_stroke_renderer_get(r, &spaint);

		*draw = _stroke_fill_paint_affine;
		if ((sw != 0.0) && spaint && (draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE))
		{
			Enesim_Renderer *fpaint;

			*draw = _stroke_paint_fill_affine;
			enesim_renderer_shape_fill_renderer_get(r, &fpaint);
			if (fpaint && thiz->sw_state.do_inner &&
						(draw_mode & ENESIM_SHAPE_DRAW_MODE_FILL))
				*draw = _stroke_paint_fill_paint_affine;
		}

		return EINA_TRUE;
	}
}

static void _circle_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	enesim_renderer_shape_cleanup(r, s);
	if (thiz->use_path)
		enesim_renderer_cleanup(thiz->path, s);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
	thiz->use_path = EINA_FALSE;
}

static void _circle_flags(Enesim_Renderer *r, const Enesim_Renderer_State *state,
		Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_ARGB8888 |
			ENESIM_RENDERER_FLAG_GEOMETRY;
}

static void _circle_hints(Enesim_Renderer *r, const Enesim_Renderer_State *state,
		Enesim_Renderer_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_COLORIZE;
}

static Eina_Bool _circle_has_changed(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES])
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	if (!thiz->changed) return EINA_FALSE;
	/* the radius */
	if (thiz->current.r != thiz->past.r)
		return EINA_TRUE;
	/* the x */
	if (thiz->current.x != thiz->past.x)
		return EINA_TRUE;
	/* the y */
	if (thiz->current.y != thiz->past.y)
		return EINA_TRUE;
	return EINA_FALSE;
}

static void _circle_feature_get(Enesim_Renderer *r, Enesim_Shape_Feature *features)
{
	*features = ENESIM_SHAPE_FLAG_FILL_RENDERER | ENESIM_SHAPE_FLAG_STROKE_RENDERER;
}

static void _circle_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	if (thiz->path)
		enesim_renderer_unref(thiz->path);
	free(thiz);
}

static Enesim_Renderer_Shape_Descriptor _circle_descriptor = {
	/* .name = 			*/ _circle_name,
	/* .free = 			*/ _circle_free,
	/* .boundings = 		*/ _circle_boundings,
	/* .destination_boundings = 	*/ _circle_destination_boundings,
	/* .flags = 			*/ _circle_flags,
	/* .hints_get = 		*/ _circle_hints,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _circle_has_changed,
	/* .feature_get =		*/ _circle_feature_get,
	/* .sw_setup = 			*/ _circle_sw_setup,
	/* .sw_cleanup = 		*/ _circle_sw_cleanup,
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
EAPI Enesim_Renderer * enesim_renderer_circle_new(void)
{
	Enesim_Renderer_Circle *thiz;
	Enesim_Renderer *r;

	thiz = calloc(1, sizeof(Enesim_Renderer_Circle));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_CIRCLE_MAGIC);
	r = enesim_renderer_shape_new(&_circle_descriptor, thiz);
	/* to maintain compatibility */
	enesim_renderer_shape_stroke_location_set(r, ENESIM_SHAPE_STROKE_INSIDE);
	return r;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_circle_x_set(Enesim_Renderer *r, double x)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	thiz->current.x = x;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_circle_y_set(Enesim_Renderer *r, double y)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_circle_center_set(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	thiz->current.x = x;
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_circle_center_get(Enesim_Renderer *r, double *x, double *y)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	if (x) *x = thiz->current.x;
	if (y) *y = thiz->current.y;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_circle_radius_set(Enesim_Renderer *r, double radius)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	if (radius < 1)
		radius = 1;
	thiz->current.r = radius;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_circle_radius_get(Enesim_Renderer *r, double *radius)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	if (radius) *radius = thiz->current.r;
}
