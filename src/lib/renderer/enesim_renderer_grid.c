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
#include "enesim_compositor.h"
#include "enesim_renderer.h"
#include "enesim_renderer_grid.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_coord_private.h"
#include "enesim_renderer_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_GRID(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Grid,					\
		enesim_renderer_grid_descriptor_get())

typedef struct _Enesim_Renderer_Grid
{
	Enesim_Renderer parent;
	struct {
		Enesim_Color color;
		unsigned int w;
		unsigned int h;
	} inside, outside;
	/* the state */
	double ox;
	double oy;
	Enesim_F16p16_Matrix matrix;
	int wt;
	int ht;
	Eina_F16p16 wi;
	Eina_F16p16 hi;
	Eina_F16p16 wwt;
	Eina_F16p16 hht;
} Enesim_Renderer_Grid;

typedef struct _Enesim_Renderer_Grid_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Grid_Class;

static inline uint32_t _grid(Enesim_Renderer_Grid *thiz, Eina_F16p16 yy, Eina_F16p16 xx)
{
	Eina_F16p16 syy;
	int sy;
	uint32_t p0;

	/* normalize the modulo */
	syy = (yy % thiz->hht);
	if (syy < 0)
		syy += thiz->hht;
	/* simplest case, we are on the grid border
	 * the whole line will be outside color */
	sy = eina_f16p16_int_to(syy);
	if (syy >= thiz->hi)
	{
		p0 = thiz->outside.color;
	}
	else
	{
		Eina_F16p16 sxx;
		int sx;

		/* normalize the modulo */
		sxx = (xx % thiz->wwt);
		if (sxx < 0)
			sxx += thiz->wwt;

		sx = eina_f16p16_int_to(sxx);
		if (sxx >= thiz->wi)
		{
			p0 = thiz->outside.color;
		}
		/* totally inside */
		else
		{
			p0 = thiz->inside.color;
			/* antialias the inner square */
			if (sx == 0)
			{
				uint16_t a;

				a = 1 + ((sxx & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, p0, thiz->outside.color);
			}
			else if (sx == (thiz->inside.w - 1))
			{
				uint16_t a;

				a = 1 + ((sxx & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, thiz->outside.color, p0);

			}
			if (sy == 0)
			{
				uint16_t a;

				a = 1 + ((syy & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, p0, thiz->outside.color);
			}
			else if (sy == (thiz->inside.h - 1))
			{
				uint16_t a;

				a = 1 + ((syy & 0xffff) >> 8);
				p0 = argb8888_interp_256(a, thiz->outside.color, p0);
			}
		}
	}
	return p0;
}


static void _span_identity(Enesim_Renderer *r,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Grid *thiz;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	int sy;
	Eina_F16p16 yy, xx;

	thiz = ENESIM_RENDERER_GRID(r);
#if 0
	enesim_coord_identity_setup(r, x, y, &xx, &yy);
	while (dst < end)
	{
		uint32_t p0;

		p0 = _grid(thiz, yy, xx);

		xx += EINA_F16P16_ONE;
		*dst++ = p0;
	}
#else
	sy = (y % thiz->ht);
	if (sy < 0)
	{
		sy += thiz->ht;
	}
	/* simplest case, all the span is outside */
	if (sy >= thiz->inside.h)
	{
		while (dst < end)
			*dst++ = thiz->outside.color;
	}
	/* we swap between the two */
	else
	{
		while (dst < end)
		{
			int sx;

			sx = (x % thiz->wt);
			if (sx < 0)
				sx += thiz->wt;

			if (sx >= thiz->inside.w)
				*dst = thiz->outside.color;
			else
				*dst = thiz->inside.color;

			dst++;
			x++;
		}
	}
#endif
}

static void _span_affine(Enesim_Renderer *r,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Grid *thiz;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	Eina_F16p16 yy, xx;

	thiz = ENESIM_RENDERER_GRID(r);
	enesim_coord_affine_setup(&xx, &yy, x, y, thiz->ox, thiz->oy, &thiz->matrix);

	while (dst < end)
	{
		uint32_t p0;

		p0 = _grid(thiz, yy, xx);

		yy += thiz->matrix.yx;
		xx += thiz->matrix.xx;
		*dst++ = p0;
	}
}

static void _span_projective(Enesim_Renderer *r,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Grid *thiz;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	Eina_F16p16 yy, xx, zz;

	thiz = ENESIM_RENDERER_GRID(r);
	enesim_coord_projective_setup(&xx, &yy, &zz, x, y, thiz->ox, thiz->oy, &thiz->matrix);

	while (dst < end)
	{
		Eina_F16p16 syy, sxx;
		uint32_t p0;

		syy = ((((int64_t)yy) << 16) / zz);
		sxx = ((((int64_t)xx) << 16) / zz);

		p0 = _grid(thiz, syy, sxx);

		yy += thiz->matrix.yx;
		xx += thiz->matrix.xx;
		zz += thiz->matrix.zx;
		*dst++ = p0;
	}
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _grid_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "grid";
}

static void _grid_sw_cleanup(Enesim_Renderer *r EINA_UNUSED, Enesim_Surface *s EINA_UNUSED)
{

}

static Eina_Bool _grid_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Grid *thiz;
	Enesim_Matrix m;
	Enesim_Matrix inv;
	Enesim_Matrix_Type type;

	thiz = ENESIM_RENDERER_GRID(r);
	if (!thiz->inside.w || !thiz->inside.h || !thiz->outside.w || !thiz->outside.h)
		return EINA_FALSE;

	thiz->ht = thiz->inside.h + thiz->outside.h;
	thiz->wt = thiz->inside.w + thiz->outside.w;
	thiz->hht = eina_f16p16_int_from(thiz->ht);
	thiz->wwt = eina_f16p16_int_from(thiz->wt);

	enesim_renderer_origin_get(r, &thiz->ox, &thiz->oy);
	enesim_renderer_transformation_get(r, &m);
	enesim_matrix_inverse(&m, &inv);
	enesim_renderer_transformation_type_get(r, &type);
	switch (type)
	{
		case ENESIM_MATRIX_IDENTITY:
		*fill = _span_identity;
		break;

		case ENESIM_MATRIX_AFFINE:
		enesim_matrix_f16p16_matrix_to(&inv, &thiz->matrix);
		*fill = _span_affine;
		break;

		case ENESIM_MATRIX_PROJECTIVE:
		enesim_matrix_f16p16_matrix_to(&inv, &thiz->matrix);
		*fill = _span_projective;
		break;

		default:
		return EINA_FALSE;

	}
	return EINA_TRUE;
}

static void _grid_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_PROJECTIVE |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Grid, Enesim_Renderer_Grid_Class,
		enesim_renderer_grid);

static void _enesim_renderer_grid_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _grid_name;
	klass->features_get = _grid_features_get;
	klass->sw_setup = _grid_sw_setup;
	klass->sw_cleanup = _grid_sw_cleanup;
}

static void _enesim_renderer_grid_instance_init(void *o)
{
	Enesim_Renderer_Grid *thiz = ENESIM_RENDERER_GRID(o);
	/* specific renderer setup */
	thiz->inside.w = 1;
	thiz->inside.h = 1;
	thiz->outside.w = 1;
	thiz->outside.h = 1;
}

static void _enesim_renderer_grid_instance_deinit(void *o EINA_UNUSED)
{
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a new grid renderer
 *
 * A grid renderer is composed of an inside box and an outside stroke.
 * Both, the inside and outside elements can be configurable through the
 * color, width and height.
 * @return The renderer
 */
EAPI Enesim_Renderer * enesim_renderer_grid_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_grid);
	return r;
}
/**
 * Sets the width of the inner box of a grid renderer
 * @param[in] r The grid renderer
 * @param[in] width The width
 */
EAPI void enesim_renderer_grid_inside_width_set(Enesim_Renderer *r, unsigned int width)
{
	Enesim_Renderer_Grid *thiz;


	thiz = ENESIM_RENDERER_GRID(r);
	thiz->inside.w = width;
	thiz->wi = eina_f16p16_int_from(width);
}
/**
 * Gets the width of the inner box of a grid renderer
 * @param[in] r The grid renderer
 * @return The width
 */
EAPI void enesim_renderer_grid_inside_width_get(Enesim_Renderer *r, unsigned int *iw)
{
	Enesim_Renderer_Grid *thiz;

	thiz = ENESIM_RENDERER_GRID(r);
	if (iw) *iw = thiz->inside.w;
}
/**
 * Sets the height of the inner box of a grid renderer
 * @param[in] r The grid renderer
 * @param[in] height The height
 */
EAPI void enesim_renderer_grid_inside_height_set(Enesim_Renderer *r, unsigned int height)
{
	Enesim_Renderer_Grid *thiz;

	thiz = ENESIM_RENDERER_GRID(r);
	thiz->inside.h = height;
	thiz->hi = eina_f16p16_int_from(height);
}
/**
 * Gets the height of the inner box of a grid renderer
 * @param[in] r The grid renderer
 * @return The height
 */
EAPI void enesim_renderer_grid_inside_height_get(Enesim_Renderer *r, unsigned int *ih)
{
	Enesim_Renderer_Grid *thiz;

	thiz = ENESIM_RENDERER_GRID(r);
	if (ih) *ih = thiz->inside.h;
}
/**
 * Sers the color of the inner box of a grid renderer
 * @param[in] r The grid renderer
 * @param[in] color The color
 */
EAPI void enesim_renderer_grid_inside_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Grid *thiz;

	thiz = ENESIM_RENDERER_GRID(r);
	thiz->inside.color = color;
}
/**
 * Gets the color of the inner box of a grid renderer
 * @param[in] r The grid renderer
 * @return The color
 */
EAPI void enesim_renderer_grid_inside_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Enesim_Renderer_Grid *thiz;

	thiz = ENESIM_RENDERER_GRID(r);
	if (color) *color = thiz->inside.color;
}
/**
 * Sets the horizontal thickness of the border of a grid renderer
 * @param[in] r The grid renderer
 * @param[in] hthickness The horizontal thickness
 */
EAPI void enesim_renderer_grid_border_hthickness_set(Enesim_Renderer *r, unsigned int hthickness)
{
	Enesim_Renderer_Grid *thiz;

	thiz = ENESIM_RENDERER_GRID(r);
	thiz->outside.h = hthickness;
}
/**
 * Gets the horizontal thickness of the border of a grid renderer
 * @param[in] r The grid renderer
 * @return The horizontal thickness
 */
EAPI void enesim_renderer_grid_border_hthickness_get(Enesim_Renderer *r, unsigned int *h)
{
	Enesim_Renderer_Grid *thiz;

	thiz = ENESIM_RENDERER_GRID(r);
	if (h) *h = thiz->outside.h;
}
/**
 * Sets the vertical thickness of the border of a grid renderer
 * @param[in] r The grid renderer
 * @param[in] vthickness The vertical thickness
 */
EAPI void enesim_renderer_grid_border_vthickness_set(Enesim_Renderer *r, unsigned int vthickness)
{
	Enesim_Renderer_Grid *thiz;

	thiz = ENESIM_RENDERER_GRID(r);
	thiz->outside.w = vthickness;
}
/**
 * Gets the vertical thickness of the border of a grid renderer
 * @param[in] r The grid renderer
 * @return The vertical thickness
 */
EAPI void enesim_renderer_grid_border_vthickness_get(Enesim_Renderer *r, unsigned int *v)
{
	Enesim_Renderer_Grid *thiz;

	thiz = ENESIM_RENDERER_GRID(r);
	if (v) *v = thiz->outside.w;
}
/**
 * Sets the color of the border of a grid renderer
 * @param[in] r The grid renderer
 * @param[in] color The color
 */
EAPI void enesim_renderer_grid_border_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	Enesim_Renderer_Grid *thiz;

	thiz = ENESIM_RENDERER_GRID(r);
	thiz->outside.color = color;
}
/**
 * Gets the color of the border of a grid renderer
 * @param[in] r The grid renderer
 * @return The color
 */
EAPI void enesim_renderer_grid_border_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	Enesim_Renderer_Grid *thiz;

	thiz = ENESIM_RENDERER_GRID(r);
	if (color) *color = thiz->outside.color;
}

