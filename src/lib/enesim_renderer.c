/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2008 Jorge Luis Zapata
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
#define ENESIM_MAGIC_RENDERER 0xe7e51402
#define ENESIM_MAGIC_CHECK_RENDERER(d)\
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_MAGIC_RENDERER))\
			EINA_MAGIC_FAIL(d, ENESIM_MAGIC_RENDERER);\
	} while(0)
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_renderer_init(Enesim_Renderer *r)
{
	EINA_MAGIC_SET(r, ENESIM_MAGIC_RENDERER);
	/* common properties */
	r->ox = 0;
	r->oy = 0;
	r->color = ENESIM_COLOR_FULL;
	enesim_f16p16_matrix_identity(&r->matrix.values);
	enesim_matrix_identity(&r->matrix.original);
}

void enesim_renderer_relative_set(Enesim_Renderer *r, Enesim_Renderer *rel,
		Enesim_Matrix *old_matrix, double *old_ox, double *old_oy)
{
	Enesim_Matrix rel_matrix, r_matrix;
	double r_ox, r_oy;
	double nox, noy;

	if (!rel) return;

	/* TODO should we use the f16p16 matrix? */
	/* multiply the matrix by the current transformation */
	enesim_renderer_transformation_get(r, &r_matrix);
	enesim_renderer_transformation_get(rel, old_matrix);
	enesim_matrix_compose(old_matrix, &r_matrix, &rel_matrix);
	enesim_renderer_transformation_set(rel, &rel_matrix);
	/* add the origin by the current origin */
	enesim_renderer_origin_get(rel, old_ox, old_oy);
	enesim_renderer_origin_get(r, &r_ox, &r_oy);
	enesim_matrix_point_transform(old_matrix, *old_ox + r_ox, *old_oy + r_oy, &nox, &noy);
	enesim_renderer_origin_set(rel, nox, noy);
	//printf("setting origin %p %g %g to %g %g (%g %g)\n", rel, *old_ox, *old_oy, nox + r_ox, noy + r_oy, nox, noy);
}

void enesim_renderer_relative_unset(Enesim_Renderer *r, Enesim_Renderer *rel,
		Enesim_Matrix *old_matrix, double old_ox, double old_oy)
{
	if (!rel) return;

	/* restore origin */
	enesim_renderer_origin_set(rel, old_ox, old_oy);
	/* restore original matrix */
	enesim_renderer_transformation_set(rel, old_matrix);
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 *
 */
EAPI Enesim_Renderer * enesim_renderer_new(Enesim_Renderer_Descriptor
		*descriptor, void *data)
{
	Enesim_Renderer *r;

	r = malloc(sizeof(Enesim_Renderer));
	enesim_renderer_init(r);
	r->sw_setup = descriptor->sw_setup;
	r->sw_cleanup = descriptor->sw_cleanup;
	r->free = descriptor->free;
	r->data = data;

	return r;
}
/**
 *
 */
EAPI Eina_Bool enesim_renderer_sw_setup(Enesim_Renderer *r)
{
	Enesim_Renderer_Sw_Fill fill;
	Eina_Bool ret;

	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (!r->sw_setup) return EINA_TRUE;
	if (r->sw_setup(r, &fill))
	{
		r->sw_fill = fill;
		return EINA_TRUE;
	}
	return EINA_FALSE;
}
/**
 *
 */
EAPI void enesim_renderer_sw_cleanup(Enesim_Renderer *r)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (r->sw_cleanup) r->sw_cleanup(r);
}
/**
 *
 */
EAPI Enesim_Renderer_Sw_Fill enesim_renderer_sw_fill_get(Enesim_Renderer *r)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	return r->sw_fill;
}
/**
 *
 */
EAPI void * enesim_renderer_data_get(Enesim_Renderer *r)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);

	return r->data;
}

/**
 * Sets the transformation matrix of a renderer
 * @param[in] r The renderer to set the transformation matrix on
 * @param[in] m The transformation matrix to set
 */
EAPI void enesim_renderer_transformation_set(Enesim_Renderer *r, Enesim_Matrix *m)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);

	if (!m)
	{
		enesim_f16p16_matrix_identity(&r->matrix.values);
		enesim_matrix_identity(&r->matrix.original);
		r->matrix.type = ENESIM_MATRIX_IDENTITY;
		return;
	}
	r->matrix.original = *m;
	enesim_matrix_f16p16_matrix_to(m, &r->matrix.values);
	r->matrix.type = enesim_f16p16_matrix_type_get(&r->matrix.values);
}

EAPI void enesim_renderer_transformation_get(Enesim_Renderer *r, Enesim_Matrix *m)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);

	if (m) *m = r->matrix.original;
}
/**
 * Deletes a renderer
 * @param[in] r The renderer to delete
 */
EAPI void enesim_renderer_delete(Enesim_Renderer *r)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	//enesim_renderer_sw_cleanup(r);
	if (r->free)
		r->free(r);
	free(r);
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_origin_set(Enesim_Renderer *r, double x, double y)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	r->ox = x;
	r->oy = y;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_origin_get(Enesim_Renderer *r, double *x, double *y)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (x) *x = r->ox;
	if (y) *y = r->oy;
}
/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	r->color = color;
}
/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Color enesim_renderer_color_get(Enesim_Renderer *r)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	return r->color;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_boundings(Enesim_Renderer *r, Eina_Rectangle *rect)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (rect && r->boundings) r->boundings(r, rect);
}

/**
 * To be documented
 * FIXME: To be fixed
 * What about the mask?
 */
EAPI void enesim_renderer_surface_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, Eina_Rectangle *clip,
		int x, int y)
{
	Enesim_Compositor_Span span;
	Enesim_Renderer_Sw_Fill fill;
	int cx = 0, cy = 0, ch, cw;
	uint32_t *ddata;
	int stride;
	Enesim_Format dfmt;

	ENESIM_MAGIC_CHECK_RENDERER(r);
	ENESIM_MAGIC_CHECK_SURFACE(s);

	if (!enesim_renderer_sw_setup(r)) return;
	fill = enesim_renderer_sw_fill_get(r);
	if (!fill) return;

	if (!clip)
		enesim_surface_size_get(s, &cw, &ch);
	else
	{
		cx = clip->x;
		cy = clip->y;
		cw = clip->w;
		ch = clip->h;
	}

	dfmt = enesim_surface_format_get(s);
	ddata = enesim_surface_data_get(s);
	stride = enesim_surface_stride_get(s);
	ddata = ddata + (cy * stride) + cx;

	/* translate the origin */
	cx -= x;
	cy -= y;
	/* fill the new span */
	if ((rop == ENESIM_FILL) && (r->color == ENESIM_COLOR_FULL))
	{
		while (ch--)
		{
			fill(r, cx, cy, cw, ddata);
			cy++;
			ddata += stride;
		}
	}
	else
	{
		uint32_t *fdata;

		span = enesim_compositor_span_get(rop, &dfmt, ENESIM_FORMAT_ARGB8888,
				r->color, ENESIM_FORMAT_NONE);

		fdata = alloca(cw * sizeof(uint32_t));
		while (ch--)
		{
			fill(r, cx, cy, cw, fdata);
			cy++;
			/* compose the filled and the destination spans */
			span(ddata, cw, fdata, r->color, NULL);
			ddata += stride;
		}
	}
	/* TODO set the format again */
	enesim_renderer_sw_cleanup(r);
}
