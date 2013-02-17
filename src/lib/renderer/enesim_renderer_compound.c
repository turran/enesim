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
#include "enesim_renderer_compound.h"

#ifdef BUILD_OPENGL
#include "Enesim_OpenGL.h"
#endif

#include "enesim_renderer_private.h"

/**
 * @todo
 * - Handle the case whenever the renderer supports the ROP itself
 * - Might be interesting to add two more functions for this renderer
 *   this functions should set/add some callbacks for pre/post setup/cleanup
 *   Basically the idea is that whenever the renderer is about to do the
 *   setup on the childs, first call the pre setup, and then after do the
 *   post setup, this is useful for cases like esvg or eon where this libraries
 *   wrap in a way a compound renderer but we have to also store the list
 *   of children added, so basically we iterate twice, one the compound itself
 *   and one the compound-wrapper
 * - Another way to optmize this is to make the setup() function of every
 *   layer thread safe, that way given that each layer does not depend on the other
 *   the setup can be done on parallel
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_renderer_compound

#define ENESIM_RENDERER_COMPOUND_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_COMPOUND_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_COMPOUND_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Compound
{
	EINA_MAGIC
	/* properties */
	Eina_List *layers;
	/* private */
	Enesim_Renderer_State rstate;
	Eina_List *visible_layers; /* FIXME maybe is time to change from lists to arrays */
	Eina_List *added;
	Eina_List *removed;

	Enesim_Renderer_Compound_Cb pre_cb;
	void *pre_data;
	Enesim_Renderer_Compound_Cb post_cb;
	void *post_data;

	Eina_Bool changed : 1;
} Enesim_Renderer_Compound;

typedef struct _Layer
{
	Enesim_Renderer *r;
	/* generated at state setup */
	Eina_Rectangle destination_bounds;
} Layer;

static inline Enesim_Renderer_Compound * _compound_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Compound *thiz;

	thiz = enesim_renderer_data_get(r);
	ENESIM_RENDERER_COMPOUND_MAGIC_CHECK(thiz);

	return thiz;
}

static inline void _compound_layer_remove(Enesim_Renderer_Compound *thiz,
		Layer *l)
{
	thiz->removed = eina_list_append(thiz->removed, l);
	thiz->layers = eina_list_remove(thiz->layers, l);
	thiz->changed = EINA_TRUE;
}

static Eina_Bool _compound_state_setup(Enesim_Renderer_Compound *thiz,
		Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Error **error)
{
	Eina_List *ll;
	Layer *l;

	if (thiz->visible_layers)
	{
		eina_list_free(thiz->visible_layers);
		thiz->visible_layers = NULL;
	}
	EINA_LIST_FREE(thiz->added, l)
	{
		/* add the recently added layers to the layers to calculate */
		thiz->layers = eina_list_append(thiz->layers, l);
	}
	/* setup every layer */
	EINA_LIST_FOREACH(thiz->layers, ll, l)
	{
		Eina_Bool visible;

		/* the position and the matrix */
		if (thiz->pre_cb)
		{
			if (!thiz->pre_cb(r, l->r, thiz->pre_data))
			{
				continue;
			}
		}
		if (!enesim_renderer_setup(l->r, s, error))
		{
			const char *name;

			enesim_renderer_name_get(l->r, &name);
			ENESIM_RENDERER_ERROR(r, error, "Child renderer %s can not setup", name);
			continue;
		}
		/* set the span given the color */
		/* FIXME fix the resulting format */
		/* FIXME what about the surface formats here? */
		enesim_renderer_destination_bounds(l->r, &l->destination_bounds, 0, 0);
		if (thiz->post_cb)
		{
			if (!thiz->post_cb(r, l->r, thiz->post_data))
			{
				continue;
			}
		}

		enesim_renderer_visibility_get(l->r, &visible);
		if (!visible) continue;

		/* ok the layer pass the whole pre/post/setup process, add it to the visible layers */
		thiz->visible_layers = eina_list_append(thiz->visible_layers, l);
	}

	return EINA_TRUE;
}

static void _compound_state_cleanup(Enesim_Renderer_Compound *thiz, Enesim_Surface *s)
{
	Layer *layer;
	Eina_List *ll;

	/* cleanup every layer */
	EINA_LIST_FOREACH(thiz->layers, ll, layer)
	{
		enesim_renderer_cleanup(layer->r, s);
	}
	/* free the visible layers list */
	EINA_LIST_FREE(thiz->visible_layers, layer)
	{

	}
	/* remove the removed layers */
	EINA_LIST_FREE(thiz->removed, layer)
	{
		/* now is safe to destroy the layer */
		enesim_renderer_unref(layer->r);
		free(layer);
	}
	thiz->changed = EINA_FALSE;
}

#if BUILD_OPENGL
static void _compound_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		const Eina_Rectangle *area, int w, int h)
{
	Enesim_Renderer_Compound *thiz;
	Enesim_Pool *pool;
	Enesim_Format format;
	Enesim_Surface *tmp;
	Enesim_Buffer_OpenGL_Data *sdata;
	Enesim_Buffer_OpenGL_Data *tmp_sdata;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_Rop rop;
	Eina_List *l;
	GLint viewport[4];
	Layer *layer;
	int sw;
	int sh;

	thiz = _compound_get(r);

	sdata = enesim_surface_backend_data_get(s);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	/* create a temporary texture */
	enesim_surface_size_get(s, &sw, &sh);
	pool = enesim_surface_pool_get(s);
	format = enesim_surface_format_get(s);
	tmp = enesim_surface_new_pool_from(format, sw, sh, pool);

	/* render each layer */
	EINA_LIST_FOREACH(thiz->layers, l, layer)
	{
		enesim_renderer_opengl_draw(layer->r, s, area, w, h);
	}
#if 0
	EINA_LIST_FOREACH(thiz->layers, l, layer)
	{
		enesim_renderer_opengl_draw(layer->r, tmp, area, w, h);
	}
	tmp_sdata = enesim_surface_backend_data_get(tmp);
	/* finally just rop the resulting texture into the real texture */
	glGetIntegerv(GL_VIEWPORT, viewport);
	enesim_renderer_rop_get(r, &rop);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, rdata->fbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			GL_TEXTURE_2D, sdata->texture, 0);
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, -1, 1);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glBindTexture(GL_TEXTURE_2D, tmp_sdata->texture);
	enesim_opengl_rop_set(rop);
	glBegin(GL_QUADS);
		glTexCoord2d(0, 1);
		glVertex2d(area->x, area->y);

		glTexCoord2d(1, 1);
		glVertex2d(area->x + area->w, area->y);

		glTexCoord2d(1, 0);
		glVertex2d(area->x + area->w, area->y + area->h);

		glTexCoord2d(0, 0);
		glVertex2d(area->x, area->y + area->h);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	enesim_opengl_rop_set(ENESIM_FILL);
#endif
	enesim_surface_unref(tmp);

}
#endif

static inline void _compound_span_layer_blend(Enesim_Renderer_Compound *thiz, int x, int y, unsigned int len, void *ddata)
{
	Eina_List *ll;
	Eina_Rectangle span;
	uint32_t *dst = ddata;

	eina_rectangle_coords_from(&span, x, y, len, 1);
	for (ll = thiz->visible_layers; ll; ll = eina_list_next(ll))
	{
		Layer *l;
		Eina_Rectangle lbounds;
		unsigned int offset;

		l = eina_list_data_get(ll);

		lbounds = l->destination_bounds;
		if (!eina_rectangle_intersection(&lbounds, &span))
		{
			continue;
		}

		offset = lbounds.x - span.x;
		enesim_renderer_sw_draw(l->r, lbounds.x, lbounds.y, lbounds.w, dst + offset);
	}
}

/* whenever the compound needs to fill, we need to zeros the whole destination buffer */
static void _compound_fill_span_blend_layer(Enesim_Renderer *r,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Compound *thiz;

	thiz = _compound_get(r);

	/* we might need to add this memset in case the layers for this span dont fill the whole area */
	memset(ddata, 0, len * sizeof(uint32_t));
	_compound_span_layer_blend(thiz, x, y, len, ddata);
}

/* whenever the compound needs to blend, we only need to draw the area of each layer */
static void _compound_blend_span_blend_layer(Enesim_Renderer *r,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Compound *thiz;

	thiz = _compound_get(r);

	/* we should only do one memset, here instead of per each layer */
	_compound_span_layer_blend(thiz, x, y, len, ddata);
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _compound_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "compound";
}

static void _compound_sw_hints(Enesim_Renderer *r,
		Enesim_Renderer_Sw_Hint *hints)
{
	Enesim_Renderer_Compound *thiz;
	Enesim_Renderer_Flag f = 0xffffffff;
	Enesim_Rop rop;
	Eina_Bool same_rop = EINA_TRUE;
	Eina_List *ll;

	thiz = _compound_get(r);
	if (!thiz->layers)
	{
		*hints = 0;
		return;
	}

	/* TODO we need to find an heuristic to set the colorize/rop flag
	 * that reduces the number of raster operations we have to do
	 * (i.e a passthrough)
	 * - if every renderer has the same rop as the compund then
	 * there is no need to do a post composition of the result. but how to
	 * handle the colorize?
	 */
	enesim_renderer_rop_get(r, &rop);
	for (ll = thiz->layers; ll; ll = eina_list_next(ll))
	{
		Layer *l = eina_list_data_get(ll);
		Enesim_Renderer *lr = l->r;
		Enesim_Renderer_Sw_Hint tmp;
		Enesim_Rop lrop;

		/* intersect with every flag */
		enesim_renderer_hints_get(lr, &tmp);
		enesim_renderer_rop_get(lr, &lrop);
		if (lrop != rop)
			same_rop = EINA_FALSE;

		f &= tmp;
	}
	if (same_rop)
		f |= ENESIM_RENDERER_HINT_ROP;
	*hints = f;
}


static Eina_Bool _compound_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Compound *thiz;
	Enesim_Rop rop;

	thiz = _compound_get(r);
	if (!_compound_state_setup(thiz, r, s, error))
		return EINA_FALSE;

	enesim_renderer_rop_get(r, &rop);
	if (rop == ENESIM_FILL)
	{
		*fill = _compound_fill_span_blend_layer;
	}
	else
	{
		*fill = _compound_blend_span_blend_layer;
	}

	return EINA_TRUE;
}

static void _compound_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Compound *thiz;

	thiz = _compound_get(r);
	_compound_state_cleanup(thiz, s);
}

static void _compound_bounds(Enesim_Renderer *r,
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Compound *thiz;
	Layer *l;
	Eina_List *ll;
	int x1 = INT_MAX;
	int y1 = INT_MAX;
	int x2 = -INT_MAX;
	int y2 = -INT_MAX;

	thiz = _compound_get(r);
	/* first the just added layers */
	EINA_LIST_FOREACH (thiz->added, ll, l)
	{
		Enesim_Renderer *lr = l->r;
		int nx1, ny1, nx2, ny2;
		Eina_Rectangle tmp;

		enesim_renderer_destination_bounds(lr, &tmp, 0, 0);
		nx1 = tmp.x;
		ny1 = tmp.y;
		nx2 = tmp.x + tmp.w;
		ny2 = tmp.y + tmp.h;

		/* pick the bigger area */
		if (nx1 < x1) x1 = nx1;
		if (ny1 < y1) y1 = ny1;
		if (nx2 > x2) x2 = nx2;
		if (ny2 > y2) y2 = ny2;
	}
	/* now the already there layers */
	EINA_LIST_FOREACH (thiz->layers, ll, l)
	{
		Enesim_Renderer *lr = l->r;
		int nx1, ny1, nx2, ny2;
		Eina_Rectangle tmp;

		enesim_renderer_destination_bounds(lr, &tmp, 0, 0);
		nx1 = tmp.x;
		ny1 = tmp.y;
		nx2 = tmp.x + tmp.w;
		ny2 = tmp.y + tmp.h;

		/* pick the bigger area */
		if (nx1 < x1) x1 = nx1;
		if (ny1 < y1) y1 = ny1;
		if (nx2 > x2) x2 = nx2;
		if (ny2 > y2) y2 = ny2;
	}
	rect->x = x1;
	rect->y = y1;
	rect->w = x2 - x1;
	rect->h = y2 - y1;
}

static void _compound_destination_bounds(Enesim_Renderer *r,
		Eina_Rectangle *bounds)
{
	Enesim_Rectangle obounds;

	_compound_bounds(r, &obounds);
	bounds->x = floor(obounds.x);
	bounds->y = floor(obounds.y);
	bounds->w = ceil(obounds.x - bounds->x + obounds.w);
	bounds->h = ceil(obounds.y - bounds->y + obounds.h);
}

static void _compound_flags(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Flag *flags)
{
	/* TODO here we should only support the destination formats of the surfaces */
	*flags = 0;
}

static void _compound_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Compound *thiz;
	Layer *l;

	thiz = _compound_get(r);
	/* just remove the visible layers as is, every visible layer
	 * should be part of the compound layers
	 */
	if (thiz->visible_layers)
	{
		eina_list_free(thiz->visible_layers);
		thiz->visible_layers = NULL;
	}
	/* check the added lists, it must be empty, if not remove it too */
	EINA_LIST_FREE(thiz->added, l)
	{
		enesim_renderer_unref(l->r);
		free(l);
	}
	enesim_renderer_compound_layer_clear(r);
	/* finally unref every removed layer */
	EINA_LIST_FREE(thiz->removed, l)
	{
		/* now is safe to destroy the layer */
		enesim_renderer_unref(l->r);
		free(l);
	}

	free(thiz);
}

static Eina_Bool _compound_is_inside(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Compound *thiz;
	Eina_List *ll;
	Eina_Bool is_inside = EINA_FALSE;

	thiz = _compound_get(r);
	for (ll = thiz->layers; ll; ll = eina_list_next(ll))
	{
		Layer *l = eina_list_data_get(ll);
		Enesim_Renderer *lr = l->r;

		/* intersect with every flag */
		is_inside = enesim_renderer_is_inside(lr, x, y);
		if (is_inside) break;
	}
	return is_inside;
}

static Eina_Bool _compound_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Compound *thiz;
	Eina_Bool ret;

	thiz = _compound_get(r);
	ret = thiz->changed;
	if (!ret)
	{
		Layer *l;
		Eina_List *ll;

		EINA_LIST_FOREACH(thiz->layers, ll, l)
		{
			if ((ret = enesim_renderer_has_changed(l->r)))
			{
				const char *child_name;

				enesim_renderer_name_get(l->r, &child_name);
				DBG("The child %s has changed", child_name);
				break;
			}
		}
	}
	return ret;
}

static void _compound_damage(Enesim_Renderer *r,
		const Eina_Rectangle *old_bounds,
		Enesim_Renderer_Damage_Cb cb, void *data)
{
	Enesim_Renderer_Compound *thiz;
	Eina_List *ll;
	Eina_Bool common_changed;
	Layer *l;

	thiz = _compound_get(r);
	common_changed = enesim_renderer_state_has_changed(r);
	/* given that we do support the visibility, color, rop, we need to take into
	 * account such change
	 */
	if (common_changed)
	{
		Eina_Rectangle current_bounds;

		enesim_renderer_destination_bounds(r, &current_bounds, 0, 0);
		cb(r, old_bounds, EINA_TRUE, data);
		cb(r, &current_bounds, EINA_FALSE, data);
		return;
	}
	/* if some layer has been added or removed after the last draw
	 * we need to inform of those areas even if those have damages
	 * or not
	 */
	else if (thiz->changed)
	{
		EINA_LIST_FOREACH(thiz->removed, ll, l)
		{
			cb(l->r, &l->destination_bounds, EINA_FALSE, data);
		}
		EINA_LIST_FOREACH(thiz->added, ll, l)
		{
			Eina_Rectangle db;

			enesim_renderer_destination_bounds(l->r, &db, 0, 0);
			cb(l->r, &db, EINA_FALSE, data);
		}
	}
	/* now for every layer send the damage */
	EINA_LIST_FOREACH(thiz->layers, ll, l)
	{
		enesim_renderer_damages_get(l->r, cb, data);
	}
}

#if BUILD_OPENGL
static Eina_Bool _compound_opengl_initialize(Enesim_Renderer *r EINA_UNUSED,
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***programs)
{
#if 0
	*num_programs = 1;
	*programs = _compound_programs;
#endif
	return EINA_TRUE;
}

static Eina_Bool _compound_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Error **error)
{
	Enesim_Renderer_Compound *thiz;

 	thiz = _compound_get(r);
	if (!_compound_state_setup(thiz, r, s, error)) return EINA_FALSE;

	*draw = _compound_opengl_draw;
	return EINA_TRUE;
}

static void _compound_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Compound *thiz;

 	thiz = _compound_get(r);
	_compound_state_cleanup(thiz, s);
}
#endif

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _compound_name,
	/* .free = 			*/ _compound_free,
	/* .bounds_get =  		*/ _compound_bounds,
	/* .destination_bounds_get = 	*/ _compound_destination_bounds,
	/* .flags = 			*/ _compound_flags,
	/* .is_inside = 		*/ _compound_is_inside,
	/* .damage = 			*/ _compound_damage,
	/* .has_changed = 		*/ _compound_has_changed,
	/* .sw_hints_get = 		*/ _compound_sw_hints,
	/* .sw_setup = 			*/ _compound_sw_setup,
	/* .sw_cleanup = 		*/ _compound_sw_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
#if BUILD_OPENGL
	/* .opengl_initialize = 	*/ _compound_opengl_initialize,
	/* .opengl_setup = 		*/ _compound_opengl_setup,
	/* .opengl_cleanup = 		*/ _compound_opengl_cleanup,
#else
	/* .opengl_initialize = 	*/ NULL,
	/* .opengl_setup =          	*/ NULL,
	/* .opengl_cleanup =        	*/ NULL
#endif
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Creates a compound renderer
 * @return The new renderer
 */
EAPI Enesim_Renderer * enesim_renderer_compound_new(void)
{
	Enesim_Renderer_Compound *thiz;
	Enesim_Renderer *r;

	thiz = calloc(1, sizeof(Enesim_Renderer_Compound));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_COMPOUND_MAGIC);

	r = enesim_renderer_new(&_descriptor, thiz);

	return r;
}
/**
 * Adds a layer
 * @param[in] r The compound renderer
 * @param[in] rend The renderer for the new layer [transfer full]
 */
EAPI void enesim_renderer_compound_layer_add(Enesim_Renderer *r,
		Enesim_Renderer *rend)
{
	Enesim_Renderer_Compound *thiz;
	Layer *l;

	if (!rend) return;
	thiz = _compound_get(r);

	l = calloc(1, sizeof(Layer));
	l->r = rend;
	thiz->added = eina_list_append(thiz->added, l);
	thiz->changed = EINA_TRUE;
}

/**
 *
 */
EAPI void enesim_renderer_compound_layer_remove(Enesim_Renderer *r,
		Enesim_Renderer *rend)
{
	Enesim_Renderer_Compound *thiz;
	Layer *layer;
	Eina_List *l;
	Eina_List *l_next;

	if (!rend) return;
	thiz = _compound_get(r);

	EINA_LIST_FOREACH_SAFE(thiz->layers, l, l_next, layer)
	{
		if (layer->r == rend)
		{
			_compound_layer_remove(thiz, layer);
			break;
		}
	}
}

/**
 * Clears up all the layers
 * @param[in] r The compound renderer
 */
EAPI void enesim_renderer_compound_layer_clear(Enesim_Renderer *r)
{
	Enesim_Renderer_Compound *thiz;
	Layer *layer;
	Eina_List *l;
	Eina_List *l_next;

	thiz = _compound_get(r);
	EINA_LIST_FOREACH_SAFE(thiz->layers, l, l_next, layer)
	{
		_compound_layer_remove(thiz, layer);
	}
	thiz->changed = EINA_TRUE;
}

/**
 *
 */
EAPI void enesim_renderer_compound_layer_set(Enesim_Renderer *r,
		Eina_List *list)
{
	Enesim_Renderer *rend;
	Eina_List *l;

	enesim_renderer_compound_layer_clear(r);
	EINA_LIST_FOREACH(list, l, rend)
	{
		enesim_renderer_compound_layer_add(r, rend);
	}
}

/**
 *
 */
EAPI void enesim_renderer_compound_layer_foreach(Enesim_Renderer *r,
		Enesim_Renderer_Compound_Cb cb, void *data)
{
	Enesim_Renderer_Compound *thiz;
	Layer *layer;
	Eina_List *l;

	thiz = _compound_get(r);
	EINA_LIST_FOREACH(thiz->layers, l, layer)
	{
		if (!cb(r, layer->r, data))
			break;
	}
}

/**
 *
 */
EAPI void enesim_renderer_compound_layer_reverse_foreach(Enesim_Renderer *r,
		Enesim_Renderer_Compound_Cb cb, void *data)
{
	Enesim_Renderer_Compound *thiz;
	Layer *layer;
	Eina_List *l;

	thiz = _compound_get(r);
	EINA_LIST_REVERSE_FOREACH(thiz->layers, l, layer)
	{
		if (!cb(r, layer->r, data))
			break;
	}
}

/**
 *
 */
EAPI void enesim_renderer_compound_pre_setup_set(Enesim_Renderer *r,
		Enesim_Renderer_Compound_Cb cb, void *data)
{
	Enesim_Renderer_Compound *thiz;

	thiz = _compound_get(r);
	thiz->pre_cb = cb;
	thiz->pre_data = data;
}

/**
 *
 */
EAPI void enesim_renderer_compound_post_setup_set(Enesim_Renderer *r,
		Enesim_Renderer_Compound_Cb cb, void *data)
{
	Enesim_Renderer_Compound *thiz;

	thiz = _compound_get(r);
	thiz->post_cb = cb;
	thiz->post_data = data;
}
