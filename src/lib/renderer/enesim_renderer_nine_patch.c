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
#include "enesim_renderer_nine_patch.h"
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
/**
 * This is a nine patch renderer. Instead of modifying the nine patch renderer
 * to support borders, we better use this wrapper. At the end we will
 * have nine surface renderers per patch. Also we need to handle the special
 * case where the coordinates are set to 0,0,w,h
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_renderer_nine_patch

#define ENESIM_RENDERER_NINE_PATCH(o) ENESIM_OBJECT_INSTANCE_CHECK(o,	\
		Enesim_Renderer_Nine_Patch,					\
		enesim_renderer_nine_patch_descriptor_get())

typedef struct _Enesim_Renderer_Nine_Patch_State
{
	Enesim_Surface *s;
	double x, y;
	double w, h;
} Enesim_Renderer_Nine_Patch_State;

typedef struct _Enesim_Renderer_Nine_Patch
{
	Enesim_Renderer parent;
	Enesim_Renderer_Nine_Patch_State current;
	Enesim_Renderer_Nine_Patch_State past;
	/* private */
	Eina_Bool changed : 1;
	Eina_Bool src_changed : 1;
} Enesim_Renderer_Nine_Patch;

typedef struct _Enesim_Renderer_Nine_Patch_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Nine_Patch_Class;

static void _nine_patch_transform_bounds(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Matrix *m,
		Enesim_Matrix_Type type,
		Enesim_Rectangle *obounds,
		Enesim_Rectangle *bounds)
{
	*bounds = *obounds;
	if (type != ENESIM_MATRIX_IDENTITY)
	{
		Enesim_Quad q;

		enesim_matrix_rectangle_transform(m, bounds, &q);
		enesim_quad_rectangle_to(&q, bounds);
	}
}

static void _nine_patch_transform_destination_bounds(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Matrix *tx EINA_UNUSED,
		Enesim_Matrix_Type type EINA_UNUSED,
		Enesim_Rectangle *obounds,
		Eina_Rectangle *bounds)
{
	enesim_rectangle_normalize(obounds, bounds);
}

static Eina_Bool _nine_patch_state_setup(Enesim_Renderer *r, Enesim_Log **l)
{
	Enesim_Renderer_Nine_Patch *thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	if (!thiz->current.s)
	{
		ENESIM_RENDERER_LOG(r, l, "No surface set");
		return EINA_FALSE;
	}

	enesim_surface_lock(thiz->current.s, EINA_FALSE);
	thiz->color = enesim_renderer_color_get(r);
	return EINA_TRUE;
}

static void _nine_patch_state_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Nine_Patch *thiz;
	Eina_Rectangle *sd;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	thiz->changed = EINA_FALSE;
	thiz->src_changed = EINA_FALSE;
	if (thiz->current.s)
		enesim_surface_unlock(thiz->current.s);
	/* swap the states */
	if (thiz->past.s)
	{
		enesim_surface_unref(thiz->past.s);
		thiz->past.s = NULL;
	}
	thiz->past.s = enesim_surface_ref(thiz->current.s);
	thiz->past.x = thiz->current.x;
	thiz->past.y = thiz->current.y;
	thiz->past.w = thiz->current.w;
	thiz->past.h = thiz->current.h;

	EINA_LIST_FREE(thiz->surface_damages, sd)
		free(sd);
}

#if BUILD_OPENGL
static void _nine_patch_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, const Eina_Rectangle *area, int x EINA_UNUSED,
		int y EINA_UNUSED)
{
	Enesim_Renderer_Nine_Patch * thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
}

#endif

static void _nine_patch_sw_span(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Nine_Patch *thiz = ENESIM_RENDERER_NINE_PATCH(r);
}

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _nine_patch_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "nine_patch";
}

static void _nine_patch_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Nine_Patch *thiz;
	Enesim_Rectangle obounds;
	Enesim_Matrix m;
	Enesim_Matrix_Type type;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	if (!thiz->current.s)
	{
		obounds.x = 0;
		obounds.y = 0;
		obounds.w = 0;
		obounds.h = 0;
	}
	else
	{
		double ox;
		double oy;

		enesim_renderer_origin_get(r, &ox, &oy);
		obounds.x = thiz->current.x;
		obounds.y = thiz->current.y;
		obounds.w = thiz->current.w;
		obounds.h = thiz->current.h;
		/* the translate */
		obounds.x += ox;
		obounds.y += oy;
	}
	enesim_renderer_transformation_get(r, &m);
	type = enesim_renderer_transformation_type_get(r);
	_nine_patch_transform_bounds(r, &m, type, &obounds, rect);
}

static void _nine_patch_sw_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Nine_Patch *thiz;

	_nine_patch_state_cleanup(r);

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	thiz->span = NULL;
}

static Eina_Bool _nine_patch_sw_state_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop, 
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Nine_Patch *thiz;
	Enesim_Format fmt;
	Enesim_Matrix m;
	Enesim_Matrix_Type mtype;
	Enesim_Quality quality;
	double x, y, w, h;
	double ox, oy;

	if (!_nine_patch_state_setup(r, l))
		return EINA_FALSE;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);

	enesim_surface_size_get(thiz->current.s, &thiz->sw, &thiz->sh);
	x = thiz->current.x;  y = thiz->current.y;
	w = thiz->current.w;  h = thiz->current.h;

	return EINA_TRUE;
}

static void _nine_patch_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_ARGB8888 |
			ENESIM_RENDERER_FEATURE_QUALITY; 
}

static void _nine_patch_sw_nine_patch_hints(Enesim_Renderer *r, Enesim_Rop rop,
		Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_COLORIZE;
	if (rop != ENESIM_ROP_FILL)
	{
		Enesim_Renderer_Nine_Patch *thiz = ENESIM_RENDERER_NINE_PATCH(r);

		if (thiz->span)
			*hints |= ENESIM_RENDERER_HINT_ROP;
	}

}

static Eina_Bool _nine_patch_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Nine_Patch *thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	if (thiz->src_changed)
	{
		if (thiz->current.s != thiz->past.s)
			return EINA_TRUE;
	}
	if (!thiz->changed) return EINA_FALSE;

	if (thiz->current.x != thiz->past.x)
		return EINA_TRUE;
	if (thiz->current.y != thiz->past.y)
		return EINA_TRUE;
	if (thiz->current.w != thiz->past.w)
		return EINA_TRUE;
	if (thiz->current.h != thiz->past.h)
		return EINA_TRUE;
	return EINA_FALSE;
}

static Eina_Bool _nine_patch_damages(Enesim_Renderer *r,
		const Eina_Rectangle *old_bounds,
		Enesim_Renderer_Damage_Cb cb, void *data)
{
	Enesim_Renderer_Nine_Patch *thiz;
	Eina_Rectangle *sd;
	Eina_Rectangle bounds;


	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	/* if we have changed just send the previous bounds
	 * and the current one
	 */
	if (enesim_renderer_has_changed(r))
	{
		Enesim_Rectangle curr_bounds;

		cb(r, old_bounds, EINA_TRUE, data);
		_nine_patch_bounds_get(r, &curr_bounds);
		enesim_rectangle_normalize(&curr_bounds, &bounds);
		cb(r, &bounds, EINA_FALSE, data);
		return EINA_TRUE;
	}
	/* in other case, send the surface damages tansformed
	 * to destination coordinates
	 */
	else
	{
		Enesim_Matrix m;
		Enesim_Matrix_Type type;
		Eina_List *l;
		Eina_Bool ret = EINA_FALSE;

		enesim_renderer_transformation_get(r, &m);
		type = enesim_renderer_transformation_type_get(r);
		EINA_LIST_FOREACH(thiz->surface_damages, l, sd)
		{
			Enesim_Rectangle sdd;

			enesim_rectangle_coords_from(&sdd, sd->x, sd->y, sd->w, sd->h);
			/* the coordinates are relative to the nine patch */
			sdd.x += thiz->current.x;
			sdd.y += thiz->current.y;
			/* TODO clip it to the source bounds */
			_nine_patch_transform_destination_bounds(r, &m, type, &sdd, &bounds);
			cb(r, &bounds, EINA_FALSE, data);
			ret = EINA_TRUE;
		}
		return ret;
	}
}

#if BUILD_OPENGL
static Eina_Bool _nine_patch_opengl_initialize(Enesim_Renderer *r EINA_UNUSED,
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***programs)
{
	*num_programs = 1;
	*programs = _nine_patch_programs;
	return EINA_TRUE;
}

static Eina_Bool _nine_patch_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l)
{
	Enesim_Renderer_Nine_Patch *thiz;

	if (!_nine_patch_state_setup(r, l)) return EINA_FALSE;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	/* create our own gl texture */
	if (thiz->current.s != thiz->past.s)
	{
		if (thiz->gl.s)
		{
			enesim_surface_unref(thiz->gl.s);
			thiz->gl.s = NULL;
		}
		/* upload the texture if we need to */
		if (enesim_surface_backend_get(thiz->current.s) !=
				ENESIM_BACKEND_OPENGL)
		{
			Enesim_Pool *pool;
			size_t sstride;
			void *sdata;
			int w, h;

			enesim_surface_size_get(thiz->current.s, &w, &h);
			enesim_surface_data_get(thiz->current.s, &sdata,
					&sstride);
			pool = enesim_surface_pool_get(s);
			thiz->gl.s = enesim_surface_new_pool_and_data_from(
					ENESIM_FORMAT_ARGB8888, w, h, pool,
					EINA_TRUE, sdata, sstride, NULL, NULL);
		}
	}

	*draw = _nine_patch_opengl_draw;
	return EINA_TRUE;
}

static void _nine_patch_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	_nine_patch_state_cleanup(r);
}
#endif
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Nine_Patch, Enesim_Renderer_Nine_Patch_Class,
		enesim_renderer_nine_patch);

static void _enesim_renderer_nine_patch_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _nine_patch_name;
	klass->bounds_get = _nine_patch_bounds_get;
	klass->features_get = _nine_patch_features_get;
	klass->damages_get = _nine_patch_damages;
	klass->has_changed = _nine_patch_has_changed;
	klass->sw_hints_get = _nine_patch_sw_nine_patch_hints;
	klass->sw_setup = _nine_patch_sw_state_setup;
	klass->sw_cleanup = _nine_patch_sw_state_cleanup;
#if BUILD_OPENGL
	klass->opengl_initialize = _nine_patch_opengl_initialize;
	klass->opengl_setup = _nine_patch_opengl_setup;
	klass->opengl_cleanup = _nine_patch_opengl_cleanup;
#endif
}

static void _enesim_renderer_nine_patch_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_renderer_nine_patch_instance_deinit(void *o)
{
	Enesim_Renderer_Nine_Patch *thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(o);
	if (thiz->current.s)
		enesim_surface_unref(thiz->current.s);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Create a new nine_patch renderer.
 *
 * @return A new nine_patch renderer.
 *
 * This function returns a newly allocated nine_patch renderer.
 */
EAPI Enesim_Renderer * enesim_renderer_nine_patch_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_nine_patch);
	return r;
}

/**
 * @brief Set the top left X coordinate of a nine_patch renderer.
 *
 * @param[in] r The nine patch renderer.
 * @param[in] x The top left X coordinate.
 *
 * This function sets the top left X coordinate of the nine patch
 * renderer @p r to the value @p x.
 */
EAPI void enesim_renderer_nine_patch_x_set(Enesim_Renderer *r, double x)
{
	Enesim_Renderer_Nine_Patch *thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	if (!thiz) return;
	thiz->current.x = x;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the top left X coordinate of a nine_patch renderer.
 *
 * @param[in] r The nine patch renderer.
 * @return The top left X coordinate.
 *
 * This function gets the top left X coordinate of the nine patch
 * renderer @p r
 */
EAPI double enesim_renderer_nine_patch_x_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Nine_Patch *thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	return thiz->current.x;
}

/**
 * @brief Set the top left Y coordinate of a nine_patch renderer.
 *
 * @param[in] r The nine patch renderer.
 * @param[in] y The top left Y coordinate.
 *
 * This function sets the top left Y coordinate of the nine patch
 * renderer @p r to the value @p y.
 */
EAPI void enesim_renderer_nine_patch_y_set(Enesim_Renderer *r, double y)
{
	Enesim_Renderer_Nine_Patch *thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	if (!thiz) return;
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the top left Y coordinate of a nine_patch renderer.
 *
 * @param[in] r The nine patch renderer.
 * @return The top left Y coordinate.
 *
 * This function gets the top left Y coordinate of the nine patch
 * renderer @p r
 */
EAPI double enesim_renderer_nine_patch_y_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Nine_Patch *thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	return thiz->current.y;
}

/**
 * @brief Set the top left coordinates of a nine_patch renderer.
 *
 * @param[in] r The nine patch renderer.
 * @param[in] x The top left X coordinate.
 * @param[in] y The top left Y coordinate.
 *
 * This function sets the top left coordinates of the nine patch
 * renderer @p r to the values @p x and @p y.
 */
EAPI void enesim_renderer_nine_patch_position_set(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Nine_Patch *thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	if (!thiz) return;
	thiz->current.x = x;
	thiz->current.y = y;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the top left coordinates of a nine_patch renderer.
 *
 * @param[in] r The nine patch renderer.
 * @param[out] x The top left X coordinate.
 * @param[out] y The top left Y coordinate.
 *
 * This function stores the top left coordinates value of the
 * nine_patch renderer @p r in the pointers @p x and @p y. These pointers
 * can be @c NULL.
 */
EAPI void enesim_renderer_nine_patch_position_get(Enesim_Renderer *r, double *x, double *y)
{
	Enesim_Renderer_Nine_Patch *thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	if (!thiz) return;
	if (x) *x = thiz->current.x;
	if (y) *y = thiz->current.y;
}

/**
 * @brief Set the width of a nine_patch renderer.
 *
 * @param[in] r The nine patch renderer.
 * @param[in] w The nine patch width.
 *
 * This function sets the width of the nine patch renderer @p r to the
 * value @p width.
 */
EAPI void enesim_renderer_nine_patch_width_set(Enesim_Renderer *r, double w)
{
	Enesim_Renderer_Nine_Patch *thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	if (!thiz) return;
	thiz->current.w = w;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the width of a nine_patch renderer.
 *
 * @param[in] r The nine patch renderer.
 * @return The nine patch width.
 *
 * This function gets the width of the nine patch renderer @p r
 */
EAPI double enesim_renderer_nine_patch_width_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Nine_Patch *thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	return thiz->current.w;
}

/**
 * @brief Set the height of a nine_patch renderer.
 *
 * @param[in] r The nine patch renderer.
 * @param[in] h The nine patch height.
 *
 * This function sets the height of the nine patch renderer @p r to the
 * value @p height.
 */
EAPI void enesim_renderer_nine_patch_height_set(Enesim_Renderer *r, double h)
{
	Enesim_Renderer_Nine_Patch *thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	if (!thiz) return;
	thiz->current.h = h;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the height of a nine_patch renderer.
 *
 * @param[in] r The nine patch renderer.
 * @return The nine patch height.
 *
 * This function gets the height of the nine patch renderer @p r
 */
EAPI double enesim_renderer_nine_patch_height_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Nine_Patch *thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	return thiz->current.h;
}

/**
 * @brief Set the size of a nine_patch renderer.
 *
 * @param[in] r The nine patch renderer.
 * @param[in] w The width.
 * @param[in] h The height.
 *
 * This function sets the size of the nine patch renderer @p r to the
 * values @p width and @p height.
 */
EAPI void enesim_renderer_nine_patch_size_set(Enesim_Renderer *r, double w, double h)
{
	Enesim_Renderer_Nine_Patch *thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	if (!thiz) return;
	thiz->current.w = w;
	thiz->current.h = h;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Retrieve the size of a nine_patch renderer.
 *
 * @param[in] r The nine patch renderer.
 * @param[out] w The width.
 * @param[out] h The height.
 *
 * This function stores the size of the nine patch renderer @p r in the
 * pointers @p width and @p height. These pointers can be @c NULL.
 */
EAPI void enesim_renderer_nine_patch_size_get(Enesim_Renderer *r, double *w, double *h)
{
	Enesim_Renderer_Nine_Patch *thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	if (!thiz) return;
	if (w) *w = thiz->current.w;
	if (h) *h = thiz->current.h;
}

/**
 * @brief Set the surface used as pixel source for the nine patch renderer
 *
 * @param[in] r The nine patch renderer.
 * @param[in] src The surface to use [transfer full]
 *
 * This function sets the source pixels to use for the nine patch renderer.
 */
EAPI void enesim_renderer_nine_patch_source_surface_set(Enesim_Renderer *r, Enesim_Surface *src)
{
	Enesim_Renderer_Nine_Patch *thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	if (thiz->current.s)
		enesim_surface_unref(thiz->current.s);
	thiz->current.s = src;
	thiz->src_changed = EINA_TRUE;
}

/**
 * @brief Retrieve the surface used as the pixel source for the nine patch renderer
 *
 * @param[in] r The nine patch renderer.
 * @return src The source surface [transfer none]
 *
 * This function returns the surface used as the pixel source
 */
EAPI Enesim_Surface * enesim_renderer_nine_patch_source_surface_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Nine_Patch *thiz;

	thiz = ENESIM_RENDERER_NINE_PATCH(r);
	return enesim_surface_ref(thiz->current.s);
}
