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
#include "enesim_renderer_gradient_radial.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_renderer_private.h"
#include "enesim_coord_private.h"
#include "enesim_renderer_gradient_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_renderer_gradient_radial

#define ENESIM_RENDERER_GRADIENT_RADIAL(o) ENESIM_OBJECT_INSTANCE_CHECK(o,	\
		Enesim_Renderer_Gradient_Radial,				\
		enesim_renderer_gradient_radial_descriptor_get())

static Enesim_Renderer_Sw_Fill _spans[ENESIM_REPEAT_MODES][ENESIM_MATRIX_TYPES];

typedef struct _Enesim_Renderer_Gradient_Radial
{
	Enesim_Renderer_Gradient parent;
	/* properties */
	struct {
		double x, y;
	} center, focus;
	double radius;
	/* state generated */
	struct {
		Enesim_F16p16_Matrix matrix;
	} sw;
	double r, zf;
	double fx, fy;
	double scale;
	int glen;
#if BUILD_OPENGL
	struct {
		Enesim_Matrix matrix;
	} gl;
#endif
	Eina_Bool simple : 1;
	Eina_Bool changed : 1;
} Enesim_Renderer_Gradient_Radial;

typedef struct _Enesim_Renderer_Gradient_Radial_Class {
	Enesim_Renderer_Gradient_Class parent;
} Enesim_Renderer_Gradient_Radial_Class;

static inline Eina_F16p16 _radial_distance(Enesim_Renderer_Gradient_Radial *thiz,
		Eina_F16p16 x, Eina_F16p16 y)
{
	Eina_F16p16 ret;
	double r = thiz->r, fx = thiz->fx, fy = thiz->fy;
	double a, b;
	double d1, d2;

	if (thiz->simple)
	{
		a = x - 65536 * thiz->center.x;
		b = y - 65536 * thiz->center.y;
		ret = thiz->scale * sqrt(a*a + b*b);
		return ret;
	}

	a = thiz->scale * (eina_f16p16_double_to(x) - (fx + thiz->center.x));
	b = thiz->scale * (eina_f16p16_double_to(y) - (fy + thiz->center.y));

	d1 = (a * fy) - (b * fx);
	d2 = fabs(((r * r) * ((a * a) + (b * b))) - (d1 * d1));
	r = ((a * fx) + (b * fy) + sqrt(d2)) * thiz->zf;

	ret = eina_f16p16_double_from(r);
	return ret;
}

#if BUILD_OPENGL
/* the only shader */
static Enesim_Renderer_OpenGL_Shader _radial_shader = {
	/* .type 	= */ ENESIM_SHADER_FRAGMENT,
	/* .name 	= */ "radial",
	/* .source	= */
#include "enesim_renderer_gradient_radial.glsl"
};

static Enesim_Renderer_OpenGL_Shader *_radial_shaders[] = {
	&_radial_shader,
	NULL,
};

/* the only program */
static Enesim_Renderer_OpenGL_Program _radial_program = {
	/* .name 		= */ "radial",
	/* .shaders 		= */ _radial_shaders,
	/* .num_shaders		= */ 1,
};

static Enesim_Renderer_OpenGL_Program *_radial_programs[] = {
	&_radial_program,
	NULL,
};

static void _radial_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, const Eina_Rectangle *area, int x, int y)
{
	Enesim_Renderer_Gradient_Radial *thiz;
	Enesim_Renderer_Gradient *g;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_OpenGL_Compiled_Program *cp;
	Enesim_Matrix m1;
	Enesim_Matrix tx;
	float matrix[16];
	int simple_u;
	int c_u;
	int f_u;
	int scale_u;
	int rad2_u;
	int zf_u;
	/* common gradient */
	int length_u;
	int stops_u;
	int repeat_u;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	g = ENESIM_RENDERER_GRADIENT(r);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	cp = &rdata->program->compiled[0];
	glUseProgramObjectARB(cp->id);

	/* upload the parameters */
	simple_u = glGetUniformLocationARB(cp->id, "radial_simple");
	c_u = glGetUniformLocationARB(cp->id, "radial_c");
	f_u = glGetUniformLocationARB(cp->id, "radial_f");
	scale_u = glGetUniformLocationARB(cp->id, "radial_scale");
	rad2_u = glGetUniformLocationARB(cp->id, "radial_rad2");
	zf_u = glGetUniformLocationARB(cp->id, "radial_zf");
	length_u = glGetUniformLocationARB(cp->id, "gradient_length");
	stops_u = glGetUniformLocationARB(cp->id, "gradient_stops");
	repeat_u = glGetUniformLocationARB(cp->id, "gradient_repeat_mode");

	glUniform1i(simple_u, thiz->simple);
	glUniform2f(c_u, thiz->center.x, thiz->center.y);
	glUniform2f(f_u, thiz->fx, thiz->fy);
	glUniform1f(scale_u, thiz->scale);
	glUniform1f(rad2_u, thiz->r * thiz->r);
	glUniform1f(zf_u, thiz->zf);

	/* common gradient */
	glUniform1i(length_u, thiz->glen);
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

static Eina_Bool _radial_setup(Enesim_Renderer *r, Enesim_Matrix *m)
{
	Enesim_Renderer_Gradient_Radial *thiz;
	Enesim_Matrix_Type type;
	double cx, cy;
	double fx, fy;
	double rad, scale, small = (1 / 8192.0);
	int glen;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	cx = thiz->center.x;
	cy = thiz->center.y;
	rad = fabs(thiz->radius);

	if (rad < small)
		return EINA_FALSE;
	thiz->r = rad;

	scale = 1;
	glen = ceil(rad) + 1;

	type = enesim_renderer_transformation_type_get(r);
	if (type != ENESIM_MATRIX_IDENTITY)
	{
		Enesim_Matrix om;
		double mx, my;

		enesim_renderer_transformation_get(r, &om);
		mx = hypot(om.xx, om.yx);
		my = hypot(om.xy, om.yy);
		scale = hypot(mx, my) / sqrt(2);
		glen = ceil(rad * scale) + 1;

		enesim_matrix_inverse(&om, m);
	}
	else
	{
		enesim_matrix_identity(m);
	}

	if (glen < 4)
	{
		scale = 3 / rad;
		glen = 4;
	}

	thiz->scale = scale;
	thiz->glen = glen;

	fx = thiz->focus.x;
	fy = thiz->focus.y;
	scale = hypot(fx - cx, fy - cy);
	if (scale + small >= rad)
	{
		double t = rad / (scale + small);

		fx = cx + t * (fx - cx);
		fy = cy + t * (fy - cy);
	}

	fx -= cx;  fy -= cy;
	thiz->fx = fx;  thiz->fy = fy;
	thiz->zf = rad / (rad*rad - (fx*fx + fy*fy));

	thiz->simple = 0;
	if ((fabs(fx) < small) && (fabs(fy) < small))
		thiz->simple = 1;
	return EINA_TRUE;
}


GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, restrict);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, repeat);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, pad);
GRADIENT_IDENTITY(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, reflect);

GRADIENT_AFFINE(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, restrict);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, repeat);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, pad);
GRADIENT_AFFINE(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, reflect);

GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, restrict);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, repeat);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, pad);
GRADIENT_PROJECTIVE(Enesim_Renderer_Gradient_Radial, ENESIM_RENDERER_GRADIENT_RADIAL, _radial_distance, reflect);
/*----------------------------------------------------------------------------*
 *                The Enesim's gradient renderer interface                    *
 *----------------------------------------------------------------------------*/
static int _radial_length(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	return thiz->glen;
}

static const char * _radial_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "gradient_radial";
}

static void _radial_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Gradient_Radial *thiz;
	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	thiz->changed = EINA_FALSE;
}

static Eina_Bool _radial_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l EINA_UNUSED)
{
	Enesim_Matrix_Type type;
	Enesim_Repeat_Mode mode;
	Enesim_Matrix m;
	Enesim_Renderer_Gradient_Radial *thiz;

	if (!_radial_setup(r, &m))
		return EINA_FALSE;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);

	type = enesim_matrix_type_get(&m);
	enesim_matrix_f16p16_matrix_to(&m, &thiz->sw.matrix);
	mode = enesim_renderer_gradient_repeat_mode_get(r);
	*fill = _spans[mode][type];

	return EINA_TRUE;
}

static void _radial_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);

	bounds->x = thiz->center.x - fabs(thiz->radius);
	bounds->y = thiz->center.y - fabs(thiz->radius);
	bounds->w = fabs(thiz->radius) * 2;
	bounds->h = fabs(thiz->radius) * 2;
}

static Eina_Bool _radial_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	if (!thiz->changed)
		return EINA_FALSE;
	/* TODO check if we have really changed */
	return EINA_TRUE;
}

#if BUILD_OPENGL
static Eina_Bool _radial_opengl_initialize(Enesim_Renderer *r EINA_UNUSED,
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***programs)
{
	*num_programs = 1;
	*programs = _radial_programs;
	return EINA_TRUE;
}

static Eina_Bool _radial_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Gradient_Radial *thiz;
	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);

	if (!_radial_setup(r, &thiz->gl.matrix))
		return EINA_FALSE;
	
	*draw = _radial_opengl_draw;
	return EINA_TRUE;
}

static void _radial_opengl_cleanup(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Surface *s EINA_UNUSED)
{
}
#endif
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_GRADIENT_DESCRIPTOR,
		Enesim_Renderer_Gradient_Radial,
		Enesim_Renderer_Gradient_Radial_Class,
		enesim_renderer_gradient_radial);

static void _enesim_renderer_gradient_radial_class_init(void *k)
{
	Enesim_Renderer_Class *r_klass;
	Enesim_Renderer_Gradient_Class *klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _radial_name;
#if BUILD_OPENGL
	r_klass->opengl_initialize = _radial_opengl_initialize;
#endif

	klass = ENESIM_RENDERER_GRADIENT_CLASS(k);
	klass->length = _radial_length;
	klass->has_changed = _radial_has_changed;
	klass->sw_setup = _radial_sw_setup;
	klass->sw_cleanup = _radial_sw_cleanup;
#if BUILD_OPENGL
	klass->opengl_setup = _radial_opengl_setup;
	klass->opengl_cleanup = _radial_opengl_cleanup;
#endif
	klass->bounds_get = _radial_bounds_get;

	_spans[ENESIM_REPEAT][ENESIM_MATRIX_IDENTITY] = _argb8888_repeat_span_identity;
	_spans[ENESIM_REPEAT][ENESIM_MATRIX_AFFINE] = _argb8888_repeat_span_affine;
	_spans[ENESIM_REPEAT][ENESIM_MATRIX_PROJECTIVE] = _argb8888_repeat_span_projective;
	_spans[ENESIM_REFLECT][ENESIM_MATRIX_IDENTITY] = _argb8888_reflect_span_identity;
	_spans[ENESIM_REFLECT][ENESIM_MATRIX_AFFINE] = _argb8888_reflect_span_affine;
	_spans[ENESIM_REFLECT][ENESIM_MATRIX_PROJECTIVE] = _argb8888_reflect_span_projective;
	_spans[ENESIM_RESTRICT][ENESIM_MATRIX_IDENTITY] = _argb8888_restrict_span_identity;
	_spans[ENESIM_RESTRICT][ENESIM_MATRIX_AFFINE] = _argb8888_restrict_span_affine;
	_spans[ENESIM_RESTRICT][ENESIM_MATRIX_PROJECTIVE] = _argb8888_restrict_span_projective;
	_spans[ENESIM_PAD][ENESIM_MATRIX_IDENTITY] = _argb8888_pad_span_identity;
	_spans[ENESIM_PAD][ENESIM_MATRIX_AFFINE] = _argb8888_pad_span_affine;
	_spans[ENESIM_PAD][ENESIM_MATRIX_PROJECTIVE] = _argb8888_pad_span_projective;
}

static void _enesim_renderer_gradient_radial_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_renderer_gradient_radial_instance_deinit(void *o EINA_UNUSED)
{
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates radial gradient renderer
 * @return The new renderer
 */
EAPI Enesim_Renderer * enesim_renderer_gradient_radial_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_gradient_radial);
	return r;
}

/**
 * Set the center of a radial gradient renderer
 * @param[in] r The gradient renderer to set the center on
 * @param[in] center_x The X coordinate of the center
 * @param[in] center_y The Y coordinate of the center
 */
EAPI void enesim_renderer_gradient_radial_center_set(Enesim_Renderer *r,
		double center_x, double center_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	thiz->center.x = center_x;
	thiz->center.y = center_y;
	thiz->changed = EINA_TRUE;
}

/**
 * Get the center of a radial gradient renderer
 * @param[in] r The gradient renderer to get the center from
 * @param[out] center_x The pointer to store the X coordinate center
 * @param[out] center_y The pointer to store the Y coordinate center
 */
EAPI void enesim_renderer_gradient_radial_center_get(Enesim_Renderer *r,
		double *center_x, double *center_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	if (center_x)
		*center_x = thiz->center.x;
	if (center_y)
		*center_y = thiz->center.y;
}

/**
 * Set the X coordinate center of a radial gradient renderer
 * @param[in] r The gradient renderer to set the center on
 * @param[in] center_x The X coordinate of the center
 */
EAPI void enesim_renderer_gradient_radial_center_x_set(Enesim_Renderer *r, double center_x)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	thiz->center.x = center_x;
	thiz->changed = EINA_TRUE;
}

/**
 * Set the Y coordinate center of a radial gradient renderer
 * @param[in] r The gradient renderer to set the center on
 * @param[in] center_y The Y coordinate of the center
 */
EAPI void enesim_renderer_gradient_radial_center_y_set(Enesim_Renderer *r, double center_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	thiz->center.y = center_y;
	thiz->changed = EINA_TRUE;
}

/**
 * Get the X coordinate center of a radial gradient renderer
 * @param[in] r The gradient renderer to get the center from
 * @return The X coordinate of the center
 */
EAPI double enesim_renderer_gradient_radial_center_x_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	return thiz->center.x;
}

/**
 * Get the Y coordinate center of a radial gradient renderer
 * @param[in] r The gradient renderer to get the center from
 * @return The Y coordinate of the center
 */
EAPI double enesim_renderer_gradient_radial_center_y_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	return thiz->center.y;
}

/**
 * Set the focus position of a radial gradient renderer
 * @param[in] r The gradient renderer to set the focus position on
 * @param[in] focus_x The X coordinate of the focus
 * @param[in] focus_y The Y coordinate of the focus
 */
EAPI void enesim_renderer_gradient_radial_focus_set(Enesim_Renderer *r, double focus_x, double focus_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	thiz->focus.x = focus_x;
	thiz->focus.y = focus_y;
	thiz->changed = EINA_TRUE;
}

/**
 * Get the focus position of a radial gradient renderer
 * @param[in] r The gradient renderer to get the center from
 * @param[out] focus_x The pointer to store the X coordinate of the focus
 * @param[out] focus_y The pointer to store the Y coordinate of the focus
 */
EAPI void enesim_renderer_gradient_radial_focus_get(Enesim_Renderer *r, 
							double *focus_x, double *focus_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	if (focus_x)
		*focus_x = thiz->focus.x;
	if (focus_y)
		*focus_y = thiz->focus.y;
}

/**
 * Set the focus X coordinate of a radial gradient renderer
 * @param[in] r The gradient renderer to set the focus X coordinate on
 * @param[in] focus_x The X coordinate of the focus
 */
EAPI void enesim_renderer_gradient_radial_focus_x_set(Enesim_Renderer *r, double focus_x)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	thiz->focus.x = focus_x;
	thiz->changed = EINA_TRUE;
}

/**
 * Set the focus Y coordinate of a radial gradient renderer
 * @param[in] r The gradient renderer to set the focus Y coordinate on
 * @param[in] focus_y The Y coordinate of the focus
 */
EAPI void enesim_renderer_gradient_radial_focus_y_set(Enesim_Renderer *r, double focus_y)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	thiz->focus.y = focus_y;
	thiz->changed = EINA_TRUE;
}

/**
 * Get the focus X coordinate of a radial gradient renderer
 * @param[in] r The gradient renderer to get the X coordinate of the focus from
 * @return The Y coordinate of the focus
 */
EAPI double enesim_renderer_gradient_radial_focus_x_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	return thiz->focus.x;
}

/**
 * Get the focus Y coordinate of a radial gradient renderer
 * @param[in] r The gradient renderer to get the Y coordinate of the focus from
 * @return The Y coordinate of the focus
 */
EAPI double enesim_renderer_gradient_radial_focus_y_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	return thiz->focus.y;
}

/**
 * @brief Set the radius of a radial gradient renderer.
 *
 * @param[in] r The radial gradient renderer.
 * @param[in] radius The radius.
 *
 * This function sets the radius of the radial gradient renderer @p r to the
 * value @p radius.
 */
EAPI void enesim_renderer_gradient_radial_radius_set(Enesim_Renderer *r, double radius)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	thiz->radius = radius;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the radius of a radial gradient renderer.
 *
 * @param[in] r The radial gradient renderer.
 * @return The radius
 */
EAPI double enesim_renderer_gradient_radial_radius_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = ENESIM_RENDERER_GRADIENT_RADIAL(r);
	return thiz->radius;
}
