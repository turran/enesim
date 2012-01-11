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
		Enesim_Renderer *paint;
		double thickness;
	} even, odd;
} Enesim_Renderer_Stripes_State;

typedef struct _Enesim_Renderer_Stripes {
	EINA_MAGIC
	/* properties */
	Enesim_Renderer_Stripes_State current;
	Enesim_Renderer_Stripes_State past;
	/* private */
	Enesim_Renderer_State old_odd;
	Enesim_Renderer_State old_even;
	Eina_Bool changed : 1;
	Enesim_Color final_color1;
	Enesim_Color final_color2;
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
	Enesim_Color c0 = thiz->final_color1;
	Enesim_Color c1 = thiz->final_color2;
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

static void _span_projective_paints(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Stripes *thiz = _stripes_get(r);
	int hh = thiz->hh, hh0 = thiz->hh0, h0 = hh0 >> 16;
	Enesim_Color c0 = thiz->final_color1;
	Enesim_Color c1 = thiz->final_color2;
	Enesim_Renderer *opaint, *epaint;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Eina_F16p16 yy, xx, zz;
	Enesim_Renderer_Sw_Data *pdata;
	unsigned int *sbuf, *s = NULL;

	opaint = thiz->current.odd.paint;
	epaint = thiz->current.even.paint;
	if (epaint)
	{
		pdata = enesim_renderer_backend_data_get(epaint, ENESIM_BACKEND_SOFTWARE);
		pdata->fill(epaint, x, y, len, dst);
	}
	if (opaint)
	{
		sbuf = alloca(len * sizeof(unsigned int));
		pdata = enesim_renderer_backend_data_get(opaint, ENESIM_BACKEND_SOFTWARE);
		pdata->fill(opaint, x, y, len, sbuf);
		s = sbuf;
	}

	enesim_renderer_projective_setup(r, x, y, &thiz->matrix, &xx, &yy, &zz);
	while (d < e)
	{
		Eina_F16p16 syy, syyy;
		unsigned int p0 = c0, p1 = c1;
		int sy;

		syyy = ((((int64_t)yy) << 16) / zz);
		syy = (syyy % hh);

		if (syy < 0)
			syy += hh;
		sy = syy >> 16;

		if (epaint)
		{
			p0 = *d;
			if (c0 != 0xffffffff)
				p0 = argb8888_mul4_sym(p0, c0);
		}
		if (sy == 0)
		{
			int a = 1 + ((syy & 0xffff) >> 8);

			if (opaint)
			{
				p1 = *s;
				if (c1 != 0xffffffff)
					p1 = argb8888_mul4_sym(p1, c1);
			}
			p0 = argb8888_interp_256(a, p0, p1);
		}
		if (syy >= hh0)
		{
			if (opaint)
			{
				p1 = *s;
				if (c1 != 0xffffffff)
					p1 = argb8888_mul4_sym(p1, c1);
			}
			if (sy == h0)
			{
				int a = 1 + ((syy & 0xffff) >> 8);

				p1 = argb8888_interp_256(a, p1, p0);
			}
			p0 = p1;

		}
		*d++ = p0;
		if (s) s++;
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
	Enesim_Color c0 = thiz->final_color1;
	Enesim_Color c1 = thiz->final_color2;
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

static void _span_affine_paints(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Stripes *thiz = _stripes_get(r);
	int ayx = thiz->matrix.yx;
	int hh = thiz->hh, hh0 = thiz->hh0, h0 = hh0 >> 16;
	Enesim_Color c0 = thiz->final_color1;
	Enesim_Color c1 = thiz->final_color2;
	Enesim_Renderer *opaint, *epaint;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Eina_F16p16 yy, xx;
	Enesim_Renderer_Sw_Data *pdata;
	unsigned int *sbuf, *s = NULL;

	opaint = thiz->current.odd.paint;
	epaint = thiz->current.even.paint;
	if (epaint)
	{
		pdata = enesim_renderer_backend_data_get(epaint, ENESIM_BACKEND_SOFTWARE);
		pdata->fill(epaint, x, y, len, dst);
	}
	if (opaint)
	{
		sbuf = alloca(len * sizeof(unsigned int));
		pdata = enesim_renderer_backend_data_get(opaint, ENESIM_BACKEND_SOFTWARE);
		pdata->fill(opaint, x, y, len, sbuf);
		s = sbuf;
	}
	
	enesim_renderer_affine_setup(r, x, y, &thiz->matrix, &xx, &yy);
	while (d < e)
	{
		unsigned int p0 = c0, p1 = c1;
		int syy = (yy % hh), sy;

		if (syy < 0)
			syy += hh;
		sy = syy >> 16;
		if (epaint)
		{
			p0 = *d;
			if (c0 != 0xffffffff)
				p0 = argb8888_mul4_sym(p0, c0);
		}
		if (sy == 0)
		{
			int a = 1 + ((syy & 0xffff) >> 8);

			if (opaint)
			{
				p1 = *s;
				if (c1 != 0xffffffff)
					p1 = argb8888_mul4_sym(p1, c1);
			}
			p0 = argb8888_interp_256(a, p0, p1);
		}
		if (syy >= hh0)
		{
			if (opaint)
			{
				p1 = *s;
				if (c1 != 0xffffffff)
					p1 = argb8888_mul4_sym(p1, c1);
			}
			if (sy == h0)
			{
				int a = 1 + ((syy & 0xffff) >> 8);

				p1 = argb8888_interp_256(a, p1, p0);
			}
			p0 = p1;
		}
		*d++ = p0;
		if (s) s++;
		yy += ayx;
	}
}

static Eina_Bool _stripes_state_setup(Enesim_Renderer_Stripes *thiz, Enesim_Renderer *r)
{
	Enesim_Color final_color1;
	Enesim_Color final_color2;
	Enesim_Color rend_color;

	final_color1 = thiz->current.even.color;
	final_color2 = thiz->current.odd.color;

	enesim_renderer_color_get(r, &rend_color);
	if (rend_color != ENESIM_COLOR_FULL)
	{
		final_color1 = argb8888_mul4_sym(rend_color, final_color1);
		final_color2 = argb8888_mul4_sym(rend_color, final_color2);
	}
	thiz->final_color1 = final_color1;
	thiz->final_color2 = final_color2;

	return EINA_TRUE;
}

static void _stripes_state_cleanup(Enesim_Renderer_Stripes *thiz)
{
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _stripes_name(Enesim_Renderer *r)
{
	return "stripes";
}

static Eina_Bool _stripes_sw_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Stripes *thiz = _stripes_get(r);

	if (!thiz)
		return EINA_FALSE;
	if (!_stripes_state_setup(thiz, r)) return EINA_FALSE;
	thiz->hh0 = (int)(thiz->current.even.thickness * 65536);
	thiz->hh = (int)(thiz->hh0 + (thiz->current.odd.thickness * 65536));

	if (thiz->current.even.paint)
	{
		enesim_renderer_relative_set(thiz->current.even.paint, state, &thiz->old_even);
		if (!enesim_renderer_setup(thiz->current.even.paint, s, error))
			return EINA_FALSE;
	}
	if (thiz->current.odd.paint)
	{
		enesim_renderer_relative_set(thiz->current.odd.paint, state, &thiz->old_odd);
		if (!enesim_renderer_setup(thiz->current.odd.paint, s, error))
			return EINA_FALSE;
	}

	enesim_matrix_f16p16_matrix_to(&state->transformation,
			&thiz->matrix);
	switch (state->transformation_type)
	{
		case ENESIM_MATRIX_IDENTITY:
		case ENESIM_MATRIX_AFFINE:
		*fill = _span_affine;
		if (thiz->current.even.paint || thiz->current.odd.paint)
			*fill = _span_affine_paints;
		break;

		case ENESIM_MATRIX_PROJECTIVE:
		*fill = _span_projective;
		if (thiz->current.even.paint || thiz->current.odd.paint)
			*fill = _span_projective_paints;
		break;

		default:
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

static void _stripes_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	if (thiz->current.even.paint)
	{
		enesim_renderer_relative_unset(thiz->current.even.paint, &thiz->old_even);
		enesim_renderer_cleanup(thiz->current.even.paint, s);
	}
	if (thiz->current.odd.paint)
	{
		enesim_renderer_relative_unset(thiz->current.odd.paint, &thiz->old_odd);
		enesim_renderer_cleanup(thiz->current.odd.paint, s);
	}
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
			ENESIM_RENDERER_FLAG_ARGB8888 |
			ENESIM_RENDERER_FLAG_COLORIZE;
}

static Eina_Bool _stripes_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	if (!thiz->changed) return EINA_FALSE;

	if (thiz->current.even.paint && enesim_renderer_has_changed(thiz->current.even.paint))
			return EINA_TRUE;
	if (thiz->current.odd.paint && enesim_renderer_has_changed(thiz->current.odd.paint))
			return EINA_TRUE;
	if (thiz->current.even.color != thiz->past.even.color)
	{
		return EINA_TRUE;
	}
	if (thiz->current.even.paint != thiz->past.even.paint)
	{
		return EINA_TRUE;
	}
	if (thiz->current.even.thickness != thiz->past.even.thickness)
	{
		return EINA_TRUE;
	}
	if (thiz->current.odd.color != thiz->past.odd.color)
	{
		return EINA_TRUE;
	}
	if (thiz->current.odd.paint != thiz->past.odd.paint)
	{
		return EINA_TRUE;
	}
	if (thiz->current.odd.thickness != thiz->past.odd.thickness)
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
	if (thiz->current.even.paint)
		enesim_renderer_unref(thiz->current.even.paint);
	if (thiz->current.odd.paint)
		enesim_renderer_unref(thiz->current.odd.paint);
	free(thiz);
}

#if BUILD_OPENGL
static Eina_Bool _stripes_opengl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		Enesim_Surface *s,
		int *num_shaders,
		Enesim_Renderer_OpenGL_Shader **shaders,
		Enesim_Error **error)
{
	Enesim_Renderer_Stripes *thiz;
	Enesim_Renderer_OpenGL_Shader *shader;

 	thiz = _stripes_get(r);
	if (!_stripes_state_setup(thiz, r)) return EINA_FALSE;

	shader = calloc(1, sizeof(Enesim_Renderer_OpenGL_Shader));
	shader->type = ENESIM_SHADER_FRAGMENT;
	shader->name = "stripes";
	shader->source =
	#include "enesim_renderer_stripes.glsl"
	shader->size = strlen(shader->source);

	*shaders = shader;
	*num_shaders = 1;

	return EINA_TRUE;
}

static Eina_Bool _stripes_opengl_shader_setup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Stripes *thiz;
	Enesim_Renderer_OpenGL_Data *rdata;
	int odd_color;
	int even_color;
	int odd_thickness;
	int even_thickness;

 	thiz = _stripes_get(r);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	even_color = glGetUniformLocationARB(rdata->program, "stripes_even_color");
	odd_color = glGetUniformLocationARB(rdata->program, "stripes_odd_color");
	even_thickness = glGetUniformLocationARB(rdata->program, "stripes_even_thickness");
	odd_thickness = glGetUniformLocationARB(rdata->program, "stripes_odd_thickness");

	glUniform4fARB(even_color,
			argb8888_red_get(thiz->final_color1) / 255.0,
			argb8888_green_get(thiz->final_color1) / 255.0,
			argb8888_blue_get(thiz->final_color1) / 255.0,
			argb8888_alpha_get(thiz->final_color1) / 255.0);
	glUniform4fARB(odd_color,
			argb8888_red_get(thiz->final_color2) / 255.0,
			argb8888_green_get(thiz->final_color2) / 255.0,
			argb8888_blue_get(thiz->final_color2) / 255.0,
			argb8888_alpha_get(thiz->final_color2) / 255.0);
	glUniform1i(even_thickness, thiz->current.even.thickness);
	glUniform1i(odd_thickness, thiz->current.odd.thickness);

	return EINA_TRUE;
}

static void _stripes_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Stripes *thiz;

 	thiz = _stripes_get(r);
	_stripes_state_cleanup(thiz);
}
#endif

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
	/* .sw_setup =			*/ _stripes_sw_setup,
	/* .sw_cleanup = 		*/ _stripes_sw_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
#if BUILD_OPENGL
	/* .opengl_setup =          	*/ _stripes_opengl_setup,
	/* .opengl_shader_setup =   	*/ _stripes_opengl_shader_setup,
	/* .opengl_cleanup =        	*/ _stripes_opengl_cleanup
#else
	/* .opengl_setup =          	*/ NULL,
	/* .opengl_shader_setup = 	*/ NULL,
	/* .opengl_cleanup =        	*/ NULL
#endif
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
	thiz->current.even.thickness = 1;
	thiz->current.odd.thickness = 1;
	thiz->current.even.color = 0xffffffff;
	thiz->current.odd.color = 0xff000000;
	thiz->changed = 1;
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
	thiz->current.even.color = color;
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
	if (color) *color = thiz->current.even.color;
}
/**
 * Sets the renderer of the even stripes
 * @param[in] r The stripes renderer
 * @param[in] paint The even stripes renderer
 */
EAPI void enesim_renderer_stripes_even_renderer_set(Enesim_Renderer *r,
		Enesim_Renderer *paint)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	if (thiz->current.even.paint == paint)
		return;
	if (thiz->current.even.paint)
		enesim_renderer_unref(thiz->current.even.paint);
	thiz->current.even.paint = paint;
	if (paint)
		thiz->current.even.paint = enesim_renderer_ref(paint);
	thiz->changed = EINA_TRUE;
}
/**
 * Gets the renderer of the even stripes
 * @param[in] r The stripes renderer
 * @param[out] paint The even stripes renderer
 */
EAPI void enesim_renderer_stripes_even_renderer_get(Enesim_Renderer *r, Enesim_Renderer **paint)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	if (paint) *paint = thiz->current.even.paint;
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
	thiz->current.even.thickness = thickness;
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
	if (thickness) *thickness = thiz->current.even.thickness;
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
	thiz->current.odd.color = color;
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
	if (color) *color = thiz->current.odd.color;
}
/**
 * Sets the renderer of the odd stripes
 * @param[in] r The stripes renderer
 * @param[in] paint The odd stripes renderer
 */
EAPI void enesim_renderer_stripes_odd_renderer_set(Enesim_Renderer *r,
		Enesim_Renderer *paint)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	if (thiz->current.odd.paint == paint)
		return;
	if (thiz->current.odd.paint)
		enesim_renderer_unref(thiz->current.odd.paint);
	thiz->current.odd.paint = paint;
	if (paint)
		thiz->current.odd.paint = enesim_renderer_ref(paint);
	thiz->changed = EINA_TRUE;
}
/**
 * Gets the renderer of the odd stripes
 * @param[in] r The stripes renderer
 * @param[out] paint The odd stripes renderer
 */
EAPI void enesim_renderer_stripes_odd_renderer_get(Enesim_Renderer *r, Enesim_Renderer **paint)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = _stripes_get(r);
	if (paint) *paint = thiz->current.odd.paint;
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
	thiz->current.odd.thickness = thickness;
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
	if (thickness) *thickness = thiz->current.odd.thickness;
}

