/* ETEX - Enesim's Text Renderer
 * Copyright (C) 2010 Jorge Luis Zapata
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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#include "Etex.h"
#include "etex_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Etex_Base
{
	Etex *etex;
	Etex_Base_State past, current;
	Etex_Font *font;
	/* interface */
	Etex_Base_Boundings boundings;
	Etex_Base_Destination_Boundings destination_boundings;
	Etex_Base_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	Etex_Base_OpenCL_Setup opencl_setup;
	Enesim_Renderer_OpenCL_Cleanup opencl_cleanup;
	Etex_Base_OpenGL_Setup opengl_setup;
	Enesim_Renderer_OpenGL_Cleanup opengl_cleanup;
	Enesim_Renderer_Has_Changed has_changed;
	Enesim_Renderer_Delete free;
	/* private */
	void *data;
	Eina_Bool changed : 1;
} Etex_Base;

static inline Etex_Base * _etex_base_get(Enesim_Renderer *r)
{
	Etex_Base *thiz;

	thiz = enesim_renderer_data_get(r);
	return thiz;
}

static Eina_Bool _etex_base_changed(Etex_Base *thiz)
{
	if (!thiz->has_changed)
		return EINA_FALSE;

	/* check that the current state is different from the past state */
	if (thiz->current.size != thiz->past.size)
	{
		return EINA_TRUE;
	}
	if ((thiz->current.font_name != thiz->past.font_name) ||
	(strcmp(thiz->current.font_name, thiz->past.font_name)))
	{
		return EINA_TRUE;
	}

	return EINA_FALSE;
}

static void _etex_base_common_setup(Etex_Base *thiz)
{
	if (_etex_base_changed(thiz))
	{
		if (thiz->font)
		{
			etex_font_unref(thiz->font);
			thiz->font = NULL;
		}
		if (thiz->current.font_name && thiz->current.size)
			thiz->font = etex_font_load(thiz->etex, thiz->current.font_name, thiz->current.size);
		thiz->has_changed = EINA_FALSE;
	}
}

static void _etex_base_common_cleanup(Etex_Base *thiz)
{
	/* now update the past state */
	thiz->past.size = thiz->current.size;
	thiz->past.font_name = thiz->current.font_name;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static void _etex_base_destination_boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Eina_Rectangle *boundings)
{
	Etex_Base *thiz;
	const Etex_Base_State *sstates[ENESIM_RENDERER_STATES];

	thiz = _etex_base_get(r);
	if (!thiz->destination_boundings)
	{
		boundings->x = INT_MIN / 2;
		boundings->y = INT_MIN / 2;
		boundings->w = INT_MAX;
		boundings->h = INT_MAX;

		return;
	}

	sstates[ENESIM_STATE_CURRENT] = &thiz->current;
	sstates[ENESIM_STATE_PAST] = &thiz->past;
	thiz->destination_boundings(r, states, sstates, boundings);

}

static void _etex_base_boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *boundings)
{
	Etex_Base *thiz;
	const Etex_Base_State *sstates[ENESIM_RENDERER_STATES];

	thiz = _etex_base_get(r);
	if (!thiz->boundings)
	{
		boundings->x = INT_MIN / 2;
		boundings->y = INT_MIN / 2;
		boundings->w = INT_MAX;
		boundings->h = INT_MAX;

		return;
	}

	sstates[ENESIM_STATE_CURRENT] = &thiz->current;
	sstates[ENESIM_STATE_PAST] = &thiz->past;
	thiz->boundings(r, states, sstates, boundings);
}


static Eina_Bool _etex_base_sw_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Etex_Base *thiz;
	const Etex_Base_State *sstates[ENESIM_RENDERER_STATES];

	thiz = _etex_base_get(r);
	if (!thiz->sw_setup) return EINA_FALSE;

	sstates[ENESIM_STATE_CURRENT] = &thiz->current;
	sstates[ENESIM_STATE_PAST] = &thiz->past;

	_etex_base_common_setup(thiz);
	return thiz->sw_setup(r, states, sstates, s, fill, error);
}

static void _etex_base_sw_cleanup(Enesim_Renderer *r,
		Enesim_Surface *s)
{
	Etex_Base *thiz;

	thiz = _etex_base_get(r);
	if (thiz->sw_cleanup)
		thiz->sw_cleanup(r, s);
	_etex_base_common_cleanup(thiz);
}

static Eina_Bool _etex_base_opengl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Error **error)
{
	Etex_Base *thiz;
	const Etex_Base_State *sstates[ENESIM_RENDERER_STATES];

	thiz = _etex_base_get(r);
	if (!thiz->opengl_setup) return EINA_FALSE;

	sstates[ENESIM_STATE_CURRENT] = &thiz->current;
	sstates[ENESIM_STATE_PAST] = &thiz->past;

	_etex_base_common_setup(thiz);
	return thiz->opengl_setup(r, states, sstates, s,
		draw, error);
}

static void _etex_base_opengl_cleanup(Enesim_Renderer *r,
		Enesim_Surface *s)
{
	Etex_Base *thiz;

	thiz = _etex_base_get(r);
	if (thiz->opengl_cleanup)
		thiz->opengl_cleanup(r, s);
	_etex_base_common_cleanup(thiz);
}

static Eina_Bool _etex_base_opencl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		const char **program_name, const char **program_source,
		size_t *program_length,
		Enesim_Error **error)
{
	Etex_Base *thiz;
	const Etex_Base_State *sstates[ENESIM_RENDERER_STATES];

	thiz = _etex_base_get(r);
	if (!thiz->opengl_setup) return EINA_FALSE;

	sstates[ENESIM_STATE_CURRENT] = &thiz->current;
	sstates[ENESIM_STATE_PAST] = &thiz->past;

	_etex_base_common_setup(thiz);
	return thiz->opencl_setup(r, states, sstates, s,
		program_name, program_source, program_length, error);
}

static void _etex_base_opencl_cleanup(Enesim_Renderer *r,
		Enesim_Surface *s)
{
	Etex_Base *thiz;

	thiz = _etex_base_get(r);
	if (thiz->opencl_cleanup)
		thiz->opencl_cleanup(r, s);
	_etex_base_common_cleanup(thiz);
}

static Eina_Bool _etex_base_has_changed(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES])
{
	Etex_Base *thiz;
	Eina_Bool ret = EINA_TRUE;

	thiz = _etex_base_get(r);
	ret = _etex_base_changed(thiz);
	if (ret)
	{
		return ret;
	}
	/* call the has_changed on the descriptor */
	if (thiz->has_changed)
		ret = thiz->has_changed(r, states);

	return ret;
}

static void _etex_base_free(Enesim_Renderer *r)
{
	Etex_Base *thiz;

	thiz = _etex_base_get(r);
	if (thiz->current.font_name)
		free(thiz->current.font_name);
	if (thiz->past.font_name)
		free(thiz->past.font_name);
	if (thiz->font)
		etex_font_unref(thiz->font);
	if (thiz->free)
		thiz->free(r);
	free(thiz);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * etex_base_new(Etex *etex,
		Etex_Base_Descriptor *descriptor,
		void *data)

{
	Etex_Base *thiz;
	Enesim_Renderer *r;
	Enesim_Renderer_Descriptor pdescriptor;

	thiz = calloc(1, sizeof(Etex_Base));
	if (!thiz)
		return NULL;

	thiz->data = data;
	thiz->etex = etex;

	thiz->free = descriptor->free;
	thiz->has_changed = descriptor->has_changed;
	thiz->sw_setup = descriptor->sw_setup;
	thiz->sw_cleanup = descriptor->sw_cleanup;
	thiz->opengl_setup = descriptor->opengl_setup;
	thiz->opengl_cleanup = descriptor->opengl_cleanup;
	thiz->opencl_setup = descriptor->opencl_setup;
	thiz->opencl_cleanup = descriptor->opencl_cleanup;
	thiz->destination_boundings = descriptor->destination_boundings;
	thiz->boundings = descriptor->boundings;
	/* set the parent descriptor */
	pdescriptor.version = ENESIM_RENDERER_API;
	pdescriptor.name = descriptor->name;
	pdescriptor.free = _etex_base_free;
	pdescriptor.boundings = _etex_base_boundings;
	pdescriptor.destination_boundings = _etex_base_destination_boundings;
	pdescriptor.flags = descriptor->flags;
	pdescriptor.hints_get = descriptor->hints_get;
	pdescriptor.is_inside = descriptor->is_inside;
	pdescriptor.damage = descriptor->damage;
	pdescriptor.has_changed = _etex_base_has_changed;
	pdescriptor.sw_setup = _etex_base_sw_setup;
	pdescriptor.sw_cleanup = _etex_base_sw_cleanup;
	pdescriptor.opencl_setup = _etex_base_opencl_setup;
	pdescriptor.opencl_kernel_setup = descriptor->opencl_kernel_setup;
	pdescriptor.opencl_cleanup = _etex_base_opencl_cleanup;
	pdescriptor.opengl_initialize = descriptor->opengl_initialize;
	pdescriptor.opengl_setup = _etex_base_opengl_setup;
	pdescriptor.opengl_cleanup = _etex_base_opengl_cleanup;

	r = enesim_renderer_new(&pdescriptor, thiz);
	if (!thiz) goto renderer_err;

	return r;

renderer_err:
	free(thiz);
	return NULL;
}

void * etex_base_data_get(Enesim_Renderer *r)
{
	Etex_Base *thiz;

	thiz = _etex_base_get(r);
	return thiz->data;
}

void etex_base_setup(Enesim_Renderer *r)
{
	Etex_Base *thiz;

	thiz = _etex_base_get(r);
	_etex_base_common_setup(thiz);
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void etex_base_font_name_set(Enesim_Renderer *r, const char *font)
{
	Etex_Base *thiz;

	if (!font) return;
	thiz = _etex_base_get(r);
	if (!thiz) return;
	if (thiz->current.font_name) free(thiz->current.font_name);
	thiz->current.font_name = strdup(font);
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void etex_base_font_name_get(Enesim_Renderer *r, const char **font)
{
	Etex_Base *thiz;

	if (!font) return;
	thiz = _etex_base_get(r);
	if (!thiz) return;
	*font = thiz->current.font_name;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void etex_base_size_set(Enesim_Renderer *r, unsigned int size)
{
	Etex_Base *thiz;

	thiz = _etex_base_get(r);
	if (!thiz) return;

	thiz->current.size = size;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void etex_base_size_get(Enesim_Renderer *r, unsigned int *size)
{
	Etex_Base *thiz;

	thiz = _etex_base_get(r);
	if (!thiz) return;

	*size = thiz->current.size;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void etex_base_max_ascent_get(Enesim_Renderer *r, int *masc)
{
	Etex_Base *thiz;

	*masc = 0;
	thiz = _etex_base_get(r);
	if (!thiz) return;
	etex_base_setup(r);
	if (!thiz->font) return;
	*masc = etex_font_max_ascent_get(thiz->font);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void etex_base_max_descent_get(Enesim_Renderer *r, int *mdesc)
{
	Etex_Base *thiz;

	*mdesc = 0;
	thiz = _etex_base_get(r);
	if (!thiz) return;
	etex_base_setup(r);
	if (!thiz->font) return;
	*mdesc = etex_font_max_descent_get(thiz->font);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Etex_Font * etex_base_font_get(Enesim_Renderer *r)
{
	Etex_Base *thiz;

	thiz = _etex_base_get(r);
	etex_base_setup(r);

	return etex_font_ref(thiz->font);
}
