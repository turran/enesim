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

/* Until we fix the new renderer */
#define NEW_RENDERER 0
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_RECTANGLE_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_RECTANGLE_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_RECTANGLE_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Rectangle_State
{
	double width;
	double height;
	double x;
	double y;
	struct {
		double radius;
		Eina_Bool tl : 1;
		Eina_Bool tr : 1;
		Eina_Bool bl : 1;
		Eina_Bool br : 1;
	} corner;
} Enesim_Renderer_Rectangle_State;

typedef struct _Enesim_Renderer_Rectangle
{
	EINA_MAGIC
	/* public properties */
	Enesim_Renderer_Rectangle_State current;
	Enesim_Renderer_Rectangle_State past;
	/* internal state */
	Eina_Bool changed : 1;
	/* for the case we use the path renderer */
	Enesim_Renderer *path;
	Enesim_Renderer_Sw_Fill fill;
	/* for our own case */
	Eina_F16p16 ww;
	Eina_F16p16 hh;
	Eina_F16p16 xx;
	Eina_F16p16 yy;
	Enesim_F16p16_Matrix matrix;
	/* the inner rectangle in case of rounded corners */
	int lxx0, rxx0;
	int tyy0, byy0;
	int rr0, irr0;
	double sw;
	unsigned char do_inner :1;
} Enesim_Renderer_Rectangle;

/* we assume tyy and lxx are inside the top left corner */
/* FIXME we should replace all the ints, we are losing precision as this renderer
 * only handled integers coords
 */
static inline void _top_left(int sx, int sy, int stw,
		uint16_t ax, uint16_t ay,
		Eina_F16p16 lxx, Eina_F16p16 tyy, Eina_F16p16 rr0, Eina_F16p16 rr1,
		uint32_t cout[4], uint16_t *ca)
{
	if ((-lxx - tyy) >= rr0)
	{
		int rr = hypot(lxx, tyy);

		*ca = 0;
		if (rr < rr1)
		{
			*ca = 256;
			if (rr > rr0)
				*ca = 256 - ((rr - rr0) >> 8);
		}
	}

	if (sx < stw)
	{
		if (cout[1] != cout[3])
			cout[1] = argb8888_interp_256(ax, cout[3], cout[1]);
		cout[0] = cout[2] = cout[3] = cout[1];
	}
	if (sy < stw)
	{
		if (cout[2] != cout[3])
			cout[2] = argb8888_interp_256(ay, cout[3], cout[2]);
		cout[0] = cout[1] = cout[3] = cout[2];
	}
}

static inline void _bottom_left(int sx, int sy, int sh, int stw,
		uint16_t ax, uint16_t ay,
		Eina_F16p16 lxx, Eina_F16p16 byy, Eina_F16p16 rr0, Eina_F16p16 rr1,
		uint32_t cout[4], uint16_t *ca)
{
	if ((-lxx + byy) >= rr0)
	{
		int rr = hypot(lxx, byy);

		*ca = 0;
		if (rr < rr1)
		{
			*ca = 256;
			if (rr > rr0)
				*ca = 256 - ((rr - rr0) >> 8);
		}
	}

	if (sx < stw)
	{
		if (cout[1] != cout[3])
			cout[1] = argb8888_interp_256(ax, cout[3], cout[1]);
		cout[0] = cout[2] = cout[3] = cout[1];
	}
	if ((sy + 1 + stw) == sh)
	{
		if (cout[0] != cout[1])
			cout[0] = argb8888_interp_256(ay, cout[1], cout[0]);
		cout[1] = cout[2] = cout[3] = cout[0];
	}
}

static inline void _left_corners(int sx, int sy, int sh, int stw,
		uint16_t ax, uint16_t ay,
		Eina_Bool tl, Eina_Bool bl,
		Eina_F16p16 lxx, Eina_F16p16 tyy, Eina_F16p16 byy,
		Eina_F16p16 rr0, Eina_F16p16 rr1,
		uint32_t cout[4], uint16_t *ca)
{
	if (lxx < 0)
	{
		if (tl && (tyy < 0))
			_top_left(sx, sy, stw, ax, ay, lxx, tyy, rr0, rr1, cout, ca);
		if (bl && (byy > 0))
			_bottom_left(sx, sy, sh, stw, ax, ay, lxx, byy, rr0, rr1, cout, ca);
	}
}

static inline void _top_right(int sx, int sy, int sw, int stw,
		uint16_t ax, uint16_t ay,
		Eina_F16p16 rxx, Eina_F16p16 tyy,
		Eina_F16p16 rr0, Eina_F16p16 rr1,
		uint32_t cout[4], uint16_t *ca)
{
	if ((rxx - tyy) >= rr0)
	{
		int rr = hypot(rxx, tyy);

		*ca = 0;
		if (rr < rr1)
		{
			*ca = 256;
			if (rr > rr0)
				*ca = 256 - ((rr - rr0) >> 8);
		}
	}

	if ((sx + 1 + stw) == sw)
	{
		if (cout[0] != cout[2])
			cout[0] = argb8888_interp_256(ax, cout[2], cout[0]);
		cout[1] = cout[2] = cout[3] = cout[0];
	}
	if (sy < stw)
	{
		if (cout[2] != cout[3])
			cout[2] = argb8888_interp_256(ay, cout[3], cout[2]);
		cout[0] = cout[1] = cout[3] = cout[2];
	}
}

static inline void _bottom_right(int sx, int sy, int sw, int sh, int stw,
		uint16_t ax, uint16_t ay,
		Eina_F16p16 rxx, Eina_F16p16 byy,
		Eina_F16p16 rr0, Eina_F16p16 rr1,
		uint32_t cout[4], uint16_t *ca)
{
	if ((rxx + byy) >= rr0)
	{
		int rr = hypot(rxx, byy);

		*ca = 0;
		if (rr < rr1)
		{
			*ca = 256;
			if (rr > rr0)
				*ca = 256 - ((rr - rr0) >> 8);
		}
	}

	if ((sx + 1 + stw) == sw)
	{
		if (cout[0] != cout[2])
			cout[0] = argb8888_interp_256(ax, cout[2], cout[0]);
		cout[1] = cout[2] = cout[3] = cout[0];
	}
	if ((sy + 1 + stw) == sh)
	{
		if (cout[0] != cout[1])
			cout[0] = argb8888_interp_256(ay, cout[1], cout[0]);
		cout[1] = cout[2] = cout[3] = cout[0];
	}
}

static inline void _right_corners(int sx, int sy, int sw, int sh, int stw,
		uint16_t ax, uint16_t ay,
		Eina_Bool tr, Eina_Bool br,
		Eina_F16p16 rxx, Eina_F16p16 tyy, Eina_F16p16 byy,
		Eina_F16p16 rr0, Eina_F16p16 rr1,
		uint32_t cout[4], uint16_t *ca)
{
	if (rxx > 0)
	{
		if (tr && (tyy < 0))
			_top_right(sx, sy, sw, stw, ax, ay, rxx, tyy, rr0, rr1, cout, ca);
		if (br && (byy > 0))
			_bottom_right(sx, sy, sw, sh, stw, ax, ay, rxx, byy, rr0, rr1, cout, ca);
	}
}

static inline void _outer_corners(int sx, int sy, int sw, int sh, uint16_t ax, uint16_t ay,
		Eina_Bool tl, Eina_Bool tr, Eina_Bool br, Eina_Bool bl,
		Eina_F16p16 lxx, Eina_F16p16 rxx, Eina_F16p16 tyy, Eina_F16p16 byy,
		Eina_F16p16 rr0, Eina_F16p16 rr1,
		uint32_t cout[4], uint16_t *ca)
{
	_left_corners(sx, sy, sh, 0, ax, ay, tl, bl, lxx, tyy, byy, rr0, rr1, cout, ca);
	_right_corners(sx, sy, sw, sh, 0, ax, ay, tr, br, rxx, tyy, byy, rr0, rr1, cout, ca);
}

static inline void _inner_corners(int sx, int sy, int sw, int sh, int stw, uint16_t ax, uint16_t ay,
		Eina_Bool tl, Eina_Bool tr, Eina_Bool br, Eina_Bool bl,
		Eina_F16p16 lxx, Eina_F16p16 rxx, Eina_F16p16 tyy, Eina_F16p16 byy,
		Eina_F16p16 rr0, Eina_F16p16 rr1,
		uint32_t cout[4], uint16_t *ca)
{
	_left_corners(sx, sy, sh, stw, ax, ay, tl, bl, lxx, tyy, byy, rr0, rr1, cout, ca);
	_right_corners(sx, sy, sw, sh, stw, ax, ay, tr, br, rxx, tyy, byy, rr0, rr1, cout, ca);
}

static inline Enesim_Renderer_Rectangle * _rectangle_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	ENESIM_RENDERER_RECTANGLE_MAGIC_CHECK(thiz);

	return thiz;
}

static Eina_Bool _rectangle_use_path(Enesim_Matrix_Type geometry_type)
{
		return EINA_TRUE;
	if (geometry_type != ENESIM_MATRIX_IDENTITY)
		return EINA_TRUE;
	return EINA_FALSE;
}

static void _rectangle_path_setup(Enesim_Renderer_Rectangle *thiz,
		double x, double y, double w, double h, double r,
		const Enesim_Renderer_State *state, const Enesim_Renderer_Shape_State *sstate)
{
	if (!thiz->path)
		thiz->path = enesim_renderer_path_new();
	/* FIXME the best approach would be to know *what* has changed
	 * because we only need to generate the vertices for x,y,w,h
	 * change
	 */
	/* FIXME or prev->geometry_transformation_type == IDENTITY && curr->geometry_transformation_type != IDENTITY */
	if (thiz->changed)
	{
		enesim_renderer_path_command_clear(thiz->path);
		/* FIXME for now handle the corners like this */
		if (thiz->current.corner.tl)
		{
			enesim_renderer_path_move_to(thiz->path, x, y + r);
			enesim_renderer_path_quadratic_to(thiz->path, x, y, x + r, y);
		}
		else
		{
			enesim_renderer_path_move_to(thiz->path, x, y);
		}
		if (thiz->current.corner.tr)
		{
			enesim_renderer_path_line_to(thiz->path, x + w - r, y);
			enesim_renderer_path_quadratic_to(thiz->path, x + w, y, x + w, y + r);
		}
		else
		{
			enesim_renderer_path_line_to(thiz->path, x + w, y);
		}
		if (thiz->current.corner.br)
		{
			enesim_renderer_path_line_to(thiz->path, x + w, y + h - r);
			enesim_renderer_path_quadratic_to(thiz->path, x + w, y + h, x + w - r, y + h);
		}
		else
		{
			enesim_renderer_path_line_to(thiz->path, x + w, y + h);
		}
		if (thiz->current.corner.bl)
		{
			enesim_renderer_path_line_to(thiz->path, x + r, y + h);
			enesim_renderer_path_quadratic_to(thiz->path, x, y + h, x, y + h - r);
		}
		else
		{
			enesim_renderer_path_line_to(thiz->path, x, y + h);
		}
		enesim_renderer_path_close(thiz->path, EINA_TRUE);
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

#if NEW_RENDERER
static uint32_t _rectangle_get_color(Eina_F16p16 xx, Eina_F16p16 yy, Eina_F16p16 ww0, Eina_F16p16 hh0,
			Enesim_Color ocolor, Enesim_Color icolor)
{
	Enesim_Color xc, yc;
	uint16_t ax = 1 + ((xx & 0xffff) >> 8);
	uint16_t ay = 1 + ((yy & 0xffff) >> 8);

	xc = icolor;
	yc = icolor;
	if (xx < 0)
		xc = argb8888_interp_256(ax, ocolor, icolor);
	else if (ww0 - xx < EINA_F16P16_ONE)
		xc = argb8888_interp_256(256 - ax, ocolor, icolor);
	if (yy < 0)
		yc = argb8888_interp_256(ay, ocolor, icolor);
	else if (hh0 - yy < EINA_F16P16_ONE)
		yc = argb8888_interp_256(256 - ay, ocolor, icolor);
	/* FIXME how to interpolate between the x and y? */
	if (xc != yc)
		xc = argb8888_interp_256(128, yc, xc);

	return xc;
}

static uint16_t _rectangle_get_alpha(Eina_F16p16 xx, Eina_F16p16 yy, Eina_F16p16 rr0, Eina_F16p16 rr1)
{
	uint16_t ca = 256;

	if ((xx + yy) >= rr0)
	{
		int rr = hypot(xx, yy);

		ca = 0;
		if (rr < rr1)
		{
			ca = 256;
			if (rr > rr0)
				ca = 256 - ((rr - rr0) >> 8);
		}
	}
	return ca;
}

static void _test_affine(Enesim_Renderer *r, int x, int y, unsigned int len,
		void *ddata)
{
	Enesim_Renderer_Rectangle *thiz = _rectangle_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Renderer *fpaint;
	Eina_F16p16 ww0 = thiz->ww;
	Eina_F16p16 hh0 = thiz->hh;
	Eina_F16p16 xx0 = thiz->xx;
	Eina_F16p16 yy0 = thiz->yy;
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	int do_inner = thiz->do_inner;
	Enesim_Color color;
	Enesim_Color ocolor;
	Enesim_Color icolor;
	int stww = (thiz->sw) * 65536;
	int stw = stww >> 16;
	int iww = ww0 - (2 * stww);
	int ihh = hh0 - (2 * stww);
	int rr0 = thiz->rr0, rr1 = rr0 + 65536;
	int irr0 = thiz->irr0, irr1 = irr0 + 65536;
	int lxx0 = thiz->lxx0, rxx0 = thiz->rxx0;
	int tyy0 = thiz->tyy0, byy0 = thiz->byy0;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	int xx, yy;
	int fill_only = 0;
	char bl = thiz->current.corner.bl, br = thiz->current.corner.br, tl = thiz->current.corner.tl, tr = thiz->current.corner.tr;

	enesim_renderer_shape_stroke_color_get(r, &ocolor);
	enesim_renderer_shape_fill_renderer_get(r, &fpaint);
	enesim_renderer_shape_fill_color_get(r, &icolor);
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);

	enesim_renderer_color_get(r, &color);
	if (color != 0xffffffff)
	{
		ocolor = argb8888_mul4_sym(color, ocolor);
		icolor = argb8888_mul4_sym(color, icolor);
	}
	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)
	{
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
	xx = eina_f16p16_sub(xx, xx0);
	yy = eina_f16p16_sub(yy, yy0);

	while (d < e)
	{
		uint32_t q0 = 0;

		if (xx < ww0 && yy < hh0 && xx > -EINA_F16P16_ONE && yy > -EINA_F16P16_ONE)
		{
			Eina_F16p16 lxx = xx - lxx0;
			Eina_F16p16 rxx = xx - rxx0;
			Eina_F16p16 tyy = yy - tyy0;
			Eina_F16p16 byy = yy - byy0;
			Eina_F16p16 ixx = xx - stww;
			Eina_F16p16 iyy = yy - stww;
			uint16_t ca = 256;
			unsigned int p0;

			p0 = _rectangle_get_color(xx, yy, ww0, hh0, 0, ocolor);
			/* top left */
			ca = _rectangle_get_alpha(-lxx, -tyy, rr0, rr1);
			/* top right */
#if 0
			if (ca == 256)
				ca = _rectangle_get_alpha(rxx, -tyy, rr0, rr1);
			/* bottom right */
			if (ca == 256)
				ca = _rectangle_get_alpha(rxx, byy, rr0, rr1);
			/* bottom left */
			if (ca == 256)
				ca = _rectangle_get_alpha(-lxx, byy, rr0, rr1);
#endif
			if (ca < 256)
				p0 = argb8888_mul_256(ca, p0);

			if (do_inner && (ixx > -EINA_F16P16_ONE)
					&& (iyy > -EINA_F16P16_ONE)
					&& (ixx < iww) && (iyy < ihh))
			{
				unsigned int op0;

				ca = 256;
				if (fpaint)
				{
					color = *d;
					if (icolor != 0xffffffff)
						color = argb8888_mul4_sym(color, icolor);
					icolor = color;
				}
				op0 = _rectangle_get_color(ixx, iyy, iww, ihh, p0, icolor);

				if (ca < 256)
					op0 = argb8888_interp_256(ca, p0, op0);
				p0 = op0;
			}
			q0 = p0;
		}
		*d++ = q0;
		xx += axx;
		yy += ayx;
	}
}

#else
/*----------------------------------------------------------------------------*
 *                               Span functions                               *
 *----------------------------------------------------------------------------*/
/* Use the internal path for drawing */
static void _rectangle_path_span(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	thiz->fill(thiz->path, x, y, len, ddata);
}

/* stroke and/or fill with possibly a fill renderer */
static void _rounded_stroke_fill_paint_affine(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Rectangle *thiz = _rectangle_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Eina_F16p16 ww0 = thiz->ww;
	Eina_F16p16 hh0 = thiz->hh;
	Eina_F16p16 xx0 = thiz->xx;
	Eina_F16p16 yy0 = thiz->yy;
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	int do_inner = thiz->do_inner;
	Enesim_Color color;
	unsigned int ocolor;
	unsigned int icolor;
	int stww = (thiz->sw) * 65536;
	int stw = stww >> 16;
	int iww = ww0 - (2 * stww);
	int ihh = hh0 - (2 * stww);
	int rr0 = thiz->rr0, rr1 = rr0 + 65536;
	int irr0 = thiz->irr0, irr1 = irr0 + 65536;
	int lxx0 = thiz->lxx0, rxx0 = thiz->rxx0;
	int tyy0 = thiz->tyy0, byy0 = thiz->byy0;
	Enesim_Renderer *fpaint;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	int xx, yy;
	int fill_only = 0;
	char bl = thiz->current.corner.bl, br = thiz->current.corner.br, tl = thiz->current.corner.tl, tr = thiz->current.corner.tr;

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
	xx = eina_f16p16_sub(xx, xx0);
	yy = eina_f16p16_sub(yy, yy0);

	while (d < e)
	{
		unsigned int q0 = 0;

		if (xx < ww0 && yy < hh0 && xx > -EINA_F16P16_ONE && yy > -EINA_F16P16_ONE)
		{
			int sx = xx >> 16;
			int sy = yy >> 16;
			int sw = ww0 >> 16;
			int sh = hh0 >> 16;
			uint16_t ca = 256;
			unsigned int op3 = 0, op2 = 0, op1 = 0, op0 = 0, p0;
			int ax = 1 + ((xx & 0xffff) >> 8);
			int ay = 1 + ((yy & 0xffff) >> 8);

			int lxx = xx - lxx0, rxx = xx - rxx0;
			int tyy = yy - tyy0, byy = yy - byy0;

			int ixx = xx - stww, iyy = yy - stww;
			int ix = ixx >> 16, iy = iyy >> 16;

			if (fill_only && fpaint)
			{
				ocolor = *d;
				if (icolor != 0xffffffff)
					ocolor = argb8888_mul4_sym(icolor, ocolor);
			}

			if ((ww0 - xx) < 65536)
				ax = 256 - ((ww0 - xx) >> 8);
			if ((hh0 - yy) < 65536)
				ay = 256 - ((hh0 - yy) >> 8);


			if ((sx > -1) && (sy > -1))
				op0 = ocolor;
			if ((sy > -1) && ((xx + 65536) < ww0))
				op1 = ocolor;
			if ((yy + 65536) < hh0)
			{
				if (sx > -1)
					op2 = ocolor;
				if ((xx + 65536) < ww0)
					op3 = ocolor;
			}
			{
				uint32_t cout[4] = { op0, op1, op2, op3 };
				_outer_corners(sx, sy, sw, sh, ax, ay, tl, tr, br, bl,
						lxx, rxx, tyy, byy, rr0, rr1, cout, &ca);


				op0 = cout[0];
				op1 = cout[1];
				op2 = cout[2];
				op3 = cout[3];
			}

			if (op0 != op1)
				op0 = argb8888_interp_256(ax, op1, op0);
			if (op2 != op3)
				op2 = argb8888_interp_256(ax, op3, op2);
			if (op0 != op2)
				op0 = argb8888_interp_256(ay, op2, op0);

			if (ca < 256)
				op0 = argb8888_mul_256(ca, op0);

			p0 = op0;

			if ( do_inner && (ixx > -65536) && (iyy > -65536) &&
				(ixx < iww) && (iyy < ihh) )
			{
				unsigned int p3 = p0, p2 = p0, p1 = p0;

				color = icolor;
				if (fpaint)
				{
					color = *d;
					if (icolor != 0xffffffff)
						color = argb8888_mul4_sym(icolor, color);
				}

				ca = 256;
				ax = 1 + ((ixx & 0xffff) >> 8);
				ay = 1 + ((iyy & 0xffff) >> 8);
				if ((iww - ixx) < 65536)
					ax = 256 - ((iww - ixx) >> 8);
				if ((ihh - iyy) < 65536)
					ay = 256 - ((ihh - iyy) >> 8);

				if ((ix > -1) & (iy > -1))
					p0 = color;
				if ((iy > -1) && ((ixx + 65536) < iww))
					p1 = color;
				if ((iyy + 65536) < ihh)
				{
					if (ix > -1)
						p2 = color;
					if ((ixx + 65536) < iww)
						p3 = color;
				}
				{
					uint32_t cout[4] = { p0, p1, p2, p3 };
					_inner_corners(sx, sy, sw, sh, stw, ax, ay, tl, tr, br, bl,
							lxx, rxx, tyy, byy, irr0, irr1, cout, &ca);


					p0 = cout[0];
					p1 = cout[1];
					p2 = cout[2];
					p3 = cout[3];
				}

				if (p0 != p1)
					p0 = argb8888_interp_256(ax, p1, p0);
				if (p2 != p3)
					p2 = argb8888_interp_256(ax, p3, p2);
				if (p0 != p2)
					p0 = argb8888_interp_256(ay, p2, p0);

				if (ca < 256)
					p0 = argb8888_interp_256(ca, p0, op0);
			}
			q0 = p0;
		}
		*d++ = q0;
		xx += axx;
		yy += ayx;
	}
}

/* stroke with a renderer and possibly fill with color */
static void _rounded_stroke_paint_fill_affine(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Rectangle *thiz = _rectangle_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Eina_F16p16 ww0 = thiz->ww;
	Eina_F16p16 hh0 = thiz->hh;
	Eina_F16p16 xx0 = thiz->xx;
	Eina_F16p16 yy0 = thiz->yy;
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	int do_inner = thiz->do_inner;
	Enesim_Color color;
	unsigned int ocolor;
	unsigned int icolor;
	int stww = (thiz->sw) * 65536;
	int stw = stww >> 16;
	int iww = ww0 - (2 * stww);
	int ihh = hh0 - (2 * stww);
	int rr0 = thiz->rr0, rr1 = rr0 + 65536;
	int irr0 = thiz->irr0, irr1 = irr0 + 65536;
	int lxx0 = thiz->lxx0, rxx0 = thiz->rxx0;
	int tyy0 = thiz->tyy0, byy0 = thiz->byy0;
	Enesim_Renderer *spaint;
	Enesim_Renderer_Sw_Data *sdata;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	int xx, yy;
	char bl = thiz->current.corner.bl, br = thiz->current.corner.br, tl = thiz->current.corner.tl, tr = thiz->current.corner.tr;

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
	xx = eina_f16p16_sub(xx, xx0);
	yy = eina_f16p16_sub(yy, yy0);

	while (d < e)
	{
		unsigned int q0 = 0;

		if (xx < ww0 && yy < hh0 && xx > -EINA_F16P16_ONE && yy > -EINA_F16P16_ONE)
		{
			int sx = xx >> 16;
			int sy = yy >> 16;
			int sw = ww0 >> 16;
			int sh = hh0 >> 16;
			uint16_t ca = 256;
			unsigned int op3 = 0, op2 = 0, op1 = 0, op0 = 0, p0;
			int ax = 1 + ((xx & 0xffff) >> 8);
			int ay = 1 + ((yy & 0xffff) >> 8);

			int lxx = xx - lxx0, rxx = xx - rxx0;
			int tyy = yy - tyy0, byy = yy - byy0;

			int ixx = xx - stww, iyy = yy - stww;
			int ix = ixx >> 16, iy = iyy >> 16;

			color = *d;
			if (ocolor != 0xffffffff)
				color = argb8888_mul4_sym(ocolor, color);

			if ((ww0 - xx) < 65536)
				ax = 256 - ((ww0 - xx) >> 8);
			if ((hh0 - yy) < 65536)
				ay = 256 - ((hh0 - yy) >> 8);


			if ((sx > -1) && (sy > -1))
				op0 = color;
			if ((sy > -1) && ((xx + 65536) < ww0))
				op1 = color;
			if ((yy + 65536) < hh0)
			{
				if (sx > -1)
					op2 = color;
				if ((xx + 65536) < ww0)
					op3 = color;
			}
			{
				uint32_t cout[4] = { op0, op1, op2, op3 };
				_outer_corners(sx, sy, sw, sh, ax, ay, tl, tr, br, bl,
						lxx, rxx, tyy, byy, rr0, rr1, cout, &ca);


				op0 = cout[0];
				op1 = cout[1];
				op2 = cout[2];
				op3 = cout[3];
			}

			if (op0 != op1)
				op0 = argb8888_interp_256(ax, op1, op0);
			if (op2 != op3)
				op2 = argb8888_interp_256(ax, op3, op2);
			if (op0 != op2)
				op0 = argb8888_interp_256(ay, op2, op0);

			if (ca < 256)
				op0 = argb8888_mul_256(ca, op0);

			p0 = op0;

			if ( do_inner && (ixx > -65536) && (iyy > -65536) &&
				(ixx < iww) && (iyy < ihh) )
			{
				unsigned int p3 = p0, p2 = p0, p1 = p0;

				color = icolor;
				ca = 256;
				ax = 1 + ((ixx & 0xffff) >> 8);
				ay = 1 + ((iyy & 0xffff) >> 8);
				if ((iww - ixx) < 65536)
					ax = 256 - ((iww - ixx) >> 8);
				if ((ihh - iyy) < 65536)
					ay = 256 - ((ihh - iyy) >> 8);

				if ((ix > -1) & (iy > -1))
					p0 = color;
				if ((iy > -1) && ((ixx + 65536) < iww))
					p1 = color;
				if ((iyy + 65536) < ihh)
				{
					if (ix > -1)
						p2 = color;
					if ((ixx + 65536) < iww)
						p3 = color;
				}
				{
					uint32_t cout[4] = { p0, p1, p2, p3 };
					_inner_corners(sx, sy, sw, sh, stw, ax, ay, tl, tr, br, bl,
							lxx, rxx, tyy, byy, irr0, irr1, cout, &ca);


					p0 = cout[0];
					p1 = cout[1];
					p2 = cout[2];
					p3 = cout[3];
				}

				if (p0 != p1)
					p0 = argb8888_interp_256(ax, p1, p0);
				if (p2 != p3)
					p2 = argb8888_interp_256(ax, p3, p2);
				if (p0 != p2)
					p0 = argb8888_interp_256(ay, p2, p0);

				if (ca < 256)
					p0 = argb8888_interp_256(ca, p0, op0);
			}
			q0 = p0;
		}
		*d++ = q0;
		xx += axx;
		yy += ayx;
	}
}

/* stroke and fill with renderers */
static void _rounded_stroke_paint_fill_paint_affine(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Rectangle *thiz = _rectangle_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Eina_F16p16 ww0 = thiz->ww;
	Eina_F16p16 hh0 = thiz->hh;
	Eina_F16p16 xx0 = thiz->xx;
	Eina_F16p16 yy0 = thiz->yy;
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	Enesim_Color color;
	unsigned int ocolor;
	unsigned int icolor;
	int stww = (thiz->sw) * 65536;
	int stw = stww >> 16;
	int iww = ww0 - (2 * stww);
	int ihh = hh0 - (2 * stww);
	int rr0 = thiz->rr0, rr1 = rr0 + 65536;
	int irr0 = thiz->irr0, irr1 = irr0 + 65536;
	int lxx0 = thiz->lxx0, rxx0 = thiz->rxx0;
	int tyy0 = thiz->tyy0, byy0 = thiz->byy0;
	Enesim_Renderer *fpaint, *spaint;
	Enesim_Renderer_Sw_Data *sdata;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	unsigned int *sbuf, *s;
	int xx, yy;
	char bl = thiz->current.corner.bl, br = thiz->current.corner.br, tl = thiz->current.corner.tl, tr = thiz->current.corner.tr;

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
	xx = eina_f16p16_sub(xx, xx0);
	yy = eina_f16p16_sub(yy, yy0);

	while (d < e)
	{
		unsigned int q0 = 0;

		if (xx < ww0 && yy < hh0 && xx > -EINA_F16P16_ONE && yy > -EINA_F16P16_ONE)
		{
			int sx = xx >> 16;
			int sy = yy >> 16;
			int sw = ww0 >> 16;
			int sh = hh0 >> 16;
			uint16_t ca = 256;
			unsigned int op3 = 0, op2 = 0, op1 = 0, op0 = 0, p0;
			int ax = 1 + ((xx & 0xffff) >> 8);
			int ay = 1 + ((yy & 0xffff) >> 8);

			int lxx = xx - lxx0, rxx = xx - rxx0;
			int tyy = yy - tyy0, byy = yy - byy0;

			int ixx = xx - stww, iyy = yy - stww;
			int ix = ixx >> 16, iy = iyy >> 16;

			color = *s;
			if (ocolor != 0xffffffff)
				color = argb8888_mul4_sym(ocolor, color);

			if ((ww0 - xx) < 65536)
				ax = 256 - ((ww0 - xx) >> 8);
			if ((hh0 - yy) < 65536)
				ay = 256 - ((hh0 - yy) >> 8);


			if ((sx > -1) && (sy > -1))
				op0 = color;
			if ((sy > -1) && ((xx + 65536) < ww0))
				op1 = color;
			if ((yy + 65536) < hh0)
			{
				if (sx > -1)
					op2 = color;
				if ((xx + 65536) < ww0)
					op3 = color;
			}
			{
				uint32_t cout[4] = { op0, op1, op2, op3 };
				_outer_corners(sx, sy, sw, sh, ax, ay, tl, tr, br, bl,
						lxx, rxx, tyy, byy, rr0, rr1, cout, &ca);


				op0 = cout[0];
				op1 = cout[1];
				op2 = cout[2];
				op3 = cout[3];
			}

			if (op0 != op1)
				op0 = argb8888_interp_256(ax, op1, op0);
			if (op2 != op3)
				op2 = argb8888_interp_256(ax, op3, op2);
			if (op0 != op2)
				op0 = argb8888_interp_256(ay, op2, op0);

			if (ca < 256)
				op0 = argb8888_mul_256(ca, op0);

			p0 = op0;

			if ((ixx > -65536) && (iyy > -65536) &&
				(ixx < iww) && (iyy < ihh))
			{
				unsigned int p3 = p0, p2 = p0, p1 = p0;

				color = *d;
				if (icolor != 0xffffffff)
					color = argb8888_mul4_sym(icolor, color);
				
				ca = 256;
				ax = 1 + ((ixx & 0xffff) >> 8);
				ay = 1 + ((iyy & 0xffff) >> 8);
				if ((iww - ixx) < 65536)
					ax = 256 - ((iww - ixx) >> 8);
				if ((ihh - iyy) < 65536)
					ay = 256 - ((ihh - iyy) >> 8);

				if ((ix > -1) & (iy > -1))
					p0 = color;
				if ((iy > -1) && ((ixx + 65536) < iww))
					p1 = color;
				if ((iyy + 65536) < ihh)
				{
					if (ix > -1)
						p2 = color;
					if ((ixx + 65536) < iww)
						p3 = color;
				}
				{
					uint32_t cout[4] = { p0, p1, p2, p3 };
					_inner_corners(sx, sy, sw, sh, stw, ax, ay, tl, tr, br, bl,
							lxx, rxx, tyy, byy, irr0, irr1, cout, &ca);


					p0 = cout[0];
					p1 = cout[1];
					p2 = cout[2];
					p3 = cout[3];
				}

				if (p0 != p1)
					p0 = argb8888_interp_256(ax, p1, p0);
				if (p2 != p3)
					p2 = argb8888_interp_256(ax, p3, p2);
				if (p0 != p2)
					p0 = argb8888_interp_256(ay, p2, p0);

				if (ca < 256)
					p0 = argb8888_interp_256(ca, p0, op0);
			}
			q0 = p0;
		}
		*d++ = q0;
		s++;
		xx += axx;
		yy += ayx;
	}
}

/* stroke and/or fill with possibly a fill renderer */
static void _rounded_stroke_fill_paint_proj(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Rectangle *thiz = _rectangle_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Color color;
	Enesim_Color ocolor;
	Enesim_Color icolor;
	Eina_F16p16 ww0 = thiz->ww;
	Eina_F16p16 hh0 = thiz->hh;
	Eina_F16p16 xx0 = thiz->xx;
	Eina_F16p16 yy0 = thiz->yy;
	int stww = (thiz->sw) * 65536;
	int stw = stww >> 16;
	int iww = ww0 - (2 * stww);
	int ihh = hh0 - (2 * stww);
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	int azx = thiz->matrix.zx;
	int do_inner = thiz->do_inner;
	int rr0 = thiz->rr0, rr1 = rr0 + 65536;
	int irr0 = thiz->irr0, irr1 = irr0 + 65536;
	int lxx0 = thiz->lxx0, rxx0 = thiz->rxx0;
	int tyy0 = thiz->tyy0, byy0 = thiz->byy0;
	Enesim_Renderer *fpaint;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	int xx, yy, zz;
	int fill_only = 0;
	char bl = thiz->current.corner.bl, br = thiz->current.corner.br, tl = thiz->current.corner.tl, tr = thiz->current.corner.tr;

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
	xx = eina_f16p16_sub(xx, xx0);
	yy = eina_f16p16_sub(yy, yy0);

	while (d < e)
	{
		unsigned int q0 = 0;

		if (zz)
		{
			int sxx = (((long long int)xx) << 16) / zz;
			int syy = (((long long int)yy) << 16) / zz;

			if (sxx < ww0 && syy < hh0 && sxx > -EINA_F16P16_ONE && syy > -EINA_F16P16_ONE)
			{
				int sx = sxx >> 16;
				int sy = syy >> 16;
				int sw = ww0 >> 16;
				int sh = hh0 >> 16;
				uint16_t ca = 256;
				unsigned int op3 = 0, op2 = 0, op1 = 0, op0 = 0, p0;
				int ax = 1 + ((sxx & 0xffff) >> 8);
				int ay = 1 + ((syy & 0xffff) >> 8);

				int lxx = sxx - lxx0, rxx = sxx - rxx0;
				int tyy = syy - tyy0, byy = syy - byy0;

				int ixx = sxx - stww, iyy = syy - stww;
				int ix = ixx >> 16, iy = iyy >> 16;

				if (fill_only && fpaint)
				{
					ocolor = *d;
					if (icolor != 0xffffffff)
						ocolor = argb8888_mul4_sym(ocolor, icolor);
				}


				if ((ww0 - sxx) < 65536)
					ax = 256 - ((ww0 - sxx) >> 8);
				if ((hh0 - syy) < 65536)
					ay = 256 - ((hh0 - syy) >> 8);

				if ((sx > -1) && (sy > -1))
					op0 = ocolor;
				if ((sy > -1) && ((sxx + 65536) < ww0))
					op1 = ocolor;
				if ((syy + 65536) < hh0)
				{
					if (sx > -1)
						op2 = ocolor;
					if ((sxx + 65536) < ww0)
						op3 = ocolor;
				}

				{
					uint32_t cout[4] = { op0, op1, op2, op3 };
					_outer_corners(sx, sy, sw, sh, ax, ay, tl, tr, br, bl,
							lxx, rxx, tyy, byy, rr0, rr1, cout, &ca);


					op0 = cout[0];
					op1 = cout[1];
					op2 = cout[2];
					op3 = cout[3];
				}

				if (op0 != op1)
					op0 = argb8888_interp_256(ax, op1, op0);
				if (op2 != op3)
					op2 = argb8888_interp_256(ax, op3, op2);
				if (op0 != op2)
					op0 = argb8888_interp_256(ay, op2, op0);

				if (ca < 256)
					op0 = argb8888_mul_256(ca, op0);

				p0 = op0;
				if ( do_inner && (ixx > -65536) && (iyy > -65536) &&
					(ixx < iww) && (iyy < ihh) )
				{
					unsigned int p3 = p0, p2 = p0, p1 = p0;

					color = icolor;
					if (fpaint)
					{
						color = *d;
						if (icolor != 0xffffffff)
							color = argb8888_mul4_sym(icolor, color);
					}

					ca = 256;
					ax = 1 + ((ixx & 0xffff) >> 8);
					ay = 1 + ((iyy & 0xffff) >> 8);
					if ((iww - ixx) < 65536)
						ax = 256 - ((iww - ixx) >> 8);
					if ((ihh - iyy) < 65536)
						ay = 256 - ((ihh - iyy) >> 8);

					if ((ix > -1) & (iy > -1))
						p0 = color;
					if ((iy > -1) && ((ixx + 65536) < iww))
						p1 = color;
					if ((iyy + 65536) < ihh)
					{
						if (ix > -1)
							p2 = color;
						if ((ixx + 65536) < iww)
							p3 = color;
					}

					{
						uint32_t cout[4] = { p0, p1, p2, p3 };
						_inner_corners(sx, sy, sw, sh, stw, ax, ay, tl, tr, br, bl,
								lxx, rxx, tyy, byy, irr0, irr1, cout, &ca);


						p0 = cout[0];
						p1 = cout[1];
						p2 = cout[2];
						p3 = cout[3];
					}

					if (p0 != p1)
						p0 = argb8888_interp_256(ax, p1, p0);
					if (p2 != p3)
						p2 = argb8888_interp_256(ax, p3, p2);
					if (p0 != p2)
						p0 = argb8888_interp_256(ay, p2, p0);

					if (ca < 256)
						p0 = argb8888_interp_256(ca, p0, op0);
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

/* stroke with a renderer and possibly fill with color */
static void _rounded_stroke_paint_fill_proj(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Rectangle *thiz = _rectangle_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Color color;
	Enesim_Color ocolor;
	Enesim_Color icolor;
	Eina_F16p16 ww0 = thiz->ww;
	Eina_F16p16 hh0 = thiz->hh;
	Eina_F16p16 xx0 = thiz->xx;
	Eina_F16p16 yy0 = thiz->yy;
	int stww = (thiz->sw) * 65536;
	int stw = stww >> 16;
	int iww = ww0 - (2 * stww);
	int ihh = hh0 - (2 * stww);
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	int azx = thiz->matrix.zx;
	int do_inner = thiz->do_inner;
	int rr0 = thiz->rr0, rr1 = rr0 + 65536;
	int irr0 = thiz->irr0, irr1 = irr0 + 65536;
	int lxx0 = thiz->lxx0, rxx0 = thiz->rxx0;
	int tyy0 = thiz->tyy0, byy0 = thiz->byy0;
	Enesim_Renderer *spaint;
	Enesim_Renderer_Sw_Data *sdata;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	int xx, yy, zz;
	int fill_only = 0;
	char bl = thiz->current.corner.bl, br = thiz->current.corner.br, tl = thiz->current.corner.tl, tr = thiz->current.corner.tr;

	enesim_renderer_shape_stroke_color_get(r, &ocolor);
	enesim_renderer_shape_stroke_renderer_get(r, &spaint);
	enesim_renderer_shape_fill_color_get(r, &icolor);
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);

	sdata = enesim_renderer_backend_data_get(spaint, ENESIM_BACKEND_SOFTWARE);
	sdata->fill(spaint, x, y, len, dst);

	if (draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE)
		icolor = 0;

	enesim_renderer_projective_setup(r, x, y, &thiz->matrix, &xx, &yy, &zz);
	xx = eina_f16p16_sub(xx, xx0);
	yy = eina_f16p16_sub(yy, yy0);

	while (d < e)
	{
		unsigned int q0 = 0;

		if (zz)
		{
			int sxx = (((long long int)xx) << 16) / zz;
			int syy = (((long long int)yy) << 16) / zz;

			if (sxx < ww0 && syy < hh0 && sxx > -EINA_F16P16_ONE && syy > -EINA_F16P16_ONE)
			{
				int sx = sxx >> 16;
				int sy = syy >> 16;
				int sw = ww0 >> 16;
				int sh = hh0 >> 16;
				uint16_t ca = 256;
				unsigned int op3 = 0, op2 = 0, op1 = 0, op0 = 0, p0;
				int ax = 1 + ((sxx & 0xffff) >> 8);
				int ay = 1 + ((syy & 0xffff) >> 8);

				int lxx = sxx - lxx0, rxx = sxx - rxx0;
				int tyy = syy - tyy0, byy = syy - byy0;

				int ixx = sxx - stww, iyy = syy - stww;
				int ix = ixx >> 16, iy = iyy >> 16;

				color = *d;
				if (ocolor != 0xffffffff)
					color = argb8888_mul4_sym(ocolor, color);

				if ((ww0 - sxx) < 65536)
					ax = 256 - ((ww0 - sxx) >> 8);
				if ((hh0 - syy) < 65536)
					ay = 256 - ((hh0 - syy) >> 8);

				if ((sx > -1) && (sy > -1))
					op0 = ocolor;
				if ((sy > -1) && ((sxx + 65536) < ww0))
					op1 = ocolor;
				if ((syy + 65536) < hh0)
				{
					if (sx > -1)
						op2 = ocolor;
					if ((sxx + 65536) < ww0)
						op3 = ocolor;
				}

				{
					uint32_t cout[4] = { op0, op1, op2, op3 };
					_outer_corners(sx, sy, sw, sh, ax, ay, tl, tr, br, bl,
							lxx, rxx, tyy, byy, rr0, rr1, cout, &ca);


					op0 = cout[0];
					op1 = cout[1];
					op2 = cout[2];
					op3 = cout[3];
				}

				if (op0 != op1)
					op0 = argb8888_interp_256(ax, op1, op0);
				if (op2 != op3)
					op2 = argb8888_interp_256(ax, op3, op2);
				if (op0 != op2)
					op0 = argb8888_interp_256(ay, op2, op0);

				if (ca < 256)
					op0 = argb8888_mul_256(ca, op0);

				p0 = op0;
				if ( do_inner && (ixx > -65536) && (iyy > -65536) &&
					(ixx < iww) && (iyy < ihh) )
				{
					unsigned int p3 = p0, p2 = p0, p1 = p0;

					color = icolor;
					ca = 256;
					ax = 1 + ((ixx & 0xffff) >> 8);
					ay = 1 + ((iyy & 0xffff) >> 8);
					if ((iww - ixx) < 65536)
						ax = 256 - ((iww - ixx) >> 8);
					if ((ihh - iyy) < 65536)
						ay = 256 - ((ihh - iyy) >> 8);

					if ((ix > -1) & (iy > -1))
						p0 = color;
					if ((iy > -1) && ((ixx + 65536) < iww))
						p1 = color;
					if ((iyy + 65536) < ihh)
					{
						if (ix > -1)
							p2 = color;
						if ((ixx + 65536) < iww)
							p3 = color;
					}

					{
						uint32_t cout[4] = { p0, p1, p2, p3 };
						_inner_corners(sx, sy, sw, sh, stw, ax, ay, tl, tr, br, bl,
								lxx, rxx, tyy, byy, irr0, irr1, cout, &ca);


						p0 = cout[0];
						p1 = cout[1];
						p2 = cout[2];
						p3 = cout[3];
					}

					if (p0 != p1)
						p0 = argb8888_interp_256(ax, p1, p0);
					if (p2 != p3)
						p2 = argb8888_interp_256(ax, p3, p2);
					if (p0 != p2)
						p0 = argb8888_interp_256(ay, p2, p0);

					if (ca < 256)
						p0 = argb8888_interp_256(ca, p0, op0);
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

/* stroke and fill with renderers */
static void _rounded_stroke_paint_fill_paint_proj(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Rectangle *thiz = _rectangle_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Color color;
	Enesim_Color ocolor;
	Enesim_Color icolor;
	Eina_F16p16 ww0 = thiz->ww;
	Eina_F16p16 hh0 = thiz->hh;
	Eina_F16p16 xx0 = thiz->xx;
	Eina_F16p16 yy0 = thiz->yy;
	int stww = (thiz->sw) * 65536;
	int stw = stww >> 16;
	int iww = ww0 - (2 * stww);
	int ihh = hh0 - (2 * stww);
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	int azx = thiz->matrix.zx;
	int rr0 = thiz->rr0, rr1 = rr0 + 65536;
	int irr0 = thiz->irr0, irr1 = irr0 + 65536;
	int lxx0 = thiz->lxx0, rxx0 = thiz->rxx0;
	int tyy0 = thiz->tyy0, byy0 = thiz->byy0;
	Enesim_Renderer *fpaint, *spaint;
	Enesim_Renderer_Sw_Data *sdata;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	unsigned int *sbuf, *s;
	int xx, yy, zz;
	char bl = thiz->current.corner.bl, br = thiz->current.corner.br, tl = thiz->current.corner.tl, tr = thiz->current.corner.tr;

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

	enesim_renderer_projective_setup(r, x, y, &thiz->matrix, &xx, &yy, &zz);
	xx = eina_f16p16_sub(xx, xx0);
	yy = eina_f16p16_sub(yy, yy0);

	while (d < e)
	{
		unsigned int q0 = 0;

		if (zz)
		{
			int sxx = (((long long int)xx) << 16) / zz;
			int syy = (((long long int)yy) << 16) / zz;

			if (sxx < ww0 && syy < hh0 && sxx > -EINA_F16P16_ONE && syy > -EINA_F16P16_ONE)
			{
				int sx = sxx >> 16;
				int sy = syy >> 16;
				int sw = ww0 >> 16;
				int sh = hh0 >> 16;
				uint16_t ca = 256;
				unsigned int op3 = 0, op2 = 0, op1 = 0, op0 = 0, p0;
				int ax = 1 + ((sxx & 0xffff) >> 8);
				int ay = 1 + ((syy & 0xffff) >> 8);

				int lxx = sxx - lxx0, rxx = sxx - rxx0;
				int tyy = syy - tyy0, byy = syy - byy0;

				int ixx = sxx - stww, iyy = syy - stww;
				int ix = ixx >> 16, iy = iyy >> 16;

				color = *s;
				if (ocolor != 0xffffffff)
					color = argb8888_mul4_sym(ocolor, color);

				if ((ww0 - sxx) < 65536)
					ax = 256 - ((ww0 - sxx) >> 8);
				if ((hh0 - syy) < 65536)
					ay = 256 - ((hh0 - syy) >> 8);

				if ((sx > -1) && (sy > -1))
					op0 = color;
				if ((sy > -1) && ((sxx + 65536) < ww0))
					op1 = color;
				if ((syy + 65536) < hh0)
				{
					if (sx > -1)
						op2 = color;
					if ((sxx + 65536) < ww0)
						op3 = color;
				}

				{
					uint32_t cout[4] = { op0, op1, op2, op3 };
					_outer_corners(sx, sy, sw, sh, ax, ay, tl, tr, br, bl,
							lxx, rxx, tyy, byy, rr0, rr1, cout, &ca);


					op0 = cout[0];
					op1 = cout[1];
					op2 = cout[2];
					op3 = cout[3];
				}

				if (op0 != op1)
					op0 = argb8888_interp_256(ax, op1, op0);
				if (op2 != op3)
					op2 = argb8888_interp_256(ax, op3, op2);
				if (op0 != op2)
					op0 = argb8888_interp_256(ay, op2, op0);

				if (ca < 256)
					op0 = argb8888_mul_256(ca, op0);

				p0 = op0;
				if ((ixx > -65536) && (iyy > -65536) &&
					(ixx < iww) && (iyy < ihh))
				{
					unsigned int p3 = p0, p2 = p0, p1 = p0;

					color = *d;
					if (icolor != 0xffffffff)
						color = argb8888_mul4_sym(icolor, color);

					ca = 256;
					ax = 1 + ((ixx & 0xffff) >> 8);
					ay = 1 + ((iyy & 0xffff) >> 8);
					if ((iww - ixx) < 65536)
						ax = 256 - ((iww - ixx) >> 8);
					if ((ihh - iyy) < 65536)
						ay = 256 - ((ihh - iyy) >> 8);

					if ((ix > -1) & (iy > -1))
						p0 = color;
					if ((iy > -1) && ((ixx + 65536) < iww))
						p1 = color;
					if ((iyy + 65536) < ihh)
					{
						if (ix > -1)
							p2 = color;
						if ((ixx + 65536) < iww)
							p3 = color;
					}

					{
						uint32_t cout[4] = { p0, p1, p2, p3 };
						_inner_corners(sx, sy, sw, sh, stw, ax, ay, tl, tr, br, bl,
								lxx, rxx, tyy, byy, irr0, irr1, cout, &ca);


						p0 = cout[0];
						p1 = cout[1];
						p2 = cout[2];
						p3 = cout[3];
					}

					if (p0 != p1)
						p0 = argb8888_interp_256(ax, p1, p0);
					if (p2 != p3)
						p2 = argb8888_interp_256(ax, p3, p2);
					if (p0 != p2)
						p0 = argb8888_interp_256(ay, p2, p0);

					if (ca < 256)
						p0 = argb8888_interp_256(ca, p0, op0);
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
#endif
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _rectangle_name(Enesim_Renderer *r)
{
	return "rectangle";
}

static Eina_Bool _rectangle_state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Rectangle *thiz;
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Renderer *spaint;
	double rad;
	double sw;
	double scaled_width;
	double scaled_height;
	double scaled_x;
	double scaled_y;

	thiz = _rectangle_get(r);
	if (!thiz || (thiz->current.width < 1) || (thiz->current.height < 1))
	{
		ENESIM_RENDERER_ERROR(r, error, "Invalid size %d %d", thiz->current.width, thiz->current.height);
		return EINA_FALSE;
	}
	scaled_width = thiz->current.width * state->sx;
	scaled_height = thiz->current.height * state->sy;

	if ((scaled_width < 1) || (scaled_height < 1))
	{
		ENESIM_RENDERER_ERROR(r, error, "Invalid scaled size %d %d", scaled_width, scaled_height);
		return EINA_FALSE;
	}

	rad = thiz->current.corner.radius;
	if (rad > (scaled_width / 2.0))
		rad = scaled_width / 2.0;
	if (rad > (scaled_height / 2.0))
		rad = scaled_height / 2.0;

	scaled_x = thiz->current.x * state->sx;
	scaled_y = thiz->current.y * state->sy;

	/* check if we should use the path approach */
	/* FIXME later */
	if (_rectangle_use_path(state->geometry_transformation_type))
	{
		_rectangle_path_setup(thiz, scaled_x, scaled_y, scaled_width, scaled_height, rad, state, sstate);
		if (!enesim_renderer_setup(thiz->path, s, error))
		{
			return EINA_FALSE;
		}
		thiz->fill = enesim_renderer_sw_fill_get(thiz->path);
		*fill = _rectangle_path_span;

		return EINA_TRUE;
	}
	/* do our own setup */
	else
	{
		/* the code from here is only meaningful for our own span functions */
		thiz->ww = eina_f16p16_double_from(scaled_width);
		thiz->hh = eina_f16p16_double_from(scaled_height);

		thiz->xx = eina_f16p16_double_from(scaled_x);
		thiz->yy = eina_f16p16_double_from(scaled_y);

		thiz->rr0 = eina_f16p16_double_from(rad);
		thiz->lxx0 = thiz->rr0;
		thiz->tyy0 = thiz->rr0;
		thiz->rxx0 = eina_f16p16_double_from(scaled_width - rad - 1);
		thiz->byy0 = eina_f16p16_double_from(scaled_height - rad - 1);

		enesim_renderer_shape_stroke_weight_get(r, &sw);
		thiz->do_inner = 1;
		if ((sw >= scaled_width / 2.0) || (sw >= scaled_height / 2.0))
		{
			sw = 0;
			thiz->do_inner = 0;
		}
		rad = rad - sw;
		if (rad < 0.0039)
			rad = 0;

		thiz->irr0 = eina_f16p16_double_from(rad);
		thiz->sw = sw;

		if (!enesim_renderer_shape_setup(r, state, s, error))
		{
			ENESIM_RENDERER_ERROR(r, error, "Shape cannot setup");
			return EINA_FALSE;
		}

		enesim_matrix_f16p16_matrix_to(&state->transformation,
				&thiz->matrix);
		enesim_renderer_shape_draw_mode_get(r, &draw_mode);
		enesim_renderer_shape_stroke_renderer_get(r, &spaint);

#if NEW_RENDERER
		*fill = _test_affine;
#else
		if (state->transformation_type == ENESIM_MATRIX_AFFINE ||
			 state->transformation_type == ENESIM_MATRIX_IDENTITY)
		{
			*fill = _rounded_stroke_fill_paint_affine;
			if ((sw != 0.0) && spaint && (draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE))
			{
				Enesim_Renderer *fpaint;

				*fill = _rounded_stroke_paint_fill_affine;
				enesim_renderer_shape_fill_renderer_get(r, &fpaint);
				if (fpaint && thiz->do_inner &&
						(draw_mode & ENESIM_SHAPE_DRAW_MODE_FILL))
					*fill = _rounded_stroke_paint_fill_paint_affine;
			}
		}
		else
		{
			*fill = _rounded_stroke_fill_paint_proj;
			if ((sw != 0.0) && spaint && (draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE))
			{
				Enesim_Renderer *fpaint;

				*fill = _rounded_stroke_paint_fill_proj;
				enesim_renderer_shape_fill_renderer_get(r, &fpaint);
				if (fpaint && thiz->do_inner &&
						(draw_mode & ENESIM_SHAPE_DRAW_MODE_FILL))
					*fill = _rounded_stroke_paint_fill_paint_proj;
			}
		}
#endif
		return EINA_TRUE;
	}
}

static void _rectangle_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);

	enesim_renderer_shape_cleanup(r, s);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

static void _rectangle_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	if (!thiz)
	{
		*flags = 0;
		return;
	}

	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_SCALE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_PROJECTIVE |
			ENESIM_RENDERER_FLAG_ARGB8888 |
			ENESIM_SHAPE_FLAG_FILL_RENDERER;
}

static void _rectangle_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	if (thiz->path)
		enesim_renderer_unref(thiz->path);
	free(thiz);
}

static void _rectangle_boundings(Enesim_Renderer *r, Enesim_Rectangle *boundings)
{
	Enesim_Renderer_Rectangle *thiz;
	double sx, sy;

	thiz = _rectangle_get(r);

	enesim_renderer_scale_get(r, &sx, &sy);
	boundings->x = thiz->current.x * sx;
	boundings->y = thiz->current.y * sy;
	boundings->w = thiz->current.width * sx;
	boundings->h = thiz->current.height * sy;
}

static Eina_Bool _rectangle_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);

	if (!thiz->changed) return EINA_FALSE;

	/* the width */
	if (thiz->current.width != thiz->past.height)
		return EINA_TRUE;
	/* the height */
	if (thiz->current.height != thiz->past.height)
		return EINA_TRUE;
	/* the x */
	if (thiz->current.x != thiz->past.x)
		return EINA_TRUE;
	/* the y */
	if (thiz->current.y != thiz->past.y)
		return EINA_TRUE;
	/* the corners */
	if ((thiz->current.corner.tl != thiz->past.corner.tl) || (thiz->current.corner.tr != thiz->past.corner.tr) ||
	     (thiz->current.corner.bl != thiz->past.corner.bl) || (thiz->current.corner.br != thiz->past.corner.br))
		return EINA_TRUE;
	/* the corner radius */
	if (thiz->current.corner.radius != thiz->past.corner.radius)
		return EINA_TRUE;

	return EINA_FALSE;
}

#if BUILD_OPENGL
/* TODO instead of trying to do our own geometry shader it might be better
 * to use the path renderer and only create a shader there, this will simplify the code
 * given that at the end we'll generate vertices too
 */
static Eina_Bool _rectangle_opengl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		Enesim_Surface *s,
		int *num_shaders,
		Enesim_Renderer_OpenGL_Shader **shaders,
		Enesim_Error **error)
{
	Enesim_Renderer_Rectangle *thiz;
	Enesim_Renderer_OpenGL_Shader *shader;

 	thiz = _rectangle_get(r);

	/* FIXME in order to generate the stroke we might need to call this twice
	 * one for the outter rectangle and one for the inner
	 */
	*num_shaders = 2;
	*shaders = shader = calloc(*num_shaders, sizeof(Enesim_Renderer_OpenGL_Shader));
	shader->type = ENESIM_SHADER_GEOMETRY;
	shader->name = "rectangle";
		//"#version 150\n"
	shader->source = 
		"#version 120\n"
		"#extension GL_EXT_geometry_shader4 : enable\n"
	#include "enesim_renderer_rectangle.glsl"
	shader->size = strlen(shader->source);

	/* FIXME for now we should use the background renderer for the color */
	/* if we have a renderer set use that one but render into another texture
	 * if it has some geometric shader
	 */
	shader++;
	shader->type = ENESIM_SHADER_FRAGMENT;
	shader->name = "stripes";
	shader->source = 
	#include "enesim_renderer_stripes.glsl"
	shader->size = strlen(shader->source);

	return EINA_TRUE;
}

static Eina_Bool _rectangle_opengl_shader_setup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Rectangle *thiz;
	Enesim_Renderer_OpenGL_Data *rdata;
	int width;
	int height;
	int x;
	int y;

 	thiz = _rectangle_get(r);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);

	x = glGetUniformLocationARB(rdata->program, "rectangle_x");
	y = glGetUniformLocationARB(rdata->program, "rectangle_y");
	width = glGetUniformLocationARB(rdata->program, "rectangle_width");
	height = glGetUniformLocationARB(rdata->program, "rectangle_height");

	glUniform1f(x, thiz->current.x);
	glUniform1f(y, thiz->current.y);
	glUniform1f(width, thiz->current.width);
	glUniform1f(height, thiz->current.height);

	/* FIXME set the background like this for now */
	{
		int odd_color;
		int even_color;
		int odd_thickness;
		int even_thickness;

		even_color = glGetUniformLocationARB(rdata->program, "stripes_even_color");
		odd_color = glGetUniformLocationARB(rdata->program, "stripes_odd_color");
		even_thickness = glGetUniformLocationARB(rdata->program, "stripes_even_thickness");
		odd_thickness = glGetUniformLocationARB(rdata->program, "stripes_odd_thickness");

		glUniform4fARB(even_color, 1.0, 0.0, 0.0, 1.0);
		glUniform4fARB(odd_color, 0.0, 0.0, 1.0, 1.0);
		glUniform1i(even_thickness, 2.0);
		glUniform1i(odd_thickness, 5.0);
	}

	return EINA_TRUE;
}

static void _rectangle_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Rectangle *thiz;

 	thiz = _rectangle_get(r);
}
#endif

static Enesim_Renderer_Shape_Descriptor _rectangle_descriptor = {
	/* .name = 			*/ _rectangle_name,
	/* .free = 			*/ _rectangle_free,
#if NEW_RENDERER
	/* .boundings = 		*/ NULL, //_rectangle_boundings,
#else
	/* .boundings = 		*/ NULL, //_rectangle_boundings,
#endif
	/* .destination_transform = 	*/ NULL,
	/* .flags = 			*/ _rectangle_flags,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _rectangle_has_changed,
	/* .sw_setup = 			*/ _rectangle_state_setup,
	/* .sw_cleanup = 		*/ _rectangle_state_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
#if BUILD_OPENGL
	/* .opengl_setup =          	*/ _rectangle_opengl_setup,
	/* .opengl_shader_setup =   	*/ _rectangle_opengl_shader_setup,
	/* .opengl_cleanup =        	*/ _rectangle_opengl_cleanup
#else
	/* .opengl_setup =          	*/ NULL,
	/* .opengl_shader_setup =   	*/ NULL,
	/* .opengl_cleanup =        	*/ NULL
#endif
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a new rectangle renderer
 * @return The new renderer
 */
EAPI Enesim_Renderer * enesim_renderer_rectangle_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Rectangle *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Rectangle));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_RECTANGLE_MAGIC);
	r = enesim_renderer_shape_new(&_rectangle_descriptor, thiz);
	return r;
}
/**
 * Sets the width of the rectangle
 * @param[in] r The rectangle renderer
 * @param[in] width The rectangle width
 */
EAPI void enesim_renderer_rectangle_width_set(Enesim_Renderer *r, double width)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	thiz->current.width = width;
	thiz->changed = EINA_TRUE;
}
/**
 * Gets the width of the rectangle
 * @param[in] r The rectangle renderer
 * @param[out] w The rectangle width
 */
EAPI void enesim_renderer_rectangle_width_get(Enesim_Renderer *r, double *width)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	if (width) *width = thiz->current.width;
}
/**
 * Sets the height of the rectangle
 * @param[in] r The rectangle renderer
 * @param[in] height The rectangle height
 */
EAPI void enesim_renderer_rectangle_height_set(Enesim_Renderer *r, double height)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	thiz->current.height = height;
	thiz->changed = EINA_TRUE;
}
/**
 * Gets the height of the rectangle
 * @param[in] r The rectangle renderer
 * @param[out] height The rectangle height
 */
EAPI void enesim_renderer_rectangle_height_get(Enesim_Renderer *r, double *height)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	if (height) *height = thiz->current.height;
}

/**
 * Sets the x of the rectangle
 * @param[in] r The rectangle renderer
 * @param[in] x The rectangle x coordinate
 */
EAPI void enesim_renderer_rectangle_x_set(Enesim_Renderer *r, double x)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	thiz->current.x = x;
	thiz->changed = EINA_TRUE;
}
/**
 * Gets the x of the rectangle
 * @param[in] r The rectangle renderer
 * @param[out] w The rectangle x coordinate
 */
EAPI void enesim_renderer_rectangle_x_get(Enesim_Renderer *r, double *x)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	if (x) *x = thiz->current.x;
}
/**
 * Sets the y of the rectangle
 * @param[in] r The rectangle renderer
 * @param[in] y The rectangle y coordinate
 */
EAPI void enesim_renderer_rectangle_y_set(Enesim_Renderer *r, double y)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
}
/**
 * Gets the y of the rectangle
 * @param[in] r The rectangle renderer
 * @param[out] y The rectangle y coordinate
 */
EAPI void enesim_renderer_rectangle_y_get(Enesim_Renderer *r, double *y)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	if (y) *y = thiz->current.y;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rectangle_position_set(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Rectangle *thiz;
	thiz = _rectangle_get(r);
	thiz->current.x = x;
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rectangle_position_get(Enesim_Renderer *r, double *x, double *y)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	if (x) *x = thiz->current.x;
	if (y) *y = thiz->current.y;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rectangle_size_set(Enesim_Renderer *r, double width, double height)
{
	Enesim_Renderer_Rectangle *thiz;
	thiz = _rectangle_get(r);
	thiz->current.width = width;
	thiz->current.height = height;
	thiz->changed = EINA_TRUE;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rectangle_size_get(Enesim_Renderer *r, double *width, double *height)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	if (width) *width = thiz->current.width;
	if (height) *height = thiz->current.height;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rectangle_corner_radius_set(Enesim_Renderer *r, double radius)
{
	Enesim_Renderer_Rectangle *thiz;
	thiz = _rectangle_get(r);
	if (!thiz) return;
	if (radius < 0)
		radius = 0;
	if (thiz->current.corner.radius == radius)
		return;
	thiz->current.corner.radius = radius;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rectangle_top_left_corner_set(Enesim_Renderer *r, Eina_Bool rounded)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	thiz->current.corner.tl = rounded;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rectangle_top_right_corner_set(Enesim_Renderer *r, Eina_Bool rounded)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	thiz->current.corner.tr = rounded;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rectangle_bottom_left_corner_set(Enesim_Renderer *r, Eina_Bool rounded)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	thiz->current.corner.bl = rounded;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rectangle_bottom_right_corner_set(Enesim_Renderer *r, Eina_Bool rounded)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	thiz->current.corner.br = rounded;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rectangle_corners_set(Enesim_Renderer *r,
		Eina_Bool tl, Eina_Bool tr, Eina_Bool bl, Eina_Bool br)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	if (!thiz) return;

	if ((thiz->current.corner.tl == tl) && (thiz->current.corner.tr == tr) &&
	     (thiz->current.corner.bl == bl) && (thiz->current.corner.br == br))
		return;
	thiz->current.corner.tl = tl;  thiz->current.corner.tr = tr;
	thiz->current.corner.bl = bl;  thiz->current.corner.br = br;
	thiz->changed = EINA_TRUE;
}
