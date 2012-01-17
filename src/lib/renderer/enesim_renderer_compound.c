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
#include "Enesim.h"
#include "enesim_private.h"
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
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
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
	Enesim_Renderer_Compound_Setup pre_cb;
	void *pre_data;
	Enesim_Renderer_Compound_Setup post_cb;
	void *post_data;
	/* private */
	Eina_Bool changed : 1;
} Enesim_Renderer_Compound;

typedef struct _Layer
{
	Enesim_Renderer *r;
	/* generated at state setup */
	Eina_Rectangle destination_boundings;
	Enesim_Compositor_Span span;
	Enesim_Renderer_State old;
} Layer;

static inline Enesim_Renderer_Compound * _compound_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Compound *thiz;

	thiz = enesim_renderer_data_get(r);
	ENESIM_RENDERER_COMPOUND_MAGIC_CHECK(thiz);

	return thiz;
}

static void _span(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Compound *thiz;
	Eina_List *ll;
	Eina_Rectangle span;
	uint32_t *tmp;
	uint32_t *dst = ddata;
	size_t tmp_size;

	thiz = _compound_get(r);
	tmp_size = sizeof(uint32_t) * len;
	tmp = alloca(tmp_size);
	eina_rectangle_coords_from(&span, x, y, len, 1);
	for (ll = thiz->layers; ll; ll = eina_list_next(ll))
	{
		Layer *l;
		Enesim_Renderer_Sw_Data *ldata;
		Eina_Rectangle lboundings;
		unsigned int offset;

		l = eina_list_data_get(ll);

		lboundings = l->destination_boundings;
		if (!eina_rectangle_intersection(&lboundings, &span))
		{
			continue;
		}
		ldata = enesim_renderer_backend_data_get(l->r, ENESIM_BACKEND_SOFTWARE);
		offset = lboundings.x - span.x;

		if (!l->span)
		{
			ldata->fill(l->r, lboundings.x, lboundings.y, lboundings.w, dst + offset);
		}
		else
		{
			Enesim_Color color;

			enesim_renderer_color_get(l->r, &color);
			memset(tmp, 0, lboundings.w * sizeof(uint32_t));
			ldata->fill(l->r, lboundings.x, lboundings.y, lboundings.w, tmp);
			l->span(dst + offset, lboundings.w, tmp, color, NULL);
		}
	}
}

static void _span_only_fill(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Compound *thiz;
	Eina_List *ll;
	Eina_Rectangle span;
	uint32_t *dst = ddata;

	thiz = _compound_get(r);
	eina_rectangle_coords_from(&span, x, y, len, 1);
	for (ll = thiz->layers; ll; ll = eina_list_next(ll))
	{
		Layer *l;
		Enesim_Renderer_Sw_Data *ldata;
		Eina_Rectangle lboundings;

		l = eina_list_data_get(ll);

		ldata = enesim_renderer_backend_data_get(l->r, ENESIM_BACKEND_SOFTWARE);
		lboundings = l->destination_boundings;
		if (!eina_rectangle_intersection(&lboundings, &span))
		{
			continue;
		}
		ldata->fill(l->r, lboundings.x, lboundings.y, lboundings.w, dst + (lboundings.x - span.x));
	}
}

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _compound_name(Enesim_Renderer *r)
{
	return "compound";
}

static Eina_Bool _compound_state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Compound *thiz;
	const Enesim_Renderer_State *state = states[ENESIM_STATE_CURRENT];
	Eina_List *ll;
	Eina_Bool only_fill = EINA_TRUE;

	thiz = _compound_get(r);
	/* setup every layer */
	for (ll = thiz->layers; ll; ll = eina_list_next(ll))
	{
		Layer *l = eina_list_data_get(ll);
		Enesim_Color color;
		Enesim_Rop rop;
		Enesim_Format fmt = ENESIM_FORMAT_ARGB8888;

		/* TODO check the flags and only generate the relative properties
		 * for those supported
		 */
		/* the position and the matrix */
		enesim_renderer_relative_set(l->r, state, &l->old);
		if (thiz->pre_cb)
		{
			if (!thiz->pre_cb(r, l->r, thiz->pre_data))
			{
				enesim_renderer_relative_unset(l->r, &l->old);
				break;
			}
		}
		if (!enesim_renderer_setup(l->r, s, error))
		{
			const char *name;

			enesim_renderer_name_get(l->r, &name);
			ENESIM_RENDERER_ERROR(r, error, "Child renderer %s can not setup", name);
			for (; ll && ll != thiz->layers; ll = eina_list_prev(ll))
			{
				Layer *pl = eina_list_data_get(ll);
				enesim_renderer_relative_unset(pl->r, &pl->old);
				enesim_renderer_cleanup(pl->r, s);
				ll = eina_list_prev(ll);
			}

			return EINA_FALSE;
		}
		/* set the span given the color */
		/* FIXME fix the resulting format */
		/* FIXME what about the surface formats here? */
		/* FIXME fix the simplest case (fill) */
		enesim_renderer_rop_get(l->r, &rop);
		enesim_renderer_color_get(l->r, &color);
		if (rop != ENESIM_FILL || color != ENESIM_COLOR_FULL)
		{
			l->span = enesim_compositor_span_get(rop, &fmt, ENESIM_FORMAT_ARGB8888,
					color, ENESIM_FORMAT_NONE);
			only_fill = EINA_FALSE;
		}
		enesim_renderer_destination_boundings(l->r, &l->destination_boundings, 0, 0);
	}
	if (only_fill)
	{
		*fill = _span_only_fill;
	}
	else
	{
		*fill = _span;
	}

	return EINA_TRUE;
}

static void _compound_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Compound *thiz;
	Eina_List *ll;

	thiz = _compound_get(r);
	/* cleanup every layer */
	for (ll = thiz->layers; ll; ll = eina_list_next(ll))
	{
		Layer *l = eina_list_data_get(ll);

		enesim_renderer_relative_unset(l->r, &l->old);
		enesim_renderer_cleanup(l->r, s);
	}
	thiz->changed = EINA_FALSE;
}

/* FIXME this both boundings are wrong
 * because they dont have the setup done when calculating the
 * boundings
 */
static void _compound_boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Compound *thiz;
	Eina_List *ll;
	int x1 = 0;
	int y1 = 0;
	int x2 = 0;
	int y2 = 0;

	thiz = _compound_get(r);
	for (ll = thiz->layers; ll; ll = eina_list_next(ll))
	{
		Layer *l = eina_list_data_get(ll);
		Enesim_Renderer *lr = l->r;
		double nx1, ny1, nx2, ny2;
		Eina_Rectangle tmp;

		enesim_renderer_destination_boundings(lr, &tmp, 0, 0);
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

static void _compound_destination_boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Eina_Rectangle *rect)
{
	Enesim_Rectangle boundings;

	_compound_boundings(r, states, &boundings);
	rect->x = floor(boundings.x);
	rect->y = floor(boundings.y);
	rect->w = ceil(boundings.w);
	rect->h = ceil(boundings.h);
}

static void _compound_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Enesim_Renderer_Compound *thiz;
	Enesim_Renderer_Flag f = 0xffffffff;
	Eina_List *ll;

	thiz = _compound_get(r);
	if (!thiz->layers)
	{
		*flags = 0;
		return;
	}

	for (ll = thiz->layers; ll; ll = eina_list_next(ll))
	{
		Layer *l = eina_list_data_get(ll);
		Enesim_Renderer *lr = l->r;
		Enesim_Renderer_Flag tmp;

		/* intersect with every flag */
		enesim_renderer_flags(lr, &tmp);
		f &= tmp;
	}
	*flags = f;
}

static void _compound_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Compound *thiz;

	thiz = _compound_get(r);
	enesim_renderer_compound_layer_clear(r);
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

static void _compound_damage(Enesim_Renderer *r, Enesim_Renderer_Damage_Cb cb, void *data)
{
	Enesim_Renderer_Compound *thiz;
	Eina_List *ll;
	Layer *l;

	thiz = _compound_get(r);

	EINA_LIST_FOREACH(thiz->layers, ll, l)
	{
		enesim_renderer_damages_get(l->r, cb, data);
	}
}

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _compound_name,
	/* .free = 			*/ _compound_free,
	/* .boundings =  		*/ _compound_boundings,
	/* .destination_boundings = 	*/ _compound_destination_boundings,
	/* .flags = 			*/ _compound_flags,
	/* .is_inside = 		*/ _compound_is_inside,
	/* .damage = 			*/ _compound_damage,
	/* .has_changed = 		*/ _compound_has_changed,
	/* .sw_setup = 			*/ _compound_state_setup,
	/* .sw_cleanup = 		*/ _compound_state_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
	/* .opengl_setup =          	*/ NULL,
	/* .opengl_shader_setup = 	*/ NULL,
	/* .opengl_cleanup =        	*/ NULL
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
 * @param[in] rend The renderer for the new layer
 */
EAPI void enesim_renderer_compound_layer_add(Enesim_Renderer *r,
		Enesim_Renderer *rend)
{
	Enesim_Renderer_Compound *thiz;
	Layer *l;

	if (!rend) return;
	thiz = _compound_get(r);

	l = calloc(1, sizeof(Layer));
	l->r = enesim_renderer_ref(rend);

	thiz->layers = eina_list_append(thiz->layers, l);
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
			enesim_renderer_unref(layer->r);
			thiz->layers = eina_list_remove_list(thiz->layers, l);
			free(layer);
			break;
		}
	}
	thiz->changed = EINA_TRUE;
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
		enesim_renderer_unref(layer->r);
		thiz->layers = eina_list_remove_list(thiz->layers, l);
		free(layer);
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


EAPI void enesim_renderer_compound_pre_setup_set(Enesim_Renderer *r,
		Enesim_Renderer_Compound_Setup cb, void *data)
{
	Enesim_Renderer_Compound *thiz;

	thiz = _compound_get(r);
	thiz->pre_cb = cb;
	thiz->pre_data = data;
}

EAPI void enesim_renderer_compound_post_setup_set(Enesim_Renderer *r,
		Enesim_Renderer_Compound_Setup cb, void *data)
{
	Enesim_Renderer_Compound *thiz;

	thiz = _compound_get(r);
	thiz->post_cb = cb;
	thiz->post_data = data;
}
