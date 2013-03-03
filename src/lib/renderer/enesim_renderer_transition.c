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
#include "enesim_private.h"
#include "libargb.h"

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"
#include "enesim_renderer_transition.h"

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
#define ENESIM_RENDERER_TRANSITION_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_TRANSITION_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_TRANSITION_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Transition {
	EINA_MAGIC
	int interp;
	struct {
		int x, y;
	} offset;

	struct {
		Enesim_Renderer *r;
	} r0, r1;
} Enesim_Renderer_Transition;

static inline Enesim_Renderer_Transition * _transition_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Transition *thiz;

	thiz = enesim_renderer_data_get(r);
	ENESIM_RENDERER_TRANSITION_MAGIC_CHECK(thiz);

	return thiz;
}

static void _transition_span_general(Enesim_Renderer *r,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Transition *t;
	Enesim_Renderer *s0, *s1;
	int interp ;
	uint32_t *dst = ddata;
	unsigned int *d = dst, *e = d + len;
	unsigned int *buf;

	t = _transition_get(r);
	s0 = t->r0.r;
	s1 = t->r1.r;
	interp = t->interp;

	if (interp == 0)
	{
		enesim_renderer_sw_draw(s0, x, y, len, d);
		return;
	}
	if (interp == 256)
	{
		enesim_renderer_sw_draw(s1, t->offset.x + x, t->offset.y + y, len, d);
		return;
	}
	buf = alloca(len * sizeof(unsigned int));
	enesim_renderer_sw_draw(s1, t->offset.x + x, t->offset.y + y, len, buf);
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
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **log)
{
	Enesim_Renderer_Transition *t;

	t = _transition_get(r);
	if (!t || !t->r0.r || !t->r1.r)
		return EINA_FALSE;

	if (!enesim_renderer_setup(t->r0.r, s, log))
		goto r0_end;
	if (!enesim_renderer_setup(t->r1.r, s, log))
		goto r1_end;

	*fill = _transition_span_general;

	return EINA_TRUE;
r1_end:
	enesim_renderer_cleanup(t->r0.r, s);
r0_end:
	return EINA_FALSE;
}

static void _transition_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Transition *t;

	t = _transition_get(r);
	enesim_renderer_cleanup(t->r0.r, s);
	enesim_renderer_cleanup(t->r1.r, s);
}

static void _transition_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Transition *trans;
	Enesim_Rectangle r0_rect;
	Enesim_Rectangle r1_rect;

	trans = _transition_get(r);
	rect->x = 0;
	rect->y = 0;
	rect->w = 0;
	rect->h = 0;

	if (!trans->r0.r) return;
	if (!trans->r1.r) return;

	enesim_renderer_bounds(trans->r0.r, &r0_rect);
	enesim_renderer_bounds(trans->r1.r, &r1_rect);

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

static void _transition_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Transition *thiz;

	thiz = _transition_get(r);
	free(thiz);
}

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .base_name_get = 		*/ _transition_name,
	/* .free = 			*/ _transition_free,
	/* .bounds_get = 		*/ _transition_bounds_get,
	/* .features_get = 		*/ _transition_features_get,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ NULL,
	/* .alpha_hints_get =		*/ NULL,
	/* .sw_hints_get = 		*/ NULL,
	/* .sw_setup = 			*/ _transition_state_setup,
	/* .sw_cleanup = 		*/ _transition_state_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
	/* .opengl_initialize =        	*/ NULL,
	/* .opengl_setup =          	*/ NULL,
	/* .opengl_cleanup =        	*/ NULL
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
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
	Enesim_Renderer_Transition *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Transition));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_TRANSITION_MAGIC);
	r = enesim_renderer_new(&_descriptor, thiz);

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
	Enesim_Renderer_Transition *t;

	t = _transition_get(r);
	if (level < 0.0000001)
		level = 0;
	if (level > 0.999999)
		level = 1;
	if (t->interp == level)
		return;
	t->interp = 1 + (255 * level);
}
/**
 * Sets the source renderer
 * @param[in] r The transition renderer
 * @param[in] r0 The source renderer
 */
EAPI void enesim_renderer_transition_source_set(Enesim_Renderer *r, Enesim_Renderer *r0)
{
	Enesim_Renderer_Transition *thiz;

	thiz = _transition_get(r);
	if (r0 == r)
		return;
	if (thiz->r0.r == r0)
		return;
	if (thiz->r0.r)
		enesim_renderer_unref(thiz->r0.r);
	thiz->r0.r = r0;
	if (thiz->r0.r)
		thiz->r0.r = enesim_renderer_ref(thiz->r0.r);
}
/**
 * Sets the target renderer
 * @param[in] r The transition renderer
 * @param[in] r1 The target renderer
 */
EAPI void enesim_renderer_transition_target_set(Enesim_Renderer *r, Enesim_Renderer *r1)
{
	Enesim_Renderer_Transition *thiz;

	thiz = _transition_get(r);
	if (r1 == r)
		return;
	if (thiz->r1.r == r1)
		return;
	if (thiz->r1.r)
		enesim_renderer_unref(thiz->r1.r);
	thiz->r1.r = r1;
	if (thiz->r1.r)
		thiz->r1.r = enesim_renderer_ref(thiz->r1.r);
}
/**
 * To be documented
 * FIXME: To be fixed
 * FIXME why do we need this?
 */
EAPI void enesim_renderer_transition_offset_set(Enesim_Renderer *r, int x, int y)
{
	Enesim_Renderer_Transition *t;

	t = _transition_get(r);
	if ((t->offset.x == x) && (t->offset.y == y))
		return;
	t->offset.x = x;
	t->offset.y = y;
}
