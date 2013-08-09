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

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_perlin.h"
#include "enesim_perlin_private.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_renderer_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_PERLIN(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Perlin,					\
		enesim_renderer_perlin_descriptor_get())

typedef struct _Enesim_Renderer_Perlin
{
	Enesim_Renderer parent;
	struct {
		double val;
		Eina_F16p16 *coeff;
	} xfreq, yfreq, ampl;
	double persistence;
	int octaves;
	Eina_Bool changed;
} Enesim_Renderer_Perlin;

typedef struct _Enesim_Renderer_Perlin_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Perlin_Class;

#if 0
static void _argb8888_span_affine(Enesim_Renderer *r, int x, int y, int len, uint32_t *dst)
{
	Enesim_Renderer_Perlin *thiz;
	uint32_t *end = dst + len;
	Eina_F16p16 xx, yy;

	thiz = ENESIM_RENDERER_PERLIN(r);
	/* end of state setup */
	renderer_affine_setup(r, x, y, &xx, &yy);
	while (dst < end)
	{
		Eina_F16p16 per;
		uint32_t p0;
		uint8_t c;

		per = enesim_perlin_get(xx, yy, thiz->octaves, thiz->xfreq.coeff,
				thiz->yfreq.coeff, thiz->ampl.coeff);
		c = ((per & 0x1ffff) >> 9);
		/* FIXME the dispmap uses a and b for x and y displacement, we must
		 * define a better way for that, so this renderer can actually build
		 * displacement maps useful for dispmap renderer
		 */
		*dst++ = 0xff << 24 | c << 16 | c << 8 | c;
		xx += thiz->matrix.xx;
		yy += thiz->matrix.yx;
	}
}
#endif

static void _argb8888_span_identity(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Perlin *thiz;
	Enesim_Color rcolor;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	Eina_F16p16 xx, yy;

	thiz = ENESIM_RENDERER_PERLIN(r);
	rcolor = r->state.current.color;
	/* end of state setup */
	xx = eina_f16p16_int_from(x);
	yy = eina_f16p16_int_from(y);
	while (dst < end)
	{
		Enesim_Color color;
		Eina_F16p16 per;
		int i;

		per = enesim_perlin_get(xx, yy, thiz->octaves, thiz->xfreq.coeff,
				thiz->yfreq.coeff, thiz->ampl.coeff);
		/* the perlin noise is on the -1,1 range, so we need to get it back to 0-255 */
		per = eina_f16p16_mul(per, eina_extra_f16p16_double_from(255));
		per = eina_f16p16_add(per, eina_extra_f16p16_double_from(255));
		per = eina_f16p16_div(per, eina_f16p16_int_from(2));
		/* it is still possible to be outside the range? */
		i = eina_f16p16_int_to(per);
		if (i < 0) i = 0;
		else if (i > 255) i = 255;
		
		color = 0xff << 24 | i << 16 | i << 8 | i;
		if (rcolor != ENESIM_COLOR_FULL)
			color = argb8888_mul4_sym(color, rcolor);
		*dst++ = color;
		xx += 65536;
	}
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _perlin_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "perlin";
}

static Eina_Bool _perlin_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Perlin *thiz;
	Enesim_Matrix_Type type;

	thiz = ENESIM_RENDERER_PERLIN(r);
	if (thiz->xfreq.coeff)
	{
		free(thiz->xfreq.coeff);
		free(thiz->yfreq.coeff);
		free(thiz->ampl.coeff);
	}
	thiz->xfreq.coeff = malloc((sizeof(Eina_F16p16) * thiz->octaves));
	thiz->yfreq.coeff = malloc((sizeof(Eina_F16p16) * thiz->octaves));
	thiz->ampl.coeff = malloc((sizeof(Eina_F16p16) * thiz->octaves));
	enesim_perlin_coeff_set(thiz->octaves, thiz->persistence, thiz->xfreq.val,
		thiz->yfreq.val, thiz->ampl.val, thiz->xfreq.coeff, thiz->yfreq.coeff,
		thiz->ampl.coeff);

	enesim_renderer_transformation_type_get(r, &type);
	if (type != ENESIM_MATRIX_IDENTITY)
		return EINA_FALSE;

	*fill = _argb8888_span_identity;
	return EINA_TRUE;
}

static void _perlin_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Perlin *thiz;

	thiz = ENESIM_RENDERER_PERLIN(r);
	if (thiz->xfreq.coeff)
		free(thiz->xfreq.coeff);
	if (thiz->yfreq.coeff)
		free(thiz->yfreq.coeff);
	if (thiz->ampl.coeff)
		free(thiz->ampl.coeff);
	thiz->changed = EINA_FALSE;
}

static void _perlin_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_ARGB8888;
}

static Eina_Bool _perlin_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Perlin *thiz;

	thiz = ENESIM_RENDERER_PERLIN(r);
	return thiz->changed;
}

static void _perlin_sw_hints_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Rop rop EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_COLORIZE;
}

/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Perlin, Enesim_Renderer_Perlin_Class,
		enesim_renderer_perlin);

static void _enesim_renderer_perlin_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _perlin_name;
	klass->features_get = _perlin_features_get;
	klass->sw_setup = _perlin_sw_setup;
	klass->sw_cleanup = _perlin_sw_cleanup;
	klass->has_changed = _perlin_has_changed;
	klass->sw_hints_get = _perlin_sw_hints_get;
}

static void _enesim_renderer_perlin_instance_init(void *o)
{
	Enesim_Renderer_Perlin *thiz = ENESIM_RENDERER_PERLIN(o);

	thiz->xfreq.val = 1; /* 1 2 4 8 ... */
	thiz->yfreq.val = 1; /* 1 2 4 8 ... */
	thiz->ampl.val = 1; /* p p2 p3 p4 ... */
	thiz->octaves = 1;
	thiz->persistence = 1;
}

static void _enesim_renderer_perlin_instance_deinit(void *o EINA_UNUSED)
{
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a new perlin renderer
 * @return The renderer
 */
EAPI Enesim_Renderer * enesim_renderer_perlin_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_perlin);
	return r;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_perlin_octaves_set(Enesim_Renderer *r, unsigned int octaves)
{
	Enesim_Renderer_Perlin *thiz;

	thiz = ENESIM_RENDERER_PERLIN(r);
	thiz->octaves = octaves;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_perlin_persistence_set(Enesim_Renderer *r, double persistence)
{
	Enesim_Renderer_Perlin *thiz;

	thiz = ENESIM_RENDERER_PERLIN(r);
	thiz->persistence = persistence;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_perlin_amplitude_set(Enesim_Renderer *r, double ampl)
{
	Enesim_Renderer_Perlin *thiz;

	thiz = ENESIM_RENDERER_PERLIN(r);
	thiz->ampl.val = ampl;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_perlin_xfrequency_set(Enesim_Renderer *r, double freq)
{
	Enesim_Renderer_Perlin *thiz;

	thiz = ENESIM_RENDERER_PERLIN(r);
	thiz->xfreq.val = freq;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_perlin_yfrequency_set(Enesim_Renderer *r, double freq)
{
	Enesim_Renderer_Perlin *thiz;

	thiz = ENESIM_RENDERER_PERLIN(r);
	thiz->yfreq.val = freq;
}
