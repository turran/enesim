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

static Eina_Bool _background_state_setup(Enesim_Renderer_Background *thiz, Enesim_Renderer *r)
{
	Enesim_Color final_color, rend_color;

 	thiz = _background_get(r);
	final_color = thiz->color;
	enesim_renderer_color_get(r, &rend_color);
	/* TODO multiply the bkg color with the rend color and use that for the span
	 */
	thiz->final_color = final_color;
	return EINA_TRUE;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _background_name(Enesim_Renderer *r)
{
	return "background";
}

static Eina_Bool _background_sw_setup(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Background *thiz;
	Enesim_Format fmt = ENESIM_FORMAT_ARGB8888;
	Enesim_Rop rop;

 	thiz = _background_get(r);

	if (!_background_state_setup(thiz, r)) return EINA_FALSE;
	enesim_renderer_rop_get(r, &rop);
	thiz->span = enesim_compositor_span_get(rop, &fmt, ENESIM_FORMAT_NONE,
			thiz->final_color, ENESIM_FORMAT_NONE);
	*fill = _span;

	return EINA_TRUE;
}

#if BUILD_OPENCL
static Eina_Bool _background_opencl_setup(Enesim_Renderer *r, Enesim_Surface *s,
		const char **program_name, const char **program_source,
		size_t *program_length, Enesim_Error **error)
{
	Enesim_Renderer_Background *thiz;

 	thiz = _background_get(r);
	if (!_background_state_setup(thiz, r)) return EINA_FALSE;

	*program_name = "background";
	*program_source =
	#include "enesim_renderer_background.cl"
	*program_length = strlen(*program_source);

	return EINA_TRUE;
}

static Eina_Bool _background_opencl_kernel_setup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Background *thiz;
	Enesim_Renderer_OpenCL_Data *rdata;

 	thiz = _background_get(r);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENCL);
	clSetKernelArg(rdata->kernel, 1, sizeof(cl_uchar4), &thiz->final_color);

	return EINA_TRUE;
}
#endif

static void _background_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Renderer_Background *thiz;

	thiz = _background_get(r);
	if (!thiz)
	{
		*flags = 0;
		return;
	}

	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_AFFINE |
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
	/* .version =               */ ENESIM_RENDERER_API,
	/* .name =                  */ _background_name,
	/* .free =                  */ _background_free,
	/* .boundings =             */ NULL,
	/* .flags =                 */ _background_flags,
	/* .is_inside =             */ NULL,
	/* .sw_setup =              */ _background_sw_setup,
	/* .sw_cleanup =            */ NULL,
#if BUILD_OPENCL
	/* .opencl_setup =          */ _background_opencl_setup,
	/* .opencl_kernel_setup =   */ _background_opencl_kernel_setup,
	/* .opencl_cleanup =        */ NULL,
#endif
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
 * Sets the color of the background
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

