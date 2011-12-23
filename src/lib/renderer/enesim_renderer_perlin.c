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
#define ENESIM_RENDERER_PERLIN_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_PERLIN_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_PERLIN_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Perlin
{
	EINA_MAGIC
	struct {
		double val;
		Eina_F16p16 *coeff;
	} xfreq, yfreq, ampl;
	double persistence;
	int octaves;
} Enesim_Renderer_Perlin;

static inline Enesim_Renderer_Perlin * _perlin_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Perlin *thiz;

	thiz = enesim_renderer_data_get(r);
	ENESIM_RENDERER_PERLIN_MAGIC_CHECK(thiz);

	return thiz;
}

#if 0
static void _argb8888_span_affine(Enesim_Renderer *r, int x, int y, unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Perlin *thiz;
	uint32_t *end = dst + len;
	Eina_F16p16 xx, yy;

	thiz = _perlin_get(r);
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

static void _argb8888_span_identity(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Perlin *thiz;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	Eina_F16p16 xx, yy;
	Eina_F16p16 *freq, *ampl, per;

	thiz = _perlin_get(r);
	/* end of state setup */
	xx = eina_f16p16_int_from(x);
	yy = eina_f16p16_int_from(y);
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
		xx += 65536;
	}
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _perlin_name(Enesim_Renderer *r)
{
	return "perlin";
}

static Eina_Bool _perlin_state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state, Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Perlin *thiz;

	thiz = _perlin_get(r);
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

	if (state->transformation_type != ENESIM_MATRIX_IDENTITY)
		return EINA_FALSE;

	*fill = _argb8888_span_identity;
	return EINA_TRUE;
}

static void _perlin_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Perlin *thiz;

	thiz = _perlin_get(r);
	if (thiz->xfreq.coeff)
		free(thiz->xfreq.coeff);
	if (thiz->yfreq.coeff)
		free(thiz->yfreq.coeff);
	if (thiz->ampl.coeff)
		free(thiz->ampl.coeff);
}

static void _perlin_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Renderer_Perlin *thiz;

	thiz = _perlin_get(r);
	if (!thiz)
	{
		*flags = 0;
		return;
	}

	*flags = ENESIM_RENDERER_FLAG_ARGB8888;
}

static void _perlin_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Perlin *thiz;

	thiz = _perlin_get(r);
	free(thiz);
}

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _perlin_name,
	/* .free = 			*/ _perlin_free,
	/* .boundings =  		*/ NULL,
	/* .destination_transform = 	*/ NULL,
	/* .flags = 			*/ _perlin_flags,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ NULL,
	/* .sw_setup = 			*/ _perlin_state_setup,
	/* .sw_cleanup = 		*/ _perlin_state_cleanup,
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
 * Creates a new perlin renderer
 * @return The renderer
 */
EAPI Enesim_Renderer * enesim_renderer_perlin_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Perlin *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Perlin));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_PERLIN_MAGIC);
	thiz->xfreq.val = 1; /* 1 2 4 8 ... */
	thiz->yfreq.val = 1; /* 1 2 4 8 ... */
	thiz->ampl.val = 1; /* p p2 p3 p4 ... */

	r = enesim_renderer_new(&_descriptor, thiz);

	return r;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_perlin_octaves_set(Enesim_Renderer *r, unsigned int octaves)
{
	Enesim_Renderer_Perlin *thiz;

	thiz = _perlin_get(r);
	thiz->octaves = octaves;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_perlin_persistence_set(Enesim_Renderer *r, double persistence)
{
	Enesim_Renderer_Perlin *thiz;

	thiz = _perlin_get(r);
	thiz->persistence = persistence;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_perlin_amplitude_set(Enesim_Renderer *r, double ampl)
{
	Enesim_Renderer_Perlin *thiz;

	thiz = _perlin_get(r);
	thiz->ampl.val = ampl;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_perlin_xfrequency_set(Enesim_Renderer *r, double freq)
{
	Enesim_Renderer_Perlin *thiz;

	thiz = _perlin_get(r);
	thiz->xfreq.val = freq;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_perlin_yfrequency_set(Enesim_Renderer *r, double freq)
{
	Enesim_Renderer_Perlin *thiz;

	thiz = _perlin_get(r);
	thiz->yfreq.val = freq;
}
