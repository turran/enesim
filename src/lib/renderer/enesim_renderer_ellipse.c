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
#define ENESIM_RENDERER_ELLIPSE_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_ELLIPSE_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_ELLIPSE_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Ellipse_State {
	double x;
	double y;
	double rx;
	double ry;
} Enesim_Renderer_Ellipse_State;

typedef struct _Enesim_Renderer_Ellipse
{
	EINA_MAGIC
	/* properties */
	Enesim_Renderer_Ellipse_State current;
	Enesim_Renderer_Ellipse_State past;
	/* private */
	Eina_Bool changed : 1;
	/* for the case we use the path renderer */
	Enesim_Renderer *path;
	Enesim_Renderer_Sw_Fill fill;
	/* for our own case */
	Enesim_F16p16_Matrix matrix;
	int xx0, yy0;
	int rr0_x, rr0_y;
	int irr0_x, irr0_y;
	int cc0, icc0;
	int fxxp, fyyp;
	int ifxxp, ifyyp;
	unsigned char do_inner :1;
} Enesim_Renderer_Ellipse;

static inline Enesim_Renderer_Ellipse * _ellipse_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	ENESIM_RENDERER_ELLIPSE_MAGIC_CHECK(thiz);

	return thiz;
}

static Eina_Bool _ellipse_use_path(Enesim_Matrix_Type geometry_type)
{
	if (geometry_type != ENESIM_MATRIX_IDENTITY)
		return EINA_TRUE;
	return EINA_FALSE;
}

static void _ellipse_path_setup(Enesim_Renderer_Ellipse *thiz,
		double x, double y, double rx, double ry,
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
		enesim_renderer_path_move_to(thiz->path, x, y - ry);
		enesim_renderer_path_arc_to(thiz->path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x + rx, y);
		enesim_renderer_path_arc_to(thiz->path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x, y + ry);
		enesim_renderer_path_arc_to(thiz->path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x - rx, y);
		enesim_renderer_path_arc_to(thiz->path, rx, ry, 0, EINA_FALSE, EINA_TRUE, x, y - ry);
	}

	enesim_renderer_color_set(thiz->path, cs->color);
	enesim_renderer_origin_set(thiz->path, cs->ox, cs->oy);
	enesim_renderer_geometry_transformation_set(thiz->path, &cs->geometry_transformation);

	enesim_renderer_shape_fill_renderer_set(thiz->path, css->fill.r);
	enesim_renderer_shape_fill_color_set(thiz->path, css->fill.color);
	enesim_renderer_shape_stroke_renderer_set(thiz->path, css->stroke.r);
	enesim_renderer_shape_stroke_weight_set(thiz->path, css->stroke.weight);
	enesim_renderer_shape_stroke_color_set(thiz->path, css->stroke.color);
	enesim_renderer_shape_draw_mode_set(thiz->path, css->draw_mode);
}
/*----------------------------------------------------------------------------*
 *                               Span functions                               *
 *----------------------------------------------------------------------------*/
/* Use the internal path for drawing */
static void _ellipse_path_span(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = _ellipse_get(r);
	thiz->fill(thiz->path, x, y, len, ddata);
}

/* these span draw functions need to be optimized further
 * eg. special cases are needed when transform == identity.
 */

/* stroke and/or fill with possibly a fill renderer */
static void _stroke_fill_paint_affine(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Ellipse *thiz = _ellipse_get(r);
	Enesim_Renderer *fpaint;
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Color color;
	Enesim_Color ocolor;
	Enesim_Color icolor;
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	int do_inner = thiz->do_inner;
	int xx0 = thiz->xx0, yy0 = thiz->yy0;
	int rr0_x = thiz->rr0_x, rr1_x = rr0_x + 65536;
	int rr0_y = thiz->rr0_y, rr1_y = rr0_y + 65536;
	int irr0_x = thiz->irr0_x, irr1_x = irr0_x + 65536;
	int irr0_y = thiz->irr0_y, irr1_y = irr0_y + 65536;
	int cc0 = thiz->cc0, cc1 = cc0 + 65536;
	int icc0 = thiz->icc0, icc1 = icc0 + 65536;
	int fxxp = xx0 + thiz->fxxp, fyyp = yy0 + thiz->fyyp;
	int fxxm = xx0 - thiz->fxxp, fyym = yy0 - thiz->fyyp;
	int ifxxp = xx0 + thiz->ifxxp, ifyyp = yy0 + thiz->ifyyp;
	int ifxxm = xx0 - thiz->ifxxp, ifyym = yy0 - thiz->ifyyp;
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
		if (rr0_x == irr0_x)  // if stroke weight 0 or too small radii
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
	if ((draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL) && do_inner && fpaint)
	{
		Enesim_Renderer_Sw_Data *sdata;

		sdata = enesim_renderer_backend_data_get(fpaint, ENESIM_BACKEND_SOFTWARE);
		sdata->fill(fpaint, x, y, len, dst);
	}

	enesim_renderer_affine_setup(r, x, y, &thiz->matrix, &xx, &yy);

	while (d < e)
	{
		unsigned int q0 = 0;

		if ((abs(xx - xx0) <= rr1_x) && (abs(yy - yy0) <= rr1_y))
		{
			unsigned int op0 = ocolor, p0;
			int a = 256;

			if (fill_only && fpaint)
			{
				op0 = *d;
				if (ocolor != 0xffffffff)
					op0 = argb8888_mul4_sym(op0, ocolor);
			}

			if (((abs(xx - xx0) >= (rr0_x / 2))) || ((abs(yy - yy0) >= (rr0_y
					/ 2))))
			{
				int rr = hypot(xx - fxxp, yy - fyyp) + hypot(xx - fxxm, yy
						- fyym);

				a = 0;
				if (rr < cc1)
				{
					a = 256;
					if (rr > cc0)
						a -= ((rr - cc0) >> 8);
				}
			}

			if (a < 256)
				op0 = argb8888_mul_256(a, op0);

			p0 = op0;
			if (do_inner && (abs(xx - xx0) <= irr1_x) && (abs(yy - yy0)
					<= irr1_y))
			{
				p0 = icolor;
				if (fpaint)
				{
					p0 = *d;
					if (icolor != 0xffffffff)
						p0 = argb8888_mul4_sym(icolor, p0);
				}

				a = 256;
				if (((abs(xx - xx0) >= (irr0_x / 2))) || ((abs(yy - yy0)
						>= (irr0_y / 2))))
				{
					int rr = hypot(xx - ifxxp, yy - ifyyp) + hypot(xx - ifxxm,
							yy - ifyym);

					a = 0;
					if (rr < icc1)
					{
						a = 256;
						if (rr > icc0)
							a -= ((rr - icc0) >> 8);
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
static void _stroke_paint_fill_affine(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Ellipse *thiz = _ellipse_get(r);
	Enesim_Renderer *spaint;
	Enesim_Renderer_Sw_Data *sdata;
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Color color;
	Enesim_Color ocolor;
	Enesim_Color icolor;
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	int do_inner = thiz->do_inner;
	int xx0 = thiz->xx0, yy0 = thiz->yy0;
	int rr0_x = thiz->rr0_x, rr1_x = rr0_x + 65536;
	int rr0_y = thiz->rr0_y, rr1_y = rr0_y + 65536;
	int irr0_x = thiz->irr0_x, irr1_x = irr0_x + 65536;
	int irr0_y = thiz->irr0_y, irr1_y = irr0_y + 65536;
	int cc0 = thiz->cc0, cc1 = cc0 + 65536;
	int icc0 = thiz->icc0, icc1 = icc0 + 65536;
	int fxxp = xx0 + thiz->fxxp, fyyp = yy0 + thiz->fyyp;
	int fxxm = xx0 - thiz->fxxp, fyym = yy0 - thiz->fyyp;
	int ifxxp = xx0 + thiz->ifxxp, ifyyp = yy0 + thiz->ifyyp;
	int ifxxm = xx0 - thiz->ifxxp, ifyym = yy0 - thiz->ifyyp;
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

	while (d < e)
	{
		unsigned int q0 = 0;

		if ((abs(xx - xx0) <= rr1_x) && (abs(yy - yy0) <= rr1_y))
		{
			unsigned int op0 = *d, p0;
			int a = 256;

			if (ocolor != 0xffffffff)
				op0 = argb8888_mul4_sym(op0, ocolor);

			if (((abs(xx - xx0) >= (rr0_x / 2))) || ((abs(yy - yy0) >= (rr0_y
					/ 2))))
			{
				int rr = hypot(xx - fxxp, yy - fyyp) + hypot(xx - fxxm, yy
						- fyym);

				a = 0;
				if (rr < cc1)
				{
					a = 256;
					if (rr > cc0)
						a -= ((rr - cc0) >> 8);
				}
			}

			if (a < 256)
				op0 = argb8888_mul_256(a, op0);

			p0 = op0;
			if (do_inner && (abs(xx - xx0) <= irr1_x) && (abs(yy - yy0)
					<= irr1_y))
			{
				p0 = icolor;
				a = 256;
				if (((abs(xx - xx0) >= (irr0_x / 2))) || ((abs(yy - yy0)
						>= (irr0_y / 2))))
				{
					int rr = hypot(xx - ifxxp, yy - ifyyp) + hypot(xx - ifxxm,
							yy - ifyym);

					a = 0;
					if (rr < icc1)
					{
						a = 256;
						if (rr > icc0)
							a -= ((rr - icc0) >> 8);
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
	Enesim_Renderer_Ellipse *thiz = _ellipse_get(r);
	Enesim_Renderer *fpaint, *spaint;
	Enesim_Renderer_Sw_Data *sdata;
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Color color;
	Enesim_Color ocolor;
	Enesim_Color icolor;
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	int xx0 = thiz->xx0, yy0 = thiz->yy0;
	int rr0_x = thiz->rr0_x, rr1_x = rr0_x + 65536;
	int rr0_y = thiz->rr0_y, rr1_y = rr0_y + 65536;
	int irr0_x = thiz->irr0_x, irr1_x = irr0_x + 65536;
	int irr0_y = thiz->irr0_y, irr1_y = irr0_y + 65536;
	int cc0 = thiz->cc0, cc1 = cc0 + 65536;
	int icc0 = thiz->icc0, icc1 = icc0 + 65536;
	int fxxp = xx0 + thiz->fxxp, fyyp = yy0 + thiz->fyyp;
	int fxxm = xx0 - thiz->fxxp, fyym = yy0 - thiz->fyyp;
	int ifxxp = xx0 + thiz->ifxxp, ifyyp = yy0 + thiz->ifyyp;
	int ifxxm = xx0 - thiz->ifxxp, ifyym = yy0 - thiz->ifyyp;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	unsigned int *sbuf, *s;
	int xx, yy;

	enesim_renderer_shape_stroke_color_get(r, &ocolor);
	enesim_renderer_shape_stroke_renderer_get(r, &spaint);
	enesim_renderer_shape_fill_color_get(r, &icolor);
	enesim_renderer_shape_fill_renderer_get(r, &fpaint);
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);

	enesim_renderer_color_get(r, &color);
	if (color != 0xffffffff)
	{
		ocolor = argb8888_mul4_sym(color, ocolor);
		icolor = argb8888_mul4_sym(color, icolor);
	}

	sdata = enesim_renderer_backend_data_get(fpaint, ENESIM_BACKEND_SOFTWARE);
	sdata->fill(fpaint, x, y, len, dst);

	sbuf = alloca(len * sizeof(unsigned int));
	sdata = enesim_renderer_backend_data_get(spaint, ENESIM_BACKEND_SOFTWARE);
	sdata->fill(spaint, x, y, len, sbuf);
	s = sbuf;

	enesim_renderer_affine_setup(r, x, y, &thiz->matrix, &xx, &yy);

	while (d < e)
	{
		unsigned int q0 = 0;

		if ((abs(xx - xx0) <= rr1_x) && (abs(yy - yy0) <= rr1_y))
		{
			unsigned int op0 = *s, p0;
			int a = 256;

			if (ocolor != 0xffffffff)
				op0 = argb8888_mul4_sym(op0, ocolor);

			if (((abs(xx - xx0) >= (rr0_x / 2))) || ((abs(yy - yy0) >= (rr0_y
					/ 2))))
			{
				int rr = hypot(xx - fxxp, yy - fyyp) + hypot(xx - fxxm, yy
						- fyym);

				a = 0;
				if (rr < cc1)
				{
					a = 256;
					if (rr > cc0)
						a -= ((rr - cc0) >> 8);
				}
			}

			if (a < 256)
				op0 = argb8888_mul_256(a, op0);

			p0 = op0;
			if ((abs(xx - xx0) <= irr1_x) && (abs(yy - yy0) <= irr1_y))
			{
				p0 = *d;
				if (icolor != 0xffffffff)
					p0 = argb8888_mul4_sym(icolor, p0);
				a = 256;
				if (((abs(xx - xx0) >= (irr0_x / 2))) || ((abs(yy - yy0)
						>= (irr0_y / 2))))
				{
					int rr = hypot(xx - ifxxp, yy - ifyyp) + hypot(xx - ifxxm,
							yy - ifyym);

					a = 0;
					if (rr < icc1)
					{
						a = 256;
						if (rr > icc0)
							a -= ((rr - icc0) >> 8);
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


static void _stroke_fill_paint_proj(Enesim_Renderer *r, int x, int y,
		unsigned int len, void  *ddata)
{
	Enesim_Renderer_Ellipse *thiz = _ellipse_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Renderer *fpaint;
	Enesim_Color color;
	Enesim_Color ocolor;
	Enesim_Color icolor;
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	int azx = thiz->matrix.zx;
	int do_inner = thiz->do_inner;
	int xx0 = thiz->xx0, yy0 = thiz->yy0;
	int rr0_x = thiz->rr0_x, rr1_x = rr0_x + 65536;
	int rr0_y = thiz->rr0_y, rr1_y = rr0_y + 65536;
	int irr0_x = thiz->irr0_x, irr1_x = irr0_x + 65536;
	int irr0_y = thiz->irr0_y, irr1_y = irr0_y + 65536;
	int cc0 = thiz->cc0, cc1 = cc0 + 65536;
	int icc0 = thiz->icc0, icc1 = icc0 + 65536;
	int fxxp = xx0 + thiz->fxxp, fyyp = yy0 + thiz->fyyp;
	int fxxm = xx0 - thiz->fxxp, fyym = yy0 - thiz->fyyp;
	int ifxxp = xx0 + thiz->ifxxp, ifyyp = yy0 + thiz->ifyyp;
	int ifxxm = xx0 - thiz->ifxxp, ifyym = yy0 - thiz->ifyyp;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	int xx, yy, zz;
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
		if (rr0_x == irr0_x)  // if stroke weight 0 or too small radii
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
	if ((draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL) && do_inner && fpaint)
	{
		Enesim_Renderer_Sw_Data *sdata;

		sdata = enesim_renderer_backend_data_get(fpaint, ENESIM_BACKEND_SOFTWARE);
		sdata->fill(fpaint, x, y, len, dst);
	}

	enesim_renderer_projective_setup(r, x, y, &thiz->matrix, &xx, &yy, &zz);

	while (d < e)
	{
		unsigned int q0 = 0;

		if (zz)
		{
			int sxx = ((((long long int)xx) << 16) / zz);
			int syy = ((((long long int)yy) << 16) / zz);

			if ((abs(sxx - xx0) <= rr1_x) && (abs(syy - yy0) <= rr1_y))
			{
				unsigned int op0 = ocolor, p0;
				int a = 256;

				if (fill_only && fpaint)
				{
					op0 = *d;
					if (ocolor != 0xffffffff)
						op0 = argb8888_mul4_sym(op0, ocolor);
				}

				if (((abs(sxx - xx0) >= (rr0_x / 2))) || ((abs(syy - yy0) >= (rr0_y / 2))))
				{
					int rr = hypot(sxx - fxxp, syy - fyyp) + hypot(sxx - fxxm, syy - fyym);

					a = 0;
					if (rr < cc1)
					{
						a = 256;
						if (rr > cc0)
							a -= ((rr - cc0) >> 8);
					}
				}

				if (a < 256)
					op0 = argb8888_mul_256(a, op0);

				p0 = op0;
				if (do_inner && (abs(sxx - xx0) <= irr1_x) && (abs(syy - yy0) <= irr1_y))
				{
					p0 = icolor;
					if (fpaint)
					{
						p0 = *d;
						if (icolor != 0xffffffff)
							p0 = argb8888_mul4_sym(icolor, p0);
					}

					a = 256;
					if (((abs(sxx - xx0) >= (irr0_x / 2))) || ((abs(syy - yy0) >= (irr0_y / 2))))
					{
						int rr = hypot(sxx - ifxxp, syy - ifyyp) + hypot(sxx - ifxxm, syy - ifyym);

						a = 0;
						if (rr < icc1)
						{
							a = 256;
							if (rr > icc0)
								a -= ((rr - icc0) >> 8);
						}
					}

					if (a < 256)
						p0 = argb8888_interp_256(a, p0, op0);
				}
				q0 = p0;
			}
		}
		*d++ = q0;
		xx += axx;
		yy += ayx;
		zz += azx;
	}
}

static void _stroke_paint_fill_proj(Enesim_Renderer *r, int x, int y,
		unsigned int len, void  *ddata)
{
	Enesim_Renderer_Ellipse *thiz = _ellipse_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Renderer *fpaint;
	Enesim_Renderer_Sw_Data *sdata;
	Enesim_Color color;
	Enesim_Color ocolor;
	Enesim_Color icolor;
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	int azx = thiz->matrix.zx;
	int do_inner = thiz->do_inner;
	int xx0 = thiz->xx0, yy0 = thiz->yy0;
	int rr0_x = thiz->rr0_x, rr1_x = rr0_x + 65536;
	int rr0_y = thiz->rr0_y, rr1_y = rr0_y + 65536;
	int irr0_x = thiz->irr0_x, irr1_x = irr0_x + 65536;
	int irr0_y = thiz->irr0_y, irr1_y = irr0_y + 65536;
	int cc0 = thiz->cc0, cc1 = cc0 + 65536;
	int icc0 = thiz->icc0, icc1 = icc0 + 65536;
	int fxxp = xx0 + thiz->fxxp, fyyp = yy0 + thiz->fyyp;
	int fxxm = xx0 - thiz->fxxp, fyym = yy0 - thiz->fyyp;
	int ifxxp = xx0 + thiz->ifxxp, ifyyp = yy0 + thiz->ifyyp;
	int ifxxm = xx0 - thiz->ifxxp, ifyym = yy0 - thiz->ifyyp;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	int xx, yy, zz;

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

	sdata = enesim_renderer_backend_data_get(fpaint, ENESIM_BACKEND_SOFTWARE);
	sdata->fill(fpaint, x, y, len, dst);

	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)
		icolor = 0;

	enesim_renderer_projective_setup(r, x, y, &thiz->matrix, &xx, &yy, &zz);

	while (d < e)
	{
		unsigned int q0 = 0;

		if (zz)
		{
			int sxx = ((((long long int)xx) << 16) / zz);
			int syy = ((((long long int)yy) << 16) / zz);

			if ((abs(sxx - xx0) <= rr1_x) && (abs(syy - yy0) <= rr1_y))
			{
				unsigned int op0 = *d, p0;
				int a = 256;

				if (ocolor != 0xffffffff)
					op0 = argb8888_mul4_sym(op0, ocolor);

				if (((abs(sxx - xx0) >= (rr0_x / 2))) || ((abs(syy - yy0) >= (rr0_y / 2))))
				{
					int rr = hypot(sxx - fxxp, syy - fyyp) + hypot(sxx - fxxm, syy - fyym);

					a = 0;
					if (rr < cc1)
					{
						a = 256;
						if (rr > cc0)
							a -= ((rr - cc0) >> 8);
					}
				}

				if (a < 256)
					op0 = argb8888_mul_256(a, op0);

				p0 = op0;
				if (do_inner && (abs(sxx - xx0) <= irr1_x) && (abs(syy - yy0) <= irr1_y))
				{
					p0 = icolor;
					if (fpaint)
					{
						p0 = *d;
						if (icolor != 0xffffffff)
							p0 = argb8888_mul4_sym(icolor, p0);
					}

					a = 256;
					if (((abs(sxx - xx0) >= (irr0_x / 2))) || ((abs(syy - yy0) >= (irr0_y / 2))))
					{
						int rr = hypot(sxx - ifxxp, syy - ifyyp) + hypot(sxx - ifxxm, syy - ifyym);

						a = 0;
						if (rr < icc1)
						{
							a = 256;
							if (rr > icc0)
								a -= ((rr - icc0) >> 8);
						}
					}

					if (a < 256)
						p0 = argb8888_interp_256(a, p0, op0);
				}
				q0 = p0;
			}
		}
		*d++ = q0;
		xx += axx;
		yy += ayx;
		zz += azx;
	}
}

static void _stroke_paint_fill_paint_proj(Enesim_Renderer *r, int x, int y,
		unsigned int len, void  *ddata)
{
	Enesim_Renderer_Ellipse *thiz = _ellipse_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Renderer *fpaint, *spaint;
	Enesim_Renderer_Sw_Data *sdata;
	Enesim_Color color;
	Enesim_Color ocolor;
	Enesim_Color icolor;
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	int azx = thiz->matrix.zx;
	int xx0 = thiz->xx0, yy0 = thiz->yy0;
	int rr0_x = thiz->rr0_x, rr1_x = rr0_x + 65536;
	int rr0_y = thiz->rr0_y, rr1_y = rr0_y + 65536;
	int irr0_x = thiz->irr0_x, irr1_x = irr0_x + 65536;
	int irr0_y = thiz->irr0_y, irr1_y = irr0_y + 65536;
	int cc0 = thiz->cc0, cc1 = cc0 + 65536;
	int icc0 = thiz->icc0, icc1 = icc0 + 65536;
	int fxxp = xx0 + thiz->fxxp, fyyp = yy0 + thiz->fyyp;
	int fxxm = xx0 - thiz->fxxp, fyym = yy0 - thiz->fyyp;
	int ifxxp = xx0 + thiz->ifxxp, ifyyp = yy0 + thiz->ifyyp;
	int ifxxm = xx0 - thiz->ifxxp, ifyym = yy0 - thiz->ifyyp;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	unsigned int *sbuf, *s;
	int xx, yy, zz;

	enesim_renderer_shape_stroke_color_get(r, &ocolor);
	enesim_renderer_shape_fill_renderer_get(r, &spaint);
	enesim_renderer_shape_fill_color_get(r, &icolor);
	enesim_renderer_shape_fill_renderer_get(r, &fpaint);
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);

	enesim_renderer_color_get(r, &color);
	if (color != 0xffffffff)
	{
		ocolor = argb8888_mul4_sym(color, ocolor);
		icolor = argb8888_mul4_sym(color, icolor);
	}

	sdata = enesim_renderer_backend_data_get(fpaint, ENESIM_BACKEND_SOFTWARE);
	sdata->fill(fpaint, x, y, len, dst);

	sbuf = alloca(len * sizeof(unsigned int));
	sdata = enesim_renderer_backend_data_get(spaint, ENESIM_BACKEND_SOFTWARE);
	sdata->fill(spaint, x, y, len, sbuf);
	s = sbuf;

	enesim_renderer_projective_setup(r, x, y, &thiz->matrix, &xx, &yy, &zz);

	while (d < e)
	{
		unsigned int q0 = 0;

		if (zz)
		{
			int sxx = ((((long long int)xx) << 16) / zz);
			int syy = ((((long long int)yy) << 16) / zz);

			if ((abs(sxx - xx0) <= rr1_x) && (abs(syy - yy0) <= rr1_y))
			{
				unsigned int op0 = *s, p0;
				int a = 256;

				if (ocolor != 0xffffffff)
					op0 = argb8888_mul4_sym(op0, ocolor);

				if (((abs(sxx - xx0) >= (rr0_x / 2))) || ((abs(syy - yy0) >= (rr0_y / 2))))
				{
					int rr = hypot(sxx - fxxp, syy - fyyp) + hypot(sxx - fxxm, syy - fyym);

					a = 0;
					if (rr < cc1)
					{
						a = 256;
						if (rr > cc0)
							a -= ((rr - cc0) >> 8);
					}
				}

				if (a < 256)
					op0 = argb8888_mul_256(a, op0);

				p0 = op0;
				if ((abs(sxx - xx0) <= irr1_x) && (abs(syy - yy0) <= irr1_y))
				{
					p0 = *d;
					if (icolor != 0xffffffff)
						p0 = argb8888_mul4_sym(icolor, p0);

					a = 256;
					if (((abs(sxx - xx0) >= (irr0_x / 2))) || ((abs(syy - yy0) >= (irr0_y / 2))))
					{
						int rr = hypot(sxx - ifxxp, syy - ifyyp) + hypot(sxx - ifxxm, syy - ifyym);

						a = 0;
						if (rr < icc1)
						{
							a = 256;
							if (rr > icc0)
								a -= ((rr - icc0) >> 8);
						}
					}

					if (a < 256)
						p0 = argb8888_interp_256(a, p0, op0);
				}
				q0 = p0;
			}
		}
		*d++ = q0;
		s++;
		xx += axx;
		yy += ayx;
		zz += azx;
	}
}

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _ellipse_name(Enesim_Renderer *r)
{
	return "ellipse";
}

static Eina_Bool _state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Ellipse *thiz;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	const Enesim_Renderer_Shape_State *css = sstates[ENESIM_STATE_CURRENT];
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Renderer *spaint;
	double rx, ry;
	double sw;

	thiz = _ellipse_get(r);
	if (!thiz || (thiz->current.rx < 1) || (thiz->current.ry < 1))
		return EINA_FALSE;

	if (_ellipse_use_path(cs->geometry_transformation_type))
	{
		_ellipse_path_setup(thiz, thiz->current.x, thiz->current.y, thiz->current.rx, thiz->current.ry, states, sstates);
		if (!enesim_renderer_setup(thiz->path, s, error))
		{
			return EINA_FALSE;
		}
		thiz->fill = enesim_renderer_sw_fill_get(thiz->path);
		*fill = _ellipse_path_span;

		return EINA_TRUE;
	}
	else
	{

		thiz->rr0_x = 65536 * (thiz->current.rx - 1);
		thiz->rr0_y = 65536 * (thiz->current.ry - 1);
		thiz->xx0 = 65536 * (thiz->current.x - 0.5);
		thiz->yy0 = 65536 * (thiz->current.y - 0.5);

		rx = thiz->current.rx - 1;
		ry = thiz->current.ry - 1;
		if (rx > ry)
		{
			thiz->fxxp = 65536 * sqrt(fabs((rx * rx) - (ry * ry)));
			thiz->fyyp = 0;
			thiz->cc0 = 2 * thiz->rr0_x;
		} else
		{
			thiz->fxxp = 0;
			thiz->fyyp = 65536 * sqrt(fabs((ry * ry) - (rx * rx)));
			thiz->cc0 = 2 * thiz->rr0_y;
		}
		enesim_renderer_shape_stroke_weight_get(r, &sw);
		thiz->do_inner = 1;
		if ((sw >= (thiz->current.rx - 1)) || (sw >= (thiz->current.ry - 1)))
		{
			sw = 0;
			thiz->do_inner = 0;
		}
		rx = thiz->current.rx - 1 - sw;
		if (rx < 0.0039)
			rx = 0;
		thiz->irr0_x = rx * 65536;
		ry = thiz->current.ry - 1 - sw;
		if (ry < 0.0039)
			ry = 0;
		thiz->irr0_y = ry * 65536;

		if (rx > ry)
		{
			thiz->ifxxp = 65536 * sqrt(fabs((rx * rx) - (ry * ry)));
			thiz->ifyyp = 0;
			thiz->icc0 = 2 * thiz->irr0_x;
		}
		else
		{
			thiz->ifxxp = 0;
			thiz->ifyyp = 65536 * sqrt(fabs((ry * ry) - (rx * rx)));
			thiz->icc0 = 2 * thiz->irr0_y;
		}

		if (!enesim_renderer_shape_setup(r, states, s, error))
			return EINA_FALSE;

		enesim_matrix_f16p16_matrix_to(&cs->transformation,
				&thiz->matrix);

		draw_mode = css->draw_mode;
		enesim_renderer_shape_stroke_renderer_get(r, &spaint);

		if (cs->transformation_type == ENESIM_MATRIX_AFFINE ||
			 cs->transformation_type == ENESIM_MATRIX_IDENTITY)
		{
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
		}
		else
		{
			*fill = _stroke_fill_paint_proj;
			if ((sw != 0.0) && spaint && (draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE))
			{
				Enesim_Renderer *fpaint;

				*fill = _stroke_paint_fill_proj;
				enesim_renderer_shape_fill_renderer_get(r, &fpaint);
				if (fpaint && thiz->do_inner &&
						(draw_mode & ENESIM_SHAPE_DRAW_MODE_FILL))
					*fill = _stroke_paint_fill_paint_proj;
			}
		}

		return EINA_TRUE;
	}
}

static void _state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = _ellipse_get(r);
	enesim_renderer_shape_cleanup(r, s);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

static void _ellipse_boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = _ellipse_get(r);
	rect->x = thiz->current.x - thiz->current.rx;
	rect->y = thiz->current.y - thiz->current.ry;
	rect->w = thiz->current.rx * 2;
	rect->h = thiz->current.ry * 2;
}

static Eina_Bool _ellipse_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = _ellipse_get(r);
	if (!thiz->changed) return EINA_FALSE;
	/* the rx */
	if (thiz->current.rx != thiz->past.rx)
		return EINA_TRUE;
	/* the ry */
	if (thiz->current.ry != thiz->past.ry)
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
	Enesim_Renderer_Ellipse *thiz;

	thiz = _ellipse_get(r);
	if (thiz->path)
		enesim_renderer_unref(thiz->path);
	free(thiz);
}

static void _ellipse_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = _ellipse_get(r);
	if (!thiz)
	{
		*flags = 0;
		return;
	}

	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_PROJECTIVE |
			ENESIM_RENDERER_FLAG_ARGB8888 |
			ENESIM_RENDERER_FLAG_GEOMETRY |
			ENESIM_RENDERER_FLAG_COLORIZE |
			ENESIM_SHAPE_FLAG_FILL_RENDERER |
			ENESIM_SHAPE_FLAG_STROKE_RENDERER;
}

static Enesim_Renderer_Shape_Descriptor _ellipse_descriptor = {
	/* .name = 			*/ _ellipse_name,
	/* .free = 			*/ _free,
	/* .boundings =  		*/ _ellipse_boundings,
	/* .destination_boundings = 	*/ NULL,
	/* .flags = 			*/ _ellipse_flags,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _ellipse_has_changed,
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
EAPI Enesim_Renderer * enesim_renderer_ellipse_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Ellipse *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Ellipse));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_ELLIPSE_MAGIC);
	r = enesim_renderer_shape_new(&_ellipse_descriptor, thiz);
	/* to maintain compatibility */
	enesim_renderer_shape_stroke_location_set(r, ENESIM_SHAPE_STROKE_INSIDE);
	return r;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_ellipse_center_set(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = _ellipse_get(r);
	if (!thiz) return;
	if ((thiz->current.x == x) && (thiz->current.y == y))
		return;
	thiz->current.x = x;
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_ellipse_center_get(Enesim_Renderer *r, double *x, double *y)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = _ellipse_get(r);
	if (x) *x = thiz->current.x;
	if (y) *y = thiz->current.y;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_ellipse_radii_set(Enesim_Renderer *r, double radius_x, double radius_y)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = _ellipse_get(r);
	if (!thiz) return;
	if (radius_x < 0.9999999)
		radius_x = 1;
	if (radius_y < 0.9999999)
		radius_y = 1;
	if ((thiz->current.rx == radius_x) && (thiz->current.ry == radius_y))
		return;
	thiz->current.rx = radius_x;
	thiz->current.ry = radius_y;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_ellipse_radii_get(Enesim_Renderer *r, double *radius_x, double *radius_y)
{
	Enesim_Renderer_Ellipse *thiz;

	thiz = _ellipse_get(r);
	if (radius_x) *radius_x = thiz->current.rx;
	if (radius_y) *radius_y = thiz->current.ry;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_ellipse_x_set(Enesim_Renderer *r, double x)
{
	Enesim_Renderer_Ellipse *thiz;
	thiz = _ellipse_get(r);
	thiz->current.x = x;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_ellipse_y_set(Enesim_Renderer *r, double y)
{
	Enesim_Renderer_Ellipse *thiz;
	thiz = _ellipse_get(r);
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_ellipse_x_radius_set(Enesim_Renderer *r, double rx)
{
	Enesim_Renderer_Ellipse *thiz;
	thiz = _ellipse_get(r);
	thiz->current.rx = rx;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_ellipse_y_radius_set(Enesim_Renderer *r, double ry)
{
	Enesim_Renderer_Ellipse *thiz;
	thiz = _ellipse_get(r);
	thiz->current.ry = ry;
	thiz->changed = EINA_TRUE;
}

