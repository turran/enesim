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
typedef struct _Hswitch
{
	/* properties */
	unsigned int w;
	unsigned int h;
	Enesim_Renderer *lrend;
	Enesim_Renderer *rrend;
	double step;
	/* private */
	Enesim_F16p16_Matrix matrix;
} Hswitch;

static inline Hswitch * _hswitch_get(Enesim_Renderer *r)
{
	Hswitch *thiz;

	thiz = enesim_renderer_data_get(r);
	return thiz;
}

static void _generic_good(Enesim_Renderer *r, int x, int y, unsigned int len, uint32_t *dst)
{
	Hswitch *thiz;
	Enesim_Renderer_Sw_Data *ldata;
	Enesim_Renderer_Sw_Data *rdata;
	Eina_F16p16 mmx;
	uint32_t *end = dst + len;
	int mx;

	thiz = _hswitch_get(r);
	mmx = eina_f16p16_double_from(thiz->w - (double)(thiz->w * thiz->step));
	mx = eina_f16p16_int_to(mmx);

	ldata = enesim_renderer_backend_data_get(thiz->lrend, ENESIM_BACKEND_SOFTWARE);
	rdata = enesim_renderer_backend_data_get(thiz->rrend, ENESIM_BACKEND_SOFTWARE);
	while (dst < end)
	{
		uint32_t p0;

		if (x > mx)
		{
			rdata->fill(thiz->rrend, x, y, 1, &p0);
		}
		else if (x < mx)
		{
			ldata->fill(thiz->lrend, x, y, 1, &p0);
		}
		else
		{
			uint32_t p1;
			uint16_t a;

			a = 1 + ((mmx & 0xffff) >> 8);

			ldata->fill(thiz->lrend, x, y, 1, &p0);
			rdata->fill(thiz->rrend, 0, y, 1, &p1);
			p0 = argb8888_interp_256(a, p0, p1);
		}
		*dst++ = p0;
		x++;
	}
}

static void _affine_good(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Hswitch *thiz;
	Enesim_Renderer_Sw_Data *ldata;
	Enesim_Renderer_Sw_Data *rdata;
	Eina_F16p16 mmx, xx, yy;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	int mx;

	thiz = _hswitch_get(r);
	enesim_renderer_affine_setup(r, x, y, &thiz->matrix, &xx, &yy);

	/* FIXME put this on the state setup */
	mmx = eina_f16p16_double_from(thiz->w - (double)(thiz->w * thiz->step));
	mx = eina_f16p16_int_to(mmx);

	ldata = enesim_renderer_backend_data_get(thiz->lrend, ENESIM_BACKEND_SOFTWARE);
	rdata = enesim_renderer_backend_data_get(thiz->rrend, ENESIM_BACKEND_SOFTWARE);
	while (dst < end)
	{
		uint32_t p0;

		x = eina_f16p16_int_to(xx);
		y = eina_f16p16_int_to(yy);
		if (x > mx)
		{
			rdata->fill(thiz->rrend, x, y, 1, &p0);
		}
		else if (x < mx)
		{
			ldata->fill(thiz->lrend, x, y, 1, &p0);
		}
		/* FIXME, what should we use here? mmx or xx?
		 * or better use a subpixel center?
		 */
		else
		{
			uint32_t p1;
			uint16_t a;

			a = 1 + ((xx & 0xffff) >> 8);
			ldata->fill(thiz->lrend, x, y, 1, &p0);
			rdata->fill(thiz->rrend, 0, y, 1, &p1);
			p0 = argb8888_interp_256(a, p1, p0);
		}
		*dst++ = p0;
		xx += thiz->matrix.xx;
		yy += thiz->matrix.yx;
	}
}

static void _generic_fast(Enesim_Renderer *r, int x, int y, unsigned int len, uint32_t *dst)
{
	Hswitch *thiz;
	Enesim_Renderer_Sw_Data *ldata;
	Enesim_Renderer_Sw_Data *rdata;
	Eina_Rectangle ir, dr;
	int mx;

	thiz = _hswitch_get(r);
	eina_rectangle_coords_from(&ir, x, y, len, 1);
	eina_rectangle_coords_from(&dr, 0, 0, thiz->w, thiz->h);
	if (!eina_rectangle_intersection(&ir, &dr))
		return;

	ldata = enesim_renderer_backend_data_get(thiz->lrend, ENESIM_BACKEND_SOFTWARE);
	rdata = enesim_renderer_backend_data_get(thiz->rrend, ENESIM_BACKEND_SOFTWARE);
	mx = (int)(thiz->w - (thiz->w * thiz->step));
	if (mx == 0)
	{
		rdata->fill(thiz->rrend, ir.x, ir.y, ir.w, dst);
	}
	else if (mx == thiz->w)
	{
		ldata->fill(thiz->lrend, ir.x, ir.y, ir.w, dst);
	}
	else
	{
		if (ir.x > mx)
		{
			rdata->fill(thiz->rrend, ir.x, ir.y, ir.w, dst);
		}
		else if (ir.x + ir.w < mx)
		{
			ldata->fill(thiz->lrend, ir.x, ir.y, ir.w, dst);
		}
		else
		{
			int w;

			w = mx - ir.x;
			ldata->fill(thiz->lrend, ir.x, ir.y, w, dst);
			dst += w;
			rdata->fill(thiz->rrend, 0, ir.y, ir.w + ir.x - mx , dst);
		}
	}
}

static const char * _hswitch_name(Enesim_Renderer *r)
{
	return "hswitch";
}

static Eina_Bool _state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Hswitch *thiz;

	thiz = _hswitch_get(r);
	if (!thiz->lrend || !thiz->rrend)
		return EINA_FALSE;
	if (!enesim_renderer_sw_setup(thiz->lrend, state, s, error))
		return EINA_FALSE;
	if (!enesim_renderer_sw_setup(thiz->rrend, state, s, error))
		return EINA_FALSE;

	enesim_matrix_f16p16_matrix_to(&state->transformation,
			&thiz->matrix);

	*fill = _affine_good;

	return EINA_TRUE;
}

static void _free(Enesim_Renderer *r)
{
	Hswitch *thiz;

	thiz = _hswitch_get(r);
	free(thiz);
}


static Enesim_Renderer_Descriptor _descriptor = {
	/* .version =    */ ENESIM_RENDERER_API,
	/* .name =       */ _hswitch_name,
	/* .free =       */ _free,
	/* .boundings =  */ NULL,
	/* .flags =      */ NULL,
	/* .is_inside =  */ NULL,
	/* .damage =     */ NULL,
	/* .sw_setup =   */ _state_setup,
	/* .sw_cleanup = */ NULL,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a new horizontal switch renderer
 *
 * A horizontal switch renderer renders an horizontal translation between
 * the left and right renderers based on the step value.
 * @return The renderer
 */
EAPI Enesim_Renderer * enesim_renderer_hswitch_new(void)
{
	Hswitch *thiz;
	Enesim_Renderer *r;

	thiz = calloc(1, sizeof(Hswitch));
	r = enesim_renderer_new(&_descriptor, thiz);

	return r;
}
/**
 * Sets the width of the renderer window
 * @param[in] r The horizontal switch renderer
 * @param[in] w The width
 */
EAPI void enesim_renderer_hswitch_w_set(Enesim_Renderer *r, int w)
{
	Hswitch *thiz;

	thiz = _hswitch_get(r);
	if (thiz->w == w)
		return;
	thiz->w = w;
}
/**
 * Sets the height of the renderer window
 * @param[in] r The horizontal switch renderer
 * @param[in] h The height
 */
EAPI void enesim_renderer_hswitch_h_set(Enesim_Renderer *r, int h)
{
	Hswitch *thiz;

	thiz = _hswitch_get(r);
	if (thiz->h == h)
		return;
	thiz->h = h;
}
/**
 * Sets the left renderer
 * @param[in] r The horizontal switch renderer
 * @param[in] left The left renderer
 */
EAPI void enesim_renderer_hswitch_left_set(Enesim_Renderer *r,
		Enesim_Renderer *left)
{
	Hswitch *thiz;

	thiz = _hswitch_get(r);
	thiz->lrend = left;
}
/**
 * Sets the right renderer
 * @param[in] r The horizontal switch renderer
 * @param[in] right The right renderer
 */
EAPI void enesim_renderer_hswitch_right_set(Enesim_Renderer *r,
		Enesim_Renderer *right)
{
	Hswitch *thiz;

	thiz = _hswitch_get(r);
	thiz->rrend = right;
}
/**
 * Sets the step
 * @param[in] r The horizontal switch renderer
 * @param[in] step The step. A value of 0 will render the left
 * renderer, a value of 1 will render the right renderer
 */
EAPI void enesim_renderer_hswitch_step_set(Enesim_Renderer *r, double step)
{
	Hswitch *thiz;

	if (step < 0)
		step = 0;
	else if (step > 1)
		step = 1;
	thiz = _hswitch_get(r);
	thiz->step = step;
}
