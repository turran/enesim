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

#include "Enesim_Text.h"
#include "enesim_text_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Renderer_Text_Base
{
	Etex *etex;
	Enesim_Renderer_Text_Base_State past, current;
	Enesim_Text_Font *font;
	/* interface */
	Enesim_Renderer_Text_Base_Boundings boundings;
	Enesim_Renderer_Text_Base_Destination_Boundings destination_boundings;
	Enesim_Renderer_Text_Base_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	Enesim_Renderer_Text_Base_OpenCL_Setup opencl_setup;
	Enesim_Renderer_OpenCL_Cleanup opencl_cleanup;
	Enesim_Renderer_Text_Base_OpenGL_Setup opengl_setup;
	Enesim_Renderer_OpenGL_Cleanup opengl_cleanup;
	Enesim_Renderer_Has_Changed has_changed;
	Enesim_Renderer_Delete free;
	/* private */
	void *data;
	Eina_Bool changed : 1;
} Enesim_Renderer_Text_Base;

static inline Enesim_Renderer_Text_Base * _enesim_renderer_text_base_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Text_Base *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	return thiz;
}

static Eina_Bool _enesim_renderer_text_base_changed(Enesim_Renderer_Text_Base *thiz)
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

static void _enesim_renderer_text_base_common_setup(Enesim_Renderer_Text_Base *thiz)
{
	if (_enesim_renderer_text_base_changed(thiz))
	{
		if (thiz->font)
		{
			enesim_text_font_unref(thiz->font);
			thiz->font = NULL;
		}
		if (thiz->current.font_name && thiz->current.size)
			thiz->font = enesim_text_font_load(thiz->etex, thiz->current.font_name, thiz->current.size);
		thiz->has_changed = EINA_FALSE;
	}
}

static void _enesim_renderer_text_base_common_cleanup(Enesim_Renderer_Text_Base *thiz)
{
	/* now update the past state */
	thiz->past.size = thiz->current.size;
	thiz->past.font_name = thiz->current.font_name;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static void _enesim_renderer_text_base_destination_boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Eina_Rectangle *boundings)
{
	Enesim_Renderer_Text_Base *thiz;
	const Enesim_Renderer_Text_Base_State *tstates[ENESIM_RENDERER_STATES];

	thiz = _enesim_renderer_text_base_get(r);
	if (!thiz->destination_boundings)
	{
		boundings->x = INT_MIN / 2;
		boundings->y = INT_MIN / 2;
		boundings->w = INT_MAX;
		boundings->h = INT_MAX;

		return;
	}

	tstates[ENESIM_STATE_CURRENT] = &thiz->current;
	tstates[ENESIM_STATE_PAST] = &thiz->past;
	thiz->destination_boundings(r, states, sstates, tstates, boundings);

}

static void _enesim_renderer_text_base_boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *boundings)
{
	Enesim_Renderer_Text_Base *thiz;
	const Enesim_Renderer_Text_Base_State *tstates[ENESIM_RENDERER_STATES];

	thiz = _enesim_renderer_text_base_get(r);
	if (!thiz->boundings)
	{
		boundings->x = INT_MIN / 2;
		boundings->y = INT_MIN / 2;
		boundings->w = INT_MAX;
		boundings->h = INT_MAX;

		return;
	}

	tstates[ENESIM_STATE_CURRENT] = &thiz->current;
	tstates[ENESIM_STATE_PAST] = &thiz->past;
	thiz->boundings(r, states, sstates, tstates, boundings);
}


static Eina_Bool _enesim_renderer_text_base_sw_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Shape_Sw_Draw *fill, Enesim_Error **error)
{
	Enesim_Renderer_Text_Base *thiz;
	const Enesim_Renderer_Text_Base_State *tstates[ENESIM_RENDERER_STATES];

	thiz = _enesim_renderer_text_base_get(r);
	if (!thiz->sw_setup) return EINA_FALSE;

	tstates[ENESIM_STATE_CURRENT] = &thiz->current;
	tstates[ENESIM_STATE_PAST] = &thiz->past;

	_enesim_renderer_text_base_common_setup(thiz);
	return thiz->sw_setup(r, states, sstates, tstates, s, fill, error);
}

static void _enesim_renderer_text_base_sw_cleanup(Enesim_Renderer *r,
		Enesim_Surface *s)
{
	Enesim_Renderer_Text_Base *thiz;

	thiz = _enesim_renderer_text_base_get(r);
	if (thiz->sw_cleanup)
		thiz->sw_cleanup(r, s);
	_enesim_renderer_text_base_common_cleanup(thiz);
}

static Eina_Bool _enesim_renderer_text_base_opengl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Error **error)
{
	Enesim_Renderer_Text_Base *thiz;
	const Enesim_Renderer_Text_Base_State *tstates[ENESIM_RENDERER_STATES];

	thiz = _enesim_renderer_text_base_get(r);
	if (!thiz->opengl_setup) return EINA_FALSE;

	tstates[ENESIM_STATE_CURRENT] = &thiz->current;
	tstates[ENESIM_STATE_PAST] = &thiz->past;

	_enesim_renderer_text_base_common_setup(thiz);
	return thiz->opengl_setup(r, states, sstates, tstates, s,
		draw, error);
}

static void _enesim_renderer_text_base_opengl_cleanup(Enesim_Renderer *r,
		Enesim_Surface *s)
{
	Enesim_Renderer_Text_Base *thiz;

	thiz = _enesim_renderer_text_base_get(r);
	if (thiz->opengl_cleanup)
		thiz->opengl_cleanup(r, s);
	_enesim_renderer_text_base_common_cleanup(thiz);
}

static Eina_Bool _enesim_renderer_text_base_opencl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Renderer_Shape_State *sstates[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		const char **program_name, const char **program_source,
		size_t *program_length,
		Enesim_Error **error)
{
	Enesim_Renderer_Text_Base *thiz;
	const Enesim_Renderer_Text_Base_State *tstates[ENESIM_RENDERER_STATES];

	thiz = _enesim_renderer_text_base_get(r);
	if (!thiz->opengl_setup) return EINA_FALSE;

	tstates[ENESIM_STATE_CURRENT] = &thiz->current;
	tstates[ENESIM_STATE_PAST] = &thiz->past;

	_enesim_renderer_text_base_common_setup(thiz);
	return thiz->opencl_setup(r, states, sstates, tstates, s,
		program_name, program_source, program_length, error);
}

static void _enesim_renderer_text_base_opencl_cleanup(Enesim_Renderer *r,
		Enesim_Surface *s)
{
	Enesim_Renderer_Text_Base *thiz;

	thiz = _enesim_renderer_text_base_get(r);
	if (thiz->opencl_cleanup)
		thiz->opencl_cleanup(r, s);
	_enesim_renderer_text_base_common_cleanup(thiz);
}

static Eina_Bool _enesim_renderer_text_base_has_changed(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES])
{
	Enesim_Renderer_Text_Base *thiz;
	Eina_Bool ret = EINA_TRUE;

	thiz = _enesim_renderer_text_base_get(r);
	ret = _enesim_renderer_text_base_changed(thiz);
	if (ret)
	{
		return ret;
	}
	/* call the has_changed on the descriptor */
	if (thiz->has_changed)
		ret = thiz->has_changed(r, states);

	return ret;
}

static void _enesim_renderer_text_base_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Text_Base *thiz;

	thiz = _enesim_renderer_text_base_get(r);
	if (thiz->current.font_name)
		free(thiz->current.font_name);
	if (thiz->past.font_name)
		free(thiz->past.font_name);
	if (thiz->font)
		enesim_text_font_unref(thiz->font);
	if (thiz->free)
		thiz->free(r);
	free(thiz);
}

static void _enesim_renderer_text_base_feature_get(Enesim_Renderer *r, Enesim_Shape_Feature *features)
{
	*features = ENESIM_SHAPE_FLAG_FILL_RENDERER;
}


/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_text_base_new(Etex *etex,
		Enesim_Renderer_Text_Base_Descriptor *descriptor,
		void *data)

{
	Enesim_Renderer_Text_Base *thiz;
	Enesim_Renderer *r;
	Enesim_Renderer_Shape_Descriptor pdescriptor;

	thiz = calloc(1, sizeof(Enesim_Renderer_Text_Base));
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
//	pdescriptor.version = ENESIM_RENDERER_API;
	pdescriptor.name = descriptor->name;
	pdescriptor.free = _enesim_renderer_text_base_free;
	pdescriptor.boundings = _enesim_renderer_text_base_boundings;
	pdescriptor.destination_boundings = _enesim_renderer_text_base_destination_boundings;
	pdescriptor.flags = descriptor->flags;
	pdescriptor.hints_get = descriptor->hints_get;
	pdescriptor.is_inside = descriptor->is_inside;
	pdescriptor.damage = descriptor->damage;
	pdescriptor.has_changed = _enesim_renderer_text_base_has_changed;
	pdescriptor.feature_get = _enesim_renderer_text_base_feature_get,
	pdescriptor.sw_setup = _enesim_renderer_text_base_sw_setup;
	pdescriptor.sw_cleanup = _enesim_renderer_text_base_sw_cleanup;
	pdescriptor.opencl_setup = _enesim_renderer_text_base_opencl_setup;
	pdescriptor.opencl_kernel_setup = descriptor->opencl_kernel_setup;
	pdescriptor.opencl_cleanup = _enesim_renderer_text_base_opencl_cleanup;
#if BUILD_OPENGL
	pdescriptor.opengl_initialize = descriptor->opengl_initialize;
	pdescriptor.opengl_setup = _enesim_renderer_text_base_opengl_setup;
	pdescriptor.opengl_cleanup = _enesim_renderer_text_base_opengl_cleanup;
#else
	pdescriptor.opengl_initialize = NULL,
	pdescriptor.opengl_setup = NULL,
	pdescriptor.opengl_cleanup = NULL
#endif

	r = enesim_renderer_shape_new(&pdescriptor, thiz);
	if (!r) goto renderer_err;

	return r;

renderer_err:
	free(thiz);
	return NULL;
}

void * enesim_renderer_text_base_data_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Text_Base *thiz;

	thiz = _enesim_renderer_text_base_get(r);
	return thiz->data;
}

void enesim_renderer_text_base_setup(Enesim_Renderer *r)
{
	Enesim_Renderer_Text_Base *thiz;

	thiz = _enesim_renderer_text_base_get(r);
	_enesim_renderer_text_base_common_setup(thiz);
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_text_base_font_name_set(Enesim_Renderer *r, const char *font)
{
	Enesim_Renderer_Text_Base *thiz;

	if (!font) return;
	thiz = _enesim_renderer_text_base_get(r);
	if (!thiz) return;
	if (thiz->current.font_name) free(thiz->current.font_name);
	thiz->current.font_name = strdup(font);
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_text_base_font_name_get(Enesim_Renderer *r, const char **font)
{
	Enesim_Renderer_Text_Base *thiz;

	if (!font) return;
	thiz = _enesim_renderer_text_base_get(r);
	if (!thiz) return;
	*font = thiz->current.font_name;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_text_base_size_set(Enesim_Renderer *r, unsigned int size)
{
	Enesim_Renderer_Text_Base *thiz;

	thiz = _enesim_renderer_text_base_get(r);
	if (!thiz) return;

	thiz->current.size = size;
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_text_base_size_get(Enesim_Renderer *r, unsigned int *size)
{
	Enesim_Renderer_Text_Base *thiz;

	thiz = _enesim_renderer_text_base_get(r);
	if (!thiz) return;

	*size = thiz->current.size;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_text_base_max_ascent_get(Enesim_Renderer *r, int *masc)
{
	Enesim_Renderer_Text_Base *thiz;

	*masc = 0;
	thiz = _enesim_renderer_text_base_get(r);
	if (!thiz) return;
	enesim_renderer_text_base_setup(r);
	if (!thiz->font) return;
	*masc = enesim_text_font_max_ascent_get(thiz->font);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_text_base_max_descent_get(Enesim_Renderer *r, int *mdesc)
{
	Enesim_Renderer_Text_Base *thiz;

	*mdesc = 0;
	thiz = _enesim_renderer_text_base_get(r);
	if (!thiz) return;
	enesim_renderer_text_base_setup(r);
	if (!thiz->font) return;
	*mdesc = enesim_text_font_max_descent_get(thiz->font);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Text_Font * enesim_renderer_text_base_font_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Text_Base *thiz;

	thiz = _enesim_renderer_text_base_get(r);
	enesim_renderer_text_base_setup(r);

	return enesim_text_font_ref(thiz->font);
}
