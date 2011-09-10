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
/*
 * The idea here is to make some common code to handle transition
 * (renderers that need a step value to achieve something) renderers.
 */
#include "Enesim.h"
#include "enesim_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Renderer_Transition
{
	/* properties */
	double step;
	Enesim_Renderer *src;
	Enesim_Renderer *tgt;
	/* private */
	void *data;
} Enesim_Renderer_Transition;

static inline Enesim_Renderer_Transition * _transition_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Transition *thiz;

	thiz = enesim_renderer_data_get(r);
	return thiz;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_transition_new(
		Enesim_Renderer_Descriptor *descriptor,
		void *data)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Transition *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Transition));
	thiz->data = data;
	if (!thiz) return NULL;

	r = enesim_renderer_new(descriptor, thiz);
	return r;

}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI void enesim_renderer_transition_step_set(Enesim_Renderer *r, double step)
{
	Enesim_Renderer_Transition *thiz;

	thiz = _transition_get(r);
	thiz->step = step;
}

EAPI void enesim_renderer_transition_step_get(Enesim_Renderer *r, double *step)
{
	Enesim_Renderer_Transition *thiz;

	thiz = _transition_get(r);
	*step = thiz->step;
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
	if (t->r0.r == r0)
		return;
	if (thiz->r0.r)
		enesim_renderer_unref(thiz->r0.r);
	thiz->r0.r = r0;
	if (thiz->r0.r)
		enesim_renderer_ref(thiz->r0.r);
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
		enesim_renderer_ref(thiz->r1.r);
}

