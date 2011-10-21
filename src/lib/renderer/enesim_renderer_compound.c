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
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Renderer_Compound
{
	Eina_List *layers;
} Enesim_Renderer_Compound;

typedef struct _Layer
{
	Enesim_Renderer *r;
	/* generated at state setup */
	Enesim_Compositor_Span span;
	Enesim_Matrix original;
	double ox, oy;
} Layer;

static inline Enesim_Renderer_Compound * _compound_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Compound *thiz;

	thiz = enesim_renderer_data_get(r);
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

		enesim_renderer_destination_boundings(l->r, &lboundings, 0, 0);
		if (!eina_rectangle_intersection(&lboundings, &span))
		{
			continue;
		}
		ldata = l->r->backend_data[ENESIM_BACKEND_SOFTWARE];
		offset = lboundings.x - span.x;

		if (!l->span)
		{
			ldata->fill(l->r, lboundings.x, lboundings.y, lboundings.w, dst + offset);
		}
		else
		{
			memset(tmp, 0, lboundings.w * sizeof(uint32_t));
			ldata->fill(l->r, lboundings.x, lboundings.y, lboundings.w, tmp);
			l->span(dst + offset, lboundings.w, tmp, l->r->color, NULL);
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

		ldata = l->r->backend_data[ENESIM_BACKEND_SOFTWARE];
		enesim_renderer_destination_boundings(l->r, &lboundings, 0, 0);
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

static Eina_Bool _compound_state_setup(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Compound *thiz;
	Eina_List *ll;
	Eina_Bool only_fill = EINA_TRUE;

	thiz = _compound_get(r);
	/* setup every layer */
	for (ll = thiz->layers; ll; ll = eina_list_next(ll))
	{
		Layer *l = eina_list_data_get(ll);
		Enesim_Format fmt = ENESIM_FORMAT_ARGB8888;

		/* TODO check the flags and only generate the relative properties
		 * for those supported
		 */
		/* the position and the matrix */
		enesim_renderer_relative_set(r, l->r, &l->original, &l->ox, &l->oy);
		if (!enesim_renderer_sw_setup(l->r, s, error))
		{
			const char *name;

			enesim_renderer_name_get(l->r, &name);
			ENESIM_RENDERER_ERROR(r, error, "Child renderer %s can not setup", name);
			for (; ll && ll != thiz->layers; ll = eina_list_prev(ll))
			{
				Layer *pl = eina_list_data_get(ll);
				enesim_renderer_relative_unset(r, pl->r, &l->original,
						 l->ox, l->oy);
				ll = eina_list_prev(ll);
			}

			return EINA_FALSE;
		}
		/* set the span given the color */
		/* FIXME fix the resulting format */
		/* FIXME what about the surface formats here? */
		/* FIXME fix the simplest case (fill) */
		if (l->r->rop != ENESIM_FILL || l->r->color != ENESIM_COLOR_FULL)
		{
			l->span = enesim_compositor_span_get(l->r->rop, &fmt, ENESIM_FORMAT_ARGB8888,
					l->r->color, ENESIM_FORMAT_NONE);
			only_fill = EINA_FALSE;
		}
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

static void _compound_state_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Compound *thiz;
	Eina_List *ll;

	thiz = _compound_get(r);
	/* cleanup every layer */
	for (ll = thiz->layers; ll; ll = eina_list_next(ll))
	{
		Layer *l = eina_list_data_get(ll);

		enesim_renderer_sw_cleanup(l->r);
		enesim_renderer_relative_unset(r, l->r, &l->original, l->ox, l->oy);
	}
}

static void _compound_boundings(Enesim_Renderer *r, Enesim_Rectangle *rect)
{
	Enesim_Renderer_Compound *thiz;
	Eina_List *ll;
	double x1 = 0;
	double y1 = 0;
	double x2 = 0;
	double y2 = 0;

	thiz = _compound_get(r);
	for (ll = thiz->layers; ll; ll = eina_list_next(ll))
	{
		Layer *l = eina_list_data_get(ll);
		Enesim_Renderer *lr = l->r;
		double nx1, ny1, nx2, ny2;
		Enesim_Rectangle tmp;

		enesim_renderer_boundings(lr, &tmp);
		nx1 = tmp.x;
		ny1 = tmp.y;
		nx2 = tmp.x + tmp.w - 1;
		ny2 = tmp.y + tmp.h - 1;

		/* pick the bigger area */
		if (nx1 < x1) x1 = nx1;
		if (ny1 < y1) y1 = ny1;
		if (nx2 > x2) x2 = nx2;
		if (ny2 > y2) y2 = ny2;
	}
	rect->x = x1;
	rect->y = y1;
	rect->w = x2 - x1 + 1;
	rect->h = y2 - y1 + 1;
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

static Enesim_Renderer_Descriptor _descriptor = {
	/* .version =    */ ENESIM_RENDERER_API,
	/* .name =       */ _compound_name,
	/* .free =       */ _compound_free,
	/* .boundings =  */ _compound_boundings,
	/* .flags =      */ _compound_flags,
	/* .is_inside =  */ _compound_is_inside,
	/* .damage =     */ NULL,
	/* .sw_setup =   */ _compound_state_setup,
	/* .sw_cleanup = */ _compound_state_cleanup
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
