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

	//printf("a = %g, b = %g\n", a, b);
	c = sqrt((a * a) - (b * b));
	//printf("c = %g\n", c);
	*f1x = cx - c;
	*f1y = cy;
	*f2x = cx + c;
	*f2y = cy;
	*max = 2 * a;
	*min = 2 * c;
	//printf("f1 %g %g, f2 %g %g min %g max %g\n", *f1x, *f1y, *f2x, *f2y, *min, *max);
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static int _radial_length(Enesim_Renderer *r)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);
	return lrint(thiz->max - thiz->min);
}

static Eina_F16p16 _radial_distance(Enesim_Renderer *r, Eina_F16p16 x,
		Eina_F16p16 y)
{
	Enesim_Renderer_Gradient_Radial *thiz;
	Eina_F16p16 res;
	double a, b;
	double ret;
	double d1, d2;

	thiz = _radial_get(r);
	a = eina_f16p16_double_to(x);
	b = eina_f16p16_double_to(y);

	d1 = a - thiz->f1.x;
	d2 = b - thiz->f1.y;

	ret = hypot(d1, d2);

	d1 = a - thiz->f2.x;
	d2 = b - thiz->f2.y;

	ret += hypot(d1, d2);
	//printf("old distance = %g\n", r);
	ret -= thiz->min;

	return eina_f16p16_double_from(ret);
}

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
				&thiz->f2.x, &thiz->f2.y, &thiz->min, &thiz->max);
	}
	else
	{
		_get_focis(thiz->center.x, thiz->center.y, thiz->radius.y,
				thiz->radius.x, &thiz->f1.y, &thiz->f1.x,
				&thiz->f2.y, &thiz->f2.x, &thiz->min, &thiz->max);
	}
	//printf("generating span for = %g (%g %g)\n", thiz->max - thiz->min, thiz->max, thiz->min);
	return EINA_TRUE;
}

static void _radial_boundings(Enesim_Renderer *r, Enesim_Rectangle *boundings)
{
	Enesim_Renderer_Gradient_Radial *thiz;

	thiz = _radial_get(r);

	boundings->x = thiz->center.x - thiz->radius.x;
	boundings->y = thiz->center.y - thiz->radius.y;
	boundings->w = thiz->radius.x * 2;
	boundings->h = thiz->radius.y * 2;
}

static Enesim_Renderer_Gradient_Descriptor _radial_descriptor = {
	/* .distance =   */ _radial_distance,
	/* .length =     */ _radial_length,
	/* .sw_setup =   */ _state_setup,
	/* .sw_cleanup = */ _state_cleanup,
	/* .free =       */ NULL,
	/* .boundings =  */ _radial_boundings,
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

