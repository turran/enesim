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
#include "enesim_renderer_image.h"
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
/**
 * @todo
 * - add support for sw and sh
 * - add support for qualities (good scaler, interpolate between the four neighbours)
 * - all the span functions have a memset ... that has to be fixed to:
 *   + whenever the color = 0, use a image renderer and call it directly
 *   + on sw hints check the same condition and use the image renderer hints
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_LOG_DEFAULT enesim_log_renderer_image

#define ENESIM_RENDERER_IMAGE(o) ENESIM_OBJECT_INSTANCE_CHECK(o,	\
		Enesim_Renderer_Image,					\
		enesim_renderer_image_descriptor_get())

typedef struct _Enesim_Renderer_Image_State
{
	Enesim_Surface *s;
	double x, y;
	double w, h;
} Enesim_Renderer_Image_State;

typedef struct _Enesim_Renderer_Image
{
	Enesim_Renderer parent;
	Enesim_Renderer_Image_State current;
	Enesim_Renderer_Image_State past;
	/* private */
	Enesim_Color color;
	uint32_t *src;
	int sw, sh;
	size_t sstride;
	Eina_F16p16 ixx, iyy;
	Eina_F16p16 iww, ihh;
	Eina_F16p16 mxx, myy;
	Eina_F16p16 nxx, nyy;
	Enesim_Matrix_F16p16 matrix;
	Enesim_Compositor_Span span;
#if BUILD_OPENGL
	struct {
		Enesim_Surface *s;
	} gl;
#endif
	Eina_List *surface_damages;
	Eina_Bool simple : 1;
	Eina_Bool changed : 1;
	Eina_Bool src_changed : 1;
} Enesim_Renderer_Image;

typedef struct _Enesim_Renderer_Image_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Image_Class;

static void _image_transform_bounds(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Matrix *m,
		Enesim_Matrix_Type type,
		Enesim_Rectangle *obounds,
		Enesim_Rectangle *bounds)
{
	*bounds = *obounds;
	if (type != ENESIM_MATRIX_TYPE_IDENTITY)
	{
		Enesim_Quad q;

		enesim_matrix_rectangle_transform(m, bounds, &q);
		enesim_quad_rectangle_to(&q, bounds);
	}
}

static void _image_transform_destination_bounds(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Matrix *tx EINA_UNUSED,
		Enesim_Matrix_Type type EINA_UNUSED,
		Enesim_Rectangle *obounds,
		Eina_Rectangle *bounds)
{
	enesim_rectangle_normalize(obounds, bounds);
}

static Eina_Bool _image_state_setup(Enesim_Renderer *r, Enesim_Log **l)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(r);
	if (!thiz->current.s)
	{
		ENESIM_RENDERER_LOG(r, l, "No surface set");
		return EINA_FALSE;
	}

	enesim_surface_lock(thiz->current.s, EINA_FALSE);
	thiz->color = enesim_renderer_color_get(r);
	return EINA_TRUE;
}

static void _image_state_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Image *thiz;
	Eina_Rectangle *sd;

	thiz = ENESIM_RENDERER_IMAGE(r);
	thiz->changed = EINA_FALSE;
	thiz->src_changed = EINA_FALSE;
	if (thiz->current.s)
		enesim_surface_unlock(thiz->current.s);
	/* swap the states */
	if (thiz->past.s)
	{
		enesim_surface_unref(thiz->past.s);
		thiz->past.s = NULL;
	}
	thiz->past.s = enesim_surface_ref(thiz->current.s);
	thiz->past.x = thiz->current.x;
	thiz->past.y = thiz->current.y;
	thiz->past.w = thiz->current.w;
	thiz->past.h = thiz->current.h;

	EINA_LIST_FREE(thiz->surface_damages, sd)
		free(sd);
}

#if BUILD_OPENGL
static Eina_Bool _image_gl_create(Enesim_Renderer_Image *thiz,
		Enesim_Surface *s)
{
	/* create our own gl texture */
	if (thiz->current.s != thiz->past.s)
	{
		if (thiz->gl.s)
		{
			enesim_surface_unref(thiz->gl.s);
			thiz->gl.s = NULL;
		}
		/* upload the texture if we need to */
		if (enesim_surface_backend_get(thiz->current.s) !=
				ENESIM_BACKEND_OPENGL)
		{
			Enesim_Pool *pool;
			size_t sstride;
			void *sdata;
			int w, h;

			enesim_surface_size_get(thiz->current.s, &w, &h);
			if (!w || !h)
				return EINA_FALSE;

			enesim_surface_sw_data_get(thiz->current.s, &sdata,
					&sstride);
			pool = enesim_surface_pool_get(s);
			thiz->gl.s = enesim_surface_new_pool_and_data_from(
					ENESIM_FORMAT_ARGB8888, w, h, pool,
					EINA_TRUE, sdata, sstride, NULL, NULL);
			if (!thiz->gl.s)
			{
				WRN("Impossible to create the gl surface");
				return EINA_FALSE;
			}
		}
	}
	return EINA_TRUE;
}

/* the only shader */
static Enesim_Renderer_OpenGL_Shader _image_shader = {
	/* .type 	= */ ENESIM_RENDERER_OPENGL_SHADER_FRAGMENT,
	/* .name 	= */ "image",
	/* .source	= */
#include "enesim_renderer_opengl_common_texture.glsl"
};

static Enesim_Renderer_OpenGL_Shader *_image_shaders[] = {
	&_image_shader,
	NULL,
};

/* the only program */
static Enesim_Renderer_OpenGL_Program _image_program = {
	/* .name 		= */ "image",
	/* .shaders 		= */ _image_shaders,
	/* .num_shaders		= */ 1,
};

static Enesim_Renderer_OpenGL_Program *_image_programs[] = {
	&_image_program,
	NULL,
};

static void _image_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, const Eina_Rectangle *area, int x EINA_UNUSED,
		int y EINA_UNUSED)
{
	Enesim_Renderer_Image * thiz;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_OpenGL_Compiled_Program *cp;
	Enesim_Surface *src;
	Enesim_Matrix sc, tx, m;
	GLfloat fm[16];
	int w, h;

	thiz = ENESIM_RENDERER_IMAGE(r);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);

	/* choose the source surface, either the uploaded or the original */
	if (thiz->gl.s)
		src = thiz->gl.s;
	else
		src = thiz->current.s;

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	/* the initial size/position */
	enesim_surface_size_get(src, &w, &h);
	enesim_matrix_translate(&tx, thiz->current.x, thiz->current.y);
	enesim_matrix_scale(&sc, thiz->current.w / (float)w, thiz->current.h / (float)h);
	/* our renderer transformation */
	enesim_renderer_transformation_get(r, &m);
	enesim_matrix_compose(&m, &tx, &m);
	enesim_matrix_compose(&m, &sc, &m);
	enesim_matrix_inverse(&m, &m);

	enesim_opengl_matrix_convert(&m, fm);
	glMultMatrixf(fm);

	cp = &rdata->program->compiled[0];
	enesim_renderer_opengl_shader_texture_setup(cp->id,
				0, src, thiz->color, 0, 0);
	enesim_opengl_target_surface_set(s);
	enesim_opengl_rop_set(rop);
	enesim_opengl_draw_area(area);

	/* don't use any program */
	glUseProgramObjectARB(0);
	glBindTexture(GL_TEXTURE_2D, 0);
	enesim_opengl_target_surface_set(NULL);
}
#endif

/* blend simple */
static void _argb8888_blend_span(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = ENESIM_RENDERER_IMAGE(r);
	uint32_t *src = thiz->src;
	uint32_t *dst = ddata;

	x -= eina_f16p16_int_to(thiz->ixx);
	y -= eina_f16p16_int_to(thiz->iyy);

	if ((y < 0) || (y >= thiz->sh) || (x >= thiz->sw) || (x + len < 0))
		return;
	if (x < 0)
	{
		len += x;  dst -= x;  x = 0;
	}
	if (len > thiz->sw - x)
		len = thiz->sw - x;
	src = argb8888_at(src, thiz->sstride, x, y);
	thiz->span(dst, len, src, thiz->color, NULL);
}


/* fast - no pre-scaling */
static void _argb8888_image_no_scale_affine_fast(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = ENESIM_RENDERER_IMAGE(r);
	uint32_t *dst = ddata, *end = dst + len;
	uint32_t *src = thiz->src;
	int sw = thiz->sw, sh = thiz->sh;
	Eina_F16p16 xx, yy;
	Enesim_Color color = thiz->color;

	if (!color)
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	xx = (thiz->matrix.xx * x) + (thiz->matrix.xx >> 1) +
		(thiz->matrix.xy * y) + (thiz->matrix.xy >> 1) +
		thiz->matrix.xz - 32768 - thiz->ixx;
	yy = (thiz->matrix.yx * x) + (thiz->matrix.yx >> 1) +
		(thiz->matrix.yy * y) + (thiz->matrix.yy >> 1) +
		thiz->matrix.yz - 32768 - thiz->iyy;

	while (dst < end)
	{
		uint32_t p0 = 0;

		x = eina_f16p16_int_to(xx);
		y = eina_f16p16_int_to(yy);

		if (((unsigned)x < (unsigned)sw) & ((unsigned)y < (unsigned)sh))
		{
			p0 = *(src + (y * sw) + x);
			if (color && p0)
				p0 = argb8888_mul4_sym(p0, color);
		}
		*dst++ = p0;  xx += thiz->matrix.xx;  yy += thiz->matrix.yx;
	}
}

/* fast - pre-scaling */
static void _argb8888_image_scale_identity_fast(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = ENESIM_RENDERER_IMAGE(r);
	uint32_t *dst = ddata, *end = dst + len;
	uint32_t *src = thiz->src;
	int sw = thiz->sw;
	Eina_F16p16 iww = thiz->iww, ihh = thiz->ihh;
	Eina_F16p16 mxx = thiz->mxx, myy = thiz->myy;
	Eina_F16p16 xx, yy, ixx, iyy;
	int iy;
	Enesim_Color color = thiz->color;

	if (!color)
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	xx = eina_f16p16_int_from(x) - thiz->ixx;
	yy = eina_f16p16_int_from(y) - thiz->iyy;

	if ((yy < 0) || (yy >= ihh))
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}

	iyy = (myy * (long long int)yy) >> 16;
	iy = eina_f16p16_int_to(iyy);
	src += (iy * sw);
	ixx = (mxx * (long long int)xx) >> 16;

	while (dst < end)
	{
		uint32_t p0 = 0;

		if ((xx >= 0) & (xx < iww))
		{
			int ix = eina_f16p16_int_to(ixx);

			p0 = *(src + ix);
			if (p0 && color)
				p0 = argb8888_mul4_sym(p0, color);
        	}
		*dst++ = p0;  xx += EINA_F16P16_ONE;  ixx += mxx;
	}
}

static void _argb8888_image_scale_affine_fast(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = ENESIM_RENDERER_IMAGE(r);
	uint32_t *dst = ddata, *end = dst + len;
	uint32_t *src = thiz->src;
	int sw = thiz->sw;
	Eina_F16p16 iww = thiz->iww, ihh = thiz->ihh;
	Eina_F16p16 mxx = thiz->mxx, myy = thiz->myy;
	Eina_F16p16 xx, yy;
	Enesim_Color color = thiz->color;

	if (!color)
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	xx = (thiz->matrix.xx * x) + (thiz->matrix.xx >> 1) +
		(thiz->matrix.xy * y) + (thiz->matrix.xy >> 1) +
		thiz->matrix.xz - 32768 - thiz->ixx;
	yy = (thiz->matrix.yx * x) + (thiz->matrix.yx >> 1) +
		(thiz->matrix.yy * y) + (thiz->matrix.yy >> 1) +
		thiz->matrix.yz - 32768 - thiz->iyy;

	while (dst < end)
	{
		uint32_t p0 = 0;

		if ( ((unsigned)xx < (unsigned)iww) &
			((unsigned)yy < (unsigned)ihh) )
		{
			Eina_F16p16 ixx, iyy;
			int ix, iy;

			ixx = (mxx * (long long int)xx) >> 16;
			ix = eina_f16p16_int_to(ixx);
			iyy = (myy * (long long int)yy) >> 16;
			iy = eina_f16p16_int_to(iyy);

			p0 = *(src + (iy * sw) + ix);

			if (p0 && color)
				p0 = argb8888_mul4_sym(p0, color);
        	}
		*dst++ = p0;  xx += thiz->matrix.xx;  yy += thiz->matrix.yx;
	}
}

/* good - no pre-scaling */
static void _argb8888_image_no_scale_identity(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = ENESIM_RENDERER_IMAGE(r);
	uint32_t *src = thiz->src;
	uint32_t *dst = ddata;


	x -= eina_f16p16_int_to(thiz->ixx);
	y -= eina_f16p16_int_to(thiz->iyy);

	if ((y < 0) || (y >= thiz->sh) || (x >= thiz->sw) || (x + len < 0) || !thiz->color)
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	if (x < 0)
	{
		x = -x;
		memset(dst, 0, sizeof(unsigned int) * x);
		len -= x;  dst += x;  x = 0;
	}
	if (len > thiz->sw - x)
	{
		memset(dst + (thiz->sw - x), 0, sizeof(unsigned int) * (len - (thiz->sw - x)));
		len = thiz->sw - x;
	}
	src = argb8888_at(src, thiz->sstride, x, y);
	thiz->span(dst, len, src, thiz->color, NULL);
}

static void _argb8888_image_no_scale_affine(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = ENESIM_RENDERER_IMAGE(r);
	uint32_t *dst = ddata, *end = dst + len;
	uint32_t *src = thiz->src;
	int sw = thiz->sw, sh = thiz->sh;
	Eina_F16p16 xx, yy;
	Enesim_Color color = thiz->color;

	if (!color)
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	xx = (thiz->matrix.xx * x) + (thiz->matrix.xx >> 1) +
		(thiz->matrix.xy * y) + (thiz->matrix.xy >> 1) +
		thiz->matrix.xz - 32768 - thiz->ixx;
	yy = (thiz->matrix.yx * x) + (thiz->matrix.yx >> 1) +
		(thiz->matrix.yy * y) + (thiz->matrix.yy >> 1) +
		thiz->matrix.yz - 32768 - thiz->iyy;

	while (dst < end)
	{
		uint32_t p0 = 0;

		x = eina_f16p16_int_to(xx);
		y = eina_f16p16_int_to(yy);

		if ( (((unsigned) (x + 1)) < ((unsigned) (sw + 1))) & (((unsigned) (y + 1)) < ((unsigned) (sh + 1))) )
		{
			uint32_t *p = src + (y * sw) + x;
			uint32_t p1 = 0, p2 = 0, p3 = 0;

			if ((x > -1) && (y > - 1))
				p0 = *p;
			if ((y > -1) && ((x + 1) < sw))
				p1 = *(p + 1);
			if ((y + 1) < sh)
			{
				if (x > -1)
					p2 = *(p + sw);
				if ((x + 1) < sw)
					p3 = *(p + sw + 1);
			}
			if (p0 | p1 | p2 | p3)
			{
				uint16_t ax = 1 + ((xx & 0xffff) >> 8);
				uint16_t ay = 1 + ((yy & 0xffff) >> 8);

				p0 = argb8888_interp_256(ax, p1, p0);
				p2 = argb8888_interp_256(ax, p3, p2);
				p0 = argb8888_interp_256(ay, p2, p0);
				if (color && p0)
					p0 = argb8888_mul4_sym(p0, color);
			}
		}
		*dst++ = p0;  xx += thiz->matrix.xx;  yy += thiz->matrix.yx;
	}
}

/* good - pre-scaling */
static void _argb8888_image_scale_identity(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = ENESIM_RENDERER_IMAGE(r);
	uint32_t *dst = ddata, *end = dst + len;
	uint32_t *src = thiz->src;
	int sw = thiz->sw, sh = thiz->sh;
	Eina_F16p16 iww = thiz->iww, ihh = thiz->ihh;
	Eina_F16p16 mxx = thiz->mxx, myy = thiz->myy;
	Eina_F16p16 xx, yy, ixx, iyy;
	int iy;
	uint16_t ay;
	Enesim_Color color = thiz->color;

	if (!color)
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	xx = eina_f16p16_int_from(x) - thiz->ixx;
	yy = eina_f16p16_int_from(y) - thiz->iyy;

	if ((yy <= -EINA_F16P16_ONE) || (yy >= ihh))
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}

	iyy = (myy * (long long int)yy) >> 16;
	iy = eina_f16p16_int_to(iyy);
	src += (iy * sw);
	ay = 1 + ((iyy & 0xffff) >> 8);
	if (yy < 0)
		ay = 1 + ((yy & 0xffff) >> 8);
	if ((ihh - yy) < EINA_F16P16_ONE)
		ay = 256 - ((ihh - yy) >> 8);
	ixx = (mxx * (long long int)xx) >> 16;

	while (dst < end)
	{
		uint32_t p0 = 0;

		if ((xx > -EINA_F16P16_ONE) & (xx < iww))
		{
			int ix = eina_f16p16_int_to(ixx);
			uint32_t *p = src + ix;
			uint32_t p1 = 0, p2 = 0, p3 = 0;

			if ((iy > -1) & (ix > -1))
				p0 = *p;
			if ((iy > -1) & ((ix + 1) < sw))
				p1 = *(p + 1);
			if ((iy + 1) < sh)
			{
				if (ix > -1)
					p2 = *(p + sw);
				if ((ix + 1) < sw)
					p3 = *(p + sw + 1);
			}
			if (p0 | p1 | p2 | p3)
			{
				uint16_t ax = 1 + ((ixx & 0xffff) >> 8);

				if (xx < 0)
					ax = 1 + ((xx & 0xffff) >> 8);
				if ((iww - xx) < EINA_F16P16_ONE)
					ax = 256 - ((iww - xx) >> 8);
				p0 = argb8888_interp_256(ax, p1, p0);
				p2 = argb8888_interp_256(ax, p3, p2);
				p0 = argb8888_interp_256(ay, p2, p0);
				if (color && p0)
					p0 = argb8888_mul4_sym(p0, color);
			}
        	}
		*dst++ = p0;  xx += EINA_F16P16_ONE;  ixx += mxx;
	}
}

static void _argb8888_image_scale_affine(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = ENESIM_RENDERER_IMAGE(r);
	uint32_t *dst = ddata, *end = dst + len;
	uint32_t *src = thiz->src;
	int sw = thiz->sw, sh = thiz->sh;
	Eina_F16p16 iww = thiz->iww, ihh = thiz->ihh;
	Eina_F16p16 mxx = thiz->mxx, myy = thiz->myy;
	Eina_F16p16 xx, yy;
	Enesim_Color color = thiz->color;

	if (!color)
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	xx = (thiz->matrix.xx * x) + (thiz->matrix.xx >> 1) +
		(thiz->matrix.xy * y) + (thiz->matrix.xy >> 1) +
		thiz->matrix.xz - 32768 - thiz->ixx;
	yy = (thiz->matrix.yx * x) + (thiz->matrix.yx >> 1) +
		(thiz->matrix.yy * y) + (thiz->matrix.yy >> 1) +
		thiz->matrix.yz - 32768 - thiz->iyy;

	while (dst < end)
	{
		uint32_t p0 = 0;

		if ( (((unsigned) (xx + EINA_F16P16_ONE)) < ((unsigned) (iww + EINA_F16P16_ONE))) &
			(((unsigned) (yy + EINA_F16P16_ONE)) < ((unsigned) (ihh + EINA_F16P16_ONE))) )
		{
			Eina_F16p16 ixx, iyy;
			int ix, iy;
			uint32_t *p, p3 = 0, p2 = 0, p1 = 0;

			ixx = (mxx * (long long int)xx) >> 16;
			ix = eina_f16p16_int_to(ixx);
			iyy = (myy * (long long int)yy) >> 16;
			iy = eina_f16p16_int_to(iyy);

			p = src + (iy * sw) + ix;

			if ((ix > -1) & (iy > -1))
				p0 = *p;
			if ((iy > -1) & ((ix + 1) < sw))
				p1 = *(p + 1);
			if ((iy + 1) < sh)
			{
				if (ix > -1)
					p2 = *(p + sw);
				if ((ix + 1) < sw)
					p3 = *(p + sw + 1);
			}
			if (p0 | p1 | p2 | p3)
			{
				uint16_t ax, ay;

				ax = 1 + ((ixx & 0xffff) >> 8);
				ay = 1 + ((iyy & 0xffff) >> 8);
				if (xx < 0)
					ax = 1 + ((xx & 0xffff) >> 8);
				if ((iww - xx) < EINA_F16P16_ONE)
					ax = 256 - ((iww - xx) >> 8);
				if (yy < 0)
					ay = 1 + ((yy & 0xffff) >> 8);
				if ((ihh - yy) < EINA_F16P16_ONE)
					ay = 256 - ((ihh - yy) >> 8);

				p0 = argb8888_interp_256(ax, p1, p0);
				p2 = argb8888_interp_256(ax, p3, p2);
				p0 = argb8888_interp_256(ay, p2, p0);
				if (color && p0)
					p0 = argb8888_mul4_sym(p0, color);
			}
        	}
		*dst++ = p0;  xx += thiz->matrix.xx;  yy += thiz->matrix.yx;
	}
}

/* best - always pre-scales */
static void _argb8888_image_scale_d_u_identity(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = ENESIM_RENDERER_IMAGE(r);
	uint32_t *dst = ddata, *end = dst + len;
	uint32_t *src = thiz->src, *q;
	int sw = thiz->sw, sh = thiz->sh;
	Eina_F16p16 iww = thiz->iww, ihh = thiz->ihh;
	Eina_F16p16 mxx = thiz->mxx, myy = thiz->myy;
	Eina_F16p16 nxx = thiz->nxx;
	Eina_F16p16 xx, yy, ixx, iyy;
	int iy;
	uint16_t ay;
	Enesim_Color color = thiz->color;

	if (!color)
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	xx = eina_f16p16_int_from(x) - thiz->ixx;
	yy = eina_f16p16_int_from(y) - thiz->iyy;

	if ((yy <= -EINA_F16P16_ONE) || (yy >= ihh))
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}

	ixx = (mxx * (long long int)xx) >> 16;
	iyy = (myy * (long long int)yy) >> 16;
	ay = 1 + ((iyy & 0xffff) >> 8);
	if (yy < 0)
		ay = 1 + ((yy & 0xffff) >> 8);
	if ((ihh - yy) < EINA_F16P16_ONE)
		ay = 256 - ((ihh - yy) >> 8);

	iy = iyy >> 16;
	q = src + (iy * sw);
	while (dst < end)
	{
		uint32_t p0 = 0;

		if ((xx > -EINA_F16P16_ONE) & (xx < iww))
		{
			uint32_t ag0 = 0, rb0 = 0;
			int ix, tx, ntx;
			Eina_F16p16 txx, ntxx;
			uint32_t *p;

			x = xx >> 16;  ix = ixx >> 16;
			txx = xx - (xx & 0xffff);  tx = txx >> 16;
			ntxx = txx + nxx;  ntx = ntxx >> 16;
			p = q + ix;
			while (ix < sw)
			{
				uint32_t p1 = 0, p2 = 0;

				if (ix > -1)
				{
					if (iy > -1)
						p1 = *p;
					if (iy + 1 < sh)
						p2 = *(p + sw);
				}
				if (p1 | p2)
				    p1 = argb8888_interp_256(ay, p2, p1);
				if (ntx != tx)
				{
					int ax;

					if (ntx != x)
					{
						ax = 65536 - (txx & 0xffff);
						ag0 += ((((p1 >> 16) & 0xff00) * ax) & 0xffff0000) +
							(((p1 & 0xff00) * ax) >> 16);
						rb0 += ((((p1 >> 8) & 0xff00) * ax) & 0xffff0000) +
							(((p1 & 0xff) * ax) >> 8);
						break;
					}
					ax = 256 + (ntxx & 0xffff);
					ag0 = ((((p1 >> 16) & 0xff00) * ax) & 0xffff0000) +
						(((p1 & 0xff00) * ax) >> 16);
					rb0 = ((((p1 >> 8) & 0xff00) * ax) & 0xffff0000) +
						(((p1 & 0xff) * ax) >> 8);
					tx = ntx;
				}
				else
				{
					ag0 += ((((p1 >> 16) & 0xff00) * nxx) & 0xffff0000) +
						(((p1 & 0xff00) * nxx) >> 16);
					rb0 += ((((p1 >> 8) & 0xff00) * nxx) & 0xffff0000) +
						(((p1 & 0xff) * nxx) >> 8);
				}
				p++;  ix++;
				txx = ntxx;  ntxx += nxx;  ntx = ntxx >> 16;
			}
			p0 = ((ag0 + 0xff00ff) & 0xff00ff00) + (((rb0 + 0xff00ff) >> 8) & 0xff00ff);
			if (color && p0)
				p0 = argb8888_mul4_sym(p0, color);
		}
		*dst++ = p0;  xx += EINA_F16P16_ONE;  ixx += mxx;
	}
}

static void _argb8888_image_scale_u_d_identity(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = ENESIM_RENDERER_IMAGE(r);
	uint32_t *dst = ddata, *end = dst + len;
	uint32_t *src = thiz->src, *q;
	int sw = thiz->sw, sh = thiz->sh;
	Eina_F16p16 iww = thiz->iww, ihh = thiz->ihh;
	Eina_F16p16 mxx = thiz->mxx, myy = thiz->myy;
	Eina_F16p16 nyy = thiz->nyy;
	Eina_F16p16 xx, yy, ixx;
	Eina_F16p16 iyy0, tyy0, ntyy0;
	int iy0, ty0, nty0;
	Enesim_Color color = thiz->color;

	if (!color)
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	xx = eina_f16p16_int_from(x) - thiz->ixx;
	yy = eina_f16p16_int_from(y) - thiz->iyy;

	if ((yy <= -EINA_F16P16_ONE) || (yy >= ihh))
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}

	ixx = (mxx * (long long int)xx) >> 16;
	iyy0 = (myy * (long long int)yy) >> 16;
	iy0 = iyy0 >> 16;
	tyy0 = yy - (yy & 0xffff);  ty0 = tyy0 >> 16;
	ntyy0 = tyy0 + nyy;  nty0 = ntyy0 >> 16;
	q = src + (iy0 * sw);
	y = yy >> 16;

	while (dst < end)
	{
		uint32_t p0 = 0;
		if ((xx > -EINA_F16P16_ONE) & (xx < iww))
		{
			uint32_t ag0 = 0, rb0 = 0;
			int ix, iy = iy0;
			Eina_F16p16 tyy = tyy0, ntyy = ntyy0;
			int ty = ty0, nty = nty0;
			uint32_t *p;
			uint16_t ax;

			ix = ixx >> 16;
			ax = 1 + ((ixx >> 8) & 0xff);
			if (xx < 0)
				ax = 1 + ((xx & 0xffff) >> 8);
			if ((iww - xx) < EINA_F16P16_ONE)
				ax = 256 - ((iww - xx) >> 8);
			p = q + ix;
			while (iy < sh)
			{
				uint32_t p3 = 0, p2 = 0;

				if (iy > -1)
				{
					if (ix > -1)
						p2 = *p;
					if (ix + 1 < sw)
						p3 = *(p + 1);
				}
				if (p2 | p3)
					p2 = argb8888_interp_256(ax, p3, p2);

				if (nty != ty)
				{
					int ay;

					if (nty != y)
					{
						ay = 65536 - (tyy & 0xffff);
						ag0 += ((((p2 >> 16) & 0xff00) * ay) & 0xffff0000) +
							(((p2 & 0xff00) * ay) >> 16);
						rb0 += ((((p2 >> 8) & 0xff00) * ay) & 0xffff0000) +
							(((p2 & 0xff) * ay) >> 8);
						break;
					}
					ay = 256 + (ntyy & 0xffff);
					ag0 += ((((p2 >> 16) & 0xff00) * ay) & 0xffff0000) +
						(((p2 & 0xff00) * ay) >> 16);
					rb0 += ((((p2 >> 8) & 0xff00) * ay) & 0xffff0000) +
						(((p2 & 0xff) * ay) >> 8);
					ty = nty;
				}
				else
				{
					ag0 += ((((p2 >> 16) & 0xff00) * nyy) & 0xffff0000) +
						(((p2 & 0xff00) * nyy) >> 16);
					rb0 += ((((p2 >> 8) & 0xff00) * nyy) & 0xffff0000) +
						(((p2 & 0xff) * nyy) >> 8);
				}

				p += sw;  iy++;
				tyy = ntyy;  ntyy += nyy;  nty = ntyy >> 16;
			}
			p0 = ((ag0 + 0xff00ff) & 0xff00ff00) + (((rb0 + 0xff00ff) >> 8) & 0xff00ff);
			if (color && p0)
				p0 = argb8888_mul4_sym(p0, color);
		}
	*dst++ = p0;  xx += EINA_F16P16_ONE;  ixx += mxx;
	}
}

static void _argb8888_image_scale_d_d_identity(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = ENESIM_RENDERER_IMAGE(r);
	uint32_t *dst = ddata, *end = dst + len;
	uint32_t *src = thiz->src, *q;
	int sw = thiz->sw, sh = thiz->sh;
	Eina_F16p16 iww = thiz->iww, ihh = thiz->ihh;
	Eina_F16p16 mxx = thiz->mxx, myy = thiz->myy;
	Eina_F16p16 nxx = thiz->nxx, nyy = thiz->nyy;
	Eina_F16p16 xx, yy, ixx;
	Eina_F16p16 iyy0, tyy0, ntyy0;
	int iy0, ty0, nty0;
	Enesim_Color color = thiz->color;

	if (!color)
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	xx = eina_f16p16_int_from(x) - thiz->ixx;
	yy = eina_f16p16_int_from(y) - thiz->iyy;

	if ((yy <= -EINA_F16P16_ONE) || (yy >= ihh))
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}

	y = yy >> 16;
	ixx = (mxx * (long long int)xx) >> 16;
	iyy0 = (myy * (long long int)yy) >> 16;  iy0 = iyy0 >> 16;
	tyy0 = yy - (yy & 0xffff);  ty0 = tyy0 >> 16;
	ntyy0 = tyy0 + nyy;  nty0 = ntyy0 >> 16;
	q = src + (iy0 * sw);

	while (dst < end)
	{
		uint32_t p0 = 0;

		if ((xx > -EINA_F16P16_ONE) & (xx < iww))
		{
			uint32_t ag0 = 0, rb0 = 0;
			int ix0 = (ixx >> 16), iy = iy0;
			Eina_F16p16 txx0 = xx - (xx & 0xffff);
			int tx0 = (txx0 >> 16);
			Eina_F16p16 ntxx0 = (txx0 + nxx);
			int ntx0 = (ntxx0 >> 16);
			Eina_F16p16 tyy = tyy0, ntyy = ntyy0;
			int ty = ty0, nty = nty0;
			uint32_t *ps = q;

			x = xx >> 16;
			while (iy < sh)
			{
				uint32_t ag2 = 0, rb2 = 0;
				Eina_F16p16 txx = txx0, ntxx = ntxx0;
				int tx = tx0, ntx = ntx0, ix = ix0;
				uint32_t *p = ps + ix;

				while (ix < sw)
				{
					uint32_t p1 = 0;

					if ((ix > -1) & (iy > -1))
						p1 = *p;

					if (ntx != tx)
					{
						int ax;

						if (ntx != x)
						{
							ax = 65536 - (txx & 0xffff);
							ag2 += ((((p1 >> 16) & 0xff00) * ax) & 0xffff0000) +
								(((p1 & 0xff00) * ax) >> 16);
							rb2 += ((((p1 >> 8) & 0xff00) * ax) & 0xffff0000) +
								(((p1 & 0xff) * ax) >> 8);
							break;
						}
						ax = 256 + (ntxx & 0xffff);
						ag2 = ((((p1 >> 16) & 0xff00) * ax) & 0xffff0000) +
							(((p1 & 0xff00) * ax) >> 16);
						rb2 = ((((p1 >> 8) & 0xff00) * ax) & 0xffff0000) +
							(((p1 & 0xff) * ax) >> 8);
						tx = ntx;
					}
					else
					{
						ag2 += ((((p1 >> 16) & 0xff00) * nxx) & 0xffff0000) +
							(((p1 & 0xff00) * nxx) >> 16);
						rb2 += ((((p1 >> 8) & 0xff00) * nxx) & 0xffff0000) +
							(((p1 & 0xff) * nxx) >> 8);
					}
					p++;  ix++;
					txx = ntxx;  ntxx += nxx;  ntx = ntxx >> 16;
				}

				if (nty != ty)
				{
					int ay;

					if (nty != y)
					{
						ay = 65536 - (tyy & 0xffff);
						ag0 += (((ag2 >> 16) * ay) & 0xffff0000) +
							(((ag2 & 0xffff) * ay) >> 16);
						rb0 += (((rb2 >> 16) * ay) & 0xffff0000) +
							(((rb2 & 0xffff) * ay) >> 16);
						break;
					}
					ay = 256 + (ntyy & 0xffff);
					ag0 = (((ag2 >> 16) * ay) & 0xffff0000) +
						(((ag2 & 0xffff) * ay) >> 16);
					rb0 = (((rb2 >> 16) * ay) & 0xffff0000) +
						(((rb2 & 0xffff) * ay) >> 16);
					ty = nty;
				}
				else
				{
					ag0 += (((ag2 >> 16) * nyy) & 0xffff0000) +
						(((ag2 & 0xffff) * nyy) >> 16);
					rb0 += (((rb2 >>16) * nyy) & 0xffff0000) +
						(((rb2 & 0xffff) * nyy) >> 16);
				}
				ps += sw;  iy++;
				tyy = ntyy;  ntyy += nyy;  nty = ntyy >> 16;
			}
			p0 = ((ag0 + 0xff00ff) & 0xff00ff00) + (((rb0 + 0xff00ff) >> 8) & 0xff00ff);
			if (color && p0)
				p0 = argb8888_mul4_sym(p0, color);
		}
		*dst++ = p0;  xx += EINA_F16P16_ONE;  ixx += mxx;
	}
}

static void _argb8888_image_scale_d_u_affine(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = ENESIM_RENDERER_IMAGE(r);
	uint32_t *dst = ddata, *end = dst + len;
	uint32_t *src = thiz->src;
	int sw = thiz->sw, sh = thiz->sh;
	Eina_F16p16 iww = thiz->iww, ihh = thiz->ihh;
	Eina_F16p16 mxx = thiz->mxx, myy = thiz->myy;
	Eina_F16p16 nxx = thiz->nxx;
	Eina_F16p16 xx, yy;
	Enesim_Color color = thiz->color;

	if (!color)
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	xx = (thiz->matrix.xx * x) + (thiz->matrix.xx >> 1) +
		(thiz->matrix.xy * y) + (thiz->matrix.xy >> 1) +
		thiz->matrix.xz - 32768 - thiz->ixx;
	yy = (thiz->matrix.yx * x) + (thiz->matrix.yx >> 1) +
		(thiz->matrix.yy * y) + (thiz->matrix.yy >> 1) +
		thiz->matrix.yz - 32768 - thiz->iyy;

	while (dst < end)
	{
		uint32_t p0 = 0;

		if ( (xx > -EINA_F16P16_ONE) & (yy > -EINA_F16P16_ONE) & (xx < iww) & (yy < ihh) )
		{
			uint32_t ag0 = 0, rb0 = 0;
			Eina_F16p16 ixx, iyy, txx, ntxx;
			int ix, iy, tx, ntx;
			uint16_t ay;
			uint32_t *p;

			x = xx >> 16;
			iyy = (myy * (long long int)yy) >> 16;  iy = iyy >> 16;
			ixx = (mxx * (long long int)xx) >> 16;  ix = ixx >> 16;
			txx = xx - (xx & 0xffff);  tx = txx >> 16;
			ntxx = txx + nxx;  ntx = ntxx >> 16;
			ay = 1 + ((iyy & 0xffff) >> 8);

			if (yy < 0)
				ay = 1 + ((yy & 0xffff) >> 8);
			if ((ihh - yy) < EINA_F16P16_ONE)
				ay = 256 - ((ihh - yy) >> 8);
			p = src + (iy * sw) + ix;

			while (ix < sw)
			{
				uint32_t p1 = 0, p2 = 0;

				if (ix > -1)
				{
					if (iy > -1)
						p1 = *p;
					if (iy + 1 < sh)
						p2 = *(p + sw);
				}
				if (p1 | p2)
				    p1 = argb8888_interp_256(ay, p2, p1);
				if (ntx != tx)
				{
					int ax;

					if (ntx != x)
					{
						ax = 65536 - (txx & 0xffff);
						ag0 += ((((p1 >> 16) & 0xff00) * ax) & 0xffff0000) +
							(((p1 & 0xff00) * ax) >> 16);
						rb0 += ((((p1 >> 8) & 0xff00) * ax) & 0xffff0000) +
							(((p1 & 0xff) * ax) >> 8);
						break;
					}
					ax = 256 + (ntxx & 0xffff);
					ag0 = ((((p1 >> 16) & 0xff00) * ax) & 0xffff0000) +
						(((p1 & 0xff00) * ax) >> 16);
					rb0 = ((((p1 >> 8) & 0xff00) * ax) & 0xffff0000) +
						(((p1 & 0xff) * ax) >> 8);
					tx = ntx;
				}
				else
				{
					ag0 += ((((p1 >> 16) & 0xff00) * nxx) & 0xffff0000) +
						(((p1 & 0xff00) * nxx) >> 16);
					rb0 += ((((p1 >> 8) & 0xff00) * nxx) & 0xffff0000) +
						(((p1 & 0xff) * nxx) >> 8);
				}
				p++;  ix++;
				txx = ntxx;  ntxx += nxx;  ntx = ntxx >> 16;
			}
			p0 = ((ag0 + 0xff00ff) & 0xff00ff00) + (((rb0 + 0xff00ff) >> 8) & 0xff00ff);
			if (color && p0)
				p0 = argb8888_mul4_sym(p0, color);
		}
		*dst++ = p0;  xx += thiz->matrix.xx;  yy += thiz->matrix.yx;
	}
}

static void _argb8888_image_scale_u_d_affine(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = ENESIM_RENDERER_IMAGE(r);
	uint32_t *dst = ddata, *end = dst + len;
	uint32_t *src = thiz->src;
	int sw = thiz->sw, sh = thiz->sh;
	Eina_F16p16 iww = thiz->iww, ihh = thiz->ihh;
	Eina_F16p16 mxx = thiz->mxx, myy = thiz->myy;
	Eina_F16p16 nyy = thiz->nyy;
	Eina_F16p16 xx, yy;
	Enesim_Color color = thiz->color;

	if (!color)
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	xx = (thiz->matrix.xx * x) + (thiz->matrix.xx >> 1) +
		(thiz->matrix.xy * y) + (thiz->matrix.xy >> 1) +
		thiz->matrix.xz - 32768 - thiz->ixx;
	yy = (thiz->matrix.yx * x) + (thiz->matrix.yx >> 1) +
		(thiz->matrix.yy * y) + (thiz->matrix.yy >> 1) +
		thiz->matrix.yz - 32768 - thiz->iyy;

	while (dst < end)
	{
		uint32_t p0 = 0;

		if ( (xx > -EINA_F16P16_ONE) & (yy > -EINA_F16P16_ONE) & (xx < iww) & (yy < ihh) )
		{
			uint32_t ag0 = 0, rb0 = 0;
			int ix, iy, ty, nty;
			Eina_F16p16 ixx, iyy, tyy, ntyy;
			uint32_t *p;
			uint16_t ax;

			y = yy >> 16;
			ixx = (mxx * (long long int)xx) >> 16;  ix = ixx >> 16;
			iyy = (myy * (long long int)yy) >> 16;  iy = iyy >> 16;
			tyy = yy - (yy & 0xffff);  ty = tyy >> 16;
			ntyy = tyy + nyy;  nty = ntyy >> 16;
			ax = 1 + ((ixx & 0xff) >> 8);
			if (xx < 0)
				ax = 1 + ((xx & 0xffff) >> 8);
			if ((iww - xx) < EINA_F16P16_ONE)
				ax = 256 - ((iww - xx) >> 8);
			p = src + (iy * sw) + ix;

			while (iy < sh)
			{
				uint32_t p3 = 0, p2 = 0;

				if (iy > -1)
				{
					if (ix > -1)
						p2 = *p;
					if (ix + 1 < sw)
						p3 = *(p + 1);
				}
				if (p2 | p3)
					p2 = argb8888_interp_256(ax, p3, p2);

				if (nty != ty)
				{
					int ay;

					if (nty != y)
					{
						ay = 65536 - (tyy & 0xffff);
						ag0 += ((((p2 >> 16) & 0xff00) * ay) & 0xffff0000) +
							(((p2 & 0xff00) * ay) >> 16);
						rb0 += ((((p2 >> 8) & 0xff00) * ay) & 0xffff0000) +
							(((p2 & 0xff) * ay) >> 8);
						break;
					}
					ay = 256 + (ntyy & 0xffff);
					ag0 += ((((p2 >> 16) & 0xff00) * ay) & 0xffff0000) +
						(((p2 & 0xff00) * ay) >> 16);
					rb0 += ((((p2 >> 8) & 0xff00) * ay) & 0xffff0000) +
						(((p2 & 0xff) * ay) >> 8);
					ty = nty;
				}
				else
				{
					ag0 += ((((p2 >> 16) & 0xff00) * nyy) & 0xffff0000) +
						(((p2 & 0xff00) * nyy) >> 16);
					rb0 += ((((p2 >> 8) & 0xff00) * nyy) & 0xffff0000) +
						(((p2 & 0xff) * nyy) >> 8);
				}

				p += sw;  iy++;
				tyy = ntyy;  ntyy += nyy;  nty = ntyy >> 16;
			}
			p0 = ((ag0 + 0xff00ff) & 0xff00ff00) + (((rb0 + 0xff00ff) >> 8) & 0xff00ff);
			if (color && p0)
				p0 = argb8888_mul4_sym(p0, color);
		}
	*dst++ = p0;  xx += thiz->matrix.xx;  yy += thiz->matrix.yx;
	}
}

static void _argb8888_image_scale_d_d_affine(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = ENESIM_RENDERER_IMAGE(r);
	uint32_t *dst = ddata, *end = dst + len;
	uint32_t *src = thiz->src;
	int sw = thiz->sw, sh = thiz->sh;
	Eina_F16p16 iww = thiz->iww, ihh = thiz->ihh;
	Eina_F16p16 mxx = thiz->mxx, myy = thiz->myy;
	Eina_F16p16 nxx = thiz->nxx, nyy = thiz->nyy;
	Eina_F16p16 xx, yy;
	Enesim_Color color = thiz->color;

	if (!color)
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	xx = (thiz->matrix.xx * x) + (thiz->matrix.xx >> 1) +
		(thiz->matrix.xy * y) + (thiz->matrix.xy >> 1) +
		thiz->matrix.xz - 32768 - thiz->ixx;
	yy = (thiz->matrix.yx * x) + (thiz->matrix.yx >> 1) +
		(thiz->matrix.yy * y) + (thiz->matrix.yy >> 1) +
		thiz->matrix.yz - 32768 - thiz->iyy;

	while (dst < end)
	{
		uint32_t p0 = 0;

		if ( (xx > -EINA_F16P16_ONE) & (yy > -EINA_F16P16_ONE) & (xx < iww) & (yy < ihh) )
		{
			uint32_t ag0 = 0, rb0 = 0;
			int ix0, iy, tx0, ntx0, ty, nty;
			Eina_F16p16 ixx, iyy, txx0, ntxx0, tyy, ntyy;
			uint32_t *ps;

			ixx = ((mxx * (long long int)xx) >> 16);  ix0 = (ixx >> 16);
			iyy = ((myy * (long long int)yy) >> 16);  iy = (iyy >> 16);

			ps = src + (iy * sw);

			tyy = yy - (yy & 0xffff);  ty = (tyy >> 16);
			ntyy = tyy + nyy;  nty = (ntyy >> 16);

			txx0 = xx - (xx & 0xffff);  tx0 = (txx0 >> 16);
			ntxx0 = (txx0 + nxx);  ntx0 = (ntxx0 >> 16);

			x = (xx >> 16);
			y = (yy >> 16);

			while (iy < sh)
			{
				uint32_t ag2 = 0, rb2 = 0;
				Eina_F16p16 txx = txx0, ntxx = ntxx0;
				int tx = tx0, ntx = ntx0, ix = ix0;
				uint32_t *p = ps + ix;

				while (ix < sw)
				{
					uint32_t p1 = 0;

					if ((ix > -1) & (iy > -1))
						p1 = *p;

					if (ntx != tx)
					{
						int ax;

						if (ntx != x)
						{
							ax = 65536 - (txx & 0xffff);
							ag2 += ((((p1 >> 16) & 0xff00) * ax) & 0xffff0000) +
								(((p1 & 0xff00) * ax) >> 16);
							rb2 += ((((p1 >> 8) & 0xff00) * ax) & 0xffff0000) +
								(((p1 & 0xff) * ax) >> 8);
							break;
						}
						ax = 256 + (ntxx & 0xffff);
						ag2 = ((((p1 >> 16) & 0xff00) * ax) & 0xffff0000) +
							(((p1 & 0xff00) * ax) >> 16);
						rb2 = ((((p1 >> 8) & 0xff00) * ax) & 0xffff0000) +
							(((p1 & 0xff) * ax) >> 8);
						tx = ntx;
					}
					else
					{
						ag2 += ((((p1 >> 16) & 0xff00) * nxx) & 0xffff0000) +
							(((p1 & 0xff00) * nxx) >> 16);
						rb2 += ((((p1 >> 8) & 0xff00) * nxx) & 0xffff0000) +
							(((p1 & 0xff) * nxx) >> 8);
					}
					p++;  ix++;
					txx = ntxx;  ntxx += nxx;  ntx = ntxx >> 16;
				}

				if (nty != ty)
				{
					int ay;

					if (nty != y)
					{
						ay = 65536 - (tyy & 0xffff);
						ag0 += (((ag2 >> 16) * ay) & 0xffff0000) +
							(((ag2 & 0xffff) * ay) >> 16);
						rb0 += (((rb2 >> 16) * ay) & 0xffff0000) +
							(((rb2 & 0xffff) * ay) >> 16);
						break;
					}
					ay = 256 + (ntyy & 0xffff);
					ag0 = (((ag2 >> 16) * ay) & 0xffff0000) +
						(((ag2 & 0xffff) * ay) >> 16);
					rb0 = (((rb2 >> 16) * ay) & 0xffff0000) +
						(((rb2 & 0xffff) * ay) >> 16);
					ty = nty;
				}
				else
				{
					ag0 += (((ag2 >> 16) * nyy) & 0xffff0000) +
						(((ag2 & 0xffff) * nyy) >> 16);
					rb0 += (((rb2 >>16) * nyy) & 0xffff0000) +
						(((rb2 & 0xffff) * nyy) >> 16);
				}
				ps += sw;  iy++;
				tyy = ntyy;  ntyy += nyy;  nty = ntyy >> 16;
			}
			p0 = ((ag0 + 0xff00ff) & 0xff00ff00) + (((rb0 + 0xff00ff) >> 8) & 0xff00ff);
			if (color && p0)
				p0 = argb8888_mul4_sym(p0, color);
		}
		*dst++ = p0;  xx += thiz->matrix.xx;  yy += thiz->matrix.yx;
	}
}

/* [downscaling|upscaling x][downscaling|upscaling y][matrix types] */
static Enesim_Renderer_Sw_Fill  _spans_best[2][2][ENESIM_MATRIX_TYPE_LAST];
/* [scaling|noscaling][matrix types] */
static Enesim_Renderer_Sw_Fill  _spans_good[2][ENESIM_MATRIX_TYPE_LAST];
/* [scaling|noscaling][matrix types] */
static Enesim_Renderer_Sw_Fill  _spans_fast[2][ENESIM_MATRIX_TYPE_LAST];
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _image_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "image";
}

static void _image_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Image *thiz;
	Enesim_Rectangle obounds;
	Enesim_Matrix m;
	Enesim_Matrix_Type type;

	thiz = ENESIM_RENDERER_IMAGE(r);
	if (!thiz->current.s)
	{
		obounds.x = 0;
		obounds.y = 0;
		obounds.w = 0;
		obounds.h = 0;
	}
	else
	{
		double ox;
		double oy;

		enesim_renderer_origin_get(r, &ox, &oy);
		obounds.x = thiz->current.x;
		obounds.y = thiz->current.y;
		obounds.w = thiz->current.w;
		obounds.h = thiz->current.h;
		/* the translate */
		obounds.x += ox;
		obounds.y += oy;
	}
	enesim_renderer_transformation_get(r, &m);
	type = enesim_renderer_transformation_type_get(r);
	_image_transform_bounds(r, &m, type, &obounds, rect);
}

static void _image_sw_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(r);
	if (thiz->current.s)
		enesim_surface_unmap(thiz->current.s, (void **)(&thiz->src), EINA_FALSE);
	thiz->span = NULL;
	_image_state_cleanup(r);
}

static Eina_Bool _image_sw_state_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop, 
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Image *thiz;
	Enesim_Format fmt;
	Enesim_Matrix m;
	Enesim_Matrix_Type mtype;
	Enesim_Quality quality;
	double x, y, w, h;
	double ox, oy;

	if (!_image_state_setup(r, l))
		return EINA_FALSE;

	thiz = ENESIM_RENDERER_IMAGE(r);
	enesim_surface_size_get(thiz->current.s, &thiz->sw, &thiz->sh);
	enesim_surface_map(thiz->current.s, (void **)(&thiz->src), &thiz->sstride);
	x = thiz->current.x;  y = thiz->current.y;
	w = thiz->current.w;  h = thiz->current.h;

	enesim_renderer_transformation_get(r, &m);
	/* use the inverse matrix */
	enesim_matrix_inverse(&m, &m);
	enesim_matrix_matrix_f16p16_to(&m, &thiz->matrix);
	mtype = enesim_matrix_f16p16_type_get(&thiz->matrix);
	if (mtype != ENESIM_MATRIX_TYPE_IDENTITY)
	{
		double sx, sy;

		sx = hypot(m.xx, m.yx);
		sy = hypot(m.xy, m.yy);
		if(fabs(1 - sx) < 1/16.0)  sx = 1;
		if(fabs(1 - sy) < 1/16.0)  sy = 1;

		if (sx && sy)
		{
			sx = 1/sx;  sy = 1/sy;
			w *= sx;  h *= sy;  x *= sx;  y *= sy;
			thiz->matrix.xx *= sx; thiz->matrix.xy *= sx; thiz->matrix.xz *= sx;
			thiz->matrix.yx *= sy; thiz->matrix.yy *= sy; thiz->matrix.yz *= sy;
			mtype = enesim_matrix_f16p16_type_get(&thiz->matrix);
		}
	}

	if ((w < 1) || (h < 1) || (thiz->sw < 1) || (thiz->sh < 1))
	{
		WRN("Size too small");
		return EINA_FALSE;
	}

	enesim_renderer_origin_get(r, &ox, &oy);
	thiz->iww = (w * 65536);
	thiz->ihh = (h * 65536);
	thiz->ixx = 65536 * (x + ox);
	thiz->iyy = 65536 * (y + oy);
	thiz->mxx = 65536;  thiz->myy = 65536;
	thiz->nxx = 65536;  thiz->nyy = 65536;

	/* FIXME we need to use the format from the destination surface */
	fmt = ENESIM_FORMAT_ARGB8888;
	quality = enesim_renderer_quality_get(r);

	if ((fabs(thiz->sw - w) > 1/256.0) || (fabs(thiz->sh - h) > 1/256.0))
	{
		double sx, isx;
		double sy, isy;
		int dx, dy, sw = thiz->sw, sh = thiz->sh;

		dx = (2*w + (1/256.0) < sw ? 1 : 0);
		dy = (2*h + (1/256.0) < sh ? 1 : 0);

		if (dx)
		{ sx = sw;  isx = w; }
		else
		{ sx = (sw > 1 ? sw - 1 : 1);  isx = (w > 1 ? w - 1 : 1); }
		if (dy)
		{ sy = sh;  isy = h; }
		else
		{ sy = (sh > 1 ? sh - 1 : 1);  isy = (h > 1 ? h - 1 : 1); }

		thiz->mxx = (sx * 65536) / isx;
		thiz->myy = (sy * 65536) / isy;
		thiz->nxx = (isx * 65536) / sx;
		thiz->nyy = (isy * 65536) / sy;

		thiz->simple = EINA_FALSE;
		if (mtype == ENESIM_MATRIX_TYPE_AFFINE)  // in case it's just a translation
		{
			thiz->ixx -= thiz->matrix.xz;  thiz->matrix.xz = 0;
			thiz->iyy -= thiz->matrix.yz;  thiz->matrix.yz = 0;
			mtype = enesim_matrix_f16p16_type_get(&thiz->matrix);
		}

		if (quality == ENESIM_QUALITY_BEST)
			*fill = _spans_best[dx][dy][mtype];
		else if (quality == ENESIM_QUALITY_GOOD)
			*fill = _spans_good[1][mtype];
		else
			*fill = _spans_fast[1][mtype];
	}
	else
	{
		thiz->simple = EINA_TRUE;
		if ((thiz->ixx & 0xffff) || (thiz->iyy & 0xffff))  // non-int translation
		{
			thiz->matrix.xz -= thiz->ixx;  thiz->ixx = 0;
			thiz->matrix.yz -= thiz->iyy;  thiz->iyy = 0;
			mtype = enesim_matrix_f16p16_type_get(&thiz->matrix);
			if (mtype != ENESIM_MATRIX_TYPE_IDENTITY)
				thiz->simple = EINA_FALSE;
		}

		if (quality == ENESIM_QUALITY_FAST)
			*fill = _spans_fast[0][mtype];
		else
			*fill = _spans_good[0][mtype];
		if (mtype == ENESIM_MATRIX_TYPE_IDENTITY)
		{
			thiz->span = enesim_compositor_span_get(rop, &fmt,
				ENESIM_FORMAT_ARGB8888, thiz->color, ENESIM_FORMAT_NONE);
			if (rop == ENESIM_ROP_BLEND)
				*fill = _argb8888_blend_span;
		}
	}

	return EINA_TRUE;
}

static void _image_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_ARGB8888 |
			ENESIM_RENDERER_FEATURE_QUALITY; 
}

static void _image_sw_image_hints(Enesim_Renderer *r, Enesim_Rop rop,
		Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_SW_HINT_COLORIZE;
	if (rop != ENESIM_ROP_FILL)
	{
		Enesim_Renderer_Image *thiz = ENESIM_RENDERER_IMAGE(r);

		if (thiz->span)
			*hints |= ENESIM_RENDERER_SW_HINT_ROP;
	}

}

static Eina_Bool _image_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(r);
	if (thiz->src_changed)
	{
		if (thiz->current.s != thiz->past.s)
			return EINA_TRUE;
	}
	if (!thiz->changed) return EINA_FALSE;

	if (thiz->current.x != thiz->past.x)
		return EINA_TRUE;
	if (thiz->current.y != thiz->past.y)
		return EINA_TRUE;
	if (thiz->current.w != thiz->past.w)
		return EINA_TRUE;
	if (thiz->current.h != thiz->past.h)
		return EINA_TRUE;
	return EINA_FALSE;
}

static Eina_Bool _image_damages(Enesim_Renderer *r,
		const Eina_Rectangle *old_bounds,
		Enesim_Renderer_Damage_Cb cb, void *data)
{
	Enesim_Renderer_Image *thiz;
	Eina_Rectangle *sd;
	Eina_Rectangle bounds;


	thiz = ENESIM_RENDERER_IMAGE(r);
	/* if we have changed just send the previous bounds
	 * and the current one
	 */
	if (enesim_renderer_has_changed(r))
	{
		Enesim_Rectangle curr_bounds;

		cb(r, old_bounds, EINA_TRUE, data);
		_image_bounds_get(r, &curr_bounds);
		enesim_rectangle_normalize(&curr_bounds, &bounds);
		cb(r, &bounds, EINA_FALSE, data);
		return EINA_TRUE;
	}
	/* in other case, send the surface damages tansformed
	 * to destination coordinates
	 */
	else
	{
		Enesim_Matrix m;
		Enesim_Matrix_Type type;
		Eina_List *l;
		Eina_Bool ret = EINA_FALSE;

		enesim_renderer_transformation_get(r, &m);
		type = enesim_renderer_transformation_type_get(r);
		EINA_LIST_FOREACH(thiz->surface_damages, l, sd)
		{
			Enesim_Rectangle sdd;

			enesim_rectangle_coords_from(&sdd, sd->x, sd->y, sd->w, sd->h);
			/* the coordinates are relative to the image */
			sdd.x += thiz->current.x;
			sdd.y += thiz->current.y;
			/* TODO clip it to the source bounds */
			_image_transform_destination_bounds(r, &m, type, &sdd, &bounds);
			cb(r, &bounds, EINA_FALSE, data);
			ret = EINA_TRUE;
		}
		return ret;
	}
}

#if BUILD_OPENGL
static Eina_Bool _image_opengl_initialize(Enesim_Renderer *r EINA_UNUSED,
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***programs)
{
	*num_programs = 1;
	*programs = _image_programs;
	return EINA_TRUE;
}

static Eina_Bool _image_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l)
{
	Enesim_Renderer_Image *thiz;

	if (!_image_state_setup(r, l)) return EINA_FALSE;

	thiz = ENESIM_RENDERER_IMAGE(r);
	if (!_image_gl_create(thiz, s))
	{
		ENESIM_RENDERER_LOG(r, l, "Failed to create the gl surface");
		return EINA_FALSE;
	}

	*draw = _image_opengl_draw;
	return EINA_TRUE;
}

static void _image_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	_image_state_cleanup(r);
}
#endif
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Image, Enesim_Renderer_Image_Class,
		enesim_renderer_image);

static void _enesim_renderer_image_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _image_name;
	klass->bounds_get = _image_bounds_get;
	klass->features_get = _image_features_get;
	klass->damages_get = _image_damages;
	klass->has_changed = _image_has_changed;
	klass->sw_hints_get = _image_sw_image_hints;
	klass->sw_setup = _image_sw_state_setup;
	klass->sw_cleanup = _image_sw_state_cleanup;
#if BUILD_OPENGL
	klass->opengl_initialize = _image_opengl_initialize;
	klass->opengl_setup = _image_opengl_setup;
	klass->opengl_cleanup = _image_opengl_cleanup;
#endif
	_spans_best[0][0][ENESIM_MATRIX_TYPE_IDENTITY] = _argb8888_image_scale_identity;
	_spans_best[0][0][ENESIM_MATRIX_TYPE_AFFINE] = _argb8888_image_scale_affine;
	_spans_best[1][0][ENESIM_MATRIX_TYPE_IDENTITY] = _argb8888_image_scale_d_u_identity;
	_spans_best[1][0][ENESIM_MATRIX_TYPE_AFFINE] = _argb8888_image_scale_d_u_affine;
	_spans_best[0][1][ENESIM_MATRIX_TYPE_IDENTITY] = _argb8888_image_scale_u_d_identity;
	_spans_best[0][1][ENESIM_MATRIX_TYPE_AFFINE] = _argb8888_image_scale_u_d_affine;
	_spans_best[1][1][ENESIM_MATRIX_TYPE_IDENTITY] = _argb8888_image_scale_d_d_identity;
	_spans_best[1][1][ENESIM_MATRIX_TYPE_AFFINE] = _argb8888_image_scale_d_d_affine;

	_spans_good[0][ENESIM_MATRIX_TYPE_IDENTITY] = _argb8888_image_no_scale_identity;
	_spans_good[0][ENESIM_MATRIX_TYPE_AFFINE] = _argb8888_image_no_scale_affine;
	_spans_good[1][ENESIM_MATRIX_TYPE_IDENTITY] = _argb8888_image_scale_identity;
	_spans_good[1][ENESIM_MATRIX_TYPE_AFFINE] = _argb8888_image_scale_affine;

	_spans_fast[0][ENESIM_MATRIX_TYPE_IDENTITY] = _argb8888_image_no_scale_identity;
	_spans_fast[0][ENESIM_MATRIX_TYPE_AFFINE] = _argb8888_image_no_scale_affine_fast;
	_spans_fast[1][ENESIM_MATRIX_TYPE_IDENTITY] = _argb8888_image_scale_identity_fast;
	_spans_fast[1][ENESIM_MATRIX_TYPE_AFFINE] = _argb8888_image_scale_affine_fast;
}

static void _enesim_renderer_image_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_renderer_image_instance_deinit(void *o)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(o);
	if (thiz->current.s)
		enesim_surface_unref(thiz->current.s);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/** @endlocal */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Create a new image renderer.
 *
 * @return A new image renderer.
 *
 * This function returns a newly allocated image renderer.
 */
EAPI Enesim_Renderer * enesim_renderer_image_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_image);
	return r;
}

/**
 * @brief Set the top left X coordinate of a image renderer.
 * @ender_prop{x}
 * @param[in] r The image renderer.
 * @param[in] x The top left X coordinate.
 *
 * This function sets the top left X coordinate of the image
 * renderer @p r to the value @p x.
 */
EAPI void enesim_renderer_image_x_set(Enesim_Renderer *r, double x)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(r);
	if (!thiz) return;
	thiz->current.x = x;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the top left X coordinate of a image renderer.
 * @ender_prop{x}
 * @param[in] r The image renderer.
 * @return The top left X coordinate.
 *
 * This function gets the top left X coordinate of the image
 * renderer @p r
 */
EAPI double enesim_renderer_image_x_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(r);
	return thiz->current.x;
}

/**
 * @brief Set the top left Y coordinate of a image renderer.
 * @ender_prop{y}
 * @param[in] r The image renderer.
 * @param[in] y The top left Y coordinate.
 *
 * This function sets the top left Y coordinate of the image
 * renderer @p r to the value @p y.
 */
EAPI void enesim_renderer_image_y_set(Enesim_Renderer *r, double y)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(r);
	if (!thiz) return;
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the top left Y coordinate of a image renderer.
 * @ender_prop{y}
 * @param[in] r The image renderer.
 * @return The top left Y coordinate.
 *
 * This function gets the top left Y coordinate of the image
 * renderer @p r
 */
EAPI double enesim_renderer_image_y_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(r);
	return thiz->current.y;
}

/**
 * @brief Set the top left coordinates of a image renderer.
 *
 * @param[in] r The image renderer.
 * @param[in] x The top left X coordinate.
 * @param[in] y The top left Y coordinate.
 *
 * This function sets the top left coordinates of the image
 * renderer @p r to the values @p x and @p y.
 */
EAPI void enesim_renderer_image_position_set(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(r);
	if (!thiz) return;
	thiz->current.x = x;
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the top left coordinates of a image renderer.
 *
 * @param[in] r The image renderer.
 * @param[out] x The top left X coordinate.
 * @param[out] y The top left Y coordinate.
 *
 * This function stores the top left coordinates value of the
 * image renderer @p r in the pointers @p x and @p y. These pointers
 * can be @c NULL.
 */
EAPI void enesim_renderer_image_position_get(Enesim_Renderer *r, double *x, double *y)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(r);
	if (!thiz) return;
	if (x) *x = thiz->current.x;
	if (y) *y = thiz->current.y;
}

/**
 * @brief Set the width of a image renderer.
 * @ender_prop{width}
 * @param[in] r The image renderer.
 * @param[in] w The image width.
 *
 * This function sets the width of the image renderer @p r to the
 * value @p width.
 */
EAPI void enesim_renderer_image_width_set(Enesim_Renderer *r, double w)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(r);
	if (!thiz) return;
	thiz->current.w = w;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the width of a image renderer.
 * @ender_prop{width}
 * @param[in] r The image renderer.
 * @return The image width.
 *
 * This function gets the width of the image renderer @p r
 */
EAPI double enesim_renderer_image_width_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(r);
	return thiz->current.w;
}

/**
 * @brief Set the height of a image renderer.
 * @ender_prop{height}
 * @param[in] r The image renderer.
 * @param[in] h The image height.
 *
 * This function sets the height of the image renderer @p r to the
 * value @p height.
 */
EAPI void enesim_renderer_image_height_set(Enesim_Renderer *r, double h)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(r);
	if (!thiz) return;
	thiz->current.h = h;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the height of a image renderer.
 * @ender_prop{height}
 * @param[in] r The image renderer.
 * @return The image height.
 *
 * This function gets the height of the image renderer @p r
 */
EAPI double enesim_renderer_image_height_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(r);
	return thiz->current.h;
}

/**
 * @brief Set the size of a image renderer.
 *
 * @param[in] r The image renderer.
 * @param[in] w The width.
 * @param[in] h The height.
 *
 * This function sets the size of the image renderer @p r to the
 * values @p width and @p height.
 */
EAPI void enesim_renderer_image_size_set(Enesim_Renderer *r, double w, double h)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(r);
	if (!thiz) return;
	thiz->current.w = w;
	thiz->current.h = h;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the size of a image renderer.
 *
 * @param[in] r The image renderer.
 * @param[out] w The width.
 * @param[out] h The height.
 *
 * This function stores the size of the image renderer @p r in the
 * pointers @p width and @p height. These pointers can be @c NULL.
 */
EAPI void enesim_renderer_image_size_get(Enesim_Renderer *r, double *w, double *h)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(r);
	if (!thiz) return;
	if (w) *w = thiz->current.w;
	if (h) *h = thiz->current.h;
}

/**
 * @brief Set the surface used as pixel source for the image renderer
 * @ender_prop{source_surface}
 * @param[in] r The image renderer.
 * @param[in] src The surface to use @ender_transfer{full}
 *
 * This function sets the source pixels to use for the image renderer.
 */
EAPI void enesim_renderer_image_source_surface_set(Enesim_Renderer *r, Enesim_Surface *src)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(r);
	if (thiz->current.s)
		enesim_surface_unref(thiz->current.s);
	thiz->current.s = src;
	thiz->src_changed = EINA_TRUE;
}

/**
 * @brief Retrieve the surface used as the pixel source for the image renderer
 * @ender_prop{source_surface}
 * @param[in] r The image renderer.
 * @return src The source surface @ender_transfer{none}
 *
 * This function returns the surface used as the pixel source
 */
EAPI Enesim_Surface * enesim_renderer_image_source_surface_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Image *thiz;

	thiz = ENESIM_RENDERER_IMAGE(r);
	return enesim_surface_ref(thiz->current.s);
}


/**
 * @brief Add an area to repaint on the renderer
 *
 * @param[in] r The image renderer.
 * @param[in] area The area to repaint
 *
 * This function adds a repaint area (damage) using the source surface coordinate
 * space. Next time a @ref enesim_renderer_damages_get is called, this new area will
 * be taken into account too.
 */
EAPI void enesim_renderer_image_damage_add(Enesim_Renderer *r, const Eina_Rectangle *area)
{
	Enesim_Renderer_Image *thiz;
	Eina_Rectangle *d;

	thiz = ENESIM_RENDERER_IMAGE(r);

	d = calloc(1, sizeof(Eina_Rectangle));
	*d = *area;
	thiz->surface_damages = eina_list_append(thiz->surface_damages, d);
}
