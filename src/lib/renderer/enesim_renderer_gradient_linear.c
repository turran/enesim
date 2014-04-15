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

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_gradient.h"
#include "enesim_renderer_gradient_linear.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#ifdef BUILD_OPENGL
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
#endif

#include "enesim_renderer_private.h"
#include "enesim_coord_private.h"
#include "enesim_renderer_gradient_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_renderer_gradient

#define ENESIM_RENDERER_GRADIENT_LINEAR(o) ENESIM_OBJECT_INSTANCE_CHECK(o,	\
		Enesim_Renderer_Gradient_Linear,				\
		enesim_renderer_gradient_linear_descriptor_get())

static Enesim_Renderer_Sw_Fill _spans[ENESIM_REPEAT_MODES][ENESIM_MATRIX_TYPES];

typedef struct _Enesim_Renderer_Gradient_Linear_State
{
	double x0;
	double x1;
	double y0;
	double y1;
} Enesim_Renderer_Gradient_Linear_State;

typedef struct _Enesim_Renderer_Gradient_Linear
{
	Enesim_Renderer_Gradient parent;
	/* public properties */
	Enesim_Renderer_Gradient_Linear_State current;
	Enesim_Renderer_Gradient_Linear_State past;
	/* private */
	Eina_Bool changed : 1;
	/* generated at state setup */
	struct {
		Enesim_Matrix_F16p16 matrix;
		Eina_F16p16 xx, yy;
		Eina_F16p16 scale;
		Eina_F16p16 ayx, ayy;
	} sw;
#if BUILD_OPENGL
	struct {
		Enesim_Matrix matrix;
		double xx, yy;
		double scale;
		double ayx, ayy;
	} gl;
#endif
	int length;
} Enesim_Renderer_Gradient_Linear;

typedef struct _Enesim_Renderer_Gradient_Linear_Class {
	Enesim_Renderer_Gradient_Class parent;
} Enesim_Renderer_Gradient_Linear_Class;

#if BUILD_OPENGL
/* the only shader */
static Enesim_Renderer_OpenGL_Shader _linear_shader = {
	/* .type 	= */ ENESIM_SHADER_FRAGMENT,
	/* .name 	= */ "linear",
	/* .source	= */
#include "enesim_renderer_gradient_linear.glsl"
};

static Enesim_Renderer_OpenGL_Shader *_linear_shaders[] = {
	&_linear_shader,
	NULL,
};

/* the only program */
static Enesim_Renderer_OpenGL_Program _linear_program = {
	/* .name 		= */ "linear",
	/* .shaders 		= */ _linear_shaders,
	/* .num_shaders		= */ 1,
};

static Enesim_Renderer_OpenGL_Program *_linear_programs[] = {
	&_linear_program,
	NULL,
};

static void _linear_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, const Eina_Rectangle *area, int x, int y)
{
	Enesim_Renderer_Gradient_Linear *thiz;
	Enesim_Renderer_Gradient *g;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_OpenGL_Compiled_Program *cp;
	Enesim_Matrix m1;
	Enesim_Matrix tx;
	float matrix[16];
	int ay_u;
	int o_u;
	int scale_u;
	/* common gradient */
	int length_u;
	int stops_u;
	int repeat_u;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	g = ENESIM_RENDERER_GRADIENT(r);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	cp = &rdata->program->compiled[0];
	glUseProgramObjectARB(cp->id);

	/* upload the parameters */
	scale_u = glGetUniformLocationARB(cp->id, "linear_scale");
	ay_u = glGetUniformLocationARB(cp->id, "linear_ay");
	o_u = glGetUniformLocationARB(cp->id, "linear_o");
	length_u = glGetUniformLocationARB(cp->id, "gradient_length");
	stops_u = glGetUniformLocationARB(cp->id, "gradient_stops");
	repeat_u = glGetUniformLocationARB(cp->id, "gradient_repeat_mode");

	glUniform2f(ay_u, thiz->gl.ayx, thiz->gl.ayy);
	glUniform2f(o_u, thiz->gl.xx, thiz->gl.yy);
	glUniform1f(scale_u, thiz->gl.scale);

	/* common gradient */
	glUniform1i(length_u, thiz->length);
	glUniform1i(repeat_u, g->state.mode);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_1D, g->gl.gen_stops);
	glUniform1i(stops_u, 0);

	/* draw */
	enesim_opengl_target_surface_set(s);
	enesim_opengl_rop_set(rop);

	/* set our transformation matrix */
	enesim_matrix_translate(&tx, -x, -y);
	enesim_matrix_compose(&thiz->gl.matrix, &tx, &m1);
	enesim_opengl_matrix_convert(&m1, matrix);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	glMultMatrixf(matrix);
	
	/* draw the specified area */
	enesim_opengl_draw_area(area);

	/* don't use any program */
	glUseProgramObjectARB(0);
	enesim_opengl_target_surface_set(NULL);
	glBindTexture(GL_TEXTURE_1D, 0);
}
#endif

/* get the input on origin coordinates and return the distance on destination
 * coordinates
 * FIXME the next iteration will use its own functions for drawing so we will
 * not need to do a transformation twice
 */
static Eina_F16p16 _linear_distance(Enesim_Renderer_Gradient_Linear *thiz, Eina_F16p16 x,
		Eina_F16p16 y)
{
	Eina_F16p16 a, b;
	Eina_F16p16 d;

	/* input is on origin coordinates */
	x = x - thiz->sw.xx;
	y = y - thiz->sw.yy;
	a = eina_f16p16_mul(thiz->sw.ayx, x);
	b = eina_f16p16_mul(thiz->sw.ayy, y);
	d = eina_f16p16_add(a, b);
	/* d is on origin coordinates */
	d = eina_f16p16_mul(d, thiz->sw.scale);

	return d;
}

#if 0
/* This will be enabled later */
static void _argb8888_pad_span_identity(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Gradient_Linear *thiz;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	Eina_F16p16 xx, yy;
	Eina_F16p16 d;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	enesim_coord_identity_setup(r, x, y, &xx, &yy);
	d = _linear_distance_internal(thiz, xx, yy);
	while (dst < end)
	{
		*dst++ = enesim_renderer_gradient_color_get(r, d);
		d += thiz->ayx;
	}
}
#endif

GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, restrict);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, repeat);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, pad);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, reflect);

GRADIENT_AFFINE(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, restrict);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, repeat);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, pad);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, reflect);

GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, restrict);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, repeat);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, pad);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Linear, ENESIM_RENDERER_GRADIENT_LINEAR, _linear_distance, reflect);


static Eina_Bool _linear_setup(Enesim_Renderer *r, Enesim_Matrix *m,
		double *ayx, double *ayy, double *scale)
{
	Enesim_Renderer_Gradient_Linear *thiz;
	Enesim_Matrix_Type type;
	double xx0, yy0;
	double x0, x1, y0, y1;
	double orig_len;
	double dst_len;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	x0 = thiz->current.x0;
	x1 = thiz->current.x1;
	y0 = thiz->current.y0;
	y1 = thiz->current.y1;

	/* calculate the increment on x and y */
	xx0 = x1 - x0;
	yy0 = y1 - y0;
	orig_len = hypot(xx0, yy0);
	*ayx = xx0 / orig_len;
	*ayy = yy0 / orig_len;

	/* TODO check that the difference between x0 - x1 and y0 - y1 is < tolerance */
	/* handle the geometry transformation */
	type = enesim_renderer_transformation_type_get(r);
	if (type != ENESIM_MATRIX_IDENTITY)
	{
		Enesim_Matrix om;

		enesim_renderer_transformation_get(r, &om);
		enesim_matrix_point_transform(&om, x0, y0, &x0, &y0);
		enesim_matrix_point_transform(&om, x1, y1, &x1, &y1);
		enesim_matrix_inverse(&om, m);
		dst_len = hypot(x1 - x0, y1 - y0);
	}
	else
	{
		dst_len = orig_len;
		enesim_matrix_identity(m);
	}

	/* the scale factor, this is useful to know the original length and
	 * transformed length scale
	 */
	*scale = dst_len / orig_len;
	/* set the length */
	thiz->length = ceil(dst_len);

	return EINA_TRUE;
}
/*----------------------------------------------------------------------------*
 *                The Enesim's gradient renderer interface                    *
 *----------------------------------------------------------------------------*/
static const char * _linear_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "gradient_linear";
}

static int _linear_length(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	return thiz->length;
}

static void _linear_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	thiz->changed = EINA_FALSE;
	thiz->past = thiz->current;
}

static Eina_Bool _linear_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Gradient_Linear *thiz;
	Enesim_Repeat_Mode mode;
	Enesim_Matrix_Type type;
	Enesim_Matrix m;
	double scale;
	double ayx, ayy;

	if (!_linear_setup(r, &m, &ayx, &ayy, &scale))
		return EINA_FALSE;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	/* we need to translate by the x0 and y0 */
	thiz->sw.xx = eina_f16p16_double_from(thiz->current.x0);
	thiz->sw.yy = eina_f16p16_double_from(thiz->current.y0);

	thiz->sw.scale = eina_f16p16_double_from(scale);
	thiz->sw.ayx = eina_f16p16_double_from(ayx);
	thiz->sw.ayy = eina_f16p16_double_from(ayy);
	enesim_matrix_matrix_f16p16_to(&m, &thiz->sw.matrix);
#if 0
	/* just override the identity case */
	if (type == ENESIM_MATRIX_IDENTITY)
		*fill = _argb8888_pad_span_identity;
#endif
	type = enesim_renderer_transformation_type_get(r);
	mode = enesim_renderer_gradient_repeat_mode_get(r);
	*fill = _spans[mode][type];

	return EINA_TRUE;
}

static Eina_Bool _linear_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);

	if (!thiz->changed)
		return EINA_FALSE;

	if (thiz->current.x0 != thiz->past.x0)
	{
		return EINA_TRUE;
	}
	if (thiz->current.x1 != thiz->past.x1)
	{
		return EINA_TRUE;
	}
	if (thiz->current.y0 != thiz->past.y0)
	{
		return EINA_TRUE;
	}
	if (thiz->current.y1 != thiz->past.y1)
	{
		return EINA_TRUE;
	}
	return EINA_FALSE;
}

#if BUILD_OPENGL
static Eina_Bool _linear_opengl_initialize(Enesim_Renderer *r EINA_UNUSED,
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***programs)
{
	*num_programs = 1;
	*programs = _linear_programs;
	return EINA_TRUE;
}

static Eina_Bool _linear_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	if (!_linear_setup(r, &thiz->gl.matrix, &thiz->gl.ayx, &thiz->gl.ayy,
				&thiz->gl.scale))
		return EINA_FALSE;
	/* we need to translate by the x0 and y0 */
	thiz->gl.xx = thiz->current.x0;
	thiz->gl.yy = thiz->current.y0;

	*draw = _linear_opengl_draw;
	return EINA_TRUE;
}

static void _linear_opengl_cleanup(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Surface *s EINA_UNUSED)
{
}
#endif
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_GRADIENT_DESCRIPTOR,
		Enesim_Renderer_Gradient_Linear,
		Enesim_Renderer_Gradient_Linear_Class,
		enesim_renderer_gradient_linear);

static void _enesim_renderer_gradient_linear_class_init(void *k)
{
	Enesim_Renderer_Class *r_klass;
	Enesim_Renderer_Gradient_Class *klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _linear_name;
#if BUILD_OPENGL
	r_klass->opengl_initialize = _linear_opengl_initialize;
#endif

	klass = ENESIM_RENDERER_GRADIENT_CLASS(k);
	klass->length = _linear_length;
	klass->has_changed = _linear_has_changed;
	klass->sw_setup = _linear_sw_setup;
	klass->sw_cleanup = _linear_sw_cleanup;
#if BUILD_OPENGL
	klass->opengl_setup = _linear_opengl_setup;
	klass->opengl_cleanup = _linear_opengl_cleanup;
#endif

	_spans[ENESIM_REPEAT_MODE_REPEAT][ENESIM_MATRIX_IDENTITY] = _argb8888_repeat_span_identity;
	_spans[ENESIM_REPEAT_MODE_REPEAT][ENESIM_MATRIX_AFFINE] = _argb8888_repeat_span_affine;
	_spans[ENESIM_REPEAT_MODE_REPEAT][ENESIM_MATRIX_PROJECTIVE] = _argb8888_repeat_span_projective;
	_spans[ENESIM_REPEAT_MODE_REFLECT][ENESIM_MATRIX_IDENTITY] = _argb8888_reflect_span_identity;
	_spans[ENESIM_REPEAT_MODE_REFLECT][ENESIM_MATRIX_AFFINE] = _argb8888_reflect_span_affine;
	_spans[ENESIM_REPEAT_MODE_REFLECT][ENESIM_MATRIX_PROJECTIVE] = _argb8888_reflect_span_projective;
	_spans[ENESIM_REPEAT_MODE_RESTRICT][ENESIM_MATRIX_IDENTITY] = _argb8888_restrict_span_identity;
	_spans[ENESIM_REPEAT_MODE_RESTRICT][ENESIM_MATRIX_AFFINE] = _argb8888_restrict_span_affine;
	_spans[ENESIM_REPEAT_MODE_RESTRICT][ENESIM_MATRIX_PROJECTIVE] = _argb8888_restrict_span_projective;
	_spans[ENESIM_REPEAT_MODE_PAD][ENESIM_MATRIX_IDENTITY] = _argb8888_pad_span_identity;
	_spans[ENESIM_REPEAT_MODE_PAD][ENESIM_MATRIX_AFFINE] = _argb8888_pad_span_affine;
	_spans[ENESIM_REPEAT_MODE_PAD][ENESIM_MATRIX_PROJECTIVE] = _argb8888_pad_span_projective;
}

static void _enesim_renderer_gradient_linear_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_renderer_gradient_linear_instance_deinit(void *o EINA_UNUSED)
{
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Create a new linear gradient renderer.
 *
 * @return A new linear gradient renderer.
 *
 * This function returns a newly allocated linear gradient renderer.
 */
EAPI Enesim_Renderer * enesim_renderer_gradient_linear_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_gradient_linear);
	return r;
}

/**
 * @brief Set the position of the linear gradient renderer.
 *
 * @param[in] r The linear gradient renderer.
 * @param[in] x0 The initial X coordinate
 * @param[in] y0 The initial Y coordinate
 * @param[in] x1 The final X coordinate
 * @param[in] y1 The final Y coordinate
 *
 * This function sets the position of the linear gradient renderer
 * @p r with the values @p x0, @p y0, @p x1, @p y1.
 */
EAPI void enesim_renderer_gradient_linear_position_set(Enesim_Renderer *r, double x0,
		double y0, double x1, double y1)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);

	thiz->current.x0 = x0;
	thiz->current.x1 = x1;
	thiz->current.y0 = y0;
	thiz->current.y1 = y1;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the position of the linear gradient renderer.
 *
 * @param[in] r The linear gradient renderer.
 * @param[out] x0 The initial X coordinate
 * @param[out] y0 The initial Y coordinate
 * @param[out] x1 The final X coordinate
 * @param[out] y1 The final Y coordinate
 *
 * This function gets the position of the linear gradient renderer
 * @p r in the pointers @p x0, @p y0, @p x1, @p y1.
 */
EAPI void enesim_renderer_gradient_linear_position_get(Enesim_Renderer *r, double *x0,
		double *y0, double *x1, double *y1)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);

	if (x0)
		*x0 = thiz->current.x0;
	if (y0)
		*y0 = thiz->current.y0;
	if (x1)
		*x1 = thiz->current.x1;
	if (y1)
		*y1 = thiz->current.y1;
}

/**
 * @brief Set the initial X coordinate of a linear gradient renderer.
 *
 * @param[in] r The linear gradient renderer.
 * @param[in] x0 The initial X coordinate
 *
 * This function sets the initial X coordinate of a the linear gradient
 * renderer @p r to the value @p x0.
 */
EAPI void enesim_renderer_gradient_linear_x0_set(Enesim_Renderer *r, double x0)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	thiz->current.x0 = x0;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the initial X coordinate of a linear gradient renderer.
 *
 * @param[in] r The linear gradient renderer.
 * @return The initial X coordinate
 *
 * This function gets the initial X coordinate of a the linear gradient
 * renderer @p r.
 */
EAPI double enesim_renderer_gradient_linear_x0_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	return thiz->current.x0;
}

/**
 * @brief Set the initial Y coordinate of a linear gradient renderer.
 *
 * @param[in] r The linear gradient renderer.
 * @param[in] y0 The initial Y coordinate
 *
 * This function sets the initial Y coordinate of a the linear gradient
 * renderer @p r to the value @p y0.
 */
EAPI void enesim_renderer_gradient_linear_y0_set(Enesim_Renderer *r, double y0)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	thiz->current.y0 = y0;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the initial Y coordinate of a linear gradient renderer.
 *
 * @param[in] r The linear gradient renderer.
 * @return The initial Y coordinate
 *
 * This function gets the initial Y coordinate of a the linear gradient
 * renderer @p r.
 */
EAPI double enesim_renderer_gradient_linear_y0_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	return thiz->current.y0;
}

/**
 * @brief Set the final X coordinate of a linear gradient renderer.
 *
 * @param[in] r The linear gradient renderer.
 * @param[in] x1 The final X coordinate
 *
 * This function sets the final X coordinate of a the linear gradient
 * renderer @p r to the value @p x1.
 */
EAPI void enesim_renderer_gradient_linear_x1_set(Enesim_Renderer *r, double x1)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	thiz->current.x1 = x1;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the final X coordinate of a linear gradient renderer.
 *
 * @param[in] r The linear gradient renderer.
 * @return The final X coordinate
 *
 * This function gets the final X coordinate of a the linear gradient
 * renderer @p r.
 */
EAPI double enesim_renderer_gradient_linear_x1_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	return thiz->current.x1;
}

/**
 * @brief Set the final Y coordinate of a linear gradient renderer.
 *
 * @param[in] r The linear gradient renderer.
 * @param[in] y1 The final Y coordinate
 *
 * This function sets the final Y coordinate of a the linear gradient
 * renderer @p r to the value @p y1.
 */
EAPI void enesim_renderer_gradient_linear_y1_set(Enesim_Renderer *r, double y1)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	thiz->current.y1 = y1;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the final Y coordinate of a linear gradient renderer.
 *
 * @param[in] r The linear gradient renderer.
 * @return The final Y coordinate
 *
 * This function gets the final Y coordinate of a the linear gradient
 * renderer @p r.
 */
EAPI double enesim_renderer_gradient_linear_y1_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Linear *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_LINEAR(r);
	return thiz->current.y1;
}

