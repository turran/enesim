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
	uint32_t *src;
	int sw, sh;
	size_t sstride;
	Eina_F16p16 ixx, iyy;
	Eina_F16p16 iww, ihh;
	Eina_F16p16 mxx, myy;
	Enesim_F16p16_Matrix matrix;
	Enesim_Compositor_Span span;
	Eina_Bool simple : 1;
	Eina_Bool changed : 1;
} Enesim_Renderer_Image;


static inline Enesim_Renderer_Image * _image_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Image *thiz;

	thiz = enesim_renderer_data_get(r);
	ENESIM_RENDERER_IMAGE_MAGIC_CHECK(thiz);

	return thiz;
}

/* blend simple */
static void _argb8888_blend_span(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = _image_get(r);
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
	thiz->span(dst, len, src, state->color, NULL);
}

/* fast */
static void _argb8888_image_no_scale_affine_fast(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = _image_get(r);
	uint32_t *dst = ddata, *end = dst + len;
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
		thiz->matrix.xz - 32768 - thiz->ixx;
	yy = (thiz->matrix.yx * x) + (thiz->matrix.yx >> 1) + 
		(thiz->matrix.yy * y) + (thiz->matrix.yy >> 1) + 
		thiz->matrix.yz - 32768 - thiz->iyy;

	while (dst < end)
	{
		uint32_t p0 = 0;

		x = eina_f16p16_int_to(xx);
		y = eina_f16p16_int_to(yy);

		if (((unsigned)x < sw) & ((unsigned)y < sh))
		{
			p0 = *(src + (y * sw) + x);
			if (p0 && color)
				p0 = argb8888_mul4_sym(p0, color);
		}
		*dst++ = p0;  xx += thiz->matrix.xx;  yy += thiz->matrix.yx;
	}
}

static void _argb8888_image_no_scale_projective_fast(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = _image_get(r);
}

static void _argb8888_image_scale_identity_fast(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = _image_get(r);
	uint32_t *dst = ddata, *end = dst + len;
	uint32_t *src = thiz->src;
	int sw = thiz->sw, sh = thiz->sh;
	Eina_F16p16 iww = thiz->iww, ihh = thiz->ihh;
	Eina_F16p16 mxx = thiz->mxx, myy = thiz->myy;
	Eina_F16p16 xx, yy, ixx, iyy;
	int iy;
	Enesim_Color color = state->color;

	if (!color)
	{
		memset(dst, 0, sizeof(unsigned int) * len);
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	xx -= thiz->ixx;
	yy -= thiz->iyy;

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
		const Enesim_Renderer_State *state,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = _image_get(r);
	uint32_t *dst = ddata, *end = dst + len;
	uint32_t *src = thiz->src;
	int sw = thiz->sw, sh = thiz->sh;
	Eina_F16p16 iww = thiz->iww, ihh = thiz->ihh;
	Eina_F16p16 mxx = thiz->mxx, myy = thiz->myy;
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
		thiz->matrix.xz - 32768 - thiz->ixx;
	yy = (thiz->matrix.yx * x) + (thiz->matrix.yx >> 1) + 
		(thiz->matrix.yy * y) + (thiz->matrix.yy >> 1) + 
		thiz->matrix.yz - 32768 - thiz->iyy;

	while (dst < end)
	{
		uint32_t p0 = 0;

		if ( ((unsigned)xx < iww) & 
			((unsigned)yy < ihh) )
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

static void _argb8888_image_scale_projective_fast(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = _image_get(r);

}


/* smooth */
static void _argb8888_image_no_scale_identity(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = _image_get(r);
	uint32_t *src = thiz->src;
	uint32_t *dst = ddata;


	x -= eina_f16p16_int_to(thiz->ixx);
	y -= eina_f16p16_int_to(thiz->iyy);

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
	uint32_t *dst = ddata, *end = dst + len;
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
		thiz->matrix.xz - 32768 - thiz->ixx;
	yy = (thiz->matrix.yx * x) + (thiz->matrix.yx >> 1) + 
		(thiz->matrix.yy * y) + (thiz->matrix.yy >> 1) + 
		thiz->matrix.yz - 32768 - thiz->iyy;

	while (dst < end)
	{
		uint32_t p0 = 0;

		x = eina_f16p16_int_to(xx);
		y = eina_f16p16_int_to(yy);

		if ( (((unsigned) (x + 1)) < (sw + 1)) & (((unsigned) (y + 1)) < (sh + 1)) )
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
	uint32_t *dst = ddata, *end = dst + len;
	uint32_t *src = thiz->src;
	int sw = thiz->sw, sh = thiz->sh;
	Eina_F16p16 iww = thiz->iww, ihh = thiz->ihh;
	Eina_F16p16 mxx = thiz->mxx, myy = thiz->myy;
	Eina_F16p16 xx, yy, ixx, iyy;
	int iy;
	uint16_t ay;
	Enesim_Color color = state->color;

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
				if (color)
					p0 = argb8888_mul4_sym(p0, color);
			}
        	}
		*dst++ = p0;  xx += EINA_F16P16_ONE;  ixx += mxx;
	}
}

static void _argb8888_image_scale_affine(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz = _image_get(r);
	uint32_t *dst = ddata, *end = dst + len;
	uint32_t *src = thiz->src;
	int sw = thiz->sw, sh = thiz->sh;
	Eina_F16p16 iww = thiz->iww, ihh = thiz->ihh;
	Eina_F16p16 mxx = thiz->mxx, myy = thiz->myy;
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
		thiz->matrix.xz - 32768 - thiz->ixx;
	yy = (thiz->matrix.yx * x) + (thiz->matrix.yx >> 1) + 
		(thiz->matrix.yy * y) + (thiz->matrix.yy >> 1) + 
		thiz->matrix.yz - 32768 - thiz->iyy;

	while (dst < end)
	{
		uint32_t p0 = 0;

		if ( (((unsigned) (xx + EINA_F16P16_ONE)) < (iww + EINA_F16P16_ONE)) & 
			(((unsigned) (yy + EINA_F16P16_ONE)) < (ihh + EINA_F16P16_ONE)) )
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
	uint8_t *src = (uint8_t *)thiz->src;
	int sw = thiz->sw;

	x -= (thiz->ixx) >> 16;
	y -= (thiz->iyy) >> 16;

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
static Enesim_Renderer_Sw_Fill  _spans_fast[2][ENESIM_MATRIX_TYPES];

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
	Enesim_Matrix_Type mtype;
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

	if ((w < 1) || (h < 1) || (thiz->sw < 1) || (thiz->sh < 1))
	{
		WRN("Size too small");
		return EINA_FALSE;
	}
	thiz->iww = (w * 65536);
	thiz->ihh = (h * 65536);
	thiz->ixx = 65536 * (x + cs->ox);
	thiz->iyy = 65536 * (y + cs->oy);
	thiz->mxx = 65536;  thiz->myy = 65536;

	/* FIXME we need to use the format from the destination surface */
	fmt = ENESIM_FORMAT_ARGB8888;
	enesim_matrix_f16p16_matrix_to(&cs->transformation, &thiz->matrix);
	mtype = enesim_f16p16_matrix_type_get(&thiz->matrix);

	if ((fabs(thiz->sw - w) > 1/256.0) || (fabs(thiz->sh - h) > 1/256.0))
	{
		double sx = thiz->sw - (1 - 1/(double)thiz->sw);
		double isx = w - (1 - 1/w);
		double sy = thiz->sh - (1 - 1/(double)thiz->sh);
		double isy = h - (1 - 1/h);

		thiz->mxx = (sx * 65536) / isx;
		thiz->myy = (sy * 65536) / isy;
		thiz->simple = 0;
		if (mtype == ENESIM_MATRIX_AFFINE)  // in case it's just a translation
		{
			thiz->ixx -= thiz->matrix.xz;  thiz->matrix.xz = 0;
			thiz->iyy -= thiz->matrix.yz;  thiz->matrix.yz = 0;
			mtype = enesim_f16p16_matrix_type_get(&thiz->matrix);
		}
		if (cs->quality == ENESIM_QUALITY_FAST)
			*fill = _spans_fast[1][mtype];
		else
			*fill = _spans[1][mtype];
	}
	else
	{
		thiz->simple = 1;
		if ((thiz->ixx & 0xffff) || (thiz->iyy & 0xffff))  // non-int translation
		{
			thiz->matrix.xz -= thiz->ixx;  thiz->ixx = 0;
			thiz->matrix.yz -= thiz->iyy;  thiz->iyy = 0;
			mtype = enesim_f16p16_matrix_type_get(&thiz->matrix);
			if (mtype != ENESIM_MATRIX_IDENTITY)
				thiz->simple = 0;
		}
		if (cs->quality == ENESIM_QUALITY_FAST)
			*fill = _spans_fast[0][mtype];
		else
			*fill = _spans[0][mtype];
		if (mtype == ENESIM_MATRIX_IDENTITY)
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
			ENESIM_RENDERER_FLAG_QUALITY; 

}

static void _image_hints(Enesim_Renderer *r, const Enesim_Renderer_State *state,
		Enesim_Renderer_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_COLORIZE;
	if (state->rop != ENESIM_FILL)
	{
		Enesim_Renderer_Image *thiz = _image_get(r);

		if (thiz->simple)
			*hints |= ENESIM_RENDERER_HINT_ROP;
	}

}

static void _image_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (thiz->current.s)
		enesim_surface_unref(thiz->current.s);
	free(thiz);
}

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _image_name,
	/* .free = 			*/ _image_free,
	/* .boundings = 		*/ _image_boundings,
	/* .destination_boundings = 	*/ _image_destination_boundings,
	/* .flags = 			*/ _image_flags,
	/* .hints_get = 			*/ _image_hints,
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

		_spans_fast[0][ENESIM_MATRIX_IDENTITY] = _argb8888_image_no_scale_identity;
		_spans_fast[0][ENESIM_MATRIX_AFFINE] = _argb8888_image_no_scale_affine_fast;
		_spans_fast[0][ENESIM_MATRIX_PROJECTIVE] = _argb8888_image_no_scale_projective_fast;
		_spans_fast[1][ENESIM_MATRIX_IDENTITY] = _argb8888_image_scale_identity_fast;
		_spans_fast[1][ENESIM_MATRIX_AFFINE] = _argb8888_image_scale_affine_fast;
		_spans_fast[1][ENESIM_MATRIX_PROJECTIVE] = _argb8888_image_scale_projective_fast;
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
