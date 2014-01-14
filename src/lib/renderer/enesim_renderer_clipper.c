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

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_clipper.h"
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
#define ENESIM_RENDERER_CLIPPER(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Clipper,					\
		enesim_renderer_clipper_descriptor_get())

typedef struct _Enesim_Renderer_Clipper_Damage_Data
{
	Eina_Rectangle *bounds;
	Enesim_Renderer_Damage_Cb real_cb;
	void *real_data;
} Enesim_Renderer_Clipper_Damage_Data;

typedef struct _Enesim_Renderer_Clipper_State
{
	Enesim_Renderer *clipped;
	double width;
	double height;
} Enesim_Renderer_Clipper_State;

typedef struct _Enesim_Renderer_Clipper {
	Enesim_Renderer parent;
	/* the properties */
	Enesim_Renderer_Clipper_State current;
	Enesim_Renderer_Clipper_State past;
	/* generated at state setup */
	Enesim_Renderer_Sw_Fill content_fill;
	/* private */
	Eina_Bool changed : 1;
} Enesim_Renderer_Clipper;

typedef struct _Enesim_Renderer_Clipper_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Clipper_Class;

static void _clipper_span(Enesim_Renderer *r,
		int x, int y, int len, void *dst)
{
	Enesim_Renderer_Clipper *thiz;

 	thiz = ENESIM_RENDERER_CLIPPER(r);
	enesim_renderer_sw_draw(thiz->current.clipped, x, y, len, dst);
}

#if BUILD_OPENGL
static void _clipper_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, const Eina_Rectangle *area, int x, int y)
{
	Enesim_Renderer_Clipper *thiz;

 	thiz = ENESIM_RENDERER_CLIPPER(r);
	enesim_renderer_opengl_draw(thiz->current.clipped, s, rop, area, x, y);
}
#endif


/* called from the optimized case of the damages to just clip the damages
 * to our own bounds
 */
static Eina_Bool _clipper_damage_cb(Enesim_Renderer *r,
		const Eina_Rectangle *area, Eina_Bool past, void *data)
{
	Enesim_Renderer_Clipper_Damage_Data *ddata = data;
	Eina_Rectangle new_area = *area;

	/* here we just intersect the damages with our bounds */
	if (eina_rectangle_intersection(&new_area, ddata->bounds))
		ddata->real_cb(r, &new_area, past, ddata->real_data);
	return EINA_TRUE;
}

static Eina_Bool _clipper_changed_basic(Enesim_Renderer_Clipper *thiz)
{
	if (!thiz->changed) return EINA_FALSE;

	if (thiz->current.width != thiz->past.width)
		return EINA_TRUE;

	if (thiz->current.height != thiz->past.height)
		return EINA_TRUE;

	if (thiz->current.clipped != thiz->past.clipped)
		return EINA_TRUE;
	return EINA_FALSE;
}

static Eina_Bool _clipper_state_setup(Enesim_Renderer_Clipper *thiz,
		Enesim_Renderer *r, Enesim_Surface *s, Enesim_Rop rop, Enesim_Log **l)
{
	if (!thiz->current.clipped)
	{
		ENESIM_RENDERER_LOG(r, l, "No clipped");
		return EINA_FALSE;
	}
	if (!enesim_renderer_setup(thiz->current.clipped, s, rop, l))
	{
		ENESIM_RENDERER_LOG(r, l, "Content renderer %s can not setup",
				enesim_renderer_name_get(thiz->current.clipped));
		return EINA_FALSE;
	}

	return EINA_TRUE;
}

static void _clipper_state_cleanup(Enesim_Renderer_Clipper *thiz,
		Enesim_Surface *s)
{
	thiz->changed = EINA_FALSE;
	thiz->past = thiz->current;

	if (!thiz->current.clipped) return;
	enesim_renderer_cleanup(thiz->current.clipped, s);
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _clipper_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "clipper";
}

static Eina_Bool _clipper_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l)
{
	Enesim_Renderer_Clipper *thiz;

 	thiz = ENESIM_RENDERER_CLIPPER(r);
	if (!_clipper_state_setup(thiz, r, s, rop, l))
		return EINA_FALSE;
	*fill = _clipper_span;
	return EINA_TRUE;
}

static void _clipper_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Clipper *thiz;

 	thiz = ENESIM_RENDERER_CLIPPER(r);
	_clipper_state_cleanup(thiz, s);
}

static void _clipper_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE;
}

static void _clipper_sw_hints_get(Enesim_Renderer *r,
		Enesim_Rop rop, Enesim_Renderer_Sw_Hint *hints)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = ENESIM_RENDERER_CLIPPER(r);
	*hints = 0;
	if (thiz->current.clipped)
	{
		Enesim_Renderer_Sw_Hint content_hints;
		Enesim_Color color, own_color;

		enesim_renderer_sw_hints_get(thiz->current.clipped, rop, &content_hints);

		if (content_hints & ENESIM_RENDERER_HINT_ROP)
			*hints |= ENESIM_RENDERER_HINT_ROP;
		color = enesim_renderer_color_get(thiz->current.clipped);
		own_color = enesim_renderer_color_get(r);
		if ((own_color == color) && (content_hints & ENESIM_RENDERER_HINT_COLORIZE))
			*hints |= ENESIM_RENDERER_HINT_COLORIZE;
	}
}

static void _clipper_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = ENESIM_RENDERER_CLIPPER(r);
	enesim_renderer_origin_get(r, &rect->x, &rect->y);
	rect->w = thiz->current.width;
	rect->h = thiz->current.height;
}

static Eina_Bool _clipper_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Clipper *thiz;
	Eina_Bool ret = EINA_FALSE;

	thiz = ENESIM_RENDERER_CLIPPER(r);
	if (thiz->current.clipped)
	{
		ret = enesim_renderer_has_changed(thiz->current.clipped);
		if (ret) goto end;
	}

	ret = _clipper_changed_basic(thiz);
end:
	return ret;
}

static Eina_Bool _clipper_damage(Enesim_Renderer *r,
		const Eina_Rectangle *old_bounds,
		Enesim_Renderer_Damage_Cb cb, void *data)
{
	Enesim_Renderer_Clipper *thiz;
	Eina_Rectangle current_bounds;

	thiz = ENESIM_RENDERER_CLIPPER(r);

	/* get the current bounds */
	enesim_renderer_destination_bounds_get(r, &current_bounds, 0, 0);
	/* if we have changed then send the old and the current */
	/* FIXME we use the origin but dont take care of the origin property here */
	if (_clipper_changed_basic(thiz))
	{
		cb(r, old_bounds, EINA_TRUE, data);
		cb(r, &current_bounds, EINA_FALSE, data);
		return EINA_TRUE;
	}
	/* if not, send the clipped only */
	else
	{
		Enesim_Renderer_Clipper_Damage_Data ddata;

		if (!thiz->current.clipped) return EINA_FALSE;
		ddata.real_cb = cb;
		ddata.real_data = data;
		ddata.bounds = &current_bounds;

		return enesim_renderer_damages_get(thiz->current.clipped, _clipper_damage_cb, &ddata);
	}
}

#if BUILD_OPENGL
static Eina_Bool _clipper_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l)
{
	Enesim_Renderer_Clipper *thiz;

 	thiz = ENESIM_RENDERER_CLIPPER(r);
	if (!_clipper_state_setup(thiz, r, s, rop, l))
		return EINA_FALSE;

	*draw = _clipper_opengl_draw;
	return EINA_TRUE;
}

static void _clipper_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Clipper *thiz;

 	thiz = ENESIM_RENDERER_CLIPPER(r);
	_clipper_state_cleanup(thiz, s);
}
#endif
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Clipper, Enesim_Renderer_Clipper_Class,
		enesim_renderer_clipper);

static void _enesim_renderer_clipper_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _clipper_name;
	klass->bounds_get = _clipper_bounds_get;
	klass->features_get = _clipper_features_get;
	klass->damages_get = _clipper_damage;
	klass->has_changed = _clipper_has_changed;
	klass->sw_hints_get = _clipper_sw_hints_get;
	klass->sw_setup = _clipper_sw_setup;
	klass->sw_cleanup = _clipper_sw_cleanup;
#if BUILD_OPENGL
	klass->opengl_setup = _clipper_opengl_setup;
	klass->opengl_cleanup = _clipper_opengl_cleanup;
#endif
}

static void _enesim_renderer_clipper_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_renderer_clipper_instance_deinit(void *o)
{
	Enesim_Renderer_Clipper *thiz = ENESIM_RENDERER_CLIPPER(o);
	if (thiz->current.clipped)
		enesim_renderer_unref(thiz->current.clipped);
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Creates a clipper renderer
 * @return The new renderer
 */
EAPI Enesim_Renderer * enesim_renderer_clipper_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_clipper);
	return r;
}

/**
 * @brief Sets the clipped renderer
 * @param[in] r The clipper renderer to set the clip to
 * @param[in] clipped The renderer to clip [transfer full]
 */
EAPI void enesim_renderer_clipper_clipped_set(Enesim_Renderer *r,
		Enesim_Renderer *clipped)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = ENESIM_RENDERER_CLIPPER(r);
	if (thiz->current.clipped)
		enesim_renderer_unref(thiz->current.clipped);
	thiz->current.clipped = clipped;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the clipped renderer
 * @param[in] r The clipper renderer to set the clip to
 * @return The clipped renderer [transfer none]
 */
EAPI Enesim_Renderer * enesim_renderer_clipper_clipped_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = ENESIM_RENDERER_CLIPPER(r);
	return enesim_renderer_ref(thiz->current.clipped);
}

/**
 * @brief Sets the width of the clipper
 * @param[in] r The clipper renderer to set width to
 * @param[in] width The width
 */
EAPI void enesim_renderer_clipper_width_set(Enesim_Renderer *r,
		double width)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = ENESIM_RENDERER_CLIPPER(r);
	thiz->current.width = width;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Sets the width of the clipper
 * @param[in] r The clipper renderer to set width to
 * @return width The width
 */
EAPI double enesim_renderer_clipper_width_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = ENESIM_RENDERER_CLIPPER(r);
	return thiz->current.width;
}

/**
 * @brief Sets the height of the clipper
 * @param[in] r The clipper renderer to set height to
 * @param[in] height The height
 */
EAPI void enesim_renderer_clipper_height_set(Enesim_Renderer *r,
		double height)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = ENESIM_RENDERER_CLIPPER(r);
	thiz->current.height = height;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Sets the height of the clipper
 * @param[in] r The clipper renderer to set height to
 * @return height The height
 */
EAPI double enesim_renderer_clipper_height_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Clipper *thiz;

	thiz = ENESIM_RENDERER_CLIPPER(r);
	return thiz->current.height;
}

