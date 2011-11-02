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
typedef struct _Enesim_Renderer_Rectangle {
	/* public properties */
	double width;
	double height;
	struct {
		double radius;
		Eina_Bool tl : 1;
		Eina_Bool tr : 1;
		Eina_Bool bl : 1;
		Eina_Bool br : 1;
	} corner;
	/* internal state */
	Eina_Bool changed : 1;
	Enesim_F16p16_Matrix matrix;
	double scaled_width;
	double scaled_height;
	/* the inner rectangle in case of rounded corners */
	int lxx0, rxx0;
	int tyy0, byy0;
	int rr0, irr0;
	int sw;
	unsigned char do_inner :1;
} Enesim_Renderer_Rectangle;

/* we assume tyy and lxx are inside the top left corner */
static inline void _outer_top_left(int sx, int sy, uint16_t ax, uint16_t ay,
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

	if (sx < 0)
	{
		if (cout[1] != cout[3])
			cout[1] = argb8888_interp_256(ay, cout[3], cout[1]);
		cout[0] = cout[2] = cout[3] = cout[1];
	}
	if (sy < 0)
	{
		if (cout[2] != cout[3])
			cout[2] = argb8888_interp_256(ax, cout[3], cout[2]);
		cout[0] = cout[1] = cout[3] = cout[2];
	}
}

static inline void _outer_bottom_left(int sx, int sy, int sh, uint16_t ax, uint16_t ay,
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

	if (sx < 0)
	{
		if (cout[1] != cout[3])
			cout[1] = argb8888_interp_256(ay, cout[3], cout[1]);
		cout[0] = cout[2] = cout[3] = cout[1];
	}
	if ((sy + 1) == sh)
	{
		if (cout[0] != cout[1])
			cout[0] = argb8888_interp_256(ax, cout[1], cout[0]);
		cout[1] = cout[2] = cout[3] = cout[0];
	}
}

static inline void _outer_left_corners(int sx, int sy, int sh, uint16_t ax, uint16_t ay,
		Eina_Bool tl, Eina_Bool bl,
		Eina_F16p16 lxx, Eina_F16p16 tyy, Eina_F16p16 byy,
		Eina_F16p16 rr0, Eina_F16p16 rr1,
		uint32_t cout[4], uint16_t *ca)
{
	if (lxx < 0)
	{
		if (tl && (tyy < 0))
			_outer_top_left(sx, sy, ax, ay, lxx, tyy, rr0, rr1, cout, ca);
		if (bl && (byy > 0))
			_outer_bottom_left(sx, sy, sh, ax, ay, lxx, byy, rr0, rr1, cout, ca);
	}
}

static inline void _outer_top_right(int sx, int sy, int sw, uint16_t ax, uint16_t ay,
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

	if ((sx + 1) == sw)
	{
		if (cout[0] != cout[2])
			cout[0] = argb8888_interp_256(ay, cout[2], cout[0]);
		cout[1] = cout[2] = cout[3] = cout[0];
	}
	if (sy < 0)
	{
		if (cout[2] != cout[3])
			cout[2] = argb8888_interp_256(ax, cout[3], cout[2]);
		cout[0] = cout[1] = cout[3] = cout[2];
	}
}

static inline void _outer_bottom_right(int sx, int sy, int sw, int sh,
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

	if ((sx + 1) == sw)
	{
		if (cout[0] != cout[2])
			cout[0] = argb8888_interp_256(ay, cout[2], cout[0]);
		cout[1] = cout[2] = cout[3] = cout[0];
	}
	if ((sy + 1) == sh)
	{
		if (cout[0] != cout[1])
			cout[0] = argb8888_interp_256(ax, cout[1], cout[0]);
		cout[1] = cout[2] = cout[3] = cout[0];
	}
}

static inline void _outer_right_corners(int sx, int sy, int sw, int sh, uint16_t ax, uint16_t ay,
		Eina_Bool tr, Eina_Bool br,
		Eina_F16p16 rxx, Eina_F16p16 tyy, Eina_F16p16 byy,
		Eina_F16p16 rr0, Eina_F16p16 rr1,
		uint32_t cout[4], uint16_t *ca)
{
	if (rxx > 0)
	{
		if (tr && (tyy < 0))
			_outer_top_right(sx, sy, sw, ax, ay, rxx, tyy, rr0, rr1, cout, ca);
		if (br && (byy > 0))
			_outer_bottom_right(sx, sy, sw, sh, ax, ay, rxx, byy, rr0, rr1, cout, ca);
	}
}

static inline void _outer_corners(int sx, int sy, int sw, int sh, uint16_t ax, uint16_t ay,
		Eina_Bool tl, Eina_Bool tr, Eina_Bool br, Eina_Bool bl,
		Eina_F16p16 lxx, Eina_F16p16 rxx, Eina_F16p16 tyy, Eina_F16p16 byy,
		Eina_F16p16 rr0, Eina_F16p16 rr1,
		uint32_t cout[4], uint16_t *ca)
{
	_outer_left_corners(sx, sy, sh, ax, ay, tl, bl, lxx, tyy, byy, rr0, rr1, cout, ca);
	_outer_right_corners(sx, sy, sw, sh, ax, ay, tr, br, rxx, tyy, byy, rr0, rr1, cout, ca);
}

#define EVAL_ROUND_OUTER_CORNERS(c0,c1,c2,c3) \
		if (lxx < 0) \
		{ \
			if (tl && (tyy < 0)) \
			{ \
				if ((-lxx - tyy) >= rr0) \
				{ \
					int rr = hypot(lxx, tyy); \
 \
					ca = 0; \
					if (rr < rr1) \
					{ \
						ca = 256; \
						if (rr > rr0) \
							ca = 256 - ((rr - rr0) >> 8); \
					} \
				} \
 \
				if (sx < 0) \
				{ \
					if (c1 != c3) \
						c1 = argb8888_interp_256(ay, c3, c1); \
					c0 = c2 = c3 = c1; \
				} \
				if (sy < 0) \
				{ \
					if (c2 != c3) \
						c2 = argb8888_interp_256(ax, c3, c2); \
					c0 = c1 = c3 = c2; \
				} \
			} \
 \
			if (bl && (byy > 0)) \
			{ \
				if ((-lxx + byy) >= rr0) \
				{ \
					int rr = hypot(lxx, byy); \
 \
					ca = 0; \
					if (rr < rr1) \
					{ \
						ca = 256; \
						if (rr > rr0) \
							ca = 256 - ((rr - rr0) >> 8); \
					} \
				} \
 \
				if (sx < 0) \
				{ \
					if (c1 != c3) \
						c1 = argb8888_interp_256(ay, c3, c1); \
					c0 = c2 = c3 = c1; \
				} \
				if ((sy + 1) == sh) \
				{ \
					if (c0 != c1) \
						c0 = argb8888_interp_256(ax, c1, c0); \
					c1 = c2 = c3 = c0; \
				} \
			} \
		} \
 \
		if (rxx > 0) \
		{ \
			if (tr && (tyy < 0)) \
			{ \
				if ((rxx - tyy) >= rr0) \
				{ \
					int rr = hypot(rxx, tyy); \
 \
					ca = 0; \
					if (rr < rr1) \
					{ \
						ca = 256; \
						if (rr > rr0) \
							ca = 256 - ((rr - rr0) >> 8); \
					} \
				} \
 \
				if ((sx + 1) == sw) \
				{ \
					if (c0 != c2) \
						c0 = argb8888_interp_256(ay, c2, c0); \
					c1 = c2 = c3 = c0; \
				} \
				if (sy < 0) \
				{ \
					if (c2 != c3) \
						c2 = argb8888_interp_256(ax, c3, c2); \
					c0 = c1 = c3 = c2; \
				} \
			} \
 \
			if (br && (byy > 0)) \
			{ \
				if ((rxx + byy) >= rr0) \
				{ \
					int rr = hypot(rxx, byy); \
 \
					ca = 0; \
					if (rr < rr1) \
					{ \
						ca = 256; \
						if (rr > rr0) \
							ca = 256 - ((rr - rr0) >> 8); \
					} \
				} \
 \
				if ((sx + 1) == sw) \
				{ \
					if (c0 != c2) \
						c0 = argb8888_interp_256(ay, c2, c0); \
					c1 = c2 = c3 = c0; \
				} \
				if ((sy + 1) == sh) \
				{ \
					if (c0 != c1) \
						c0 = argb8888_interp_256(ax, c1, c0); \
					c1 = c2 = c3 = c0; \
				} \
			} \
		}


#define EVAL_ROUND_INNER_CORNERS(c0,c1,c2,c3) \
		if (lxx < 0) \
		{ \
			if (tl && (tyy < 0)) \
			{ \
				if ((-lxx - tyy) >= irr0) \
				{ \
					int rr = hypot(lxx, tyy); \
 \
					ca = 0; \
					if (rr < irr1) \
					{ \
						ca = 256; \
						if (rr > irr0) \
							ca = 256 - ((rr - irr0) >> 8); \
					} \
				} \
 \
				if (sx < stw) \
				{ \
					if (c1 != c3) \
						c1 = argb8888_interp_256(ay, c3, c1); \
					c0 = c2 = c3 = c1; \
				} \
				if (sy < stw) \
				{ \
					if (c2 != c3) \
						c2 = argb8888_interp_256(ax, c3, c2); \
					c0 = c1 = c3 = c2; \
				} \
			} \
 \
			if (bl && (byy > 0)) \
			{ \
				if ((-lxx + byy) >= irr0) \
				{ \
					int rr = hypot(lxx, byy); \
 \
					ca = 0; \
					if (rr < irr1) \
					{ \
						ca = 256; \
						if (rr > irr0) \
							ca = 256 - ((rr - irr0) >> 8); \
					} \
				} \
 \
				if (sx < stw) \
				{ \
					if (c1 != c3) \
						c1 = argb8888_interp_256(ay, c3, c1); \
					c0 = c2 = c3 = c1; \
				} \
				if ((sy + 1 + stw) == sh) \
				{ \
					if (c0 != c1) \
						c0 = argb8888_interp_256(ax, c1, c0); \
					c1 = c2 = c3 = c0; \
				} \
			} \
		} \
 \
		if (rxx > 0) \
		{ \
			if (tr && (tyy < 0)) \
			{ \
				if ((rxx - tyy) >= irr0) \
				{ \
					int rr = hypot(rxx, tyy); \
 \
					ca = 0; \
					if (rr < irr1) \
					{ \
						ca = 256; \
						if (rr > irr0) \
							ca = 256 - ((rr - irr0) >> 8); \
					} \
				} \
 \
				if ((sx + 1 + stw) == sw) \
				{ \
					if (c0 != c2) \
						c0 = argb8888_interp_256(ay, c2, c0); \
					c1 = c2 = c3 = c0; \
				} \
				if (sy < stw) \
				{ \
					if (c2 != c3) \
						c2 = argb8888_interp_256(ax, c3, c2); \
					c0 = c1 = c3 = c2; \
				} \
			} \
 \
			if (br && (byy > 0)) \
			{ \
				if ((rxx + byy) >= irr0) \
				{ \
					int rr = hypot(rxx, byy); \
 \
					ca = 0; \
					if (rr < irr1) \
					{ \
						ca = 256; \
						if (rr > irr0) \
							ca = 256 - ((rr - irr0) >> 8); \
					} \
				} \
 \
				if ((sx + 1 + stw) == sw) \
				{ \
					if (c0 != c2) \
						c0 = argb8888_interp_256(ay, c2, c0); \
					c1 = c2 = c3 = c0; \
				} \
				if ((sy + 1 + stw) == sh) \
				{ \
					if (c0 != c1) \
						c0 = argb8888_interp_256(ax, c1, c0); \
					c1 = c2 = c3 = c0; \
				} \
			} \
		}

static inline Enesim_Renderer_Rectangle * _rectangle_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	return thiz;
}

static void _span_rounded_color_stroked_paint_filled_affine(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Rectangle *thiz = _rectangle_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	int sw = thiz->scaled_width, sh = thiz->scaled_height;
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	int do_inner = thiz->do_inner;
	unsigned int ocolor;
	unsigned int icolor;
	int stw = thiz->sw;
	int rr0 = thiz->rr0, rr1 = rr0 + 65536;
	int irr0 = thiz->irr0, irr1 = irr0 + 65536;
	int lxx0 = thiz->lxx0, rxx0 = thiz->rxx0;
	int tyy0 = thiz->tyy0, byy0 = thiz->byy0;
	Enesim_Renderer *fpaint;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	int xx, yy;
	int fill_only = 0;
	char bl = thiz->corner.bl, br = thiz->corner.br, tl = thiz->corner.tl, tr = thiz->corner.tr;

	enesim_renderer_shape_stroke_color_get(r, &ocolor);
	enesim_renderer_shape_fill_color_get(r, &icolor);
 	enesim_renderer_shape_fill_renderer_get(r, &fpaint);
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);

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

	while (d < e)
	{
		unsigned int q0 = 0;
		int sx = (xx >> 16);
		int sy = (yy >> 16);

		if ((((unsigned) (sx + 1)) < (sw + 1)) && (((unsigned) (sy + 1)) < (sh + 1)))
		{
			int ca = 256;
			unsigned int op3 = 0, op2 = 0, op1 = 0, op0 = 0, p0;
			int ax = 1 + ((xx & 0xffff) >> 8);
			int ay = 1 + ((yy & 0xffff) >> 8);

			int lxx = xx - lxx0, rxx = xx - rxx0;
			int tyy = yy - tyy0, byy = yy - byy0;

			if (fill_only && fpaint)
			{
				ocolor = *d;
				if (icolor != 0xffffffff)
					ocolor = argb8888_mul4_sym(icolor, ocolor);
			}

			if ((sx > -1) & (sy > -1))
				op0 = ocolor;
			if ((sy > -1) & ((sx + 1) < sw))
				op1 = ocolor;
			if ((sy + 1) < sh)
			{
				if (sx > -1)
					op2 = ocolor;
				if ((sx + 1) < sw)
					op3 = ocolor;
			}
#if 0
			{
				uint32_t cout[4] = { op0, op1, op2, op3 };
				_outer_corners(sx, sy, sw, sh, ax, ay, tl, tr, br, bl,
						lxx, rxx, tyy, byy, rr0, rr1, cout, &ca);


				op0 = cout[0];
				op1 = cout[1];
				op2 = cout[2];
				op3 = cout[3];
			}
#else
			EVAL_ROUND_OUTER_CORNERS(op0,op1,op2,op3)
#endif

			if (op0 != op1)
				op0 = argb8888_interp_256(ax, op1, op0);
			if (op2 != op3)
				op2 = argb8888_interp_256(ax, op3, op2);
			if (op0 != op2)
				op0 = argb8888_interp_256(ay, op2, op0);

			if (ca < 256)
				op0 = argb8888_mul_256(ca, op0);

			p0 = op0;
			if (do_inner && ((((unsigned) (sx - stw + 1)) < (sw - (2 * stw) + 1))
				 && (((unsigned) (sy - stw + 1)) < (sh - (2 * stw) + 1))))
			{
				unsigned int p3 = p0, p2 = p0, p1 = p0;
				unsigned int color = icolor;

				if (fpaint)
				{
					color = *d;
					if (icolor != 0xffffffff)
						color = argb8888_mul4_sym(icolor, color);
				}

				ca = 256;
				if ((sx > (stw - 1)) & (sy > (stw - 1)))
					p0 = color;
				if ((sy > (stw - 1)) & ((sx + 1 + stw) < sw))
					p1 = color;
				if ((sy + 1 + stw) < sh)
				{
					if (sx > (stw - 1))
						p2 = color;
					if ((sx + 1 + stw) < sw)
						p3 = color;
				}

				EVAL_ROUND_INNER_CORNERS(p0,p1,p2,p3)

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

static void _span_rounded_color_stroked_paint_filled_proj(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Rectangle *thiz = _rectangle_get(r);
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Color ocolor;
	Enesim_Color icolor;
	int sw = thiz->scaled_width, sh = thiz->scaled_height;
	int axx = thiz->matrix.xx;
	int ayx = thiz->matrix.yx;
	int azx = thiz->matrix.zx;
	int do_inner = thiz->do_inner;
	int stw = thiz->sw;
	int rr0 = thiz->rr0, rr1 = rr0 + 65536;
	int irr0 = thiz->irr0, irr1 = irr0 + 65536;
	int lxx0 = thiz->lxx0, rxx0 = thiz->rxx0;
	int tyy0 = thiz->tyy0, byy0 = thiz->byy0;
	Enesim_Renderer *fpaint;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	int xx, yy, zz;
	int fill_only = 0;
	char bl = thiz->corner.bl, br = thiz->corner.br, tl = thiz->corner.tl, tr = thiz->corner.tr;

	enesim_renderer_shape_stroke_color_get(r, &ocolor);
	enesim_renderer_shape_fill_color_get(r, &icolor);
 	enesim_renderer_shape_fill_renderer_get(r, &fpaint);
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);

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
	while (d < e)
	{
		unsigned int q0 = 0;

		if (zz)
		{
			int sxx = (((long long int)xx) << 16) / zz;
			int sx = sxx >> 16;
			int syy = (((long long int)yy) << 16) / zz;
			int sy = syy >> 16;

			if ((((unsigned) (sx + 1)) < (sw + 1)) && (((unsigned) (sy + 1)) < (sh + 1)))
			{
				int ca = 256;
				unsigned int op3 = 0, op2 = 0, op1 = 0, op0 = 0, p0;
				int ax = 1 + ((sxx & 0xffff) >> 8);
				int ay = 1 + ((syy & 0xffff) >> 8);

				int lxx = sxx - lxx0, rxx = sxx - rxx0;
				int tyy = syy - tyy0, byy = syy - byy0;

				if (fill_only && fpaint)
				{
					ocolor = *d;
					if (icolor != 0xffffffff)
						ocolor = argb8888_mul4_sym(ocolor, icolor);
				}

				if ((sx > -1) & (sy > -1))
					op0 = ocolor;
				if ((sy > -1) & ((sx + 1) < sw))
					op1 = ocolor;
				if ((sy + 1) < sh)
				{
					if (sx > -1)
						op2 = ocolor;
					if ((sx + 1) < sw)
						op3 = ocolor;
				}

				EVAL_ROUND_OUTER_CORNERS(op0,op1,op2,op3)

				if (op0 != op1)
					op0 = argb8888_interp_256(ax, op1, op0);
				if (op2 != op3)
					op2 = argb8888_interp_256(ax, op3, op2);
				if (op0 != op2)
					op0 = argb8888_interp_256(ay, op2, op0);

				if (ca < 256)
					op0 = argb8888_mul_256(ca, op0);

				p0 = op0;
				if (do_inner && ((((unsigned) (sx - stw + 1)) < (sw - (2 * stw) + 1))
					 && (((unsigned) (sy - stw + 1)) < (sh - (2 * stw) + 1))))
				{
					unsigned int p3 = p0, p2 = p0, p1 = p0;
					unsigned int color = icolor;

					if (fpaint)
					{
						color = *d;
						if (icolor != 0xffffffff)
							color = argb8888_mul4_sym(icolor, color);
					}

					ca = 256;
					if ((sx > (stw - 1)) & (sy > (stw - 1)))
						p0 = color;
					if ((sy > (stw - 1)) & ((sx + 1 + stw) < sw))
						p1 = color;
					if ((sy + 1 + stw) < sh)
					{
						if (sx > (stw - 1))
							p2 = color;
						if ((sx + 1 + stw) < sw)
							p3 = color;
					}

					EVAL_ROUND_INNER_CORNERS(p0,p1,p2,p3)

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

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _rectangle_name(Enesim_Renderer *r)
{
	return "rectangle";
}

static Eina_Bool _rectangle_state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Rectangle *thiz;
	double rad;
	double sw;
	double sx, sy;

	thiz = _rectangle_get(r);
	if (!thiz || (thiz->width < 1) || (thiz->height < 1))
	{
		ENESIM_RENDERER_ERROR(r, error, "Invalid size %d %d", thiz->width, thiz->height);
		return EINA_FALSE;
	}
	enesim_renderer_scale_get(r, &sx, &sy);
	thiz->scaled_width = thiz->width * sx;
	thiz->scaled_height = thiz->height * sy;
	if ((thiz->scaled_width < 1) || (thiz->scaled_height < 1))
	{
		ENESIM_RENDERER_ERROR(r, error, "Invalid scaled size %d %d", thiz->scaled_width, thiz->scaled_height);
		return EINA_FALSE;
	}

	rad = thiz->corner.radius;
	if (rad > (thiz->scaled_width / 2.0))
		rad = thiz->scaled_width / 2.0;
	if (rad > (thiz->scaled_height / 2.0))
		rad = thiz->scaled_height / 2.0;

	thiz->rr0 = rad * 65536;
	thiz->lxx0 = thiz->tyy0 = thiz->rr0;
	thiz->rxx0 = (thiz->scaled_width - rad - 1) * 65536;
	thiz->byy0 = (thiz->scaled_height - rad - 1) * 65536;

	enesim_renderer_shape_stroke_weight_get(r, &sw);
	thiz->do_inner = 1;
	if ((sw >= (thiz->scaled_width / 2.0)) || (sw >= (thiz->scaled_height / 2.0)))
	{
		sw = 0;
		thiz->do_inner = 0;
	}
	rad = rad - sw;
	if (rad < 0.0039)
		rad = 0;

	thiz->irr0 = rad * 65536;
	thiz->sw = sw;

	if (!enesim_renderer_shape_setup(r, state, s, error))
	{
		ENESIM_RENDERER_ERROR(r, error, "Shape cannot setup");
		return EINA_FALSE;
	}

	enesim_matrix_f16p16_matrix_to(&state->transformation,
			&thiz->matrix);
	*fill = _span_rounded_color_stroked_paint_filled_proj;
	if (state->transformation_type == ENESIM_MATRIX_AFFINE || state->transformation_type == ENESIM_MATRIX_IDENTITY)
		*fill = _span_rounded_color_stroked_paint_filled_affine;

	return EINA_TRUE;
}

static void _rectangle_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);

	enesim_renderer_shape_cleanup(r, s);
	thiz->changed = EINA_FALSE;
//	if (thiz->stroke.paint)
//		enesim_renderer_sw_cleanup(thiz->stroke.paint);
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
			ENESIM_RENDERER_FLAG_ARGB8888;
}

static void _rectangle_free(Enesim_Renderer *r)
{
}

static void _rectangle_boundings(Enesim_Renderer *r, Enesim_Rectangle *boundings)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);

	boundings->x = 0;
	boundings->y = 0;
	boundings->w = thiz->width;
	boundings->h = thiz->height;
}

static Eina_Bool _rectangle_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	if (!thiz->changed) return EINA_FALSE;
	/* later we can have a real state of properties and check them
	 * here, agains the past and current states */
	return EINA_TRUE;
}

static Enesim_Renderer_Descriptor _rectangle_descriptor = {
	/* .version =    */ ENESIM_RENDERER_API,
	/* .name =       */ _rectangle_name,
	/* .free =       */ _rectangle_free,
	/* .boundings =  */ _rectangle_boundings,
	/* .flags =      */ _rectangle_flags,
	/* .is_inside =  */ NULL,
	/* .damage =     */ NULL,
	/* .has_changed =*/ _rectangle_has_changed,
	/* .sw_setup =   */ _rectangle_state_setup,
	/* .sw_cleanup = */ _rectangle_state_cleanup
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
	thiz->width = width;
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
	if (width) *width = thiz->width;
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
	thiz->height = height;
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
	if (height) *height = thiz->height;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rectangle_size_set(Enesim_Renderer *r, double width, double height)
{
	Enesim_Renderer_Rectangle *thiz;
	thiz = _rectangle_get(r);
	thiz->width = width;
	thiz->height = height;
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
	if (width) *width = thiz->width;
	if (height) *height = thiz->height;
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
	if (thiz->corner.radius == radius)
		return;
	thiz->corner.radius = radius;
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
	thiz->corner.tl = rounded;
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
	thiz->corner.tr = rounded;
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
	thiz->corner.bl = rounded;
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
	thiz->corner.br = rounded;
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

	if ((thiz->corner.tl == tl) && (thiz->corner.tr == tr) &&
	     (thiz->corner.bl == bl) && (thiz->corner.br == br))
		return;
	thiz->corner.tl = tl;  thiz->corner.tr = tr;
	thiz->corner.bl = bl;  thiz->corner.br = br;
	thiz->changed = EINA_TRUE;
}
