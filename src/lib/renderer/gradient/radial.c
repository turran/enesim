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
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Renderer_Gradient_Radial
{
	/* properties */
	struct {
		double x, y;
	} center, radius;
	/* state generated */
	struct {
		double x, y;
	} f1, f2;
	double min;
	double max;
} Enesim_Renderer_Gradient_Radial;

static inline Enesim_Renderer_Gradient_Radial * _radial_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = enesim_renderer_gradient_data_get(r);
	return thiz;
}

static inline void _get_focis(double cx, double cy, double a, double b,
		double *f1x, double *f1y, double *f2x, double *f2y,
		double *min, double *max)
{
	double c;

	c = sqrt(a * a - b * b);
	*f1x = cx - c;
	*f1y = cy;
	*f2x = cx + c;
	*f2y = cy;
	*max = 2 * a;
	*min = 2 * c;
}

static inline Eina_F16p16 _radial_distance(Enesim_Renderer_Gradient_Radial *thiz, Eina_F16p16 x,
		Eina_F16p16 y)
{
	Eina_F16p16 a, b;

}

static inline uint32_t _radial_pad(Enesim_Renderer *r, Eina_F16p16 p)
{
	int fp;
	uint32_t v;
	uint32_t *data;
	int data_length;

	enesim_renderer_gradient_pixels_get(r, &data, &data_length);
	fp = eina_f16p16_int_to(p);
	if (fp < 0)
	{
		v = data[0];
	}
	else if (fp >= data_length - 1)
	{
		v = data[data_length - 1];
	}
	else
	{
		uint16_t a;

		a = eina_f16p16_fracc_get(p) >> 8;
		v = argb8888_interp_256(1 + a, data[fp + 1], data[fp]);
	}

	return v;
}

static void _argb8888_pad_span_identity(Enesim_Renderer *r, int x, int y,
		unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Gradient_Radial *thiz;
	uint32_t *end = dst + len;
	Eina_F16p16 xx, yy;
	Eina_F16p16 d;

	thiz = _radial_get(r);
	renderer_identity_setup(r, x, y, &xx, &yy);
	d = _radial_distance(thiz, xx, yy);
	while (dst < end)
	{
		*dst++ = _radial_pad(r, d);
		d += EINA_F16P16_ONE;
	}
	/* FIXME is there some mmx bug there? the interp_256 already calls this
	 * but the float support is fucked up
	 */
#ifdef EFL_HAVE_MMX
	_mm_empty();
#endif
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static void _state_cleanup(Enesim_Renderer *r)
{
}

static Eina_Bool _state_setup(Enesim_Renderer *r, Enesim_Renderer_Sw_Fill *fill)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	/* TODO check that the radius are positive */
	if (thiz->radius.x > thiz->radius.y)
	{
		_get_focis(thiz->center.x, thiz->center.y, thiz->radius.x,
				thiz->radius.y, &thiz->f1.x, &thiz->f1.y,
				&thiz->f2.x, &thiz->f2.y, &thiz->max, &thiz->min);
	}
	else
	{
		_get_focis(thiz->center.x, thiz->center.y, thiz->radius.y,
				thiz->radius.x, &thiz->f1.y, &thiz->f1.x,
				&thiz->f2.y, &thiz->f2.x, &thiz->max, &thiz->min);
	}
	enesim_renderer_gradient_state_setup(r, lrint(thiz->max - thiz->min));
	*fill = _argb8888_pad_span_identity;
	return EINA_TRUE;
}

static Enesim_Renderer_Descriptor _radial_descriptor = {
	/* .sw_setup =   */ _state_setup,
	/* .sw_cleanup = */ _state_cleanup,
	/* .free =       */ NULL,
	/* .boundings =  */ NULL,
	/* .flags =      */ NULL,
	/* .is_inside =  */ 0
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * FIXME
 * To be documented
 */
EAPI Enesim_Renderer * enesim_renderer_gradient_radial_new(void)
{
	Enesim_Renderer_Gradient_Radial *thiz;
	Enesim_Renderer *r;

	thiz = calloc(1, sizeof(Enesim_Renderer_Gradient_Radial));
	if (!thiz) return NULL;
	r = enesim_renderer_gradient_new(&_radial_descriptor, thiz);

	return r;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_center_x_set(Enesim_Renderer *r, double v)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	thiz->center.x = v;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_center_y_set(Enesim_Renderer *r, double v)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	thiz->center.y = v;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_radius_y_set(Enesim_Renderer *r, double v)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	thiz->radius.y = v;
}
/**
 * FIXME
 * To be documented
 */
EAPI void enesim_renderer_gradient_radial_radius_x_set(Enesim_Renderer *r, double v)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	thiz->radius.x = v;
}

