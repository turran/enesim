/* ENESIM - Drawing Library
 * Copyright (C) 2007-2013 Jorge Luis Zapata
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
#include "enesim_private.h"
#include "libargb.h"

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_checker.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#ifdef BUILD_OPENCL
#include "Enesim_OpenCL.h"
#endif

#ifdef BUILD_OPENGL
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
#endif

#include "enesim_coord_private.h"
#include "enesim_renderer_private.h"

/**
 * @todo
 * - Optimize the case where both colors are the same
 * - There's a bug on the affine renderer, easily reproducible whenever we scale
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_CHECKER(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Checker,					\
		enesim_renderer_checker_descriptor_get())

typedef struct _Enesim_Renderer_Checker_State
{
	Enesim_Color color1;
	Enesim_Color color2;
	int sw;
	int sh;
} Enesim_Renderer_Checker_State;

typedef struct _Enesim_Renderer_Checker
{
	Enesim_Renderer parent;
	/* properties */
	Enesim_Renderer_Checker_State current;
	Enesim_Renderer_Checker_State past;
	/* private */
	Eina_Bool changed : 1;
	Enesim_F16p16_Matrix matrix;
	Enesim_Color final_color1;
	Enesim_Color final_color2;
	double ox;
	double oy;
	Eina_F16p16 ww, hh;
	Eina_F16p16 ww2, hh2;
} Enesim_Renderer_Checker;

typedef struct _Enesim_Renderer_Checker_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Checker_Class;

#if BUILD_OPENGL
/* the only shader */
static Enesim_Renderer_OpenGL_Shader _checker_shader = {
	/* .type 	= */ ENESIM_SHADER_FRAGMENT,
	/* .name 	= */ "checker",
	/* .source	= */
#include "enesim_renderer_checker.glsl"
};

static Enesim_Renderer_OpenGL_Shader *_checker_shaders[] = {
	&_checker_shader,
	NULL,
};

/* the only program */
static Enesim_Renderer_OpenGL_Program _checker_program = {
	/* .name 		= */ "checker",
	/* .shaders 		= */ _checker_shaders,
	/* .num_shaders		= */ 1,
};

static Enesim_Renderer_OpenGL_Program *_checker_programs[] = {
	&_checker_program,
	NULL,
};

/* odd = color2 */
static Eina_Bool _checker_opengl_shader_setup(GLenum pid,
		Enesim_Color odd_color, Enesim_Color even_color,
		int sw, int sh)
{
	int odd_color_u;
	int even_color_u;
	int sw_u;
	int sh_u;

	even_color_u = glGetUniformLocationARB(pid, "checker_even_color");
	odd_color_u = glGetUniformLocationARB(pid, "checker_odd_color");
	sw_u = glGetUniformLocationARB(pid, "checker_sw");
	sh_u = glGetUniformLocationARB(pid, "checker_sh");

	glUniform4fARB(even_color_u,
			argb8888_red_get(even_color) / 255.0,
			argb8888_green_get(even_color) / 255.0,
			argb8888_blue_get(even_color) / 255.0,
			argb8888_alpha_get(even_color) / 255.0);
	glUniform4fARB(odd_color_u,
			argb8888_red_get(odd_color) / 255.0,
			argb8888_green_get(odd_color) / 255.0,
			argb8888_blue_get(odd_color) / 255.0,
			argb8888_alpha_get(odd_color) / 255.0);
	glUniform1i(sw_u, sw);
	glUniform1i(sh_u, sh);

	return EINA_TRUE;
}

static void _checker_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED,
		const Eina_Rectangle *area, int w, int h)
{
	Enesim_Renderer_Checker *thiz;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_OpenGL_Compiled_Program *cp;

	thiz = ENESIM_RENDERER_CHECKER(r);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	cp = &rdata->program->compiled[0];
	glUseProgramObjectARB(cp->id);
	_checker_opengl_shader_setup(cp->id,
			thiz->final_color2, thiz->final_color1,
			thiz->current.sw, thiz->current.sh);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, 0, h, -1, 1);

	glBegin(GL_QUADS);
		glTexCoord2d(area->x, area->y);
		glVertex2d(area->x, area->y);

		glTexCoord2d(area->x + area->w, area->y);
		glVertex2d(area->x + area->w, area->y);

		glTexCoord2d(area->x + area->w, area->y + area->h);
		glVertex2d(area->x + area->w, area->y + area->h);

		glTexCoord2d(area->x, area->y + area->h);
		glVertex2d(area->x, area->y + area->h);
	glEnd();
}
#endif

static Eina_Bool _checker_state_setup(Enesim_Renderer *r, Enesim_Renderer_Checker *thiz)
{
	Enesim_Color final_color1;
	Enesim_Color final_color2;
	Enesim_Color rend_color;

	final_color1 = thiz->current.color1;
	final_color2 = thiz->current.color2;

	rend_color = enesim_renderer_color_get(r);
	if (rend_color != ENESIM_COLOR_FULL)
	{
		final_color1 = argb8888_mul4_sym(rend_color, final_color1);
		final_color2 = argb8888_mul4_sym(rend_color, final_color2);
	}
	enesim_renderer_origin_get(r, &thiz->ox, &thiz->oy);
	thiz->final_color1 = final_color1;
	thiz->final_color2 = final_color2;
	return EINA_TRUE;
}

static void _checker_state_cleanup(Enesim_Renderer_Checker *thiz)
{
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}
/*----------------------------------------------------------------------------*
 *                               Span functions                               *
 *----------------------------------------------------------------------------*/
static void _span_identity(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Checker *thiz;
	Eina_F16p16 yy, xx;
	int w2;
	int h2;
	uint32_t color[2];
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	int sy;

	thiz = ENESIM_RENDERER_CHECKER(r);
	w2 = thiz->current.sw * 2;
	h2 = thiz->current.sh * 2;
	color[0] = thiz->final_color1;
	color[1] = thiz->final_color2;

	/* translate to the origin */
	enesim_coord_identity_setup(&xx, &yy, x, y, thiz->ox, thiz->oy);
	/* normalize the modulo */
	sy = ((yy  >> 16) % h2);
	if (sy < 0)
	{
		sy += h2;
	}
	/* swap the colors */
	if (sy >= thiz->current.sh)
	{
		color[0] = thiz->final_color2;
		color[1] = thiz->final_color1;
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

static void _span_affine(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Checker *thiz;
	Eina_F16p16 yy, xx, ww, hh, ww2, hh2;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;

	thiz = ENESIM_RENDERER_CHECKER(r);
	enesim_coord_affine_setup(&xx, &yy, x, y, thiz->ox, thiz->oy, &thiz->matrix);
	ww = thiz->ww;
	ww2 = thiz->ww2;
	hh = thiz->hh;
	hh2 = thiz->hh2;

	while (dst < end)
	{
		Eina_F16p16 syy, sxx;
		uint32_t color[2] = {thiz->final_color1, thiz->final_color2};
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
			color[0] = thiz->final_color2;
			color[1] = thiz->final_color1;
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

static void _span_projective(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Checker *thiz;
	Eina_F16p16 yy, xx, zz, ww, hh, ww2, hh2;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;

	thiz = ENESIM_RENDERER_CHECKER(r);
	/* translate to the origin */
	enesim_coord_projective_setup(&xx, &yy, &zz, x, y, thiz->ox, thiz->oy, &thiz->matrix);
	ww = thiz->ww;
	ww2 = thiz->ww2;
	hh = thiz->hh;
	hh2 = thiz->hh2;

	while (dst < end)
	{
		Eina_F16p16 syy, sxx, syyy, sxxx;
		uint32_t color[2] = {thiz->final_color1, thiz->final_color2};
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
			color[0] = thiz->final_color2;
			color[1] = thiz->final_color1;
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
static const char * _checker_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "checker";
}

static void _checker_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Checker *thiz;

	thiz = ENESIM_RENDERER_CHECKER(r);
	_checker_state_cleanup(thiz);
}

static Eina_Bool _checker_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Checker *thiz;
	Enesim_Matrix_Type type;
	Enesim_Matrix matrix;
	Enesim_Matrix inv;

	thiz = ENESIM_RENDERER_CHECKER(r);
	_checker_state_setup(r, thiz);

	thiz->ww = eina_f16p16_int_from(thiz->current.sw);
	thiz->ww2 = thiz->ww * 2;
	thiz->hh = eina_f16p16_int_from(thiz->current.sh);
	thiz->hh2 = thiz->hh * 2;

	enesim_renderer_transformation_type_get(r, &type);
	enesim_renderer_transformation_get(r, &matrix);
	switch (type)
	{
		case ENESIM_MATRIX_IDENTITY:
		*fill = _span_identity;
		break;

		case ENESIM_MATRIX_AFFINE:
		enesim_matrix_inverse(&matrix, &inv);
		enesim_matrix_f16p16_matrix_to(&inv,
				&thiz->matrix);
		*fill = _span_affine;
		break;

		case ENESIM_MATRIX_PROJECTIVE:
		enesim_matrix_inverse(&matrix, &inv);
		enesim_matrix_f16p16_matrix_to(&inv,
				&thiz->matrix);
		*fill = _span_projective;
		break;

		default:
		return EINA_FALSE;
	}

	return EINA_TRUE;
}

static Eina_Bool _checker_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Checker *thiz;

	thiz = ENESIM_RENDERER_CHECKER(r);
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

static void _checker_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_PROJECTIVE |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static void _checker_sw_hints(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Rop rop EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_COLORIZE;
}

#if BUILD_OPENGL
static Eina_Bool _checker_opengl_initialize(Enesim_Renderer *r EINA_UNUSED,
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***programs)
{
	*num_programs = 1;
	*programs = _checker_programs;
	return EINA_TRUE;
}

static Eina_Bool _checker_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Checker *thiz;

 	thiz = ENESIM_RENDERER_CHECKER(r);
	if (!_checker_state_setup(r, thiz)) return EINA_FALSE;

	*draw = _checker_opengl_draw;

	return EINA_TRUE;
}

static void _checker_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Checker *thiz;

 	thiz = ENESIM_RENDERER_CHECKER(r);
	_checker_state_cleanup(thiz);
}
#endif
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/

ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Checker, Enesim_Renderer_Checker_Class,
		enesim_renderer_checker);

static void _enesim_renderer_checker_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _checker_name;
	klass->bounds_get = NULL;
	klass->features_get = _checker_features_get;
	klass->is_inside = NULL;
	klass->damages_get = NULL;
	klass->has_changed = _checker_has_changed;
	klass->alpha_hints_get = NULL;
	klass->sw_hints_get = _checker_sw_hints;
	klass->sw_setup = _checker_sw_setup;
	klass->sw_cleanup = _checker_sw_cleanup;
	klass->opencl_setup = NULL;
	klass->opencl_kernel_setup = NULL;
	klass->opencl_cleanup =	NULL;
#if BUILD_OPENGL
	klass->opengl_initialize = _checker_opengl_initialize;
	klass->opengl_setup = _checker_opengl_setup;
	klass->opengl_cleanup = _checker_opengl_cleanup;
#endif
}

static void _enesim_renderer_checker_instance_init(void *o)
{
	Enesim_Renderer_Checker *thiz = ENESIM_RENDERER_CHECKER(o);

	/* specific renderer setup */
	thiz->current.sw = 1;
	thiz->current.sh = 1;
}

static void _enesim_renderer_checker_instance_deinit(void *o EINA_UNUSED)
{
}
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

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_checker);
	return r;
}

/**
 * @brief Sets the color of the even squares
 * @param[in] r The checker renderer
 * @param[in] color The color
 */
EAPI void enesim_renderer_checker_even_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Checker *thiz;

	thiz = ENESIM_RENDERER_CHECKER(r);

	thiz->current.color1 = color;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the color of the even squares
 * @param[in] r The checker renderer
 * @return The color
 */
EAPI Enesim_Color enesim_renderer_checker_even_color_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Checker *thiz;

	thiz = ENESIM_RENDERER_CHECKER(r);
	return thiz->current.color1;
}

/**
 * @brief Sets the color of the odd squares
 * @param[in] r The checker renderer
 * @param[in] color The color
 */
EAPI void enesim_renderer_checker_odd_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Checker *thiz;

	thiz = ENESIM_RENDERER_CHECKER(r);

	thiz->current.color2 = color;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the color of the odd squares
 * @param[in] r The checker renderer
 * @return The color
 */
EAPI Enesim_Color enesim_renderer_checker_odd_color_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Checker *thiz;

	thiz = ENESIM_RENDERER_CHECKER(r);
	return thiz->current.color2;
}

/**
 * @brief Sets the width of the checker rectangles
 * @param[in] width The width
 */
EAPI void enesim_renderer_checker_width_set(Enesim_Renderer *r, int width)
{
	Enesim_Renderer_Checker *thiz;

	thiz = ENESIM_RENDERER_CHECKER(r);

	thiz->current.sw = width;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the width of the checker rectangles
 * @returns The width
 */
EAPI int enesim_renderer_checker_width_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Checker *thiz;

	thiz = ENESIM_RENDERER_CHECKER(r);
	return thiz->current.sh;
}

/**
 * @brief Sets the height of the checker rectangles
 * @param[in] height The height
 */
EAPI void enesim_renderer_checker_height_set(Enesim_Renderer *r, int height)
{
	Enesim_Renderer_Checker *thiz;

	thiz = ENESIM_RENDERER_CHECKER(r);

	thiz->current.sh = height;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the height of the checker rectangles
 * @return The height
 */
EAPI int enesim_renderer_checker_height_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Checker *thiz;

	thiz = ENESIM_RENDERER_CHECKER(r);
	return thiz->current.sh;
}

