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
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_proxy.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#ifdef BUILD_OPENCL
#include "Enesim_OpenCL.h"
#endif

#ifdef BUILD_OPENGL
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
#endif

#include "enesim_renderer_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_PROXY(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Proxy,						\
		enesim_renderer_proxy_descriptor_get())

typedef struct _Enesim_Renderer_Proxy {
	Enesim_Renderer parent;
	/* the properties */
	Enesim_Renderer *proxied;
	/* generated at state setup */
	Enesim_Renderer_Sw_Fill proxied_fill;
} Enesim_Renderer_Proxy;

typedef struct _Enesim_Renderer_Proxy_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Proxy_Class;

/* whenever the proxy needs to fill, we need to zeros the whole destination buffer
 * before given that the proxied renderer will blend and thus expects a clean
 * buffer
 */
static void _proxy_fill_span_blend(Enesim_Renderer *r,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Proxy *thiz;

	thiz = ENESIM_RENDERER_PROXY(r);

	memset(ddata, 0, len * sizeof(uint32_t));
	enesim_renderer_sw_draw(thiz->proxied, x, y, len, ddata);
}

/* whenever the proxy needs to blend, we just draw the inner renderer */
static void _proxy_blend_or_equal_span(Enesim_Renderer *r,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Proxy *thiz;

	thiz = ENESIM_RENDERER_PROXY(r);
	enesim_renderer_sw_draw(thiz->proxied, x, y, len, ddata);
}

#if BUILD_OPENGL
static void _proxy_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		const Eina_Rectangle *area, int w, int h)
{
	Enesim_Renderer_Proxy *thiz;

 	thiz = ENESIM_RENDERER_PROXY(r);
	enesim_renderer_opengl_draw(thiz->proxied, s, area, w, h);
}
#endif

static Eina_Bool _proxy_state_setup(Enesim_Renderer_Proxy *thiz,
		Enesim_Renderer *r, Enesim_Surface *s, Enesim_Log **l)
{
	if (!thiz->proxied)
	{
		ENESIM_RENDERER_LOG(r, l, "No proxied");
		return EINA_FALSE;
	}

	if (!enesim_renderer_setup(thiz->proxied, s, l))
	{
		const char *name;

		enesim_renderer_name_get(thiz->proxied, &name);
		ENESIM_RENDERER_LOG(r, l, "Proxy renderer %s can not setup", name);
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
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l)
{
	Enesim_Renderer_Proxy *thiz;
	Enesim_Rop rop;
	Enesim_Rop proxy_rop;

 	thiz = ENESIM_RENDERER_PROXY(r);
	if (!_proxy_state_setup(thiz, r, s, l))
		return EINA_FALSE;

	enesim_renderer_rop_get(r, &rop);
	enesim_renderer_rop_get(thiz->proxied, &proxy_rop);
	if (rop != proxy_rop && rop == ENESIM_FILL)
	{
		*fill = _proxy_fill_span_blend;
	}
	else
	{
		*fill = _proxy_blend_or_equal_span;
	}

	return EINA_TRUE;
}

static void _proxy_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Proxy *thiz;

 	thiz = ENESIM_RENDERER_PROXY(r);
	_proxy_state_cleanup(thiz, s);
}

static void _proxy_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	/* we dont support anything */
	*features = 0;
}

static void _proxy_sw_hints_get(Enesim_Renderer *r,
		Enesim_Renderer_Sw_Hint *hints)
{
	Enesim_Renderer_Proxy *thiz;
	Enesim_Renderer_Sw_Hint proxied_hints;
	const Enesim_Renderer_State *state;
	const Enesim_Renderer_State *proxied_state;

	thiz = ENESIM_RENDERER_PROXY(r);
	*hints = 0;
	if (!thiz->proxied)
		return;

	state = enesim_renderer_state_get(r);
	proxied_state = enesim_renderer_state_get(thiz->proxied);
	enesim_renderer_sw_hints_get(thiz->proxied, &proxied_hints);
	/* check if we can to colorize */
	if (proxied_state->current.color == state->current.color)
		*hints |= ENESIM_RENDERER_HINT_COLORIZE;
	/* check if we can rop */
	if (proxied_state->current.rop == state->current.rop)
		*hints |= ENESIM_RENDERER_HINT_ROP;
}

static void _proxy_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Proxy *thiz;

	thiz = ENESIM_RENDERER_PROXY(r);
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

static Eina_Bool _proxy_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Proxy *thiz;
	Eina_Bool ret = EINA_FALSE;

	thiz = ENESIM_RENDERER_PROXY(r);
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

	thiz = ENESIM_RENDERER_PROXY(r);
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

#if BUILD_OPENGL
static Eina_Bool _proxy_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l)
{
	Enesim_Renderer_Proxy *thiz;

 	thiz = ENESIM_RENDERER_PROXY(r);
	if (!_proxy_state_setup(thiz, r, s, l))
		return EINA_FALSE;

	*draw = _proxy_opengl_draw;
	return EINA_TRUE;
}

static void _proxy_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Proxy *thiz;

 	thiz = ENESIM_RENDERER_PROXY(r);
	_proxy_state_cleanup(thiz, s);
}
#endif
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Proxy, Enesim_Renderer_Proxy_Class,
		enesim_renderer_proxy);

static void _enesim_renderer_proxy_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _proxy_name;
	klass->bounds_get = _proxy_bounds_get;
	klass->features_get = _proxy_features_get;
	klass->damages_get = _proxy_damage;
	klass->has_changed = _proxy_has_changed;
	klass->sw_hints_get = _proxy_sw_hints_get;
	klass->sw_setup = _proxy_sw_setup;
	klass->sw_cleanup = _proxy_sw_cleanup;
#if BUILD_OPENGL
	klass->opengl_setup = _proxy_opengl_setup;
	klass->opengl_cleanup = _proxy_opengl_cleanup;
#endif
}

static void _enesim_renderer_proxy_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_renderer_proxy_instance_deinit(void *o)
{
	Enesim_Renderer_Proxy *thiz;

	thiz = ENESIM_RENDERER_PROXY(o);
	if (thiz->proxied)
		enesim_renderer_unref(thiz->proxied);
}
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

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_proxy);
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

	thiz = ENESIM_RENDERER_PROXY(r);
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

	thiz = ENESIM_RENDERER_PROXY(r);
	if (!proxied) return;
	*proxied = thiz->proxied;
	if (thiz->proxied)
		thiz->proxied = enesim_renderer_ref(thiz->proxied);
}
