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
#define ENESIM_RENDERER_STRIPES_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_STRIPES_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_STRIPES_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Stripes_State {
	struct {
		Enesim_Color color;
		double thickness;
	} s0, s1;
} Enesim_Renderer_Stripes_State;

typedef struct _Enesim_Renderer_Stripes {
	EINA_MAGIC
	/* properties */
	Enesim_Renderer_Stripes_State current;
	Enesim_Renderer_Stripes_State past;
	/* private */
	Eina_Bool changed : 1;
	int hh0, hh;
	Enesim_F16p16_Matrix matrix;
} Enesim_Renderer_Stripes;

static inline Enesim_Renderer_Stripes * _stripes_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = enesim_renderer_data_get(r);
	ENESIM_RENDERER_STRIPES_MAGIC_CHECK(thiz);

	return thiz;
}

static void _span_projective(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Stripes *thiz = _stripes_get(r);
	int hh = thiz->hh, hh0 = thiz->hh0, h0 = hh0 >> 16;
	unsigned int c0 = thiz->current.s0.color, c1 = thiz->current.s1.color;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Eina_F16p16 yy, xx, zz;

	enesim_renderer_projective_setup(r, x, y, &thiz->matrix, &xx, &yy, &zz);

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
		yy += thiz->matrix.yx;
		zz += thiz->matrix.zx;
	}
}

static void _span_affine(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Stripes *thiz = _stripes_get(r);
	int ayx = thiz->matrix.yx;
	int hh = thiz->hh, hh0 = thiz->hh0, h0 = hh0 >> 16;
	unsigned int c0 = thiz->current.s0.color, c1 = thiz->current.s1.color;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Eina_F16p16 yy, xx;

	enesim_renderer_affine_setup(r, x, y, &thiz->matrix, &xx, &yy);
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

static Eina_Bool _setup_state(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Stripes *thiz = _stripes_get(r);

	if (!thiz)
		return EINA_FALSE;
	thiz->hh0 = (int)(thiz->current.s0.thickness * 65536);
	thiz->hh = (int)(thiz->hh0 + (thiz->current.s1.thickness * 65536));

	enesim_matrix_f16p16_matrix_to(&state->transformation,
			&thiz->matrix);
	switch (state->transformation_type)
	{
		case ENESIM_MATRIX_IDENTITY:
		case ENESIM_MATRIX_AFFINE:
		*fill = _span_affine;
		break;

		case ENESIM_MATRIX_PROJECTIVE:
		*fill = _span_projective;
		break;

		default:
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

static void _cleanup_state(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
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

static Eina_Bool _stripes_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	if (!thiz->changed) return EINA_FALSE;

	if (thiz->current.s0.color != thiz->past.s0.color)
	{
		return EINA_TRUE;
	}
	if (thiz->current.s0.thickness != thiz->past.s0.thickness)
	{
		return EINA_TRUE;
	}
	if (thiz->current.s1.color != thiz->past.s1.color)
	{
		return EINA_TRUE;
	}
	if (thiz->current.s1.thickness != thiz->past.s1.thickness)
	{
		return EINA_TRUE;
	}

	return EINA_FALSE;
}

static void _free(Enesim_Renderer *r)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	if (!thiz) return;
	free(thiz);
}

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _stripes_name,
	/* .free = 			*/ _free,
	/* .boundings = 		*/ NULL,
	/* .destination_transform = 	*/ NULL,
	/* .flags = 			*/ _stripes_flags,
	/* .is_inside = 		*/ NULL,
	/* .damage =			*/ NULL,
	/* .has_changed = 		*/ _stripes_has_changed,
	/* .sw_setup =			*/ _setup_state,
	/* .sw_cleanup = 		*/ _cleanup_state,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
	/* .opengl_setup =          	*/ NULL,
	/* .opengl_shader_setup = 	*/ NULL,
	/* .opengl_cleanup =        	*/ NULL
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
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_STRIPES_MAGIC);
	/* specific renderer setup */
	thiz->current.s0.thickness = 1;
	thiz->current.s1.thickness = 1;
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
	thiz->current.s0.color = color;
	thiz->changed = EINA_TRUE;
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
	if (color) *color = thiz->current.s0.color;
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
	thiz->current.s0.thickness = thickness;
	thiz->changed = EINA_TRUE;
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
	if (thickness) *thickness = thiz->current.s0.thickness;
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
	thiz->current.s1.color = color;
	thiz->changed = EINA_TRUE;
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
	if (color) *color = thiz->current.s1.color;
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
	thiz->current.s1.thickness = thickness;
	thiz->changed = EINA_TRUE;
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
	if (thickness) *thickness = thiz->current.s1.thickness;
}

