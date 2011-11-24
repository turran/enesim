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
typedef struct _Enesim_Renderer_Line_State
{
	double x0;
	double y0;
	double x1;
	double y1;
} Enesim_Renderer_Line_State;

typedef struct _Enesim_Renderer_Line
{
	/* properties */
	Enesim_Renderer_Line_State current;
	/* private */
	Enesim_Renderer_Line_State past;
	Eina_Bool changed : 1;
	Enesim_F16p16_Matrix matrix;
	Eina_F16p16 a01, b01, c01;
	Eina_F16p16 a01_np, b01_np, c01_np;
	Eina_F16p16 a01_nm, b01_nm, c01_nm;
	Eina_F16p16 rr;
	Eina_F16p16 lxx, rxx, tyy, byy;
} Enesim_Renderer_Line;

static inline Enesim_Renderer_Line * _line_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Line *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	return thiz;
}

static void _span(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _line_name(Enesim_Renderer *r)
{
	return "line";
}

static Eina_Bool _line_state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Line *thiz;
	Eina_F16p16 x01_fp, y01_fp;
	double x0, x1, y0, y1;
	double x01, y01;
	double len;

	thiz = _line_get(r);

	x0 = thiz->current.x0;
	x1 = thiz->current.x1;
	y0 = thiz->current.y0;
	y1 = thiz->current.y1;

	if (y1 < y0)
	{
		thiz->byy = eina_f16p16_double_from(y0);
		thiz->tyy = eina_f16p16_double_from(y1);
	}
	else
	{
		thiz->byy = eina_f16p16_double_from(y1);
		thiz->tyy = eina_f16p16_double_from(y0);
	}

	if (x1 < x0)
	{
		thiz->lxx = eina_f16p16_double_from(x0);
		thiz->rxx = eina_f16p16_double_from(x1);
	}
	else
	{
		thiz->lxx = eina_f16p16_double_from(x1);
		thiz->rxx = eina_f16p16_double_from(x0);
	}

	x01 = x1 - x0;
	y01 = y1 - y0;

	if ((len = hypot(x01, y01)) < 1)
		return EINA_FALSE;
#if 0
   /* normalize to line length so that aa works well */
   o->a01 = -(y01 * 65536) / len;
   o->b01 = (x01 * 65536) / len;
   o->c01 = (65536 * ((y1 * x0) - (x1 * y0))) / len;

   o->a01_np = (x01 * 65536) / len;
   o->b01_np = (y01 * 65536) / len;
   o->c01_np = -(65536 * ((y0 * y01) + (x0 * x01))) / len;

   o->a01_nm = -(x01 * 65536) / len;
   o->b01_nm = -(y01 * 65536) / len;
   o->c01_nm = (65536 * ((y1 * y01) + (x1 * x01))) / len;

   o->rr = 32768 * (f->stroke.weight + 1);
   if (o->rr < 32768) o->rr = 32768;

   if (f->draw_mode != DRAW_MODE_STROKE)
	return 0;
#endif
	enesim_matrix_f16p16_matrix_to(&state->transformation,
			&thiz->matrix);
	*fill = _span;

	return EINA_TRUE;
}

static void _line_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);

	enesim_renderer_shape_cleanup(r, s);
	thiz->past = thiz->current;
	thiz->changed = EINA_FALSE;
}

static void _line_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_PROJECTIVE |
			ENESIM_RENDERER_FLAG_ARGB8888;
}

static void _line_free(Enesim_Renderer *r)
{
}

static void _line_boundings(Enesim_Renderer *r, Enesim_Rectangle *boundings)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);

	boundings->x = thiz->current.x0;
	boundings->y = thiz->current.y0;
	boundings->w = fabs(thiz->current.x1 - thiz->current.x0);
	boundings->h = fabs(thiz->current.y1 - thiz->current.y0);
}

static Eina_Bool _line_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);

	if (!thiz->changed) return EINA_FALSE;

	/* x0 */
	if (thiz->current.x0 != thiz->past.x0)
		return EINA_TRUE;
	/* y0 */
	if (thiz->current.y0 != thiz->past.y0)
		return EINA_TRUE;
	/* x1 */
	if (thiz->current.x1 != thiz->past.x1)
		return EINA_TRUE;
	/* y1 */
	if (thiz->current.y1 != thiz->past.y1)
		return EINA_TRUE;

	return EINA_FALSE;
}


static Enesim_Renderer_Descriptor _line_descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _line_name,
	/* .free = 			*/ _line_free,
	/* .boundings = 		*/ _line_boundings,
	/* .destination_transform = 	*/ NULL,
	/* .flags = 			*/ _line_flags,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ _line_has_changed,
	/* .sw_setup = 			*/ _line_state_setup,
	/* .sw_cleanup = 		*/ _line_state_cleanup
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
EAPI Enesim_Renderer * enesim_renderer_line_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Line *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Line));
	if (!thiz) return NULL;
	r = enesim_renderer_shape_new(&_line_descriptor, thiz);
	return r;
}

EAPI void enesim_renderer_line_x0_set(Enesim_Renderer *r, double x0)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);
	thiz->current.x0 = x0;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_line_x0_get(Enesim_Renderer *r, double *x0)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);
	*x0 = thiz->current.x0;
}


EAPI void enesim_renderer_line_y0_set(Enesim_Renderer *r, double y0)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);
	thiz->current.y0 = y0;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_line_y0_get(Enesim_Renderer *r, double *y0)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);
	*y0 = thiz->current.y0;
}

EAPI void enesim_renderer_line_x1_set(Enesim_Renderer *r, double x1)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);
	thiz->current.x1 = x1;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_line_x1_get(Enesim_Renderer *r, double *x1)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);
	*x1 = thiz->current.x1;
}


EAPI void enesim_renderer_line_y1_set(Enesim_Renderer *r, double y1)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);
	thiz->current.y1 = y1;
	thiz->changed = EINA_TRUE;
}

EAPI void enesim_renderer_line_y1_get(Enesim_Renderer *r, double *y1)
{
	Enesim_Renderer_Line *thiz;

	thiz = _line_get(r);
	*y1 = thiz->current.y1;
}
