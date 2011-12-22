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

typedef struct _Enesim_Renderer_Image
{
	EINA_MAGIC
	Enesim_Surface *s;
	int x, y;
	unsigned int w, h;
	/* private */
	Enesim_F16p16_Matrix matrix;
	int *yoff;
	int *xoff;
	Enesim_Compositor_Point point;
	Enesim_Compositor_Span span;
} Enesim_Renderer_Image;

static inline void _offsets(unsigned int cs, unsigned int cl, unsigned int sl, int *off)
{
	int c;

	for (c = 0; c < cl; c++)
	{
		off[c] = ((c + cs) * sl) / cl;
	}
}

static inline Enesim_Renderer_Image * _image_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Image *thiz;

	thiz = enesim_renderer_data_get(r);
	ENESIM_RENDERER_IMAGE_MAGIC_CHECK(thiz);

	return thiz;
}

#if 0
static void _scale_good(Surface *s, int x, int y, unsigned int len, uint32_t *dst)
{
	uint32_t sstride;
	uint32_t *src;
	Eina_Rectangle ir, dr;
	int sy;
	int sw, sh;

	if (y < s->r.oy || y > s->r.oy + s->w)
	{
		while (len--)
			*dst++ = 0;
		return;
	}

	enesim_surface_data_get(s->s, &src, &stride);
	enesim_surface_size_get(s->s, &sw, &sh);
	sy = s->yoff[y - s->r.oy];
	src += sstride * sy;
	x -= s->r.ox;

	while (len--)
	{
		if (x >= 0 && x < s->w)
		{
			uint32_t p0, p1, p2, p3;
			uint32_t *ssrc;
			int sx;


			sx = s->xoff[x];
			ssrc = src + sx;
			p0 = *(ssrc);
			if ((sx + 1) < sw)
				p1 = *(ssrc + 1);
			if ((sy + 1) < sh)
			{
				if (sx > -1)
					p2 = *(ssrc + sstride);
				if ((sx + 1) < sw)
					p3 = *(ssrc + sstride + 1);
			}
			p0 = argb8888_interp_256(128, p1, p0);
			p2 = argb8888_interp_256(128, p3, p2);
			p0 = argb8888_interp_256(128, p2, p0);
			*dst = p0;
		}
		else
			*dst = 0;
		x++;
		dst++;
	}
}
#endif

static void _scale_fast_identity(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz;
	size_t sstride;
	uint32_t *src;
	uint32_t *dst = ddata;
	double ox, oy;

	thiz = _image_get(r);
	enesim_renderer_origin_get(r, &ox, &oy);
	y -= oy;
	x -= ox;

	enesim_surface_data_get(thiz->s, (void **)&src, &sstride);
	src = argb8888_at(src, sstride, 0, thiz->yoff[y]);

	while (len--)
	{
		if (x >= 0 && x < thiz->w)
		{
			*dst = *(src + thiz->xoff[x]);
		}
		else
			*dst = 0;
		x++;
		dst++;
	}
}

static void _scale_fast_affine(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz;
	size_t sstride;
	uint32_t *src;
	uint32_t *dst = ddata;
	Eina_F16p16 xx, yy;
	int sw, sh;

	thiz = _image_get(r);
	enesim_renderer_affine_setup(r, x, y, &thiz->matrix, &xx, &yy);

	enesim_surface_data_get(thiz->s, (void **)&src, &sstride);
	enesim_surface_size_get(thiz->s, &sw, &sh);

	while (len--)
	{
		uint32_t p0 = 0;
		int sx, sy;

		x = eina_f16p16_int_to(xx);
		y = eina_f16p16_int_to(yy);

		if (x >= 0 && x < thiz->w && y >= 0 && y < thiz->h)
		{
			sy = thiz->yoff[y];
			sx = thiz->xoff[x];
			p0 = argb8888_sample_good(src, sstride, sw, sh, xx, yy, sx, sy);
		}

		*dst++ = p0;
		yy += thiz->matrix.yx;
		xx += thiz->matrix.xx;
	}
}

static void _a8_to_argb8888_noscale(Enesim_Renderer *r, int x, int y, unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Image *thiz;
	size_t sstride;
	uint8_t *src;
	double ox, oy;

	thiz = _image_get(r);
	enesim_renderer_origin_get(r, &ox, &oy);
	x -= ox;
	y -= oy;

	enesim_surface_data_get(thiz->s, (void **)&src, &sstride);
	src += (sstride * y) + x;
	while (len--)
	{
		if (x >= 0 && x < thiz->w)
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

static void _argb8888_to_argb8888_noscale(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Image *thiz;
	Enesim_Color color;
	size_t sstride;
	uint32_t *src;
	uint32_t *dst = ddata;
	double ox, oy;

 	thiz = _image_get(r);
	enesim_renderer_origin_get(r, &ox, &oy);
	enesim_renderer_color_get(r, &color);
	x -= ox;
	y -= oy;

	enesim_surface_data_get(thiz->s, (void **)&src, &sstride);
	src = argb8888_at(src, sstride, x, y);
	thiz->span(dst, len, src, color, NULL);
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _image_name(Enesim_Renderer *r)
{
	return "image";
}

static void _image_boundings(Enesim_Renderer *r, Enesim_Rectangle *rect)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz->s)
	{
		rect->x = 0;
		rect->y = 0;
		rect->w = 0;
		rect->h = 0;
	}
	else
	{
		rect->x = thiz->x;
		rect->y = thiz->y;
		rect->w = thiz->w;
		rect->h = thiz->h;
	}
}

static void _image_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (thiz->xoff)
	{
		free(thiz->xoff);
		thiz->xoff = NULL;
	}

	if (thiz->yoff)
	{
		free(thiz->yoff);
		thiz->yoff = NULL;
	}
}

static Eina_Bool _image_state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state, Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Image *thiz;
	Enesim_Format fmt;
	int sw, sh;

	thiz = _image_get(r);
	if (thiz->w < 1 || thiz->h < 1)
	{
		WRN("Wrong size %d %d", thiz->w, thiz->h);
		return EINA_FALSE;
	}

	if (!thiz->s)
	{
		WRN("No surface set");
		return EINA_FALSE;
	}

	_image_state_cleanup(r, s);

	enesim_surface_size_get(thiz->s, &sw, &sh);
	/* FIXME we need to use the format from the destination surface */
	fmt = ENESIM_FORMAT_ARGB8888;

	if (sw != thiz->w || sh != thiz->h)
	{
		/* as we need to scale we can only use the point compositor */
		thiz->point = enesim_compositor_point_get(state->rop, &fmt,
				ENESIM_FORMAT_ARGB8888, state->color, ENESIM_FORMAT_NONE);
		if (!thiz->point)
		{
			WRN("Not suitable point compositor for sfmt %d and color %08x",
					fmt, state->color);
			return EINA_FALSE;
		}

		thiz->xoff = malloc(sizeof(int) * thiz->w);
		thiz->yoff = malloc(sizeof(int) * thiz->h);
		_offsets(thiz->x, thiz->w, sw, thiz->xoff);
		_offsets(thiz->y, thiz->h, sh, thiz->yoff);

		if (state->transformation_type == ENESIM_MATRIX_IDENTITY)
			*fill = _scale_fast_identity;
		else if (state->transformation_type == ENESIM_MATRIX_AFFINE)
		{
			enesim_matrix_f16p16_matrix_to(&state->transformation,
					&thiz->matrix);
			*fill = _scale_fast_affine;
		}
	}
	else
	{
		/* we can use directly a span compositor */
		thiz->span = enesim_compositor_span_get(state->rop, &fmt,
				ENESIM_FORMAT_ARGB8888, state->color, ENESIM_FORMAT_NONE);
		if (!thiz->span)
		{
			WRN("Not suitable span compositor for sfmt %d and color %08x",
					fmt, state->color);
			return EINA_FALSE;
		}
		*fill = _argb8888_to_argb8888_noscale;
	}
	return EINA_TRUE;
}

static void _image_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz)
	{
		*flags = 0;
		return;
	}

	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_ARGB8888;
			//| ENESIM_RENDERER_FLAG_COLORIZE
			//| ENESIM_RENDERER_FLAG_ROP;
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
	/* .destination_transform = 	*/ NULL,
	/* .flags = 			*/ _image_flags,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ NULL,
	/* .sw_setup = 			*/ _image_state_setup,
	/* .sw_cleanup = 		*/ _image_state_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL
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
EAPI void enesim_renderer_image_x_set(Enesim_Renderer *r, int x)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;
	thiz->x = x;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_image_y_set(Enesim_Renderer *r, int y)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;
	thiz->y = y;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_image_w_set(Enesim_Renderer *r, int w)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;
	thiz->w = w;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_image_h_set(Enesim_Renderer *r, int h)
{
	Enesim_Renderer_Image *thiz;

	thiz = _image_get(r);
	if (!thiz) return;
	thiz->h = h;
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

	if (thiz->s)
		enesim_surface_unref(thiz->s);
	thiz->s = src;
	if (thiz->s)
		thiz->s = enesim_surface_ref(thiz->s);
}
