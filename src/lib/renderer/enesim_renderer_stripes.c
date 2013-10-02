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
#include "enesim_renderer_stripes.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#ifdef BUILD_OPENGL
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
#endif

#include "enesim_coord_private.h"
#include "enesim_renderer_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_STRIPES(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Stripes,					\
		enesim_renderer_stripes_descriptor_get())

typedef struct _Enesim_Renderer_Stripes_State {
	struct {
		Enesim_Color color;
		Enesim_Renderer *paint;
		double thickness;
	} even, odd;
} Enesim_Renderer_Stripes_State;

typedef struct _Enesim_Renderer_Stripes {
	Enesim_Renderer parent;
	/* properties */
	Enesim_Renderer_Stripes_State current;
	Enesim_Renderer_Stripes_State past;
	/* private */
	Eina_Bool changed : 1;
	Enesim_Color final_color1;
	Enesim_Color final_color2;
	int hh0, hh;
	Enesim_F16p16_Matrix matrix;
	double ox;
	double oy;
} Enesim_Renderer_Stripes;

typedef struct _Enesim_Renderer_Stripes_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Stripes_Class;

#if BUILD_OPENGL

/* the only shader */
static Enesim_Renderer_OpenGL_Shader _stripes_shader = {
	/* .type 	= */ ENESIM_SHADER_FRAGMENT,
	/* .name 	= */ "stripes",
	/* .source	= */
#include "enesim_renderer_stripes.glsl"
};

static Enesim_Renderer_OpenGL_Shader *_stripes_shaders[] = {
	&_stripes_shader,
	NULL,
};

/* the only program */
static Enesim_Renderer_OpenGL_Program _stripes_program = {
	/* .name 		= */ "stripes",
	/* .shaders 		= */ _stripes_shaders,
	/* .num_shaders		= */ 1,
};

static Enesim_Renderer_OpenGL_Program *_stripes_programs[] = {
	&_stripes_program,
	NULL,
};

/* odd = final2 */
static Eina_Bool _stripes_opengl_shader_setup(GLenum pid,
		Enesim_Color even_color, Enesim_Color odd_color,
		double even_thickness, double odd_thickness)
{
	int odd_color_u;
	int even_color_u;
	int odd_thickness_u;
	int even_thickness_u;

	glUseProgramObjectARB(pid);
	even_color_u = glGetUniformLocationARB(pid, "stripes_even_color");
	odd_color_u = glGetUniformLocationARB(pid, "stripes_odd_color");
	even_thickness_u = glGetUniformLocationARB(pid, "stripes_even_thickness");
	odd_thickness_u = glGetUniformLocationARB(pid, "stripes_odd_thickness");

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
	glUniform1i(even_thickness_u, even_thickness);
	glUniform1i(odd_thickness_u, odd_thickness);

	return EINA_TRUE;
}


static void _stripes_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED,
		const Eina_Rectangle *area, int w, int h)
{
	Enesim_Renderer_Stripes *thiz;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_OpenGL_Compiled_Program *cp;

	thiz = ENESIM_RENDERER_STRIPES(r);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	cp = &rdata->program->compiled[0];
	_stripes_opengl_shader_setup(cp->id,
			thiz->final_color1, thiz->final_color2,
			thiz->current.even.thickness,
			thiz->current.odd.thickness);

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
	/* don't use any program */
	glUseProgramObjectARB(0);
}
#endif

static void _span_projective(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Stripes *thiz = ENESIM_RENDERER_STRIPES(r);
	int hh = thiz->hh, hh0 = thiz->hh0, h0 = hh0 >> 16;
	Enesim_Color c0 = thiz->final_color1;
	Enesim_Color c1 = thiz->final_color2;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Eina_F16p16 yy, xx, zz;

	enesim_coord_projective_setup(&xx, &yy, &zz, x, y, thiz->ox, thiz->oy, &thiz->matrix);

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

static void _span_projective_paints(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Stripes *thiz = ENESIM_RENDERER_STRIPES(r);
	int hh = thiz->hh, hh0 = thiz->hh0, h0 = hh0 >> 16;
	Enesim_Color c0 = thiz->final_color1;
	Enesim_Color c1 = thiz->final_color2;
	Enesim_Renderer *opaint, *epaint;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Eina_F16p16 yy, xx, zz;
	unsigned int *sbuf, *s = NULL;

	opaint = thiz->current.odd.paint;
	epaint = thiz->current.even.paint;
	if (epaint)
	{
		enesim_renderer_sw_draw(epaint, x, y, len, dst);
	}
	if (opaint)
	{
		sbuf = alloca(len * sizeof(unsigned int));
		enesim_renderer_sw_draw(opaint, x, y, len, sbuf);
		s = sbuf;
	}

	enesim_coord_projective_setup(&xx, &yy, &zz, x, y, thiz->ox, thiz->oy, &thiz->matrix);
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

static void _span_affine(Enesim_Renderer *r,
		int x, int y,
		int len, void *ddata)
{
	Enesim_Renderer_Stripes *thiz = ENESIM_RENDERER_STRIPES(r);
	int ayx = thiz->matrix.yx;
	int hh = thiz->hh, hh0 = thiz->hh0, h0 = hh0 >> 16;
	Enesim_Color c0 = thiz->final_color1;
	Enesim_Color c1 = thiz->final_color2;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Eina_F16p16 yy, xx;

	enesim_coord_affine_setup(&xx, &yy, x, y, thiz->ox, thiz->oy,  &thiz->matrix);
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

static void _span_affine_paints(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Stripes *thiz = ENESIM_RENDERER_STRIPES(r);
	int ayx = thiz->matrix.yx;
	int hh = thiz->hh, hh0 = thiz->hh0, h0 = hh0 >> 16;
	Enesim_Color c0 = thiz->final_color1;
	Enesim_Color c1 = thiz->final_color2;
	Enesim_Renderer *opaint, *epaint;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	Eina_F16p16 yy, xx;
	unsigned int *sbuf, *s = NULL;

	opaint = thiz->current.odd.paint;
	epaint = thiz->current.even.paint;
	if (epaint)
	{
		enesim_renderer_sw_draw(epaint, x, y, len, dst);
	}
	if (opaint)
	{
		sbuf = alloca(len * sizeof(unsigned int));
		enesim_renderer_sw_draw(opaint, x, y, len, sbuf);
		s = sbuf;
	}

	enesim_coord_affine_setup(&xx, &yy, x, y, thiz->ox, thiz->oy, &thiz->matrix);
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

	rend_color = enesim_renderer_color_get(r);
	if (rend_color != ENESIM_COLOR_FULL)
	{
		final_color1 = argb8888_mul4_sym(rend_color, final_color1);
		final_color2 = argb8888_mul4_sym(rend_color, final_color2);
	}
	thiz->final_color1 = final_color1;
	thiz->final_color2 = final_color2;

	return EINA_TRUE;
}

static void _stripes_state_cleanup(Enesim_Renderer_Stripes *thiz, Enesim_Surface *s)
{
	if (thiz->current.even.paint)
	{
		enesim_renderer_cleanup(thiz->current.even.paint, s);
	}
	if (thiz->current.odd.paint)
	{
		enesim_renderer_cleanup(thiz->current.odd.paint, s);
	}
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _stripes_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "stripes";
}

static Eina_Bool _stripes_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l)
{
	Enesim_Renderer_Stripes *thiz = ENESIM_RENDERER_STRIPES(r);
	Enesim_Matrix_Type type;
	Enesim_Matrix matrix;
	Enesim_Matrix inv;

	if (!thiz)
		return EINA_FALSE;
	if (!_stripes_state_setup(thiz, r)) return EINA_FALSE;
	thiz->hh0 = (int)(thiz->current.even.thickness * 65536);
	thiz->hh = (int)(thiz->hh0 + (thiz->current.odd.thickness * 65536));

	if (thiz->current.even.paint)
	{
		if (!enesim_renderer_setup(thiz->current.even.paint, s, rop, l))
			return EINA_FALSE;
	}
	if (thiz->current.odd.paint)
	{
		if (!enesim_renderer_setup(thiz->current.odd.paint, s, rop, l))
			return EINA_FALSE;
	}

	enesim_renderer_origin_get(r, &thiz->ox, &thiz->oy);
	type = enesim_renderer_transformation_type_get(r);
	enesim_renderer_transformation_get(r, &matrix);
	enesim_matrix_inverse(&matrix, &inv);
	enesim_matrix_f16p16_matrix_to(&inv,
			&thiz->matrix);
	switch (type)
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

	thiz = ENESIM_RENDERER_STRIPES(r);
	_stripes_state_cleanup(thiz, s);
}

static void _stripes_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_PROJECTIVE |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static void _stripes_sw_hints(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Rop rop EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_COLORIZE;
}

static Eina_Bool _stripes_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = ENESIM_RENDERER_STRIPES(r);
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

#if BUILD_OPENGL
static Eina_Bool _stripes_opengl_initialize(Enesim_Renderer *r EINA_UNUSED,
		int *num_shaders,
		Enesim_Renderer_OpenGL_Program ***programs)
{
	*programs = _stripes_programs;
	*num_shaders = 1;
	return EINA_TRUE;
}

static Eina_Bool _stripes_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED,
		Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Stripes *thiz;

 	thiz = ENESIM_RENDERER_STRIPES(r);
	if (!_stripes_state_setup(thiz, r)) return EINA_FALSE;

	*draw = _stripes_opengl_draw;

	return EINA_TRUE;
}

static void _stripes_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Stripes *thiz;

 	thiz = ENESIM_RENDERER_STRIPES(r);
	_stripes_state_cleanup(thiz, s);
}
#endif
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Stripes, Enesim_Renderer_Stripes_Class,
		enesim_renderer_stripes);

static void _enesim_renderer_stripes_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _stripes_name;
	klass->features_get = _stripes_features_get;
	klass->has_changed = _stripes_has_changed;
	klass->sw_hints_get = _stripes_sw_hints;
	klass->sw_setup = _stripes_sw_setup;
	klass->sw_cleanup = _stripes_sw_cleanup;
#if BUILD_OPENGL
	klass->opengl_initialize = _stripes_opengl_initialize;
	klass->opengl_setup = _stripes_opengl_setup;
	klass->opengl_cleanup = _stripes_opengl_cleanup;
#endif
}

static void _enesim_renderer_stripes_instance_init(void *o)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = ENESIM_RENDERER_STRIPES(o);
	/* specific renderer setup */
	thiz->current.even.thickness = 1;
	thiz->current.odd.thickness = 1;
	thiz->current.even.color = 0xffffffff;
	thiz->current.odd.color = 0xff000000;
	thiz->changed = 1;
}

static void _enesim_renderer_stripes_instance_deinit(void *o)
{
	Enesim_Renderer_Stripes *thiz;

	thiz = ENESIM_RENDERER_STRIPES(o);
	if (thiz->current.even.paint)
		enesim_renderer_unref(thiz->current.even.paint);
	if (thiz->current.odd.paint)
		enesim_renderer_unref(thiz->current.odd.paint);
}
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

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_stripes);
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

	thiz = ENESIM_RENDERER_STRIPES(r);
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

	thiz = ENESIM_RENDERER_STRIPES(r);
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

	thiz = ENESIM_RENDERER_STRIPES(r);
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

	thiz = ENESIM_RENDERER_STRIPES(r);
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

	thiz = ENESIM_RENDERER_STRIPES(r);
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

	thiz = ENESIM_RENDERER_STRIPES(r);
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

	thiz = ENESIM_RENDERER_STRIPES(r);
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

	thiz = ENESIM_RENDERER_STRIPES(r);
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

	thiz = ENESIM_RENDERER_STRIPES(r);
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

	thiz = ENESIM_RENDERER_STRIPES(r);
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

	thiz = ENESIM_RENDERER_STRIPES(r);
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

	thiz = ENESIM_RENDERER_STRIPES(r);
	if (thickness) *thickness = thiz->current.odd.thickness;
}

