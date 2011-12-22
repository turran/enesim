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
#define ENESIM_RENDERER_GRID_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_GRID_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_GRID_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Grid
{
	EINA_MAGIC
	struct {
		Enesim_Color color;
		unsigned int w;
		unsigned int h;
	} inside, outside;
	/* the state */
	Enesim_F16p16_Matrix matrix;
	int wt;
	int ht;
	Eina_F16p16 wi;
	Eina_F16p16 hi;
	Eina_F16p16 wwt;
	Eina_F16p16 hht;
} Enesim_Renderer_Grid;

static inline Enesim_Renderer_Grid * _grid_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Grid *thiz;

	thiz = enesim_renderer_data_get(r);
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_GRID_MAGIC);

	return thiz;
}

static inline uint32_t _grid(Enesim_Renderer_Grid *thiz, Eina_F16p16 yy, Eina_F16p16 xx)
{
	Eina_F16p16 syy;
	int sy;
	uint32_t p0;

	/* normalize the modulo */
	syy = (yy % thiz->hht);
	if (syy < 0)
		syy += thiz->hht;
	/* simplest case, we are on the grid border
	 * the whole line will be outside color */
	sy = eina_f16p16_int_to(syy);
	if (syy >= thiz->hi)
	{
		p0 = thiz->outside.color;
	}
	else
	{
		Eina_F16p16 sxx;
		int sx;

		/* normalize the modulo */
		sxx = (xx % thiz->wwt);
		if (sxx < 0)
			sxx += thiz->wwt;

		sx = eina_f16p16_int_to(sxx);
		if (sxx >= thiz->wi)
		{
			p0 = thiz->outside.color;
		}
		/* totally inside */
		else
		{
			p0 = thiz->inside.color;
			/* antialias the inner square */
			if (sx == 0)
			{
				uint16_t a;

				a = 1 + ((sxx & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, p0, thiz->outside.color);
			}
			else if (sx == (thiz->inside.w - 1))
			{
				uint16_t a;

				a = 1 + ((sxx & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, thiz->outside.color, p0);

			}
			if (sy == 0)
			{
				uint16_t a;

				a = 1 + ((syy & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, p0, thiz->outside.color);
			}
			else if (sy == (thiz->inside.h - 1))
			{
				uint16_t a;

				a = 1 + ((syy & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, thiz->outside.color, p0);
			}
		}
	}
	return p0;
}


static void _span_identity(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Grid *thiz;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	int sy;
	Eina_F16p16 yy, xx;

	thiz = _grid_get(r);
#if 0
	enesim_renderer_identity_setup(r, x, y, &xx, &yy);
	while (dst < end)
	{
		uint32_t p0;

		p0 = _grid(thiz, yy, xx);

		xx += EINA_F16P16_ONE;
		*dst++ = p0;
	}
#else
	sy = (y % thiz->ht);
	if (sy < 0)
	{
		sy += thiz->ht;
	}
	/* simplest case, all the span is outside */
	if (sy >= thiz->inside.h)
	{
		while (dst < end)
			*dst++ = thiz->outside.color;
	}
	/* we swap between the two */
	else
	{
		while (dst < end)
		{
			int sx;

			sx = (x % thiz->wt);
			if (sx < 0)
				sx += thiz->wt;

			if (sx >= thiz->inside.w)
				*dst = thiz->outside.color;
			else
				*dst = thiz->inside.color;

			dst++;
			x++;
		}
	}
#endif
}

static void _span_affine(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Grid *thiz;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	int sy;
	Eina_F16p16 yy, xx;

	thiz = _grid_get(r);
	enesim_renderer_affine_setup(r, x, y, &thiz->matrix, &xx, &yy);

	while (dst < end)
	{
		uint32_t p0;

		p0 = _grid(thiz, yy, xx);

		yy += thiz->matrix.yx;
		xx += thiz->matrix.xx;
		*dst++ = p0;
	}
}

static void _span_projective(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Grid *thiz;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	int sy;
	Eina_F16p16 yy, xx, zz;

	thiz = _grid_get(r);
	enesim_renderer_projective_setup(r, x, y, &thiz->matrix, &xx, &yy, &zz);

	while (dst < end)
	{
		Eina_F16p16 syy, sxx;
		uint32_t p0;

		syy = ((((int64_t)yy) << 16) / zz);
		sxx = ((((int64_t)xx) << 16) / zz);

		p0 = _grid(thiz, syy, sxx);

		yy += thiz->matrix.yx;
		xx += thiz->matrix.xx;
		zz += thiz->matrix.zx;
		*dst++ = p0;
	}
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _grid_name(Enesim_Renderer *r)
{
	return "grid";
}

static void _state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{

}

static Eina_Bool _state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Grid *thiz;

	thiz = _grid_get(r);
	if (!thiz->inside.w || !thiz->inside.h || !thiz->outside.w || !thiz->outside.h)
		return EINA_FALSE;

	thiz->ht = thiz->inside.h + thiz->outside.h;
	thiz->wt = thiz->inside.w + thiz->outside.w;
	thiz->hht = eina_f16p16_int_from(thiz->ht);
	thiz->wwt = eina_f16p16_int_from(thiz->wt);

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

static void _grid_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Renderer_Grid *thiz;

	thiz = _grid_get(r);
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
	Enesim_Renderer_Grid *thiz;

	thiz = _grid_get(r);
	free(thiz);
}

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _grid_name,
	/* .free = 			*/ _free,
	/* .boundings = 		*/ NULL,
	/* .destination_transform = 	*/ NULL,
	/* .flags = 			*/ _grid_flags,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ NULL,
	/* .sw_setup = 			*/ _state_setup,
	/* .sw_cleanup = 		*/ _state_cleanup,
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
 * Creates a new grid renderer
 *
 * A grid renderer is composed of an inside box and an outside stroke.
 * Both, the inside and outside elements can be configurable through the
 * color, width and height.
 * @return The renderer
 */
EAPI Enesim_Renderer * enesim_renderer_grid_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Grid *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Grid));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_GRID_MAGIC);
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
	Enesim_Renderer_Grid *thiz;


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
	Enesim_Renderer_Grid *thiz;

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
	Enesim_Renderer_Grid *thiz;

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
	Enesim_Renderer_Grid *thiz;

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
	Enesim_Renderer_Grid *thiz;

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
	Enesim_Renderer_Grid *thiz;

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
	Enesim_Renderer_Grid *thiz;

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
	Enesim_Renderer_Grid *thiz;

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
	Enesim_Renderer_Grid *thiz;

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
	Enesim_Renderer_Grid *thiz;

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
	Enesim_Renderer_Grid *thiz;

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
	Enesim_Renderer_Grid *thiz;

	thiz = _grid_get(r);
	if (color) *color = thiz->outside.color;
}

