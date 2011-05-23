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
typedef struct _Background Enesim_Renderer_Clipper;

struct _Background {
	/* the properties */
	Enesim_Renderer *content;
	double width;
	double height;
	/* the content properties */
	Enesim_Rop old_rop;
	Enesim_Matrix old_matrix;
	double old_x;
	double old_y;
	/* generated at state setup */
	Enesim_Renderer_Sw_Fill content_fill;
};

static inline Enesim_Renderer_Clipper * _clipper_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = enesim_renderer_data_get(r);
	return thiz;
}

static void _span(Enesim_Renderer *r, int x, int y,
		unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Clipper *thiz;
	Enesim_Matrix_Type matrix_type;

 	thiz = _clipper_get(r);
	thiz->content_fill(thiz->content, x, y, len, dst);
}

static _content_cleanup(Enesim_Renderer_Clipper *thiz)
{
	enesim_renderer_sw_cleanup(thiz->content);
	enesim_renderer_origin_set(thiz->content, thiz->old_x, thiz->old_y);
	enesim_renderer_transformation_set(thiz->content, &thiz->old_matrix);
	/* FIXME add the rop */
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static Eina_Bool _clipper_state_setup(Enesim_Renderer *r, Enesim_Renderer_Sw_Fill *fill)
{
	Enesim_Renderer_Clipper *thiz;
	Enesim_Matrix matrix;
	double ox, oy;

 	thiz = _clipper_get(r);
	if (!thiz->content) return EINA_FALSE;

	enesim_renderer_origin_get(r, &ox, &oy);
	enesim_renderer_origin_get(thiz->content, &thiz->old_x, &thiz->old_y);
	enesim_renderer_origin_set(thiz->content, ox, oy);
	enesim_renderer_transformation_get(r, &matrix);
	enesim_renderer_transformation_get(thiz->content, &thiz->old_matrix);
	enesim_renderer_transformation_set(thiz->content, &matrix);
	/* FIXME add the rop */
	if (!enesim_renderer_sw_setup(thiz->content))
	{
		/* restore the values */
		_content_cleanup(thiz);
		return EINA_FALSE;
	}
	*fill = _span;
	thiz->content_fill = enesim_renderer_sw_fill_get(thiz->content);
	return EINA_TRUE;
}

static void _clipper_state_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Clipper *thiz;

 	thiz = _clipper_get(r);
	if (!thiz->content) return;

	_content_cleanup(thiz);
}


static void _clipper_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = _clipper_get(r);
	if (!thiz || !thiz->content)
	{
		*flags = 0;
		return;
	}

	enesim_renderer_flags(thiz->content, flags);
}

static void _clipper_boundings(Enesim_Renderer *r, Enesim_Rectangle *rect)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = _clipper_get(r);
	rect->x = 0;
	rect->y = 0;
	rect->w = thiz->width;
	rect->h = thiz->height;
}

static void _clipper_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = _clipper_get(r);
	free(thiz);
}

static Enesim_Renderer_Descriptor _descriptor = {
	/* .sw_setup =   */ _clipper_state_setup,
	/* .sw_cleanup = */ _clipper_state_cleanup,
	/* .free =       */ _clipper_free,
	/* .boundings =  */ _clipper_boundings,
	/* .flags =      */ _clipper_flags,
	/* .is_inside =  */ 0
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a clipper renderer
 * @return The new renderer
 */
EAPI Enesim_Renderer * enesim_renderer_clipper_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Clipper *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Clipper));
	if (!thiz) return NULL;
	r = enesim_renderer_new(&_descriptor, thiz);
	return r;
}

EAPI void enesim_renderer_clipper_content_set(Enesim_Renderer *r,
		Enesim_Renderer *content)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = _clipper_get(r);
	thiz->content = content;
}

EAPI void enesim_renderer_clipper_content_get(Enesim_Renderer *r,
		Enesim_Renderer **content)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = _clipper_get(r);
	*content = thiz->content;
}

EAPI void enesim_renderer_clipper_width_set(Enesim_Renderer *r,
		double width)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = _clipper_get(r);
	thiz->width = width;

}

EAPI void enesim_renderer_clipper_width_get(Enesim_Renderer *r,
		double *width)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = _clipper_get(r);
	*width = thiz->width;
}

EAPI void enesim_renderer_clipper_height_set(Enesim_Renderer *r,
		double height)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = _clipper_get(r);
	thiz->height = height;

}

EAPI void enesim_renderer_clipper_height_get(Enesim_Renderer *r,
		double *height)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = _clipper_get(r);
	*height = thiz->height;
}

