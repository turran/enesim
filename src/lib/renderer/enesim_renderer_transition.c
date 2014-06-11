/* ENESIM - Drawing Library
 * Copyright (C) 2007-2013 Jorge Luis Zapata
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
#include "libargb.h"

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_transition.h"
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
/** @cond internal */
#define ENESIM_RENDERER_TRANSITION(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Transition,					\
		enesim_renderer_transition_descriptor_get())

typedef struct _Enesim_Renderer_Transition {
	int interp;
	double level;
	Enesim_Renderer *src;
	Enesim_Renderer *tgt;
} Enesim_Renderer_Transition;

typedef struct _Enesim_Renderer_Transition_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Transition_Class;

static void _transition_span_general(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Transition *thiz;
	Enesim_Renderer *s0, *s1;
	int interp ;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	unsigned int *buf;

	thiz = ENESIM_RENDERER_TRANSITION(r);
	s0 = thiz->src;
	s1 = thiz->tgt;
	interp = thiz->interp;

	if (interp == 0)
	{
		enesim_renderer_sw_draw(s0, x, y, len, d);
		return;
	}
	if (interp == 256)
	{
		enesim_renderer_sw_draw(s1, x, y, len, d);
		return;
	}
	buf = alloca(len * sizeof(unsigned int));
	enesim_renderer_sw_draw(s1, x, y, len, buf);
	enesim_renderer_sw_draw(s0, x, y, len, d);

	while (d < e)
	{
		uint32_t p0 = *d, p1 = *buf;

		*d++ = argb8888_interp_256(interp, p1, p0);
		buf++;
	}
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _transition_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "transition";
}

static Eina_Bool _transition_state_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l)
{
	Enesim_Renderer_Transition *thiz;
	double level;

	thiz = ENESIM_RENDERER_TRANSITION(r);
	if (!thiz || !thiz->src || !thiz->tgt)
		return EINA_FALSE;

	if (!enesim_renderer_setup(thiz->src, s, rop, l))
		goto r0_end;
	if (!enesim_renderer_setup(thiz->tgt, s, rop, l))
		goto r1_end;
	level = thiz->level;
	if (level < 0)
		level = 0;
	if (level > 1)
		level = 1;
	thiz->interp = 1 + (255 * level);

	*fill = _transition_span_general;

	return EINA_TRUE;
r1_end:
	enesim_renderer_cleanup(thiz->src, s);
r0_end:
	return EINA_FALSE;
}

static void _transition_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Transition *thiz;

	thiz = ENESIM_RENDERER_TRANSITION(r);
	enesim_renderer_cleanup(thiz->src, s);
	enesim_renderer_cleanup(thiz->tgt, s);
}

static void _transition_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Transition *trans;
	Enesim_Rectangle r0_rect;
	Enesim_Rectangle r1_rect;

	trans = ENESIM_RENDERER_TRANSITION(r);
	rect->x = 0;
	rect->y = 0;
	rect->w = 0;
	rect->h = 0;

	if (!trans->src) return;
	if (!trans->tgt) return;

	enesim_renderer_bounds_get(trans->src, &r0_rect);
	enesim_renderer_bounds_get(trans->tgt, &r1_rect);

	rect->x = r0_rect.x < r1_rect.x ? r0_rect.x : r1_rect.x;
	rect->y = r0_rect.y < r1_rect.y ? r0_rect.y : r1_rect.y;
	rect->w = r0_rect.w > r1_rect.w ? r0_rect.w : r1_rect.w;
	rect->h = r0_rect.h > r1_rect.h ? r0_rect.h : r1_rect.h;
}

static void _transition_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_ARGB8888;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Transition, Enesim_Renderer_Transition_Class,
		enesim_renderer_transition);

static void _enesim_renderer_transition_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _transition_name;
	klass->bounds_get = _transition_bounds_get;
	klass->features_get = _transition_features_get;
	klass->sw_setup = _transition_state_setup;
	klass->sw_cleanup = _transition_state_cleanup;
}

static void _enesim_renderer_transition_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_renderer_transition_instance_deinit(void *o)
{
	Enesim_Renderer_Transition *thiz;

	thiz = ENESIM_RENDERER_TRANSITION(o);
	if (thiz->src)
		enesim_renderer_unref(thiz->src);
	if (thiz->tgt)
		enesim_renderer_unref(thiz->tgt);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a transition renderer
 * @return The new renderer
 */
EAPI Enesim_Renderer * enesim_renderer_transition_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_transition);
	return r;
}

/**
 * Sets the transition level
 * @param[in] r The transition renderer
 * @param[in] level The transition level. A value of 0 will render
 * the source renderer, a value of 1 will render the target renderer
 * and any other value will inteprolate between both renderers
 */
EAPI void enesim_renderer_transition_level_set(Enesim_Renderer *r, double level)
{
	Enesim_Renderer_Transition *thiz;

	thiz = ENESIM_RENDERER_TRANSITION(r);
	thiz->level = level;
}

/**
 * Gets the transition level
 * @param[in] r The transition renderer
 * @return The level
 */
EAPI double enesim_renderer_transition_level_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Transition *thiz;

	thiz = ENESIM_RENDERER_TRANSITION(r);
	return thiz->level;
}

/**
 * Sets the source renderer
 * @param[in] r The transition renderer
 * @param[in] r0 The source renderer
 */
EAPI void enesim_renderer_transition_source_set(Enesim_Renderer *r, Enesim_Renderer *r0)
{
	Enesim_Renderer_Transition *thiz;

	thiz = ENESIM_RENDERER_TRANSITION(r);
	if (thiz->src)
		enesim_renderer_unref(thiz->src);
	thiz->src = r0;
}

/**
 * Gets the source renderer
 * @param[in] r The transition renderer
 * @return The source renderer
 */
EAPI Enesim_Renderer * enesim_renderer_transition_source_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Transition *thiz;

	thiz = ENESIM_RENDERER_TRANSITION(r);
	return enesim_renderer_ref(thiz->src);
}

/**
 * Sets the target renderer
 * @param[in] r The transition renderer
 * @param[in] r1 The target renderer
 */
EAPI void enesim_renderer_transition_target_set(Enesim_Renderer *r, Enesim_Renderer *r1)
{
	Enesim_Renderer_Transition *thiz;

	thiz = ENESIM_RENDERER_TRANSITION(r);
	if (thiz->tgt)
		enesim_renderer_unref(thiz->tgt);
	thiz->tgt = r1;
}

/**
 * Gets the target renderer
 * @param[in] r The transition renderer
 * @return The target renderer
 */
EAPI Enesim_Renderer * enesim_renderer_transition_target_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Transition *thiz;

	thiz = ENESIM_RENDERER_TRANSITION(r);
	return enesim_renderer_ref(thiz->tgt);
}


