/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2008 Jorge Luis Zapata
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
	int w, h;
	struct {
		double radius;
		Eina_Bool tl : 1;
		Eina_Bool tr : 1;
		Eina_Bool bl : 1;
		Eina_Bool br : 1;
	} corner;
	/* internal state */
	int lxx0, rxx0;
	int tyy0, byy0;
	int rr0, irr0;
	int sw;
	unsigned char do_inner :1;
} Enesim_Renderer_Rectangle;

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

static void _span_norounded_nooutlined_paint_filled_identity(Enesim_Renderer *r, int x,
		int y, unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Rectangle *thiz;

}

static void _span_rounded_color_outlined_paint_filled_affine(Enesim_Renderer *r, int x, int y,
		unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Rectangle *thiz;
	Enesim_Shape_Draw_Mode draw_mode;
	int sw = thiz->w, sh = thiz->h;
	int axx = r->matrix.values.xx, axy = r->matrix.values.xy, axz = r->matrix.values.xz;
	int ayx = r->matrix.values.yx, ayy = r->matrix.values.yy, ayz = r->matrix.values.yz;
	int do_inner = thiz->do_inner;
	unsigned int ocolor;
	unsigned int icolor;
	int stw = thiz->sw;
	int rr0 = thiz->rr0, rr1 = rr0 + 65536;
	int irr0 = thiz->irr0, irr1 = irr0 + 65536;
	int lxx0 = thiz->lxx0, rxx0 = thiz->rxx0;
	int tyy0 = thiz->tyy0, byy0 = thiz->byy0;
	Enesim_Renderer *fpaint;
	unsigned int *d = dst, *e = d + len;
	int xx, yy;
	int fill_only = 0;
	char bl = thiz->corner.bl, br = thiz->corner.br, tl = thiz->corner.tl, tr = thiz->corner.tr;

	enesim_renderer_shape_outline_color_get(r, &ocolor);
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
			fpaint->sw_fill(fpaint, x, y, len, dst);
	}
	if ((draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL) && do_inner && fpaint)
		fpaint->sw_fill(fpaint, x, y, len, dst);

        renderer_affine_setup(r, x, y, &xx, &yy);

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
		*d++ = q0;
		xx += axx;
		yy += ayx;
	}
}

static void _span_rounded_color_outlined_paint_filled_proj(Enesim_Renderer *r, int x, int y,
		unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Rectangle *thiz;
	Enesim_Shape_Draw_Mode draw_mode;
	Enesim_Color ocolor;
	Enesim_Color icolor;
	int sw = thiz->w, sh = thiz->h;
	int axx = r->matrix.values.xx, axy = r->matrix.values.xy, axz = r->matrix.values.xz;
	int ayx = r->matrix.values.yx, ayy = r->matrix.values.yy, ayz = r->matrix.values.yz;
	int azx = r->matrix.values.zx, azy = r->matrix.values.zy, azz = r->matrix.values.zz;
	int do_inner = thiz->do_inner;
	int stw = thiz->sw;
	int rr0 = thiz->rr0, rr1 = rr0 + 65536;
	int irr0 = thiz->irr0, irr1 = irr0 + 65536;
	int lxx0 = thiz->lxx0, rxx0 = thiz->rxx0;
	int tyy0 = thiz->tyy0, byy0 = thiz->byy0;
	Enesim_Renderer *fpaint;
	unsigned int *d = dst, *e = d + len;
	int xx, yy, zz;
	int fill_only = 0;
	char bl = thiz->corner.bl, br = thiz->corner.br, tl = thiz->corner.tl, tr = thiz->corner.tr;

	enesim_renderer_shape_outline_color_get(r, &ocolor);
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
		if (fpaint)  fpaint->sw_fill(fpaint, x, y, len, dst);
	}
	if ((draw_mode == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL) && do_inner && fpaint)
		fpaint->sw_fill(fpaint, x, y, len, dst);
	renderer_projective_setup(r, x, y, &xx, &yy, &zz);
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
static Eina_Bool _state_setup(Enesim_Renderer *r, Enesim_Renderer_Sw_Fill *fill)
{
	Enesim_Renderer_Rectangle *thiz;
	double rad;
	double sw;

	thiz = _rectangle_get(r);
	if (!thiz || (thiz->w < 1) || (thiz->h < 1))
	{
		WRN("Invalid size %d %d", thiz->w, thiz->h);
		return EINA_FALSE;
	}

	rad = thiz->corner.radius;
	if (rad > (thiz->w / 2.0))
		rad = thiz->w / 2.0;
	if (rad > (thiz->h / 2.0))
		rad = thiz->h / 2.0;

	thiz->rr0 = rad * 65536;
	thiz->lxx0 = thiz->tyy0 = thiz->rr0;
	thiz->rxx0 = (thiz->w - rad - 1) * 65536;
	thiz->byy0 = (thiz->h - rad - 1) * 65536;

	enesim_renderer_shape_outline_weight_get(r, &sw);
	thiz->do_inner = 1;
	if ((sw >= (thiz->w / 2.0)) || (sw >= (thiz->h / 2.0)))
	{
		sw = 0;
		thiz->do_inner = 0;
	}
	rad = rad - sw;
	if (rad < 0.0039)
		rad = 0;

	thiz->irr0 = rad * 65536;
	thiz->sw = sw;

	if (!enesim_renderer_shape_sw_setup(r))
	{
		printf("thiz cannot setup 2\n");
		return EINA_FALSE;
	}

	*fill = _span_rounded_color_outlined_paint_filled_proj;
	if (r->matrix.type == ENESIM_MATRIX_AFFINE || r->matrix.type == ENESIM_MATRIX_IDENTITY)
		*fill = _span_rounded_color_outlined_paint_filled_affine;

	return EINA_TRUE;
}

static void _state_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Rectangle *thiz;

	enesim_renderer_shape_sw_cleanup(r);
//	if (thiz->stroke.paint)
//		enesim_renderer_sw_cleanup(thiz->stroke.paint);
}

static void _free(Enesim_Renderer *r)
{
}

static void _boundings(Enesim_Renderer *r, Enesim_Rectangle *thiz)
{
	Enesim_Renderer_Rectangle *rct = (Enesim_Renderer_Rectangle *)r;

	thiz->x = 0;
	thiz->y = 0;
	thiz->w = rct->w;
	thiz->h = rct->h;
}

static Enesim_Renderer_Descriptor _rectangle_descriptor = {
	/* .sw_setup =   */ _state_setup,
	/* .sw_cleanup = */ _state_cleanup,
	/* .free =       */ _free,
	/* .boundings =  */ _boundings,
	/* .flags =      */ NULL,
	/* .is_inside =  */ NULL
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
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rectangle_width_set(Enesim_Renderer *r, unsigned int w)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	thiz->w = w;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rectangle_width_get(Enesim_Renderer *r, unsigned int *w)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	if (w) *w = thiz->w;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rectangle_height_set(Enesim_Renderer *r, unsigned int h)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	thiz->h = h;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rectangle_height_get(Enesim_Renderer *r, unsigned int *h)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	if (h) *h = thiz->h;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rectangle_size_set(Enesim_Renderer *r, unsigned int w, unsigned int h)
{
	Enesim_Renderer_Rectangle *thiz;
	thiz = _rectangle_get(r);
	thiz->w = w;
	thiz->h = h;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rectangle_size_get(Enesim_Renderer *r, unsigned int *w, unsigned int *h)
{
	Enesim_Renderer_Rectangle *thiz;

	thiz = _rectangle_get(r);
	if (w) *w = thiz->w;
	if (h) *h = thiz->h;
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
}
