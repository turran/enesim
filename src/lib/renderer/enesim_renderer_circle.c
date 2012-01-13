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

typedef struct _Enesim_Renderer_Circle {
	EINA_MAGIC
	/* public properties */
	Enesim_Renderer_Circle_State current;
	Enesim_Renderer_Circle_State past;
	/* private */
	Eina_Bool changed : 1;
	/* for the case we use the path renderer */
	Enesim_Renderer *path;
	Enesim_Renderer_Sw_Fill fill;
	/* for our own case */
	Enesim_F16p16_Matrix matrix;
	int xx0, yy0;
	int rr0, irr0;
	unsigned char do_inner :1;
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
		const Enesim_Renderer_State *state, const Enesim_Renderer_Shape_State *sstate)
{
	if (!thiz->path)
		thiz->path = enesim_renderer_path_new();
	/* generate the four arcs */
	if (thiz->changed)
	{
		enesim_renderer_path_command_clear(thiz->path);
		enesim_renderer_path_move_to(thiz->path, x, y -r);
		enesim_renderer_path_arc_to(thiz->path, r, r, 0, EINA_FALSE, EINA_TRUE, x + r, y);
		enesim_renderer_path_arc_to(thiz->path, r, r, 0, EINA_FALSE, EINA_TRUE, x, y + r);
		enesim_renderer_path_arc_to(thiz->path, r, r, 0, EINA_FALSE, EINA_TRUE, x - r, y);
		enesim_renderer_path_arc_to(thiz->path, r, r, 0, EINA_FALSE, EINA_TRUE, x, y - r);
	}

	enesim_renderer_color_set(thiz->path, state->color);
	enesim_renderer_origin_set(thiz->path, state->ox, state->oy);
	enesim_renderer_geometry_transformation_set(thiz->path, &state->geometry_transformation);

	enesim_renderer_shape_fill_renderer_set(thiz->path, sstate->fill.r);
	enesim_renderer_shape_fill_color_set(thiz->path, sstate->fill.color);
	enesim_renderer_shape_stroke_renderer_set(thiz->path, sstate->stroke.r);
	enesim_renderer_shape_stroke_weight_set(thiz->path, sstate->stroke.weight);
	enesim_renderer_shape_stroke_color_set(thiz->path, sstate->stroke.color);
	enesim_renderer_shape_draw_mode_set(thiz->path, sstate->draw_mode);
}
/*----------------------------------------------------------------------------*
 *                               Span functions                               *
 *----------------------------------------------------------------------------*/
/* Use the internal path for drawing */
static void _circle_path_span(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	thiz->fill(thiz->path, x, y, len, ddata);
}
/*
 these span draw functions need to be optimized further
 eg. special cases are needed when transform == identity.
 */

/* stroke and/or fill with possibly a fill renderer */
static void _stroke_fill_paint_affine(Enesim_Renderer *r, int x, int y,
 		unsigned int len, void *ddata)
{
	Enesim_Renderer_Circle *thiz = _circle_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Renderer *fpaint;
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	int do_inner = thiz->do_inner;
	Enesim_Color color;
	unsigned int ocolor;
	unsigned int icolor;
	int rr0 = thiz->rr0, rr1 = rr0 + 65536;
	int irr0 = thiz->irr0, irr1 = irr0 + 65536;
	int rr2 = rr1 * 1.41421357, irr2 = irr1 * 1.41421357; // sqrt(2)
	int xx0 = thiz->xx0, yy0 = thiz->yy0;
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
			Enesim_Renderer_Sw_Data *sdata;

			sdata = enesim_renderer_backend_data_get(fpaint, ENESIM_BACKEND_SOFTWARE);
			sdata->fill(fpaint, x, y, len, dst);
		}
	}
	if ((draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL) && do_inner
			&& fpaint)
	{
		Enesim_Renderer_Sw_Data *sdata;

		sdata = enesim_renderer_backend_data_get(fpaint, ENESIM_BACKEND_SOFTWARE);
		sdata->fill(fpaint, x, y, len, dst);
	}

        enesim_renderer_affine_setup(r, x, y, &thiz->matrix, &xx, &yy);
	xx -= xx0;
	yy -= yy0;
	while (d < e)
	{
		unsigned int q0 = 0;

		if ((abs(xx) <= rr1) && (abs(yy) <= rr1))
		{
			unsigned int op0 = ocolor, p0;
			int a = 256;

			if (fill_only && fpaint)
				op0 = argb8888_mul4_sym(*d, op0);

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
				op0 = argb8888_mul_256(a, op0);

			p0 = op0;
			if (do_inner && (abs(xx) <= irr1) && (abs(yy) <= irr1))
			{
				p0 = icolor;
				if (fpaint)
				{
					p0 = *d;
					if (icolor != 0xffffffff)
						p0 = argb8888_mul4_sym(icolor, p0);
				}
				a = 256;
				if (abs(xx) + abs(yy) >= irr0)
				{
					a = 0;
					if (abs(xx) + abs(yy) <= irr2)
					{
						int rr = hypot(xx, yy);

						if (rr < irr1)
						{
							a = 256;
							if (rr > irr0)
								a -= ((rr - irr0) >> 8);
						}
					}
				}

				if (a < 256)
					p0 = argb8888_interp_256(a, p0, op0);
			}
			q0 = p0;
		}
		*d++ = q0;
		xx += axx;
		yy += ayx;
	}
}

/* stroke with a renderer and possibly fill with color */
static void _stroke_paint_fill_affine(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Circle *thiz = _circle_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Renderer *spaint;
	Enesim_Renderer_Sw_Data *sdata;
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	int do_inner = thiz->do_inner;
	Enesim_Color color;
	unsigned int ocolor;
	unsigned int icolor;
	int rr0 = thiz->rr0, rr1 = rr0 + 65536;
	int irr0 = thiz->irr0, irr1 = irr0 + 65536;
	int rr2 = rr1 * 1.41421357, irr2 = irr1 * 1.41421357; // sqrt(2)
	int xx0 = thiz->xx0, yy0 = thiz->yy0;
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

	sdata = enesim_renderer_backend_data_get(spaint, ENESIM_BACKEND_SOFTWARE);
	sdata->fill(spaint, x, y, len, dst);

	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)
		icolor = 0;

        enesim_renderer_affine_setup(r, x, y, &thiz->matrix, &xx, &yy);
	xx -= xx0;
	yy -= yy0;
	while (d < e)
	{
		unsigned int q0 = 0;

		if ((abs(xx) <= rr1) && (abs(yy) <= rr1))
		{
			unsigned int op0 = *d, p0;
			int a = 256;

			if (ocolor != 0xffffffff)
				op0 = argb8888_mul4_sym(ocolor, op0);
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
				op0 = argb8888_mul_256(a, op0);

			p0 = op0;
			if (do_inner && (abs(xx) <= irr1) && (abs(yy) <= irr1))
			{
				p0 = icolor;
				a = 256;
				if (abs(xx) + abs(yy) >= irr0)
				{
					a = 0;
					if (abs(xx) + abs(yy) <= irr2)
					{
						int rr = hypot(xx, yy);

						if (rr < irr1)
						{
							a = 256;
							if (rr > irr0)
								a -= ((rr - irr0) >> 8);
						}
					}
				}

				if (a < 256)
					p0 = argb8888_interp_256(a, p0, op0);
			}
			q0 = p0;
		}
		*d++ = q0;
		xx += axx;
		yy += ayx;
	}
}

/* stroke and fill with renderers */
static void _stroke_paint_fill_paint_affine(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Circle *thiz = _circle_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Renderer *fpaint, *spaint;
	Enesim_Renderer_Sw_Data *sdata;
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	unsigned int ocolor;
	unsigned int icolor;
	int rr0 = thiz->rr0, rr1 = rr0 + 65536;
	int irr0 = thiz->irr0, irr1 = irr0 + 65536;
	int rr2 = rr1 * 1.41421357, irr2 = irr1 * 1.41421357; // sqrt(2)
	int xx0 = thiz->xx0, yy0 = thiz->yy0;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	unsigned int *sbuf, *s;
	int xx, yy;

	enesim_renderer_shape_stroke_color_get(r, &ocolor);
	enesim_renderer_shape_stroke_renderer_get(r, &spaint);
	enesim_renderer_shape_fill_color_get(r, &icolor);
	enesim_renderer_shape_fill_renderer_get(r, &fpaint);
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);

	sdata = enesim_renderer_backend_data_get(fpaint, ENESIM_BACKEND_SOFTWARE);
	sdata->fill(fpaint, x, y, len, dst);

	sbuf = alloca(len * sizeof(unsigned int));
	sdata = enesim_renderer_backend_data_get(spaint, ENESIM_BACKEND_SOFTWARE);
	sdata->fill(spaint, x, y, len, sbuf);
	s = sbuf;

        enesim_renderer_affine_setup(r, x, y, &thiz->matrix, &xx, &yy);
	xx -= xx0;
	yy -= yy0;
	while (d < e)
	{
		unsigned int q0 = 0;

		if ((abs(xx) <= rr1) && (abs(yy) <= rr1))
		{
			unsigned int op0 = *s, p0;
			int a = 256;

			if (ocolor != 0xffffffff)
				op0 = argb8888_mul4_sym(ocolor, op0);

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
				op0 = argb8888_mul_256(a, op0);

			p0 = op0;
			if ((abs(xx) <= irr1) && (abs(yy) <= irr1))
			{
				p0 = *d;
				if (icolor != 0xffffffff)
					p0 = argb8888_mul4_sym(icolor, p0);
				a = 256;
				if (abs(xx) + abs(yy) >= irr0)
				{
					a = 0;
					if (abs(xx) + abs(yy) <= irr2)
					{
						int rr = hypot(xx, yy);

						if (rr < irr1)
						{
							a = 256;
							if (rr > irr0)
								a -= ((rr - irr0) >> 8);
						}
					}
				}

				if (a < 256)
					p0 = argb8888_interp_256(a, p0, op0);
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

static void _boundings(Enesim_Renderer *r, Enesim_Rectangle *rect)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	rect->x = thiz->current.x - thiz->current.r;
	rect->y = thiz->current.y - thiz->current.r;
	rect->w = rect->h = thiz->current.r * 2;
}

static Eina_Bool _state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Circle *thiz;
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Renderer *spaint;
	double rad;
	double sw;

	thiz = _circle_get(r);
	if (!thiz || (thiz->current.r < 1))
		return EINA_FALSE;

	if (_circle_use_path(state->geometry_transformation_type))
	{
		_circle_path_setup(thiz, thiz->current.x, thiz->current.y, thiz->current.r, state, sstate);
		if (!enesim_renderer_setup(thiz->path, s, error))
		{
			return EINA_FALSE;
		}
		thiz->fill = enesim_renderer_sw_fill_get(thiz->path);
		*fill = _circle_path_span;

		return EINA_TRUE;
	}
	else
	{

		thiz->rr0 = 65536 * (thiz->current.r - 1);
		thiz->xx0 = 65536 * (thiz->current.x - 0.5);
		thiz->yy0 = 65536 * (thiz->current.y - 0.5);

		enesim_renderer_shape_stroke_weight_get(r, &sw);
		thiz->do_inner = 1;
		if (sw >= (thiz->current.r - 1))
		{
			sw = 0;
			thiz->do_inner = 0;
		}
		rad = thiz->current.r - 1 - sw;
		if (rad < 0.0039)
			rad = 0;

		thiz->irr0 = rad * 65536;

		if (!enesim_renderer_shape_setup(r, state, s, error))
			return EINA_FALSE;

		enesim_matrix_f16p16_matrix_to(&state->transformation,
				&thiz->matrix);

		enesim_renderer_shape_draw_mode_get(r, &draw_mode);
		enesim_renderer_shape_stroke_renderer_get(r, &spaint);

		*fill = _stroke_fill_paint_affine;
		if ((sw != 0.0) && spaint && (draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE))
		{
			Enesim_Renderer *fpaint;

			*fill = _stroke_paint_fill_affine;
			enesim_renderer_shape_fill_renderer_get(r, &fpaint);
			if (fpaint && thiz->do_inner &&
						(draw_mode & ENESIM_SHAPE_DRAW_MODE_FILL))
				*fill = _stroke_paint_fill_paint_affine;
		}

		return EINA_TRUE;
	}
}

static void _state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	enesim_renderer_shape_cleanup(r, s);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

static void _flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	if (!thiz)
	{
		*flags = 0;
		return;
	}

	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_ARGB8888 |
			ENESIM_SHAPE_FLAG_FILL_RENDERER;
}

static Eina_Bool _circle_has_changed(Enesim_Renderer *r)
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

static void _free(Enesim_Renderer *r)
{
	Enesim_Renderer_Circle *thiz;

	thiz = _circle_get(r);
	if (thiz->path)
		enesim_renderer_unref(thiz->path);
	free(thiz);
}

static Enesim_Renderer_Shape_Descriptor _circle_descriptor = {
	/* .name = 			*/ _circle_name,
	/* .free = 			*/ _free,
	/* .boundings = 		*/ _boundings,
	/* .destination_transform = 	*/ NULL,
	/* .flags = 			*/ _flags,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _circle_has_changed,
	/* .sw_setup = 			*/ _state_setup,
	/* .sw_cleanup = 		*/ _state_cleanup,
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
