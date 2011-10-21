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
typedef struct _Enesim_Renderer_Stripes {
	struct {
		Enesim_Color color;
		double thickness;
	} s0, s1;

	int hh0, hh;
} Enesim_Renderer_Stripes;

static inline Enesim_Renderer_Stripes * _stripes_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = enesim_renderer_data_get(r);
	return thiz;
}

static void _span_projective(Enesim_Renderer *p, int x, int y,
		unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Stripes *thiz = _stripes_get(p);
	int hh = thiz->hh, hh0 = thiz->hh0, h0 = hh0 >> 16;
	unsigned int c0 = thiz->s0.color, c1 = thiz->s1.color;
	unsigned int *d = dst, *e = d + len;
	Eina_F16p16 yy, xx, zz;

	renderer_projective_setup(p, x, y, &xx, &yy, &zz);

	while (d < e)
	{
		Eina_F16p16 syy, syyy;
		unsigned int p0 = c0;
		int sy;

		syyy = ((((int64_t)yy) << 16) / zz);
		syy = (syyy % hh);

		if (syy < 0)
			syy += hh;
		sy = syy >> 16;

		if (sy == 0)
		{
			int a = 1 + ((syy & 0xffff) >> 8);

			p0 = argb8888_interp_256(a, c0, c1);
		}
		if (syy >= hh0)
		{
			p0 = c1;
			if (sy == h0)
			{
				int a = 1 + ((syy & 0xffff) >> 8);

				p0 = argb8888_interp_256(a, c1, c0);
			}
		}
		*d++ = p0;
		yy += p->matrix.values.yx;
		zz += p->matrix.values.zx;
	}
}

static void _span_affine(Enesim_Renderer *r, int x, int y,
		unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Stripes *thiz = _stripes_get(r);
	int ayx = r->matrix.values.yx, ayy = r->matrix.values.yy, ayz = r->matrix.values.yz;
	int hh = thiz->hh, hh0 = thiz->hh0, h0 = hh0 >> 16;
	unsigned int c0 = thiz->s0.color, c1 = thiz->s1.color;
	unsigned int *d = dst, *e = d + len;
	Eina_F16p16 yy, xx;

	renderer_affine_setup(r, x, y, &xx, &yy);
	while (d < e)
	{
		unsigned int p0 = c0;
		int syy = (yy % hh), sy;

		if (syy < 0)
			syy += hh;
		sy = syy >> 16;

		if (sy == 0)
		{
			int a = 1 + ((syy & 0xffff) >> 8);

			p0 = argb8888_interp_256(a, c0, c1);
		}
		if (syy >= hh0)
		{
			p0 = c1;
			if (sy == h0)
			{
				int a = 1 + ((syy & 0xffff) >> 8);

				p0 = argb8888_interp_256(a, c1, c0);
			}
		}
		*d++ = p0;
		yy += ayx;
	}
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _stripes_name(Enesim_Renderer *r)
{
	return "stripes";
}

static Eina_Bool _setup_state(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Stripes *thiz = _stripes_get(r);

	if (!thiz)
		return EINA_FALSE;
	thiz->hh0 = (int)(thiz->s0.thickness * 65536);
	thiz->hh = (int)(thiz->hh0 + (thiz->s1.thickness * 65536));

	if (r->matrix.type == ENESIM_MATRIX_IDENTITY)
		*fill = _span_affine;
	else if (r->matrix.type == ENESIM_MATRIX_AFFINE)
		*fill = _span_affine;
	else
		*fill = _span_projective;
	return EINA_TRUE;
}

static void _cleanup_state(Enesim_Renderer *p)
{
}

static void _stripes_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	if (!thiz)
	{
		*flags = 0;
		return;
	}

	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_PROJECTIVE |
			ENESIM_RENDERER_FLAG_ARGB8888;
}

static void _free(Enesim_Renderer *r)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	if (!thiz) return;
	free(thiz);
}

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version =    */ ENESIM_RENDERER_API,
	/* .name =       */ _stripes_name,
	/* .free =       */ _free,
	/* .boundings =  */ NULL,
	/* .flags =      */ _stripes_flags,
	/* .is_inside =  */ NULL,
	/* .damage =     */ NULL,
	/* .sw_setup =   */ _setup_state,
	/* .sw_cleanup = */ _cleanup_state
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a stripe renderer
 * @return The new renderer
 */
EAPI Enesim_Renderer * enesim_renderer_stripes_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Stripes *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Stripes));
	if (!thiz)
		return NULL;
	/* specific renderer setup */
	thiz->s0.thickness = 1;
	thiz->s1.thickness = 1;
	r = enesim_renderer_new(&_descriptor, thiz);

	return r;
}
/**
 * Sets the color of the even stripes
 * @param[in] r The stripes renderer
 * @param[in] color The even stripes color
 */
EAPI void enesim_renderer_stripes_even_color_set(Enesim_Renderer *r,
		Enesim_Color color)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	thiz->s0.color = color;
}
/**
 * Gets the color of the even stripes
 * @param[in] r The stripes renderer
 * @param[out] color The even stripes color
 */
EAPI void enesim_renderer_stripes_even_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	if (color) *color = thiz->s0.color;
}
/**
 * Sets the thickness of the even stripes
 * @param[in] r The stripes renderer
 * @param[in] thickness The even stripes thickness
 */
EAPI void enesim_renderer_stripes_even_thickness_set(Enesim_Renderer *r,
		double thickness)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	if (thickness < 0.99999)
		thickness = 1;
	thiz->s0.thickness = thickness;
}
/**
 * Gets the thickness of the even stripes
 * @param[in] r The stripes renderer
 * @param[out] thickness The even stripes thickness
 */
EAPI void enesim_renderer_stripes_even_thickness_get(Enesim_Renderer *r, double *thickness)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	if (thickness) *thickness = thiz->s0.thickness;
}
/**
 * Sets the color of the odd stripes
 * @param[in] r The stripes renderer
 * @param[in] color The odd stripes color
 */
EAPI void enesim_renderer_stripes_odd_color_set(Enesim_Renderer *r,
		Enesim_Color color)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	thiz->s1.color = color;
}
/**
 * Gets the color of the odd stripes
 * @param[in] r The stripes renderer
 * @param[out] color The odd stripes color
 */
EAPI void enesim_renderer_stripes_odd_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	if (color) *color = thiz->s1.color;
}
/**
 * Sets the thickness of the odd stripes
 * @param[in] r The stripes renderer
 * @param[in] thickness The odd stripes thickness
 */
EAPI void enesim_renderer_stripes_odd_thickness_set(Enesim_Renderer *r,
		double thickness)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	if (thickness < 0.99999)
		thickness = 1;
	thiz->s1.thickness = thickness;
}
/**
 * Gets the thickness of the odd stripes
 * @param[in] r The stripes renderer
 * @param[out] thickness The odd stripes thickness
 */
EAPI void enesim_renderer_stripes_odd_thickness_get(Enesim_Renderer *r, double *thickness)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	if (thickness) *thickness = thiz->s1.thickness;
}

