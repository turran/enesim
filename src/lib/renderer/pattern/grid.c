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
typedef struct _Grid
{
	struct {
		Enesim_Color color;
		unsigned int w;
		unsigned int h;
	} inside, outside;
	/* the state */
	int wt;
	int ht;
	Eina_F16p16 wi;
	Eina_F16p16 hi;
	Eina_F16p16 wwt;
	Eina_F16p16 hht;
} Grid;

static inline Grid * _grid_get(Enesim_Renderer *r)
{
	Grid *thiz;

	thiz = enesim_renderer_data_get(r);
	return thiz;
}

static inline uint32_t _grid(Grid *g, Eina_F16p16 yy, Eina_F16p16 xx)
{
	Eina_F16p16 syy;
	int sy;
	uint32_t p0;

	/* normalize the modulo */
	syy = (yy % g->hht);
	if (syy < 0)
		syy += g->hht;
	/* simplest case, we are on the grid border
	 * the whole line will be outside color */
	sy = eina_f16p16_int_to(syy);
	if (syy >= g->hi)
	{
		p0 = g->outside.color;
	}
	else
	{
		Eina_F16p16 sxx;
		int sx;

		/* normalize the modulo */
		sxx = (xx % g->wwt);
		if (sxx < 0)
			sxx += g->wwt;

		sx = eina_f16p16_int_to(sxx);
		if (sxx >= g->wi)
		{
			p0 = g->outside.color;
		}
		/* totally inside */
		else
		{
			p0 = g->inside.color;
			/* antialias the inner square */
			if (sx == 0)
			{
				uint16_t a;

				a = 1 + ((sxx & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, p0, g->outside.color);
			}
			else if (sx == (g->inside.w - 1))
			{
				uint16_t a;

				a = 1 + ((sxx & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, g->outside.color, p0);

			}
			if (sy == 0)
			{
				uint16_t a;

				a = 1 + ((syy & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, p0, g->outside.color);
			}
			else if (sy == (g->inside.h - 1))
			{
				uint16_t a;

				a = 1 + ((syy & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, g->outside.color, p0);
			}
		}
	}
	return p0;
}


static void _span_identity(Enesim_Renderer *r, int x, int y, unsigned int len, uint32_t *dst)
{
	Grid *g;
	uint32_t *end = dst + len;
	int sy;
	Eina_F16p16 yy, xx;

	g = _grid_get(r);
#if 0
	renderer_identity_setup(r, x, y, &xx, &yy);
	while (dst < end)
	{
		uint32_t p0;

		p0 = _grid(g, yy, xx);

		xx += EINA_F16P16_ONE;
		*dst++ = p0;
	}
#else
	sy = (y % g->ht);
	if (sy < 0)
	{
		sy += g->ht;
	}
	/* simplest case, all the span is outside */
	if (sy >= g->inside.h)
	{
		while (dst < end)
			*dst++ = g->outside.color;
	}
	/* we swap between the two */
	else
	{
		while (dst < end)
		{
			int sx;

			sx = (x % g->wt);
			if (sx < 0)
				sx += g->wt;

			if (sx >= g->inside.w)
				*dst = g->outside.color;
			else
				*dst = g->inside.color;

			dst++;
			x++;
		}
	}
#endif
}

static void _span_affine(Enesim_Renderer *r, int x, int y, unsigned int len, uint32_t *dst)
{
	Grid *g;
	uint32_t *end = dst + len;
	int sy;
	Eina_F16p16 yy, xx;

	g = _grid_get(r);
	renderer_affine_setup(r, x, y, &xx, &yy);

	while (dst < end)
	{
		uint32_t p0;

		p0 = _grid(g, yy, xx);

		yy += r->matrix.values.yx;
		xx += r->matrix.values.xx;
		*dst++ = p0;
	}
}

static void _span_projective(Enesim_Renderer *r, int x, int y, unsigned int len, uint32_t *dst)
{
	Grid *g;
	uint32_t *end = dst + len;
	int sy;
	Eina_F16p16 yy, xx, zz;

	g = _grid_get(r);
	renderer_projective_setup(r, x, y, &xx, &yy, &zz);

	while (dst < end)
	{
		Eina_F16p16 syy, sxx;
		uint32_t p0;

		syy = ((((int64_t)yy) << 16) / zz);
		sxx = ((((int64_t)xx) << 16) / zz);

		p0 = _grid(g, syy, sxx);

		yy += r->matrix.values.yx;
		xx += r->matrix.values.xx;
		zz += r->matrix.values.zx;
		*dst++ = p0;
	}
}

static void _state_cleanup(Enesim_Renderer *r)
{

}

static Eina_Bool _state_setup(Enesim_Renderer *r, Enesim_Renderer_Sw_Fill *fill)
{
	Grid *g;

	g = _grid_get(r);
	if (!g->inside.w || !g->inside.h || !g->outside.w || !g->outside.h)
		return EINA_FALSE;

	g->ht = g->inside.h + g->outside.h;
	g->wt = g->inside.w + g->outside.w;
	g->hht = eina_f16p16_int_from(g->ht);
	g->wwt = eina_f16p16_int_from(g->wt);

	if (r->matrix.type == ENESIM_MATRIX_IDENTITY)
		//*fill = _span_identity;
		*fill = _span_affine;
	else if (r->matrix.type == ENESIM_MATRIX_AFFINE)
		*fill = _span_affine;
	else
		*fill = _span_projective;
	return EINA_TRUE;
}

static void _grid_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Grid *thiz;

	thiz = _grid_get(r);
	if (!thiz)
	{
		*flags = 0;
		return;
	}

	*flags = ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_PERSPECTIVE |
			ENESIM_RENDERER_FLAG_ARGB8888;
}

static void _free(Enesim_Renderer *r)
{
	Grid *thiz;

	thiz = _grid_get(r);
	free(thiz);
}

static Enesim_Renderer_Descriptor _descriptor = {
	.sw_setup = _state_setup,
	.sw_cleanup = _state_cleanup,
	.flags = _grid_flags,
	.free = _free,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a new grid renderer
 *
 * A grid renderer is composed of an inside box and an outside outline.
 * Both, the inside and outside elements can be configurable through the
 * color, width and height.
 * @return The renderer
 */
EAPI Enesim_Renderer * enesim_renderer_grid_new(void)
{
	Enesim_Renderer *r;
	Grid *thiz;

	thiz = calloc(1, sizeof(Grid));
	/* specific renderer setup */
	thiz->inside.w = 1;
	thiz->inside.h = 1;
	thiz->outside.w = 1;
	thiz->outside.h = 1;
	/* common renderer setup */
	r = enesim_renderer_new(&_descriptor, thiz);

	return r;
}
/**
 * Sets the width of the inner box of a grid renderer
 * @param[in] r The grid renderer
 * @param[in] width The width
 */
EAPI void enesim_renderer_grid_inside_width_set(Enesim_Renderer *r, unsigned int width)
{
	Grid *thiz;


	thiz = _grid_get(r);
	thiz->inside.w = width;
	thiz->wi = eina_f16p16_int_from(width);
}
/**
 * Gets the width of the inner box of a grid renderer
 * @param[in] r The grid renderer
 * @return The width
 */
EAPI void enesim_renderer_grid_inside_width_get(Enesim_Renderer *r, unsigned int *iw)
{
	Grid *thiz;

	thiz = _grid_get(r);
	if (iw) *iw = thiz->inside.w;
}
/**
 * Sets the height of the inner box of a grid renderer
 * @param[in] r The grid renderer
 * @param[in] height The height
 */
EAPI void enesim_renderer_grid_inside_height_set(Enesim_Renderer *r, unsigned int height)
{
	Grid *thiz;

	thiz = _grid_get(r);
	thiz->inside.h = height;
	thiz->hi = eina_f16p16_int_from(height);
}
/**
 * Gets the height of the inner box of a grid renderer
 * @param[in] r The grid renderer
 * @return The height
 */
EAPI void enesim_renderer_grid_inside_height_get(Enesim_Renderer *r, unsigned int *ih)
{
	Grid *thiz;

	thiz = _grid_get(r);
	if (ih) *ih = thiz->inside.h;
}
/**
 * Sers the color of the inner box of a grid renderer
 * @param[in] r The grid renderer
 * @param[in] color The color
 */
EAPI void enesim_renderer_grid_inside_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Grid *thiz;

	thiz = _grid_get(r);
	thiz->inside.color = color;
}
/**
 * Gets the color of the inner box of a grid renderer
 * @param[in] r The grid renderer
 * @return The color
 */
EAPI void enesim_renderer_grid_inside_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Grid *thiz;

	thiz = _grid_get(r);
	if (color) *color = thiz->inside.color;
}
/**
 * Sets the horizontal thickness of the border of a grid renderer
 * @param[in] r The grid renderer
 * @param[in] hthickness The horizontal thickness
 */
EAPI void enesim_renderer_grid_border_hthickness_set(Enesim_Renderer *r, unsigned int hthickness)
{
	Grid *thiz;

	thiz = _grid_get(r);
	thiz->outside.h = hthickness;
}
/**
 * Gets the horizontal thickness of the border of a grid renderer
 * @param[in] r The grid renderer
 * @return The horizontal thickness
 */
EAPI void enesim_renderer_grid_border_hthickness_get(Enesim_Renderer *r, unsigned int *h)
{
	Grid *thiz;

	thiz = _grid_get(r);
	if (h) *h = thiz->outside.h;
}
/**
 * Sets the vertical thickness of the border of a grid renderer
 * @param[in] r The grid renderer
 * @param[in] vthickness The vertical thickness
 */
EAPI void enesim_renderer_grid_border_vthickness_set(Enesim_Renderer *r, unsigned int vthickness)
{
	Grid *thiz;

	thiz = _grid_get(r);
	thiz->outside.w = vthickness;
}
/**
 * Gets the vertical thickness of the border of a grid renderer
 * @param[in] r The grid renderer
 * @return The vertical thickness
 */
EAPI void enesim_renderer_grid_border_vthickness_get(Enesim_Renderer *r, unsigned int *v)
{
	Grid *thiz;

	thiz = _grid_get(r);
	if (v) *v = thiz->outside.w;
}
/**
 * Sets the color of the border of a grid renderer
 * @param[in] r The grid renderer
 * @param[in] color The color
 */
EAPI void enesim_renderer_grid_border_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Grid *thiz;

	thiz = _grid_get(r);
	thiz->outside.color = color;
}
/**
 * Gets the color of the border of a grid renderer
 * @param[in] r The grid renderer
 * @return The color
 */
EAPI void enesim_renderer_grid_border_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Grid *thiz;

	thiz = _grid_get(r);
	if (color) *color = thiz->outside.color;
}

