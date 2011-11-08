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
typedef struct _Enesim_Renderer_Pattern_State
{
	double x;
	double y;
	double width;
	double height;
} Enesim_Renderer_Pattern_State;

typedef struct _Enesim_Renderer_Pattern {
	/* the properties */
	Enesim_Renderer_Pattern_State current;
	Enesim_Renderer_Pattern_State past;
	/* generated at state setup */
	/* private */
	Eina_Bool changed : 1;
} Enesim_Renderer_Pattern;

static inline Enesim_Renderer_Pattern * _pattern_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = enesim_renderer_data_get(r);
	return thiz;
}

static void _span(Enesim_Renderer *p, int x, int y,
		unsigned int len, void *ddata)
{
	Enesim_Renderer_Pattern *thiz = _pattern_get(p);
	uint32_t *dst = ddata;

}

static Eina_Bool _pattern_state_setup(Enesim_Renderer_Pattern *thiz, Enesim_Renderer *r)
{
	Enesim_Color final_color, rend_color;

 	thiz = _pattern_get(r);
	return EINA_TRUE;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _pattern_name(Enesim_Renderer *r)
{
	return "pattern";
}

static Eina_Bool _pattern_sw_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Pattern *thiz;

 	thiz = _pattern_get(r);

	if (!_pattern_state_setup(thiz, r)) return EINA_FALSE;
	*fill = _span;

	return EINA_TRUE;
}

static void _pattern_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Pattern *thiz;

 	thiz = _pattern_get(r);
	thiz->changed = EINA_FALSE;
}

static void _pattern_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
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

static void _pattern_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	free(thiz);
}

static Eina_Bool _pattern_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Pattern *thiz;

	thiz = _pattern_get(r);
	if (!thiz->changed) return EINA_FALSE;
	/* FIXME */
	return EINA_TRUE;
}

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version =               */ ENESIM_RENDERER_API,
	/* .name =                  */ _pattern_name,
	/* .free =                  */ _pattern_free,
	/* .boundings =             */ NULL,
	/* .flags =                 */ _pattern_flags,
	/* .is_inside =             */ NULL,
	/* .damage =                */ NULL,
	/* .has_changed =           */ _pattern_has_changed,
	/* .sw_setup =              */ _pattern_sw_setup,
	/* .sw_cleanup =            */ _pattern_sw_cleanup,
};

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a pattern renderer
 * @return The new renderer
 */
EAPI Enesim_Renderer * enesim_renderer_pattern_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Pattern *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Pattern));
	if (!thiz) return NULL;
	r = enesim_renderer_new(&_descriptor, thiz);
	return r;
}

