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
typedef struct _Enesim_Renderer_Background {
	/* the properties */
	Enesim_Color color;
	/* generated at state setup */
	Enesim_Color final_color;
	Enesim_Compositor_Span span;
} Enesim_Renderer_Background;

static inline Enesim_Renderer_Background * _background_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Background *thiz;

	thiz = enesim_renderer_data_get(r);
	return thiz;
}
static void _span(Enesim_Renderer *p, int x, int y,
		unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Background *thiz = _background_get(p);

	thiz->span(dst, len, NULL, thiz->final_color, NULL);
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static Eina_Bool _background_state_setup(Enesim_Renderer *r, Enesim_Renderer_Sw_Fill *fill)
{
	Enesim_Renderer_Background *thiz;
	Enesim_Format fmt = ENESIM_FORMAT_ARGB8888;
	Enesim_Rop rop;
	Enesim_Color final_color, rend_color;

 	thiz = _background_get(r);
	final_color = thiz->color;
	enesim_renderer_color_get(r, &rend_color);
	/* TODO multiply the bkg color with the rend color and use that for the span
	 */
	enesim_renderer_rop_get(r, &rop);
	thiz->final_color = final_color;
	thiz->span = enesim_compositor_span_get(rop, &fmt, ENESIM_FORMAT_NONE,
			final_color, ENESIM_FORMAT_NONE);
	*fill = _span;
	return EINA_TRUE;
}

static void _background_state_cleanup(Enesim_Renderer *r)
{
}


static void _background_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Renderer_Background *thiz;

	thiz = _background_get(r);
	if (!thiz)
	{
		*flags = 0;
		return;
	}

	*flags = ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_PROJECTIVE |
			ENESIM_RENDERER_FLAG_ARGB8888 |
			ENESIM_RENDERER_FLAG_ROP;

}

static void _background_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Background *thiz;

	thiz = _background_get(r);
	free(thiz);
}

static Enesim_Renderer_Descriptor _descriptor = {
	.sw_setup = _background_state_setup,
	.sw_cleanup = _background_state_cleanup,
	.flags = _background_flags,
	.free = _background_free,
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a background renderer
 * @return The new renderer
 */
EAPI Enesim_Renderer * enesim_renderer_background_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Background *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Background));
	if (!thiz) return NULL;
	r = enesim_renderer_new(&_descriptor, thiz);
	return r;
}
/**
 * Sets the color of background
 * @param[in] r The background renderer
 * @param[in] color The background color
 */
EAPI void enesim_renderer_background_color_set(Enesim_Renderer *r,
		Enesim_Color color)
{
	Enesim_Renderer_Background *thiz;

	thiz = _background_get(r);
	thiz->color = color;
}

