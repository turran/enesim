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
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_background.h"
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

#include "enesim_renderer_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_RENDERER_BACKGROUND(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Background,					\
		enesim_renderer_background_descriptor_get())

typedef struct _Enesim_Renderer_Background {
	Enesim_Renderer parent;
	/* the properties */
	Enesim_Color color;
	/* generated at state setup */
	Enesim_Color final_color;
	Enesim_Compositor_Span span;
	/* private */
	Eina_Bool changed : 1;
} Enesim_Renderer_Background;

typedef struct _Enesim_Renderer_Background_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Background_Class;

static void _background_span(Enesim_Renderer *r,
		int x EINA_UNUSED, int y EINA_UNUSED, int len, void *ddata)
{
	Enesim_Renderer_Background *thiz = ENESIM_RENDERER_BACKGROUND(r);
	uint32_t *dst = ddata;

	thiz->span(dst, len, NULL, thiz->final_color, NULL);
}

#if BUILD_OPENGL

/* the only shader */
static Enesim_Renderer_OpenGL_Shader _background_shader = {
	/* .type 	= */ ENESIM_RENDERER_OPENGL_SHADER_FRAGMENT,
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

static void _background_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, const Eina_Rectangle *area, int x EINA_UNUSED,
		int y EINA_UNUSED)
{
	Enesim_Renderer_Background * thiz;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_OpenGL_Compiled_Program *cp;

	thiz = ENESIM_RENDERER_BACKGROUND(r);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);

	cp = &rdata->program->compiled[0];
	enesim_renderer_opengl_shader_ambient_setup(cp->id, thiz->final_color);
	enesim_opengl_target_surface_set(s);
	enesim_opengl_rop_set(rop);
	enesim_opengl_draw_area(area);

	/* don't use any program */
	glUseProgramObjectARB(0);
	enesim_opengl_target_surface_set(NULL);
}
#endif

static Eina_Bool _background_state_setup(Enesim_Renderer_Background *thiz, Enesim_Renderer *r)
{
	Enesim_Color final_color, rend_color;

	final_color = thiz->color;
	rend_color = enesim_renderer_color_get(r);
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
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Background *thiz;
	Enesim_Format fmt = ENESIM_FORMAT_ARGB8888;

 	thiz = ENESIM_RENDERER_BACKGROUND(r);

	if (!_background_state_setup(thiz, r)) return EINA_FALSE;
	thiz->span = enesim_compositor_span_get(rop, &fmt, ENESIM_FORMAT_NONE,
			thiz->final_color, ENESIM_FORMAT_NONE);
	*fill = _background_span;

	return EINA_TRUE;
}

static void _background_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Background *thiz;

 	thiz = ENESIM_RENDERER_BACKGROUND(r);
	thiz->changed = EINA_FALSE;
}

#if BUILD_OPENCL
static Eina_Bool _background_opencl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		const char **program_name, const char **program_source,
		size_t *program_length, Enesim_Log **l)
{
	Enesim_Renderer_Background *thiz;

 	thiz = ENESIM_RENDERER_BACKGROUND(r);
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

 	thiz = ENESIM_RENDERER_BACKGROUND(r);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENCL);
	clSetKernelArg(rdata->kernel, 1, sizeof(cl_uchar4), &thiz->final_color);

	return EINA_TRUE;
}

static void _background_opencl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Background *thiz;

 	thiz = ENESIM_RENDERER_BACKGROUND(r);
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
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Background *thiz;

 	thiz = ENESIM_RENDERER_BACKGROUND(r);
	if (!_background_state_setup(thiz, r)) return EINA_FALSE;

	*draw = _background_opengl_draw;
	return EINA_TRUE;
}

static void _background_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Background *thiz;

 	thiz = ENESIM_RENDERER_BACKGROUND(r);
	_background_state_cleanup(thiz);
}
#endif

static void _background_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_PROJECTIVE |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static void _background_sw_hints(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Rop rop EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_SW_HINT_ROP | ENESIM_RENDERER_SW_HINT_COLORIZE;
}

static Eina_Bool _background_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Background *thiz;

	thiz = ENESIM_RENDERER_BACKGROUND(r);
	if (!thiz->changed) return EINA_FALSE;
	return EINA_TRUE;
}

/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Background, Enesim_Renderer_Background_Class,
		enesim_renderer_background);

static void _enesim_renderer_background_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _background_name;
	klass->bounds_get = NULL;
	klass->features_get = _background_features_get;
	klass->is_inside = NULL;
	klass->damages_get = NULL;
	klass->has_changed =  _background_has_changed;
	klass->alpha_hints_get = NULL;
	klass->sw_hints_get = _background_sw_hints;
	klass->sw_setup = _background_sw_setup;
	klass->sw_cleanup = _background_sw_cleanup;
#if BUILD_OPENCL
	klass->opencl_setup = _background_opencl_setup;
	klass->opencl_kernel_setup = _background_opencl_kernel_setup;
	klass->opencl_cleanup = _background_opencl_cleanup;
#endif
#if BUILD_OPENGL
	klass->opengl_initialize = _background_opengl_initialize;
	klass->opengl_setup = _background_opengl_setup;
	klass->opengl_cleanup = _background_opengl_cleanup;
#endif
}

static void _enesim_renderer_background_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_renderer_background_instance_deinit(void *o EINA_UNUSED)
{
}
/** @endcond */
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

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_background);
	return r;
}

/**
 * @brief Sets the color of the background
 * @param[in] r The background renderer to set the color to
 * @param[in] color The background color
 */
EAPI void enesim_renderer_background_color_set(Enesim_Renderer *r,
		Enesim_Color color)
{
	Enesim_Renderer_Background *thiz;

	thiz = ENESIM_RENDERER_BACKGROUND(r);
	thiz->color = color;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the color of the background
 * @param[in] r The background renderer to get the color from
 * @return The color of the renderer
 */
EAPI Enesim_Color enesim_renderer_background_color_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Background *thiz;

	thiz = ENESIM_RENDERER_BACKGROUND(r);
	return thiz->color;
}
