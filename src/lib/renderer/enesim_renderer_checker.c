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
/**
 * @todo
 * - Optimize the case where both colors are the same
 * - There's a bug on the affine renderer, easily reproducible whenever we scale
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Renderer_Checker_State
{
	Enesim_Color color1;
	Enesim_Color color2;
	int sw;
	int sh;
} Enesim_Renderer_Checker_State;

typedef struct _Enesim_Renderer_Checker
{
	/* properties */
	Enesim_Renderer_Checker_State current;
	Enesim_Renderer_Checker_State past;
	/* private */
	Eina_Bool changed : 1;
	Enesim_F16p16_Matrix matrix;
	Eina_F16p16 ww, hh;
	Eina_F16p16 ww2, hh2;
} Enesim_Renderer_Checker;

Enesim_Renderer_Checker * _checker_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Checker *thiz;

	thiz = enesim_renderer_data_get(r);
	return thiz;
}

static void _span_identity(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Checker *thiz;
	Eina_F16p16 yy, xx;
	int w2;
	int h2;
	uint32_t color[2];
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	int sy;

	thiz = _checker_get(r);
	w2 = thiz->current.sw * 2;
	h2 = thiz->current.sh * 2;
	color[0] = thiz->current.color1;
	color[1] = thiz->current.color2;

	/* translate to the origin */
	enesim_renderer_identity_setup(r, x, y, &xx, &yy);
	/* normalize the modulo */
	sy = ((yy  >> 16) % h2);
	if (sy < 0)
	{
		sy += h2;
	}
	/* swap the colors */
	if (sy >= thiz->current.sh)
	{
		color[0] = thiz->current.color2;
		color[1] = thiz->current.color1;
	}
	while (dst < end)
	{
		int sx;
		uint32_t p0;

		sx = ((xx >> 16) % w2);
		if (sx < 0)
		{
			sx += w2;
		}
		/* choose the correct color */
		if (sx >= thiz->current.sw)
		{
			p0 = color[0];
		}
		else
		{
			p0 = color[1];
		}
		*dst++ = p0;
		xx += EINA_F16P16_ONE;
	}
}

static void _span_affine(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Checker *thiz;
	Eina_F16p16 yy, xx, ww, hh, ww2, hh2;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;

	thiz = _checker_get(r);
	enesim_renderer_affine_setup(r, x, y, &thiz->matrix, &xx, &yy);
	ww = thiz->ww;
	ww2 = thiz->ww2;
	hh = thiz->hh;
	hh2 = thiz->hh2;

	while (dst < end)
	{
		Eina_F16p16 syy, sxx;
		uint32_t color[2] = {thiz->current.color1, thiz->current.color2};
		uint32_t p0;
		int sx, sy;

		/* normalize the modulo */
		syy = (yy % hh2);
		if (syy < 0)
		{
			syy += hh2;
		}
		sxx = (xx % ww2);
		if (sxx < 0)
		{
			sxx += ww2;
		}
		sy = eina_f16p16_int_to(syy);
		sx = eina_f16p16_int_to(sxx);
		/* choose the correct color */
		if (syy >= hh)
		{
			color[0] = thiz->current.color2;
			color[1] = thiz->current.color1;
		}
		if (sxx >= ww)
		{
			p0 = color[0];
			/* antialias the borders */
			if (sy == 0 || sy == thiz->current.sh)
			{
				uint16_t a;

				a = 1 + ((syy & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, p0, color[1]);
			}
			if (sx == 0 || sx == thiz->current.sw)
			{
				uint16_t a;

				a = 1 + ((sxx & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, p0, color[1]);
			}
		}
		else
		{
			p0 = color[1];

			/* antialias the borders */
			if (sy == 0 || sy == thiz->current.sh)
			{
				uint16_t a;

				a = 1 + ((syy & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, p0, color[0]);
			}
			if (sx == 0 || sx == thiz->current.sw)
			{
				uint16_t a;

				a = 1 + ((sxx & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, p0, color[0]);
			}
		}
		yy += thiz->matrix.yx;
		xx += thiz->matrix.xx;
		*dst++ = p0;
	}
}

static void _span_projective(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Checker *thiz;
	Eina_F16p16 yy, xx, zz, ww, hh, ww2, hh2;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;

	thiz = _checker_get(r);
	/* translate to the origin */
	enesim_renderer_projective_setup(r, x, y, &thiz->matrix, &xx, &yy, &zz);
	ww = thiz->ww;
	ww2 = thiz->ww2;
	hh = thiz->hh;
	hh2 = thiz->hh2;

	while (dst < end)
	{
		Eina_F16p16 syy, sxx, syyy, sxxx;
		uint32_t color[2] = {thiz->current.color1, thiz->current.color2};
		uint32_t p0;
		int sx, sy;

		syyy = ((((int64_t)yy) << 16) / zz);
		sxxx = ((((int64_t)xx) << 16) / zz);
		/* normalize the modulo */
		syy = (syyy % hh2);
		if (syy < 0)
		{
			syy += hh2;
		}
		sxx = (sxxx % ww2);
		if (sxx < 0)
		{
			sxx += ww2;
		}
		sy = eina_f16p16_int_to(syy);
		sx = eina_f16p16_int_to(sxx);
		/* choose the correct color */
		if (syy >= hh)
		{
			color[0] = thiz->current.color2;
			color[1] = thiz->current.color1;
		}
		if (sxx >= ww)
		{
			p0 = color[0];

			/* antialias the borders */
			if (sy == 0 || sy == thiz->current.sh)
			{
				uint16_t a;

				a = 1 + ((syy & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, p0, color[1]);
			}
			if (sx == 0 || sx == thiz->current.sw)
			{
				uint16_t a;

				a = 1 + ((sxx & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, p0, color[1]);
			}
		}
		else
		{
			p0 = color[1];
			/* antialias the borders */
			if (sy == 0 || sy == thiz->current.sh)
			{
				uint16_t a;

				a = 1 + ((syy & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, p0, color[0]);
			}
			if (sx == 0 || sx == thiz->current.sw)
			{
				uint16_t a;

				a = 1 + ((sxx & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, p0, color[0]);
			}
		}
		yy += thiz->matrix.yx;
		xx += thiz->matrix.xx;
		zz += thiz->matrix.zx;
		*dst++ = p0;
	}
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _checker_name(Enesim_Renderer *r)
{
	return "checker";
}

static void _checker_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Checker *thiz;

	thiz = _checker_get(r);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

static Eina_Bool _checker_state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Checker *thiz;

	thiz = _checker_get(r);
	thiz->ww = eina_f16p16_int_from(thiz->current.sw);
	thiz->ww2 = thiz->ww * 2;
	thiz->hh = eina_f16p16_int_from(thiz->current.sh);
	thiz->hh2 = thiz->hh * 2;

	switch (state->transformation_type)
	{
		case ENESIM_MATRIX_IDENTITY:
		*fill = _span_identity;
		break;

		case ENESIM_MATRIX_AFFINE:
		enesim_matrix_f16p16_matrix_to(&state->transformation,
				&thiz->matrix);
		*fill = _span_affine;
		break;

		case ENESIM_MATRIX_PROJECTIVE:
		enesim_matrix_f16p16_matrix_to(&state->transformation,
				&thiz->matrix);
		*fill = _span_projective;
		break;

		default:
		return EINA_FALSE;
	}

	return EINA_TRUE;
}

static void _checker_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Checker *thiz;

	thiz = _checker_get(r);
	free(thiz);
}

static Eina_Bool _checker_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Checker *thiz;

	thiz = _checker_get(r);
	if (!thiz->changed) return EINA_FALSE;

	if (thiz->current.color1 != thiz->past.color1)
		return EINA_TRUE;
	if (thiz->current.color2 != thiz->past.color2)
		return EINA_TRUE;
	if (thiz->current.sw != thiz->past.sw)
		return EINA_TRUE;
	if (thiz->current.sh != thiz->past.sh)
		return EINA_TRUE;

	return EINA_FALSE;
}

static void _checker_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Renderer_Checker *thiz;

	thiz = _checker_get(r);
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

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _checker_name,
	/* .free = 			*/ _checker_free,
	/* .boundings = 		*/ NULL,
	/* .destination_transform = 	*/ NULL,
	/* .flags = 			*/ _checker_flags,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _checker_has_changed,
	/* .sw_setup = 			*/ _checker_state_setup,
	/* .sw_cleanup = 		*/ _checker_state_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a checker renderer
 * @return The new renderer
 */
EAPI Enesim_Renderer * enesim_renderer_checker_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Checker *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Checker));
	if (!thiz) return NULL;

	/* specific renderer setup */
	thiz->current.sw = 1;
	thiz->current.sh = 1;
	r = enesim_renderer_new(&_descriptor, thiz);

	return r;
}
/**
 * Sets the color of the even squares
 * @param[in] r The checker renderer
 * @param[in] color The color
 */
EAPI void enesim_renderer_checker_even_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Checker *thiz;

	thiz = _checker_get(r);

	thiz->current.color1 = color;
	thiz->changed = EINA_TRUE;
}
/**
 * Gets the color of the even squares
 * @param[in] r The checker renderer
 * @return The color
 */
EAPI void enesim_renderer_checker_even_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Enesim_Renderer_Checker *thiz;

	thiz = _checker_get(r);
	if (color) *color = thiz->current.color1;
}
/**
 * Sets the color of the odd squares
 * @param[in] r The checker renderer
 * @param[in] color The color
 */
EAPI void enesim_renderer_checker_odd_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Checker *thiz;

	thiz = _checker_get(r);

	thiz->current.color2 = color;
	thiz->changed = EINA_TRUE;
}
/**
 * Gets the color of the odd squares
 * @param[in] r The checker renderer
 * @return The color
 */
EAPI void enesim_renderer_checker_odd_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Enesim_Renderer_Checker *thiz;

	thiz = _checker_get(r);
	if (color) *color = thiz->current.color2;
}
/**
 * Sets the width of the checker rectangles
 * @param[in] width The width
 */
EAPI void enesim_renderer_checker_width_set(Enesim_Renderer *r, int width)
{
	Enesim_Renderer_Checker *thiz;

	thiz = _checker_get(r);

	thiz->current.sw = width;
	thiz->changed = EINA_TRUE;
}
/**
 * Gets the width of the checker rectangles
 * @returns The width
 */
EAPI void enesim_renderer_checker_width_get(Enesim_Renderer *r, int *width)
{
	Enesim_Renderer_Checker *thiz;

	thiz = _checker_get(r);
	if (width) *width = thiz->current.sw;
}
/**
 * Sets the height of the checker rectangles
 * @param[in] height The height
 */
EAPI void enesim_renderer_checker_height_set(Enesim_Renderer *r, int height)
{
	Enesim_Renderer_Checker *thiz;

	thiz = _checker_get(r);

	thiz->current.sh = height;
	thiz->changed = EINA_TRUE;
}
/**
 * Gets the height of the checker rectangles
 * @returns The height
 */
EAPI void enesim_renderer_checker_heigth_get(Enesim_Renderer *r, int *height)
{
	Enesim_Renderer_Checker *thiz;

	thiz = _checker_get(r);
	if (height) *height = thiz->current.sh;
}

