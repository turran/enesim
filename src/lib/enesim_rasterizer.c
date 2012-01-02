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
typedef struct _Enesim_Rasterizer
{
	const Enesim_Polygon *polygon;
	Eina_Bool changed : 1;
	void *data;
} Enesim_Rasterizer;

static inline Enesim_Rasterizer * _rasterizer_get(Enesim_Renderer *r)
{
	Enesim_Rasterizer *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	return thiz;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _rasterizer_name(Enesim_Renderer *r)
{
	return "rasterizer";
}

static Eina_Bool _rasterizer_state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	/* TODO call our own setup */
	return EINA_TRUE;
}

static void _rasterizer_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Rasterizer *thiz;

	thiz = _rasterizer_get(r);

	enesim_renderer_shape_cleanup(r, s);
	thiz->changed = EINA_FALSE;
}

static void _rasterizer_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Rasterizer *thiz;

	thiz = _rasterizer_get(r);
}

static void _rasterizer_free(Enesim_Renderer *r)
{

}

static void _rasterizer_boundings(Enesim_Renderer *r, Enesim_Renderer *boundings)
{
	Enesim_Rasterizer *thiz;

	thiz = _rasterizer_get(r);
}

static Eina_Bool _rasterizer_has_changed(Enesim_Renderer *r)
{
	Enesim_Rasterizer *thiz;

	thiz = _rasterizer_get(r);
	return thiz->changed;
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_rasterizer_new(Enesim_Rasterizer_Descriptor *d, void *data)
{
	Enesim_Renderer *r;
	Enesim_Rasterizer *thiz;
	Enesim_Renderer_Descriptor pdescriptor = {
		/* .version = 			*/ ENESIM_RENDERER_API,
		/* .name = 			*/ _rasterizer_name,
		/* .free = 			*/ _rasterizer_free,
		/* .boundings = 		*/ NULL,
		/* .destination_transform = 	*/ NULL,
		/* .flags = 			*/ _rasterizer_flags,
		/* .is_inside = 		*/ NULL,
		/* .damage = 			*/ NULL,
		/* .has_changed = 		*/ _rasterizer_has_changed,
		/* .sw_setup = 			*/ _rasterizer_state_setup,
		/* .sw_cleanup = 		*/ _rasterizer_state_cleanup,
		/* .opencl_setup =		*/ NULL,
		/* .opencl_kernel_setup =	*/ NULL,
		/* .opencl_cleanup =		*/ NULL,
		/* .opengl_setup =          	*/ NULL,
		/* .opengl_shader_setup =   	*/ NULL,
		/* .opengl_cleanup =        	*/ NULL
	};

	thiz = calloc(1, sizeof(Enesim_Rasterizer));
	if (!thiz) return NULL;
	r = enesim_renderer_shape_new(&pdescriptor, thiz);
	thiz->data = data;
	return r;
}

void enesim_rasterizier_polygon_set(Enesim_Renderer *r, const Enesim_Polygon *p)
{
	Enesim_Rasterizer *thiz;

	thiz = _rasterizer_get(r);
	thiz->polygon = p;
	thiz->changed = EINA_TRUE;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

