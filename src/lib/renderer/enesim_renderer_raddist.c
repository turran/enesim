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
#include "enesim_private.h"
#include "libargb.h"

#include "enesim_eina.h"
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
#include "enesim_renderer_raddist.h"

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
#define ENESIM_RENDERER_RADDIST_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_RADDIST_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_RADDIST_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Raddist_State
{
	double scale;
	double radius;
	/* the x and y origin of the circle */
	int orx;
	int ory;
} Enesim_Renderer_Raddist_State;

typedef struct _Enesim_Renderer_Raddist
{
	EINA_MAGIC
	Enesim_Surface *src;
	Enesim_Renderer_Raddist_State current;
	Enesim_Renderer_Raddist_State past;
	Eina_Bool changed : 1;
	Eina_Bool src_changed : 1;
} Enesim_Renderer_Raddist;

static inline Enesim_Renderer_Raddist * _raddist_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Raddist *thiz;

	thiz = enesim_renderer_data_get(r);
	ENESIM_RENDERER_RADDIST_MAGIC_CHECK(thiz);

	return thiz;
}

static void _span_identity(Enesim_Renderer *r,
		int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Raddist *thiz;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	double r_inv;
	uint32_t *src;
	int sw, sh;
	size_t sstride;
	double ox, oy;

	thiz = _raddist_get(r);
	/* setup the parameters */
	enesim_surface_size_get(thiz->src, &sw, &sh);
	enesim_surface_data_get(thiz->src, (void **)&src, &sstride);
	/* FIXME move this to the setup */
	r_inv = 1.0f / thiz->current.radius;

	enesim_renderer_origin_get(r, &ox, &oy);
	x -= (int)ox;
	y -= (int)oy;

	x -= thiz->current.orx;
	y -= thiz->current.ory;

	while (dst < end)
	{
		Eina_F16p16 sxx, syy;
		uint32_t p0;
		int sx, sy;
		double rad = hypot(x, y);

		rad = (((thiz->current.scale * (thiz->current.radius - rad)) + rad) * r_inv);
		sxx = eina_f16p16_double_from((rad * x) + thiz->current.orx);
		syy = eina_f16p16_double_from((rad * y) + thiz->current.ory);

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
static const char * _raddist_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "raddist";
}

static Eina_Bool _raddist_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error EINA_UNUSED)
{
	Enesim_Renderer_Raddist *thiz;
	Enesim_Matrix_Type type;

	thiz = _raddist_get(r);
	if (!thiz->src) return EINA_FALSE;

	enesim_renderer_transformation_type_get(r, &type);
	if (type == ENESIM_MATRIX_IDENTITY)
		*fill = _span_identity;
	else
		return EINA_FALSE;
	return EINA_TRUE;
}

static void _raddist_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Raddist *thiz;

	thiz = _raddist_get(r);
	thiz->changed = EINA_FALSE;
	thiz->src_changed = EINA_FALSE;
	thiz->past = thiz->current;
}

static void _bounds(Enesim_Renderer *r,
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Raddist *thiz;

	thiz = _raddist_get(r);
	if (!thiz->src)
	{
		rect->x = 0;
		rect->y = 0;
		rect->w = 0;
		rect->h = 0;
	}
	else
	{
		int sw, sh;

		enesim_surface_size_get(thiz->src, &sw, &sh);
		rect->x = 0;
		rect->y = 0;
		rect->w = sw;
		rect->h = sh;
	}
}

static Eina_Bool _raddist_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Raddist *thiz;

	thiz = _raddist_get(r);
	if (thiz->src_changed) return EINA_TRUE;
	if (!thiz->changed) return EINA_FALSE;

	if (thiz->current.orx != thiz->past.orx)
		return EINA_TRUE;
	if (thiz->current.ory != thiz->past.ory)
		return EINA_TRUE;
	if (thiz->current.scale != thiz->past.scale)
		return EINA_TRUE;
	if (thiz->current.radius != thiz->past.radius)
		return EINA_TRUE;
	return EINA_FALSE;
}

static void _raddist_flags(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_ARGB8888;
}

static void _raddist_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Raddist *thiz;

	thiz = _raddist_get(r);
	free(thiz);
}

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .base_name_get = 		*/ _raddist_name,
	/* .free = 			*/ _raddist_free,
	/* .bounds_get =  		*/ _bounds,
	/* .destination_bounds_get = 	*/ NULL,
	/* .flags_get = 		*/ _raddist_flags,
	/* .is_inside = 		*/ NULL,
	/* .damages_get = 		*/ NULL,
	/* .has_changed = 		*/ _raddist_has_changed,
	/* .sw_hints_get = 		*/ NULL,
	/* .sw_setup = 			*/ _raddist_sw_setup,
	/* .sw_cleanup = 		*/ _raddist_sw_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
	/* .opengl_setup =          	*/ NULL,
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
	Enesim_Renderer_Raddist *thiz;

	if (!radius)
		radius = 1;
	thiz = _raddist_get(r);
	thiz->current.radius = radius;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_raddist_factor_set(Enesim_Renderer *r, double factor)
{
	Enesim_Renderer_Raddist *thiz;

	thiz = _raddist_get(r);
	if (factor > 1.0)
		factor = 1.0;
	thiz->current.scale = factor;
	thiz->changed = EINA_TRUE;
}

EAPI double enesim_renderer_raddist_factor_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Raddist *thiz;

	thiz = _raddist_get(r);
	return thiz->current.scale;
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
	thiz->src_changed = EINA_TRUE;
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
	Enesim_Renderer_Raddist *thiz;

	thiz = _raddist_get(r);
	thiz->current.orx = ox;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_raddist_y_set(Enesim_Renderer *r, int oy)
{
	Enesim_Renderer_Raddist *thiz;

	thiz = _raddist_get(r);
	thiz->current.ory = oy;
	thiz->changed = EINA_TRUE;
}
