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
/**
 * @todo
 * - add support for sw and sh
 * - add support for qualities (good scaler, interpolate between the four neighbours)
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_IMAGE_MAGIC_CHECK(d)\
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_IMAGE_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_IMAGE_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Image_State
{
	Enesim_Surface *s;
	double x, y;
	double w, h;
} Enesim_Renderer_Image_State;

typedef struct _Enesim_Renderer_Image
{
	EINA_MAGIC
	Enesim_Renderer_Image_State current;
	Enesim_Renderer_Image_State past;
	/* private */
	unsigned int *src;
	int sw, sh;
	int sstride;
	int xx, yy;
	double w, h;
	int iw, ih;
	int mxx, myy;
	Enesim_F16p16_Matrix matrix;
	Enesim_Compositor_Span span;
	Eina_Bool scaled : 1;
	Eina_Bool changed : 1;
} Enesim_Renderer_Image;


static inline Enesim_Renderer_Image * _image_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Image *thiz;

	thiz = enesim_renderer_data_get(r);
	ENESIM_RENDERER_IMAGE_MAGIC_CHECK(thiz);

	return thiz;
}


static void _argb8888_blend_span(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = _image_get(r);
	uint32_t *src = thiz->src;
	uint32_t *dst = ddata;

	x -= (thiz->xx >> 16);
	y -= (thiz->yy >> 16);

	if ((y < 0) || (y >= thiz->sh) || (x >= thiz->sw) || (x + len < 0))
		return;
	if (x < 0)
	{
		len += x;  dst -= x;  x = 0;
	}
	if (len > thiz->sw - x)
		len = thiz->sw - x;
	src = argb8888_at(src, thiz->sstride, x, y);
	thiz->span(dst, len, src, state->color, NULL);
}

static void _argb8888_image_no_scale_identity(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = _image_get(r);
	uint32_t *src = thiz->src;
	uint32_t *dst = ddata;


	x -= (thiz->xx >> 16);
	y -= (thiz->yy >> 16);

	if ((y < 0) || (y >= thiz->sh) || (x >= thiz->sw) || (x + len < 0) || !state->color)
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
	thiz->span(dst, len, src, state->color, NULL);
}

static void _argb8888_image_no_scale_affine(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = _image_get(r);
	uint32_t *dst = ddata, end = dst + len;
	uint32_t *src = thiz->src;
	int sw = thiz->sw, sh = thiz->sh;
	Eina_F16p16 xx, yy;
	Enesim_Color color = state->color;

	if (!color)
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	xx = (thiz->matrix.xx * x) + (thiz->matrix.xx >> 1) + 
		(thiz->matrix.xy * y) + (thiz->matrix.xy >> 1) + 
		thiz->matrix.xz - 32768 - thiz->xx;
	yy = (thiz->matrix.yx * x) + (thiz->matrix.yx >> 1) + 
		(thiz->matrix.yy * y) + (thiz->matrix.yy >> 1) + 
		thiz->matrix.yz - 32768 - thiz->yy;

	while (dst < end)
	{
		uint32_t p0 = 0;

		x = eina_f16p16_int_to(xx);
		y = eina_f16p16_int_to(yy);

		if ( (((unsigned) (x + 1)) < (sw + 1)) & (((unsigned) (y + 1)) < (sh + 1)) )
		{
			uint32_t *p, p1 = 0, p2 = 0, p3 = 0;

			p = src + (y * sw) + x;

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
				uint16_t ax, ay;

				ax = 1 + ((xx & 0xffff) >> 8);
				ay = 1 + ((yy & 0xffff) >> 8);

				p0 = argb8888_interp_256(ax, p1, p0);
				p2 = argb8888_interp_256(ax, p3, p2);
				p0 = argb8888_interp_256(ay, p2, p0);
				if (color)
					p0 = argb8888_mul4_sym(p0, color);
			}
		}
		*dst++ = p0;  xx += thiz->matrix.xx;  yy += thiz->matrix.yx;
	}
}

static void _argb8888_image_no_scale_projective(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = _image_get(r);
}

static void _argb8888_image_scale_identity(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = _image_get(r);
	uint32_t *dst = ddata, end = dst + len;
	uint32_t *src = thiz->src;
	int sw = thiz->sw, sh = thiz->sh;
	int iw = thiz->iw, ih = thiz->ih;
	int mxx = thiz->mxx, myy = thiz->myy;
	int ixx, iyy, iy;
	int ay;
	Enesim_Color color = state->color;

	if (!color)
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	x -= (thiz->xx >> 16);
	y -= (thiz->yy >> 16);
	if ((y < 0) || (y >= ih))
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}

	iyy = myy * y;
	iy = iyy >> 16;
	src += (iy * sw);
	ay = 1 + ((iyy & 0xffff) >> 8);
	ixx = mxx * x;

	while (dst < end)
	{
		unsigned int  p0 = 0;

		if (((unsigned) x) < iw)
		{
			int  ix = ixx >> 16;
			int  ax = 1 + ((ixx & 0xffff) >> 8);
			unsigned int *p = src + ix, p3, p2, p1;

			p0 = p1 = p2 = p3 = *p;
			if ((ix + 1) < sw)
				p1 = *(p + 1);
			if ((iy + 1) < sh)
			{
				p2 = *(p + sw);
				if ((ix + 1) < sw)
				p3 = *(p + sw + 1);
			}
			if (p0 | p1 | p2 | p3)
			{
				p0 = argb8888_interp_256(ax, p1, p0);
				p2 = argb8888_interp_256(ax, p3, p2);
				p0 = argb8888_interp_256(ay, p2, p0);
				if (color)
					p0 = argb8888_mul4_sym(p0, color);
			}
        	}
		*dst++ = p0;  x++;  ixx += mxx;
	}
}

static void _argb8888_image_scale_affine(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = _image_get(r);
	uint32_t *dst = ddata, end = dst + len;
	uint32_t *src = thiz->src;
	int sw = thiz->sw, sh = thiz->sh;
	int iw = thiz->iw, ih = thiz->ih;
	int mxx = thiz->mxx, myy = thiz->myy;
	int xx, yy;
	Enesim_Color color = state->color;

	if (!color)
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	xx = (thiz->matrix.xx * x) + (thiz->matrix.xx >> 1) + 
		(thiz->matrix.xy * y) + (thiz->matrix.xy >> 1) + 
		thiz->matrix.xz - 32768 - thiz->xx;
	yy = (thiz->matrix.yx * x) + (thiz->matrix.yx >> 1) + 
		(thiz->matrix.yy * y) + (thiz->matrix.yy >> 1) + 
		thiz->matrix.yz - 32768 - thiz->yy;

	while (dst < end)
	{
		unsigned int  p0 = 0;

		x = (xx >> 16);
		y = (yy >> 16);
		if ( (((unsigned) (x + 1)) < (iw + 1)) & (((unsigned) (y + 1)) < (ih + 1)) )
		{
			int  ixx, ix, iyy, iy;
			int  ax, ay;
			unsigned int *p, p3 = 0, p2 = 0, p1 = 0;

			ixx = (mxx * xx) >> 16;  ix = ixx >> 16;
			iyy = (myy * yy) >> 16;  iy = iyy >> 16;
			ax = 1 + ((ixx >> 8) & 0xff);
			ay = 1 + ((iyy >> 8) & 0xff);

			if ((x < 0) || ((x + 2) > iw))
				ax = 1 + ((xx >> 8) & 0xff);
			if ((y < 0) || ((y + 2) > ih))
				ay = 1 + ((yy >> 8) & 0xff);

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
				p0 = argb8888_interp_256(ax, p1, p0);
				p2 = argb8888_interp_256(ax, p3, p2);
				p0 = argb8888_interp_256(ay, p2, p0);
				if (color)
					p0 = argb8888_mul4_sym(p0, color);
			}
        	}
		*dst++ = p0;  xx += thiz->matrix.xx;  yy += thiz->matrix.yx;
	}
}

static void _argb8888_image_scale_projective(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = _image_get(r);

}

static void _a8_to_argb8888_noscale(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		int x, int y, unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Image *thiz = _image_get(r);
	uint8_t *src = thiz->src;
	int sw = thiz->sw;

	x -= state->ox;
	y -= state->oy;

	src += (thiz->sstride * y) + x;
	while (len--)
	{
		if (x >= 0 && x < sw)
		{
			uint8_t a = *src;
			*dst = a << 24 | a << 16 | a << 8 | a;
		}
		else
			*dst = 0;
		x++;
		dst++;
		src++;
	}
}

static Enesim_Renderer_Sw_Fill  _spans[2][ENESIM_MATRIX_TYPES];

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _image_name(Enesim_Renderer *r)
{
	return "image";
}

static void _image_boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Image *thiz;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];

	thiz = _image_get(r);
	if (!thiz->current.s)
	{
		rect->x = 0;
		rect->y = 0;
		rect->w = 0;
		rect->h = 0;
	}
	else
	{
		rect->x = cs->sx * thiz->current.x;
		rect->y = cs->sy * thiz->current.y;
		rect->w = cs->sx * thiz->current.w;
		rect->h = cs->sy * thiz->current.h;
		/* the translate */
		rect->x += cs->ox;
		rect->y += cs->oy;
	}
}

static void _image_destination_boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Eina_Rectangle *boundings)
{
	Enesim_Rectangle oboundings;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];

	_image_boundings(r, states, &oboundings);
	/* apply the inverse matrix */
	if (cs->transformation_type != ENESIM_MATRIX_IDENTITY)
	{
		Enesim_Quad q;
		Enesim_Matrix m;

		enesim_matrix_inverse(&cs->transformation, &m);
		enesim_matrix_rectangle_transform(&m, &oboundings, &q);
		enesim_quad_rectangle_to(&q, &oboundings);
		/* fix the antialias scaling */
		oboundings.x -= m.xx;
		oboundings.y -= m.yy;
		oboundings.w += m.xx;
		oboundings.h += m.yy;
	}
	boundings->x = floor(oboundings.x);
	boundings->y = floor(oboundings.y);
	boundings->w = ceil(oboundings.w);
	boundings->h = ceil(oboundings.h);
}

static void _image_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
}

static Eina_Bool _image_state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES], Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Image *thiz;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];
	Enesim_Format fmt;
	double x, y, w, h;

	thiz = _image_get(r);
	if (!thiz->current.s)
	{
		WRN("No surface set");
		return EINA_FALSE;
	}

	enesim_surface_size_get(thiz->current.s, &thiz->sw, &thiz->sh);
	enesim_surface_data_get(thiz->current.s, (void **)(&thiz->src), &thiz->sstride);
	x = thiz->current.x;  y = thiz->current.y;
	w = thiz->current.w;  h = thiz->current.h;
	x *= cs->sx;  y *= cs->sy;
	w *= cs->sx;  h *= cs->sy;

	thiz->iw = thiz->w = w;
	thiz->ih = thiz->h = h;
	if ((thiz->iw < 1) || (thiz->ih < 1) || (thiz->sw < 1) || (thiz->sh < 1))
	{
		WRN("Size too small");
		return EINA_FALSE;
	}
	thiz->xx = 65536 * (x + cs->ox);
	thiz->yy = 65536 * (y + cs->oy);
	thiz->mxx = 65536;  thiz->myy = 65536;

	/* FIXME we need to use the format from the destination surface */
	fmt = ENESIM_FORMAT_ARGB8888;
	enesim_matrix_f16p16_matrix_to(&cs->transformation, &thiz->matrix);

	if ((fabs(thiz->sw - w) > 1/256.0) || (fabs(thiz->sh - h) > 1/256.0))
	{
		Enesim_Matrix_Type mtype;
		double sx = thiz->iw / w;
		double sy = thiz->ih / h;

		thiz->scaled = 1;
		thiz->matrix.xx *= sx;  thiz->matrix.xy *= sx;
		thiz->matrix.yy *= sy;  thiz->matrix.yx *= sy;
		
		mtype = enesim_f16p16_matrix_type_get(&thiz->matrix);

		if (thiz->sw != thiz->iw || thiz->sh != thiz->ih)
		{
			if ((thiz->sw > 1) && (thiz->iw > 1))
				thiz->mxx = ((thiz->sw - 1) << 16) / (thiz->iw - 1);
			else
				thiz->mxx = (thiz->sw << 16) / thiz->iw;
			if ((thiz->sh > 1) && (thiz->ih > 1))
				thiz->myy = ((thiz->sh - 1) << 16) / (thiz->ih - 1);
			else
				thiz->myy = (thiz->sh << 16) / thiz->ih;

			*fill = _spans[1][mtype];
		}
		else
		{
			*fill = _spans[0][mtype];
			if (mtype == ENESIM_MATRIX_IDENTITY)
			{
				thiz->span = enesim_compositor_span_get(cs->rop, &fmt,
						ENESIM_FORMAT_ARGB8888, cs->color, ENESIM_FORMAT_NONE);
				if (cs->rop == ENESIM_BLEND)
					*fill = _argb8888_blend_span;
			}
		}
	}
	else
	{
		thiz->scaled = 0;
		*fill = _spans[0][cs->transformation_type];
		if (cs->transformation_type == ENESIM_MATRIX_IDENTITY)
		{
			thiz->span = enesim_compositor_span_get(cs->rop, &fmt,
				ENESIM_FORMAT_ARGB8888, cs->color, ENESIM_FORMAT_NONE);
			if (cs->rop == ENESIM_BLEND)
				*fill = _argb8888_blend_span;
		}
	}
	return EINA_TRUE;
}

static void _image_flags(Enesim_Renderer *r, const Enesim_Renderer_State *state,
		Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_SCALE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_PROJECTIVE |
			ENESIM_RENDERER_FLAG_ARGB8888 |
			//ENESIM_RENDERER_FLAG_QUALITY |
			ENESIM_RENDERER_FLAG_COLORIZE;

	if ((state->transformation_type == ENESIM_MATRIX_IDENTITY) && (state->rop != ENESIM_FILL))
	{
		Enesim_Renderer_Image *thiz = _image_get(r);

		if (!thiz->scaled)
			*flags |= ENESIM_RENDERER_FLAG_ROP;
	}
}

static void _image_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	free(thiz);
}

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _image_name,
	/* .free = 			*/ _image_free,
	/* .boundings = 		*/ _image_boundings,
	/* .destination_boundings = 	*/ _image_destination_boundings,
	/* .flags = 			*/ _image_flags,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ NULL,
	/* .sw_setup = 			*/ _image_state_setup,
	/* .sw_cleanup = 		*/ _image_state_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
	/* .opengl_setup =          	*/ NULL,
	/* .opengl_shader_setup = 	*/ NULL,
	/* .opengl_cleanup =        	*/ NULL
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Renderer * enesim_renderer_image_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Image *thiz;
	static Eina_Bool spans_initialized = EINA_FALSE;

	if (!spans_initialized)
	{
		spans_initialized = EINA_TRUE;
		_spans[0][ENESIM_MATRIX_IDENTITY] = _argb8888_image_no_scale_identity;
		_spans[0][ENESIM_MATRIX_AFFINE] = _argb8888_image_no_scale_affine;
		_spans[0][ENESIM_MATRIX_PROJECTIVE] = _argb8888_image_no_scale_projective;
		_spans[1][ENESIM_MATRIX_IDENTITY] = _argb8888_image_scale_identity;
		_spans[1][ENESIM_MATRIX_AFFINE] = _argb8888_image_scale_affine;
		_spans[1][ENESIM_MATRIX_PROJECTIVE] = _argb8888_image_scale_projective;
	}

	thiz = calloc(1, sizeof(Enesim_Renderer_Image));
	if (!thiz) return NULL;

	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_IMAGE_MAGIC);
	r = enesim_renderer_new(&_descriptor, thiz);

	return r;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_image_x_set(Enesim_Renderer *r, double x)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;
	thiz->current.x = x;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_image_x_get(Enesim_Renderer *r, double *x)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;
	if (x) *x = thiz->current.x;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_image_y_set(Enesim_Renderer *r, double y)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;
	thiz->current.y = y;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_image_y_get(Enesim_Renderer *r, double *y)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;
	if (y) *y = thiz->current.y;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_image_position_set(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;
	thiz->current.x = x;
	thiz->current.y = y;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_image_position_get(Enesim_Renderer *r, double *x, double *y)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;
	if (x) *x = thiz->current.x;
	if (y) *y = thiz->current.y;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_image_width_set(Enesim_Renderer *r, double w)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;
	thiz->current.w = w;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_image_width_get(Enesim_Renderer *r, double *w)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;
	if (w) *w = thiz->current.w;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_image_height_set(Enesim_Renderer *r, double h)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;
	thiz->current.h = h;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_image_height_get(Enesim_Renderer *r, double *h)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;
	if (h) *h = thiz->current.h;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_image_size_set(Enesim_Renderer *r, double w, double h)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;
	thiz->current.w = w;
	thiz->current.h = h;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_image_size_get(Enesim_Renderer *r, double *w, double *h)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;
	if (w) *w = thiz->current.w;
	if (h) *h = thiz->current.h;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_image_src_set(Enesim_Renderer *r, Enesim_Surface *src)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;

	if (thiz->current.s)
		enesim_surface_unref(thiz->current.s);
	thiz->current.s = src;
	if (thiz->current.s)
		thiz->current.s = enesim_surface_ref(thiz->current.s);
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_image_src_get(Enesim_Renderer *r, Enesim_Surface **src)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;
	if (*src)
		*src = thiz->current.s;
}
