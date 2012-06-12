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
#include "libargb.h"
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
		const Enesim_Renderer_State *state,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Background *thiz = _background_get(r);
	uint32_t *dst = ddata;

	thiz->span(dst, len, NULL, thiz->final_color, NULL);
}

#if BUILD_OPENGL
static Eina_Bool _background_opengl_shader_setup(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Shader *shader)
{
	Enesim_Renderer_Background *thiz;
	Enesim_Renderer_OpenGL_Data *rdata;
	int final_color;

 	thiz = _background_get(r);
	if (strcmp(shader->name, "background"))
		return EINA_FALSE;

	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	final_color = glGetUniformLocationARB(rdata->program, "background_final_color");
	glUniform4fARB(final_color,
			argb8888_red_get(thiz->final_color) / 255.0,
			argb8888_green_get(thiz->final_color) / 255.0,
			argb8888_blue_get(thiz->final_color) / 255.0,
			argb8888_alpha_get(thiz->final_color) / 255.0);

	return EINA_TRUE;
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
static const char * _background_name(Enesim_Renderer *r)
{
	return "background";
}

static Eina_Bool _background_sw_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
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

static void _background_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
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
static Eina_Bool _background_opengl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Define_Geometry *define_geometry,
		Enesim_Renderer_OpenGL_Shader_Setup *shader_setup,
		int *num_shaders,
		Enesim_Renderer_OpenGL_Shader **shaders,
		Enesim_Error **error)
{
	Enesim_Renderer_Background *thiz;
	Enesim_Renderer_OpenGL_Shader *shader;

 	thiz = _background_get(r);
	if (!_background_state_setup(thiz, r)) return EINA_FALSE;

	*shader_setup = _background_opengl_shader_setup;
	*define_geometry = NULL;

	shader = calloc(1, sizeof(Enesim_Renderer_OpenGL_Shader));
	shader->type = ENESIM_SHADER_FRAGMENT;
	shader->name = "background";
	shader->source =
	#include "enesim_renderer_background.glsl"
	shader->size = strlen(shader->source);

	*shaders = shader;
	*num_shaders = 1;

	return EINA_TRUE;
}

static void _background_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Background *thiz;

 	thiz = _background_get(r);
	_background_state_cleanup(thiz);
}
#endif

static void _background_flags(Enesim_Renderer *r, const Enesim_Renderer_State *state,
		Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_PROJECTIVE |
			ENESIM_RENDERER_FLAG_ARGB8888;

}

static void _background_hints(Enesim_Renderer *r, const Enesim_Renderer_State *state,
		Enesim_Renderer_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_ROP | ENESIM_RENDERER_HINT_COLORIZE;

}

static void _background_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Background *thiz;

	thiz = _background_get(r);
	free(thiz);
}

static Eina_Bool _background_has_changed(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES])
{
	Enesim_Renderer_Background *thiz;

	thiz = _background_get(r);
	if (!thiz->changed) return EINA_FALSE;
	/* FIXME */
	return EINA_TRUE;
}

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _background_name,
	/* .free = 			*/ _background_free,
	/* .boundings = 		*/ NULL,
	/* .destination_boundings =	*/ NULL,
	/* .flags = 			*/ _background_flags,
	/* .hints_get = 		*/ _background_hints,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _background_has_changed,
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
	/* .opengl_setup = 		*/ _background_opengl_setup,
	/* .opengl_cleanup = 		*/ _background_opengl_cleanup,
#else
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


