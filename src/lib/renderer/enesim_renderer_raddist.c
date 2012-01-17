/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2011 Jorge Luis Zapata
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

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_RADDIST_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_RADDIST_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_RADDIST_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Raddist
{
	EINA_MAGIC
	Enesim_Surface *src;
	double scale;
	double radius;
	/* the x and y origin of the circle */
	int orx, ory;
} Enesim_Renderer_Raddist;

static inline Enesim_Renderer_Raddist * _raddist_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Raddist *thiz;

	thiz = enesim_renderer_data_get(r);
	ENESIM_RENDERER_RADDIST_MAGIC_CHECK(thiz);

	return thiz;
}

static void _span_identity(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Raddist *rd;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	double r_inv;
	uint32_t *src;
	int sw, sh;
	size_t sstride;
	double ox, oy;

	rd = _raddist_get(r);
	/* setup the parameters */
	enesim_surface_size_get(rd->src, &sw, &sh);
	enesim_surface_data_get(rd->src, (void **)&src, &sstride);
	/* FIXME move this to the setup */
	r_inv = 1.0f / rd->radius;

	enesim_renderer_origin_get(r, &ox, &oy);
	x -= (int)ox;
	y -= (int)oy;

	x -= rd->orx;
	y -= rd->ory;

	while (dst < end)
	{
		Eina_F16p16 sxx, syy;
		uint32_t p0;
		int sx, sy;
		double r = hypot(x, y);

		r = (((rd->scale * (rd->radius - r)) + r) * r_inv);
		sxx = eina_f16p16_double_from((r * x) + rd->orx);
		syy = eina_f16p16_double_from((r * y) + rd->ory);

		sy = (syy >> 16);
		sx = (sxx >> 16);
		p0 = argb8888_sample_good(src, sstride, sw, sh, sxx, syy, sx, sy);

		*dst++ = p0;
		x++;
	}
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _raddist_name(Enesim_Renderer *r)
{
	return "raddist";
}

static Eina_Bool _state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Raddist *rd;
	const Enesim_Renderer_State *cs = states[ENESIM_STATE_CURRENT];

	rd = _raddist_get(r);
	if (!rd->src) return EINA_FALSE;

	*fill = _span_identity;
	if (cs->transformation_type == ENESIM_MATRIX_IDENTITY)
		*fill = _span_identity;
	else
		return EINA_FALSE;
	return EINA_TRUE;
}

static void _boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Raddist *rd;

	rd = _raddist_get(r);
	if (!rd->src)
	{
		rect->x = 0;
		rect->y = 0;
		rect->w = 0;
		rect->h = 0;
	}
	else
	{
		int sw, sh;

		enesim_surface_size_get(rd->src, &sw, &sh);
		rect->x = 0;
		rect->y = 0;
		rect->w = sw;
		rect->h = sh;
	}
}

static void _raddist_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Renderer_Raddist *thiz;

	thiz = _raddist_get(r);
	if (!thiz)
	{
		*flags = 0;
		return;
	}

	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_ARGB8888;
}

static void _free(Enesim_Renderer *r)
{
	Enesim_Renderer_Raddist *thiz;

	thiz = _raddist_get(r);
	free(thiz);
}

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _raddist_name,
	/* .free = 			*/ _free,
	/* .boundings =  		*/ _boundings,
	/* .destination_boundings = 	*/ NULL,
	/* .flags = 			*/ _raddist_flags,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ NULL,
	/* .sw_setup = 			*/ _state_setup,
	/* .sw_cleanup = 		*/ NULL,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
	/* .opengl_setup =          	*/ NULL,
	/* .opengl_shader_setup = 	*/ NULL,
	/* .opengl_cleanup =        	*/ NULL
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Renderer * enesim_renderer_raddist_new(void)
{
	Enesim_Renderer_Raddist *thiz;
	Enesim_Renderer *r;

	thiz = calloc(1, sizeof(Enesim_Renderer_Raddist));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_RADDIST_MAGIC);
	r = enesim_renderer_new(&_descriptor, thiz);

	return r;
}

EAPI void enesim_renderer_raddist_radius_set(Enesim_Renderer *r, double radius)
{
	Enesim_Renderer_Raddist *rd;

	if (!radius)
		radius = 1;
	rd = _raddist_get(r);
	rd->radius = radius;
}

EAPI void enesim_renderer_raddist_factor_set(Enesim_Renderer *r, double factor)
{
	Enesim_Renderer_Raddist *rd;

	rd = _raddist_get(r);
	if (factor > 1.0)
		factor = 1.0;
	rd->scale = factor;
}

EAPI double enesim_renderer_raddist_factor_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Raddist *rd;

	rd = _raddist_get(r);
	return rd->scale;
}

EAPI void enesim_renderer_raddist_src_set(Enesim_Renderer *r, Enesim_Surface *src)
{
	Enesim_Renderer_Raddist *thiz;

	thiz = _raddist_get(r);
	if (thiz->src)
		enesim_surface_unref(thiz->src);
	thiz->src = src;
	if (thiz->src)
		thiz->src = enesim_surface_ref(thiz->src);
}

EAPI void enesim_renderer_raddist_src_get(Enesim_Renderer *r, Enesim_Surface **src)
{
	Enesim_Renderer_Raddist *thiz;

	if (!src) return;
	thiz = _raddist_get(r);
	*src = thiz->src;
	if (thiz->src)
		thiz->src = enesim_surface_ref(thiz->src);
}

EAPI void enesim_renderer_raddist_x_set(Enesim_Renderer *r, int ox)
{
	Enesim_Renderer_Raddist *rd;

	rd = _raddist_get(r);
	rd->orx = ox;
}

EAPI void enesim_renderer_raddist_y_set(Enesim_Renderer *r, int oy)
{
	Enesim_Renderer_Raddist *rd;

	rd = _raddist_get(r);
	rd->ory = oy;
}
