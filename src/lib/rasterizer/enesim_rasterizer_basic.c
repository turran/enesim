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

#if 0
static void figure_stroke_fill_paint_affine_simple(Enesim_Renderer *r, int x,
		int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Figure *thiz = _figure_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *fpaint;
	int stroke = 0;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Enesim_Renderer_Figure_Edge *edges, *edge;
	Enesim_Renderer_Figure_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	double ox, oy;

	int axx = thiz->matrix.xx, axy = thiz->matrix.xy, axz =
			thiz->matrix.xz;
	int ayy = thiz->matrix.yy, ayz = thiz->matrix.yz;
	int xx = (axx * x) + (axx >> 1) + (axy * y) + (axy >> 1) + axz - 32768;
	int yy = (ayy * y) + (ayy >> 1) + ayz - 32768;

	enesim_renderer_shape_stroke_color_get(r, &scolor);
	enesim_renderer_shape_fill_color_get(r, &fcolor);
 	enesim_renderer_shape_fill_renderer_get(r, &fpaint);
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);
	enesim_renderer_origin_get(r, &ox, &oy);

	xx -= eina_f16p16_double_from(ox);
	yy -= eina_f16p16_double_from(oy);

	if ((((yy >> 16) + 1) < (thiz->tyy >> 16)) ||
			((yy >> 16) > (1 + (thiz->byy >> 16))))
	{
get_out:
		while (d < e)
			*d++ = 0;
		return;
	}

	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL)
	{
		scolor = fcolor;
		stroke = 0;
		if (fpaint)
		{
			Enesim_Renderer_Sw_Data *sdata;

			sdata = enesim_renderer_backend_data_get(fpaint, ENESIM_BACKEND_SOFTWARE);
			sdata->fill(fpaint, x, y, len, dst);
		}
	}
	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL)
	{
		stroke = 1;
		if (fpaint)
		{
			Enesim_Renderer_Sw_Data *sdata;

			sdata = enesim_renderer_backend_data_get(fpaint, ENESIM_BACKEND_SOFTWARE);
			sdata->fill(fpaint, x, y, len, dst);
		}
	}
	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)
	{
		fcolor = 0;
		fpaint = NULL;
		stroke = 1;
	}

	edges = alloca(nvectors * sizeof(Enesim_Renderer_Figure_Edge));
	edge = edges;
	while (n < nvectors)
	{
		int xx0 = v->xx0, xx1 = v->xx1;
		int yy0 = v->yy0, yy1 = v->yy1;

		if (xx1 < xx0)
		{
			xx0 = xx1;
			xx1 = v->xx0;
		}
		if (yy1 < yy0)
		{
			yy0 = yy1;
			yy1 = v->yy0;
		}
		if ((((yy + 0xffff)) >= (yy0)) && ((yy) <= ((yy1 + 0xffff))))
		{
			edge->xx0 = xx0;
			edge->xx1 = xx1;
			edge->yy0 = yy0;
			edge->yy1 = yy1;
			edge->de = (v->a * (long long int) axx) >> 16;
			edge->e = ((v->a * (long long int) xx) >> 16) +
					((v->b * (long long int) yy) >> 16) +
					v->c;
			edge++;
			nedges++;
		}
		n++;
		v++;
	}
	if (!nedges)
		goto get_out;

	while (d < e)
	{
		unsigned int p0 = 0;
		int count = 0;
		int a = 0;

		n = 0;
		edge = edges;
		while (n < nedges)
		{
			int ee = edge->e;

			if ((yy >= edge->yy0) && (yy < edge->yy1))
			{
				count++;
				if (ee < 0)
					count -= 2;
			}
			if (ee < 0)
				ee = -ee;

			if ((ee < 65536) && ((xx + 0xffff) >= edge->xx0) && (xx
					<= (0xffff + edge->xx1)))
			{
				if (a < 16384)
					a = 65536 - ee;
				else
					a = (a + (65536 - ee)) / 2;
			}

			edge->e += edge->de;
			edge++;
			n++;
		}

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
		xx += axx;
	}
}

static void figure_stroke_fill_paint_affine(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Figure *thiz = _figure_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *fpaint;
	int stroke = 0;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Enesim_Renderer_Figure_Edge *edges, *edge;
	Enesim_Renderer_Figure_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0, nedges = 0;
	int y0, y1;

	int axx = thiz->matrix.xx, axy = thiz->matrix.xy, axz =
			thiz->matrix.xz;
	int ayx = thiz->matrix.yx, ayy = thiz->matrix.yy, ayz =
			thiz->matrix.yz;
	int xx = (axx * x) + (axx >> 1) + (axy * y) + (axy >> 1) + axz - 32768;
	int yy = (ayx * x) + (ayx >> 1) + (ayy * y) + (ayy >> 1) + ayz - 32768;

	enesim_renderer_shape_stroke_color_get(r, &scolor);
	enesim_renderer_shape_fill_color_get(r, &fcolor);
 	enesim_renderer_shape_fill_renderer_get(r, &fpaint);
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);

	if (((ayx <= 0) && ((yy >> 16) + 1 < (thiz->tyy >> 16))) || ((ayx >= 0)
			&& ((yy >> 16) > 1 + (thiz->byy >> 16))))
	{
		while (d < e)
			*d++ = 0;
		return;
	}

	len--;
	y0 = yy >> 16;
	y1 = yy + (len * ayx);
	y1 = y1 >> 16;
	if (y1 < y0)
	{
		y0 = y1;
		y1 = yy >> 16;
	}
	edges = alloca(nvectors * sizeof(Enesim_Renderer_Figure_Edge));
	edge = edges;
	while (n < nvectors)
	{
		int xx0, yy0;
		int xx1, yy1;

		xx0 = v->xx0;
		xx1 = v->xx1;
		if (xx1 < xx0)
		{
			xx0 = xx1;
			xx1 = v->xx0;
		}
		yy0 = v->yy0;
		yy1 = v->yy1;
		if (yy1 < yy0)
		{
			yy0 = yy1;
			yy1 = v->yy0;
		}
		if ((y0 <= (yy1 >> 16)) && (y1 >= (yy0 >> 16)))
		{
			edge->xx0 = xx0;
			edge->xx1 = xx1;
			edge->yy0 = yy0;
			edge->yy1 = yy1;
			edge->de = ((v->a * (long long int) axx) >> 16) +
					((v->b * (long long int) ayx) >> 16);
			edge->e = ((v->a * (long long int) xx) >> 16) +
					((v->b * (long long int) yy) >> 16) +
					v->c;
			edge++;
			nedges++;
		}
		n++;
		v++;
	}

	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL)
	{
		scolor = fcolor;
		stroke = 0;
		if (fpaint)
		{
			Enesim_Renderer_Sw_Data *sdata;

			sdata = enesim_renderer_backend_data_get(fpaint, ENESIM_BACKEND_SOFTWARE);
			sdata->fill(fpaint, x, y, len, dst);
		}
	}
	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL)
	{
		stroke = 1;
		if (fpaint)
		{
			Enesim_Renderer_Sw_Data *sdata;

			sdata = enesim_renderer_backend_data_get(fpaint, ENESIM_BACKEND_SOFTWARE);
			sdata->fill(fpaint, x, y, len, dst);
		}
	}
	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)
	{
		fcolor = 0;
		fpaint = NULL;
		stroke = 1;
	}

	while (d < e)
	{
		unsigned int p0 = 0;
		int count = 0;
		int a = 0;

		n = 0;
		edge = edges;
		while (n < nedges)
		{
			int ee = edge->e;

			if (((yy + 0xffff) >= edge->yy0) &&
					(yy <= (edge->yy1 + 0xffff)))
			{
				if ((yy >= edge->yy0) && (yy < edge->yy1))
				{
					count++;
					if (ee < 0)
						count -= 2;
				}
				if (ee < 0)
					ee = -ee;
				if ((ee < 65536) &&
						((xx + 0xffff) >= edge->xx0) &&
						(xx <= (0xffff + edge->xx1)))
				{
					if (a < 16384)
						a = 65536 - ee;
					else
						a = (a + (65536 - ee)) / 2;
				}
			}

			edge->e += edge->de;
			edge++;
			n++;
		}

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
			p0 = MUL_A_65536(a, scolor);

		*d++ = p0;
		yy += ayx;
		xx += axx;
	}
}

static void figure_stroke_fill_paint_proj(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Figure *thiz = _figure_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Color fcolor;
	Enesim_Color scolor;
	Enesim_Renderer *fpaint;
	int stroke = 0;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Enesim_Renderer_Figure_Edge *edges, *edge;
	Enesim_Renderer_Figure_Vector *v = thiz->vectors;
	int nvectors = thiz->nvectors, n = 0;

	int axx = thiz->matrix.xx, axy = thiz->matrix.xy, axz =
			thiz->matrix.xz;
	int ayx = thiz->matrix.yx, ayy = thiz->matrix.yy, ayz =
			thiz->matrix.yz;
	int azx = thiz->matrix.zx, azy = thiz->matrix.zy, azz =
			thiz->matrix.zz;
	int xx = (axx * x) + (axx >> 1) + (axy * y) + (axy >> 1) + axz - 32768;
	int yy = (ayx * x) + (ayx >> 1) + (ayy * y) + (ayy >> 1) + ayz - 32768;
	int zz = (azx * x) + (azx >> 1) + (azy * y) + (azy >> 1) + azz;
	
	enesim_renderer_shape_stroke_color_get(r, &scolor);
	enesim_renderer_shape_fill_color_get(r, &fcolor);
 	enesim_renderer_shape_fill_renderer_get(r, &fpaint);
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);

	edges = alloca(nvectors * sizeof(Enesim_Renderer_Figure_Edge));
	edge = edges;
	while (n < nvectors)
	{
		edge->xx0 = v->xx0;
		edge->xx1 = v->xx1;
		if (edge->xx1 < edge->xx0)
		{
			edge->xx0 = edge->xx1;
			edge->xx1 = v->xx0;
		}
		edge->yy0 = v->yy0;
		edge->yy1 = v->yy1;
		if (edge->yy1 < edge->yy0)
		{
			edge->yy0 = edge->yy1;
			edge->yy1 = v->yy0;
		}
		edge->de = ((v->a * (long long int) axx) >> 16) +
				((v->b * (long long int) ayx) >> 16) +
				((v->c * (long long int) azx) >> 16);
		edge->e = ((v->a * (long long int) xx) >> 16) +
				((v->b * (long long int) yy) >> 16) +
				((v->c * (long long int) zz) >> 16);
		n++;
		v++;
		edge++;
	}

	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_FILL)
	{
		scolor = fcolor;
		stroke = 0;
		if (fpaint)
		{
			Enesim_Renderer_Sw_Data *sdata;

			sdata = enesim_renderer_backend_data_get(fpaint, ENESIM_BACKEND_SOFTWARE);
			sdata->fill(fpaint, x, y, len, dst);
		}
	}
	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL)
	{
		stroke = 1;
		if (fpaint)
		{
			Enesim_Renderer_Sw_Data *sdata;

			sdata = enesim_renderer_backend_data_get(fpaint, ENESIM_BACKEND_SOFTWARE);
			sdata->fill(fpaint, x, y, len, dst);
		}
	}
	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)
	{
		fcolor = 0;
		fpaint = NULL;
		stroke = 1;
	}

	while (d < e)
	{
		unsigned int p0 = 0;
		int count = 0;
		int a = 0;
		int sxx, syy;

		n = 0;
		edge = edges;
		if (zz)
		{
			syy = (((long long int) yy) << 16) / zz;
			sxx = (((long long int) xx) << 16) / zz;

			while (n < nvectors)
			{
				int ee = (((long long int) edge->e) << 16) / zz;

				if (((syy + 0xffff) >= edge->yy0) &&
						(syy <= (edge->yy1 + 0xffff)))
				{
					if ((syy >= edge->yy0) &&
							(syy < edge->yy1))
					{
						count++;
						if (ee < 0)
							count -= 2;
					}
					if (ee < 0)
						ee = -ee;
					if ((ee < 65536) &&
							((sxx + 0xffff) >= edge->xx0) &&
							(sxx <= (0xffff + edge->xx1)))
					{
						if (a < 16384)
							a = 65536 - ee;
						else
							a = (a + (65536 - ee)) / 2;
					}
				}
				edge->e += edge->de;
				edge++;
				n++;
			}
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
				p0 = MUL_A_65536(a, scolor);
		}
		*d++ = p0;
		xx += axx;
		yy += ayx;
		zz += azx;
	}
}
#endif
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

