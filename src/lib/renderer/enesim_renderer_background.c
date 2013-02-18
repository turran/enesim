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
#include "enesim_private.h"
#include "libargb.h"

#include "enesim_main.h"
#include "enesim_error.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"
#include "enesim_renderer_background.h"

#ifdef BUILD_OPENCL
#include "Enesim_OpenCL.h"
#endif

#ifdef BUILD_OPENGL
#include "Enesim_OpenGL.h"
#endif

#include "enesim_renderer_private.h"
#include "enesim_renderer_simple_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_BACKGROUND_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_BACKGROUND_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_BACKGROUND_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Background {
	EINA_MAGIC
	/* the properties */
	Enesim_Color color;
	/* generated at state setup */
	Enesim_Color final_color;
	Enesim_Compositor_Span span;
	/* private */
	Eina_Bool changed : 1;
} Enesim_Renderer_Background;

static inline Enesim_Renderer_Background * _background_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Background *thiz;

	thiz = enesim_renderer_data_get(r);
	ENESIM_RENDERER_BACKGROUND_MAGIC_CHECK(thiz);

	return thiz;
}

static void _background_span(Enesim_Renderer *r,
		int x EINA_UNUSED, int y EINA_UNUSED,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Background *thiz = _background_get(r);
	uint32_t *dst = ddata;

	thiz->span(dst, len, NULL, thiz->final_color, NULL);
}

#if BUILD_OPENGL

/* the only shader */
static Enesim_Renderer_OpenGL_Shader _background_shader = {
	/* .type 	= */ ENESIM_SHADER_FRAGMENT,
	/* .name 	= */ "background",
	/* .source	= */
#include "enesim_renderer_opengl_common_ambient.glsl"
};

static Enesim_Renderer_OpenGL_Shader *_background_shaders[] = {
	&_background_shader,
	NULL,
};

/* the only program */
static Enesim_Renderer_OpenGL_Program _background_program = {
	/* .name 		= */ "background",
	/* .shaders 		= */ _background_shaders,
	/* .num_shaders		= */ 1,
};

static Enesim_Renderer_OpenGL_Program *_background_programs[] = {
	&_background_program,
	NULL,
};

static Eina_Bool _background_opengl_shader_setup(GLenum pid,
		Enesim_Color final_color)
{
	int final_color_u;

	glUseProgramObjectARB(pid);
	final_color_u = glGetUniformLocationARB(pid, "ambient_final_color");
	glUniform4fARB(final_color_u,
			argb8888_red_get(final_color) / 255.0,
			argb8888_green_get(final_color) / 255.0,
			argb8888_blue_get(final_color) / 255.0,
			argb8888_alpha_get(final_color) / 255.0);

	return EINA_TRUE;
}


static void _background_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED,
		const Eina_Rectangle *area, int w, int h)
{
	Enesim_Renderer_Background * thiz;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_Renderer_OpenGL_Compiled_Program *cp;

	thiz = _background_get(r);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);

	cp = &rdata->program->compiled[0];
	_background_opengl_shader_setup(cp->id, thiz->final_color);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, -1, 1);

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

static Eina_Bool _background_state_setup(Enesim_Renderer_Background *thiz, Enesim_Renderer *r)
{
	Enesim_Color final_color, rend_color;

	final_color = thiz->color;
	enesim_renderer_color_get(r, &rend_color);
	if (rend_color != ENESIM_COLOR_FULL)
		final_color = argb8888_mul4_sym(rend_color, final_color);
	thiz->final_color = final_color;
	return EINA_TRUE;
}

static void _background_state_cleanup(Enesim_Renderer_Background *thiz)
{
	thiz->changed = EINA_FALSE;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _background_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "background";
}

static Eina_Bool _background_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error EINA_UNUSED)
{
	Enesim_Renderer_Background *thiz;
	Enesim_Format fmt = ENESIM_FORMAT_ARGB8888;
	Enesim_Rop rop;

 	thiz = _background_get(r);

	if (!_background_state_setup(thiz, r)) return EINA_FALSE;
	enesim_renderer_rop_get(r, &rop);
	thiz->span = enesim_compositor_span_get(rop, &fmt, ENESIM_FORMAT_NONE,
			thiz->final_color, ENESIM_FORMAT_NONE);
	*fill = _background_span;

	return EINA_TRUE;
}

static void _background_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Background *thiz;

 	thiz = _background_get(r);
	thiz->changed = EINA_FALSE;
}

#if BUILD_OPENCL
static Eina_Bool _background_opencl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		const char **program_name, const char **program_source,
		size_t *program_length, Enesim_Error **error)
{
	Enesim_Renderer_Background *thiz;

 	thiz = _background_get(r);
	if (!_background_state_setup(thiz, r)) return EINA_FALSE;

	*program_name = "background";
	*program_source =
	#include "enesim_renderer_background.cl"
	*program_length = strlen(*program_source);

	return EINA_TRUE;
}

static Eina_Bool _background_opencl_kernel_setup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Background *thiz;
	Enesim_Renderer_OpenCL_Data *rdata;

 	thiz = _background_get(r);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENCL);
	clSetKernelArg(rdata->kernel, 1, sizeof(cl_uchar4), &thiz->final_color);

	return EINA_TRUE;
}

static void _background_opencl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Background *thiz;

 	thiz = _background_get(r);
	_background_state_cleanup(thiz);
}
#endif

#if BUILD_OPENGL
static Eina_Bool _background_opengl_initialize(Enesim_Renderer *r EINA_UNUSED,
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***programs)
{
	*num_programs = 1;
	*programs = _background_programs;
	return EINA_TRUE;
}

static Eina_Bool _background_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Error **error EINA_UNUSED)
{
	Enesim_Renderer_Background *thiz;

 	thiz = _background_get(r);
	if (!_background_state_setup(thiz, r)) return EINA_FALSE;

	*draw = _background_opengl_draw;
	return EINA_TRUE;
}

static void _background_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Background *thiz;

 	thiz = _background_get(r);
	_background_state_cleanup(thiz);
}
#endif

static void _background_flags(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_PROJECTIVE |
			ENESIM_RENDERER_FLAG_ARGB8888;
}

static void _background_sw_hints(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_ROP | ENESIM_RENDERER_HINT_COLORIZE;
}

static void _background_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Background *thiz;

	thiz = _background_get(r);
	free(thiz);
}

static Eina_Bool _background_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Background *thiz;

	thiz = _background_get(r);
	if (!thiz->changed) return EINA_FALSE;
	return EINA_TRUE;
}

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .base_name_get = 		*/ _background_name,
	/* .free = 			*/ _background_free,
	/* .bounds_get = 		*/ NULL,
	/* .destination_bounds_get =	*/ NULL,
	/* .flags_get = 		*/ _background_flags,
	/* .is_inside = 		*/ NULL,
	/* .damages_get = 		*/ NULL,
	/* .has_changed = 		*/ _background_has_changed,
	/* .sw_hints_get = 		*/ _background_sw_hints,
	/* .sw_setup = 			*/ _background_sw_setup,
	/* .sw_cleanup = 		*/ _background_sw_cleanup,
#if BUILD_OPENCL
	/* .opencl_setup = 		*/ _background_opencl_setup,
	/* .opencl_kernel_setup =   	*/ _background_opencl_kernel_setup,
	/* .opencl_cleanup = 		*/ _background_opencl_cleanup,
#else
	/* .opencl_setup = 		*/ NULL,
	/* .opencl_kernel_setup = 	*/ NULL,
	/* .opencl_cleanup = 		*/ NULL,
#endif
#if BUILD_OPENGL
	/* .opengl_initialize = 	*/ _background_opengl_initialize,
	/* .opengl_setup = 		*/ _background_opengl_setup,
	/* .opengl_cleanup = 		*/ _background_opengl_cleanup,
#else
	/* .opengl_initialize = 	*/ NULL,
	/* .opengl_setup = 		*/ NULL,
	/* .opengl_cleanup = 		*/ NULL
#endif
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a background renderer
 * @return The new renderer
 */
EAPI Enesim_Renderer * enesim_renderer_background_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Background *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Background));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_BACKGROUND_MAGIC);
	r = enesim_renderer_new(&_descriptor, thiz);
	return r;
}
/**
 * Sets the color of the background
 * @param[in] r The background renderer
 * @param[in] color The background color
 */
EAPI void enesim_renderer_background_color_set(Enesim_Renderer *r,
		Enesim_Color color)
{
	Enesim_Renderer_Background *thiz;

	thiz = _background_get(r);
	thiz->color = color;
	thiz->changed = EINA_TRUE;
}

/**
 *
 */
EAPI void enesim_renderer_background_color_get(Enesim_Renderer *r,
		Enesim_Color *color)
{
	Enesim_Renderer_Background *thiz;

	thiz = _background_get(r);
	*color = thiz->color;
}


