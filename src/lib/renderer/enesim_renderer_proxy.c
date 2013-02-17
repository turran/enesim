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
#include "enesim_private.h"

#include "enesim_main.h"
#include "enesim_error.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"
#include "enesim_renderer_proxy.h"

#ifdef BUILD_OPENCL
#include "Enesim_OpenCL.h"
#endif

#ifdef BUILD_OPENGL
#include "Enesim_OpenGL.h"
#endif

#include "enesim_renderer_private.h"
#include "enesim_renderer_simple_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_PROXY_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_PROXY_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_PROXY_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Proxy {
	EINA_MAGIC
	/* the properties */
	Enesim_Renderer *proxied;
	/* generated at state setup */
	Enesim_Renderer_Sw_Fill proxied_fill;
} Enesim_Renderer_Proxy;

static inline Enesim_Renderer_Proxy * _proxy_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Proxy *thiz;

	thiz = enesim_renderer_data_get(r);
	ENESIM_RENDERER_PROXY_MAGIC_CHECK(thiz);

	return thiz;
}

static void _proxy_span(Enesim_Renderer *r,
		int x, int y,
		unsigned int len, void *dst)
{
	Enesim_Renderer_Proxy *thiz;

 	thiz = _proxy_get(r);
	/* FIXME isnt enough to call the proxied fill */
	enesim_renderer_sw_draw(thiz->proxied, x, y, len, dst);
}

#if BUILD_OPENGL
static void _proxy_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		const Eina_Rectangle *area, int w, int h)
{
	Enesim_Renderer_Proxy *thiz;

 	thiz = _proxy_get(r);
	enesim_renderer_opengl_draw(thiz->proxied, s, area, w, h);
}
#endif

static Eina_Bool _proxy_state_setup(Enesim_Renderer_Proxy *thiz,
		Enesim_Renderer *r, Enesim_Surface *s, Enesim_Error **error)
{
	if (!thiz->proxied)
	{
		ENESIM_RENDERER_ERROR(r, error, "No proxied");
		return EINA_FALSE;
	}

	if (!enesim_renderer_setup(thiz->proxied, s, error))
	{
		const char *name;

		enesim_renderer_name_get(thiz->proxied, &name);
		ENESIM_RENDERER_ERROR(r, error, "Proxy renderer %s can not setup", name);
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

static void _proxy_state_cleanup(Enesim_Renderer_Proxy *thiz,
		Enesim_Surface *s)
{
	if (!thiz->proxied) return;
	enesim_renderer_cleanup(thiz->proxied, s);
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _proxy_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "proxy";
}

static Eina_Bool _proxy_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Proxy *thiz;

 	thiz = _proxy_get(r);
	if (!_proxy_state_setup(thiz, r, s, error))
		return EINA_FALSE;

	*fill = _proxy_span;
	return EINA_TRUE;
}

static void _proxy_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Proxy *thiz;

 	thiz = _proxy_get(r);
	_proxy_state_cleanup(thiz, s);
}

static void _proxy_flags(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Flag *flags)
{
	/* we dont support anything */
	*flags = 0;
}

static void _proxy_hints(Enesim_Renderer *r,
		Enesim_Renderer_Sw_Hint *hints)
{
	Enesim_Renderer_Proxy *thiz;

	thiz = _proxy_get(r);
	*hints = 0;
	if (!thiz->proxied)
		return;
	enesim_renderer_hints_get(thiz->proxied, hints);
}

static void _proxy_bounds(Enesim_Renderer *r,
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Proxy *thiz;

	thiz = _proxy_get(r);
	if (!thiz->proxied)
	{
		rect->x = 0;
		rect->y = 0;
		rect->w = 0;
		rect->h = 0;
		return;
	}
	enesim_renderer_bounds(thiz->proxied, rect);
}

static void _proxy_destination_bounds(Enesim_Renderer *r,
		Eina_Rectangle *bounds)
{
	Enesim_Renderer_Proxy *thiz;

	thiz = _proxy_get(r);
	if (!thiz->proxied)
	{
		bounds->x = 0;
		bounds->y = 0;
		bounds->w = 0;
		bounds->h = 0;
		return;
	}
	enesim_renderer_destination_bounds(thiz->proxied, bounds, 0, 0);
}

static Eina_Bool _proxy_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Proxy *thiz;
	Eina_Bool ret = EINA_FALSE;

	thiz = _proxy_get(r);
	if (thiz->proxied)
		ret = enesim_renderer_has_changed(thiz->proxied);
	return ret;
}

static void _proxy_damage(Enesim_Renderer *r,
		const Eina_Rectangle *old_bounds,
		Enesim_Renderer_Damage_Cb cb, void *data)
{
	Enesim_Renderer_Proxy *thiz;
	Eina_Bool common_changed;

	thiz = _proxy_get(r);
	/* we need to take care of the visibility */
	common_changed = enesim_renderer_state_has_changed(r);
	if (common_changed)
	{
		Eina_Rectangle current_bounds;

		enesim_renderer_destination_bounds(r, &current_bounds, 0, 0);
		cb(r, old_bounds, EINA_TRUE, data);
		cb(r, &current_bounds, EINA_FALSE, data);
		return;
	}
	if (!thiz->proxied)
		return;
	enesim_renderer_damages_get(thiz->proxied,
			cb, data);
}

static void _proxy_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Proxy *thiz;

	thiz = _proxy_get(r);
	if (thiz->proxied)
		enesim_renderer_unref(thiz->proxied);
	free(thiz);
}

#if BUILD_OPENGL
static Eina_Bool _proxy_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Error **error)
{
	Enesim_Renderer_Proxy *thiz;

 	thiz = _proxy_get(r);
	if (!_proxy_state_setup(thiz, r, s, error))
		return EINA_FALSE;

	*draw = _proxy_opengl_draw;
	return EINA_TRUE;
}

static void _proxy_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Proxy *thiz;

 	thiz = _proxy_get(r);
	_proxy_state_cleanup(thiz, s);
}
#endif

static Enesim_Renderer_Descriptor _descriptor = {
	/* .name_get = 			*/ _proxy_name,
	/* .free = 			*/ _proxy_free,
	/* .bounds_get = 		*/ _proxy_bounds,
	/* .destination_bounds_get =	*/ _proxy_destination_bounds,
	/* .flags_get =			*/ _proxy_flags,
	/* .hints_get = 		*/ _proxy_hints,
	/* .is_inside = 		*/ NULL,
	/* .damages_get =		*/ _proxy_damage,
	/* .has_changed = 		*/ _proxy_has_changed,
	/* .sw_setup = 			*/ _proxy_sw_setup,
	/* .sw_cleanup = 		*/ _proxy_sw_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
#if BUILD_OPENGL
	/* .opengl_initialize = 	*/ NULL,
	/* .opengl_setup = 		*/ _proxy_opengl_setup,
	/* .opengl_cleanup = 		*/ _proxy_opengl_cleanup,
#else
	/* .opengl_initialize = 	*/ NULL,
	/* .opengl_setup = 		*/ NULL,
	/* .opengl_cleanup = 		*/ NULL
#endif
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a proxy renderer
 * @return The new renderer
 */
EAPI Enesim_Renderer * enesim_renderer_proxy_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Proxy *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Proxy));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_PROXY_MAGIC);
	r = enesim_renderer_new(&_descriptor, thiz);
	return r;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_proxy_proxied_set(Enesim_Renderer *r,
		Enesim_Renderer *proxied)
{
	Enesim_Renderer_Proxy *thiz;

	thiz = _proxy_get(r);
	if (thiz->proxied)
		enesim_renderer_unref(thiz->proxied);
	thiz->proxied = proxied;
	if (thiz->proxied)
		thiz->proxied = enesim_renderer_ref(thiz->proxied);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_proxy_proxied_get(Enesim_Renderer *r,
		Enesim_Renderer **proxied)
{
	Enesim_Renderer_Proxy *thiz;

	thiz = _proxy_get(r);
	if (!proxied) return;
	*proxied = thiz->proxied;
	if (thiz->proxied)
		thiz->proxied = enesim_renderer_ref(thiz->proxied);
}
