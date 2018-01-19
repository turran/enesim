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
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_background.h"
#include "enesim_renderer_compound.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#ifdef BUILD_OPENGL
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
#endif

#ifdef BUILD_OPENCL
#include "Enesim_OpenCL.h"
#endif

#include "enesim_renderer_private.h"
#include "enesim_buffer_private.h"
#include "enesim_surface_private.h"

/**
 * @todo
 * - Handle the case whenever the renderer supports the ROP itself
 * - Another way to optmize this is to make the setup() function of every
 *   layer thread safe, that way given that each layer does not depend on the other
 *   the setup can be done on parallel
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_LOG_DEFAULT enesim_log_renderer_compound

#define ENESIM_RENDERER_COMPOUND(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Compound,					\
		enesim_renderer_compound_descriptor_get())

struct _Enesim_Renderer_Compound_Layer
{
	Enesim_Renderer *r;
	Enesim_Renderer *owner;
	Enesim_Rop rop;
	int ref;
	/* generated at state setup */
	Eina_Rectangle destination_bounds;
};

typedef struct _Enesim_Renderer_Compound
{
	Enesim_Renderer parent;
	/* properties */
	Eina_List *layers;
	/* private */
	Enesim_Renderer_State rstate;
	Eina_List *visible_layers; /* FIXME maybe is time to change from lists to arrays */
	Eina_List *added;
	Eina_List *removed;
#if BUILD_OPENGL
	Enesim_Surface *opengl_s;
#endif
#if BUILD_OPENCL
	Enesim_Surface *opencl_s;
#endif
	Enesim_Renderer_Compound_Layer background;
	Enesim_Renderer *cur_background;
	Enesim_Renderer *prev_background;

	Eina_Bool changed : 1;
	Eina_Bool background_enabled : 1;
} Enesim_Renderer_Compound;

typedef struct _Enesim_Renderer_Compound_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Compound_Class;

static inline void _compound_layer_remove(Enesim_Renderer_Compound *thiz,
		Enesim_Renderer_Compound_Layer *l)
{
	thiz->removed = eina_list_append(thiz->removed, l);
	thiz->changed = EINA_TRUE;
}

static inline void _compound_layer_sw_hints_merge(Enesim_Renderer_Compound_Layer *l, Enesim_Rop rop,
		Eina_Bool *same_rop, Enesim_Renderer_Sw_Hint *h)
{
	Enesim_Renderer_Sw_Hint tmp;

	/* intersect with every flag */
	enesim_renderer_sw_hints_get(l->r, l->rop, &tmp);
	if (l->rop != rop)
		*same_rop = EINA_FALSE;
	*h &= tmp;
}

static inline void _compound_layer_span_draw(Enesim_Renderer_Compound_Layer *l, Eina_Rectangle *span, void *ddata)
{
	Eina_Rectangle lbounds;
	uint32_t *dst = ddata;
	unsigned int offset;

	lbounds = l->destination_bounds;
	if (!eina_rectangle_intersection(&lbounds, span))
	{
		DBG("Drawing span %" EINA_RECTANGLE_FORMAT " and bounds %"
				EINA_RECTANGLE_FORMAT " do not intersect on "
				"'%s'", EINA_RECTANGLE_ARGS (span),
				EINA_RECTANGLE_ARGS (&lbounds),
				l->r->name);
		return;
	}

	DBG("Drawing span %" EINA_RECTANGLE_FORMAT " in '%s' with bounds %"
			EINA_RECTANGLE_FORMAT, EINA_RECTANGLE_ARGS (span),
			l->r->name,  EINA_RECTANGLE_ARGS (&lbounds));
	offset = lbounds.x - span->x;
	enesim_renderer_sw_draw(l->r, lbounds.x, lbounds.y, lbounds.w, dst + offset);
}

static Eina_Bool _compound_state_setup(Enesim_Renderer_Compound *thiz,
		Enesim_Renderer *r, Enesim_Surface *s, Enesim_Rop rop EINA_UNUSED,
		Enesim_Log **l)
{
	Enesim_Renderer_Compound_Layer *layer;
	Eina_List *ll;

	/* setup the background */
	if (thiz->background_enabled)
	{
		if (!enesim_renderer_setup(thiz->background.r, s, thiz->background.rop, l))
		{
			ENESIM_RENDERER_LOG(r, l, "Background renderer can not setup");
			return EINA_FALSE;
		}
		enesim_renderer_destination_bounds_get(thiz->background.r,
				&thiz->background.destination_bounds, 0, 0, NULL);
	}

	if (thiz->visible_layers)
	{
		eina_list_free(thiz->visible_layers);
		thiz->visible_layers = NULL;
	}
	EINA_LIST_FREE(thiz->added, layer)
	{
		/* add the recently added layers to the layers to calculate */
		thiz->layers = eina_list_append(thiz->layers, layer);
	}
	/* setup every layer */
	EINA_LIST_FOREACH(thiz->layers, ll, layer)
	{
		Eina_Bool visible;

		if (!enesim_renderer_setup(layer->r, s, layer->rop, NULL))
		{
			ENESIM_RENDERER_LOG(r, l, "Layer '%s' can not setup",
					enesim_renderer_name_get(layer->r));
			DBG("Layer '%s' on '%s' failed to setup",
					layer->r->name, r->name);
			continue;
		}
		/* set the span given the color */
		/* FIXME fix the resulting format */
		/* FIXME what about the surface formats here? */
		enesim_renderer_destination_bounds_get(layer->r,
				&layer->destination_bounds, 0, 0, NULL);
		visible = enesim_renderer_visibility_get(layer->r);
		if (!visible)
		{
			DBG("Layer '%s' on '%s' not visible, not adding it",
					layer->r->name, r->name);
			continue;
		}

		/* ok the layer pass the whole pre/post/setup process, add it to the visible layers */
		DBG("Adding layer '%s' on '%s'", layer->r->name, r->name);
		thiz->visible_layers = eina_list_append(thiz->visible_layers, layer);
	}

	/* TODO in case every layer failed and no background enabled, is an error */
	return EINA_TRUE;
}

static void _compound_state_cleanup(Enesim_Renderer_Compound *thiz, Enesim_Surface *s)
{
	Enesim_Renderer_Compound_Layer *layer;
	Eina_List *ll;

	/* cleanup the background */
	if (thiz->background_enabled)
	{
		enesim_renderer_cleanup(thiz->background.r, s);
	}

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
		enesim_renderer_compound_layer_unref(layer);
	}
	thiz->changed = EINA_FALSE;
}

#if BUILD_OPENGL
/* the only shader */
static Enesim_Renderer_OpenGL_Shader _compound_shader = {
	/* .type 	= */ ENESIM_RENDERER_OPENGL_SHADER_FRAGMENT,
	/* .name 	= */ "compound",
	/* .source	= */
#include "enesim_renderer_opengl_common_texture.glsl"
};

static Enesim_Renderer_OpenGL_Shader *_compound_shaders[] = {
	&_compound_shader,
	NULL,
};

/* the only program */
static Enesim_Renderer_OpenGL_Program _compound_program = {
	/* .name 		= */ "compound",
	/* .shaders 		= */ _compound_shaders,
	/* .num_shaders		= */ 1,
};

static Enesim_Renderer_OpenGL_Program *_compound_programs[] = {
	&_compound_program,
	NULL,
};

static inline void _compound_layer_opengl_draw(
		Enesim_Renderer_Compound_Layer *l, Enesim_Surface *s,
		const Eina_Rectangle *area, int x, int y)
{
	Eina_Rectangle final;

	/* transform the surface area to renderer coordinates */
	final = *area;
	final.x -= x;
	final.y -= y;

	if (!eina_rectangle_intersection(&final, &l->destination_bounds))
		return;
	/* only draw the layer clipped to the renderer bounds */
	final.x += x;
	final.y += y;

	enesim_renderer_opengl_draw(l->r, s, l->rop, &final, x, y);
}

#if 0
/* the simple case just draws with the renderer rop instead of the layer rop */
static void _compound_opengl_draw_simple(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop EINA_UNUSED, const Eina_Rectangle *area, int x, int y)
{
	Enesim_Renderer_Compound *thiz;
	Eina_List *ll;

	thiz = ENESIM_RENDERER_COMPOUND(r);

	/* draw every layer */
	/* first the background */
	if (thiz->background_enabled)
	{
		Enesim_Renderer_Compound_Layer *l = &thiz->background;
		_compound_layer_opengl_draw(l, s, area, x, y);
	}
	/* now the layers */
	for (ll = thiz->visible_layers; ll; ll = eina_list_next(ll))
	{
		Enesim_Renderer_Compound_Layer *l;

		l = eina_list_data_get(ll);
		_compound_layer_opengl_draw(l, s, area, x, y);
	}
}
#endif

static void _compound_opengl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, const Eina_Rectangle *area, int x, int y)
{
	Enesim_Renderer_Compound *thiz;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_OpenGL_Compiled_Program *cp;
	Eina_Rectangle larea;
	Eina_List *ll;

	thiz = ENESIM_RENDERER_COMPOUND(r);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);

	/* create our temporary surface */
	if (thiz->opengl_s)
	{
		int w, h;
		enesim_surface_size_get(thiz->opengl_s, &w, &h);
		if (area->w != w || area->h != h)
		{
			enesim_surface_unref(thiz->opengl_s);
			thiz->opengl_s = NULL;
		}
	}

	if (!thiz->opengl_s)
	{
		Enesim_Pool *pool;

		pool = enesim_surface_pool_get(s);
		thiz->opengl_s = enesim_surface_new_pool_from(ENESIM_FORMAT_ARGB8888,
				area->w, area->h, pool);
	}
	/* draw every layer */
	/* first the background */
	eina_rectangle_coords_from(&larea, 0, 0, area->w, area->h);
	if (thiz->background_enabled)
	{
		Enesim_Renderer_Compound_Layer *l = &thiz->background;
		_compound_layer_opengl_draw(l, thiz->opengl_s, &larea, -(area->x - x), -(area->y - y));
	}
	/* now the layers */
	for (ll = thiz->visible_layers; ll; ll = eina_list_next(ll))
	{
		Enesim_Renderer_Compound_Layer *l;

		l = eina_list_data_get(ll);
		_compound_layer_opengl_draw(l, thiz->opengl_s, &larea, -(area->x - x), -(area->y - y));
	}

	/* draw the temporary surface */
	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	cp = &rdata->program->compiled[0];
	enesim_renderer_opengl_shader_texture_setup(cp->id,
				0, thiz->opengl_s, ENESIM_COLOR_FULL, area->x, area->y);
	enesim_opengl_target_surface_set(s);
	enesim_opengl_rop_set(rop);
	enesim_opengl_draw_area(area);

	/* don't use any program */
	glUseProgramObjectARB(0);
	enesim_opengl_target_surface_set(NULL);
	glBindTexture(GL_TEXTURE_2D, 0);
}
#endif

#ifdef BUILD_OPENCL
static inline void _compound_layer_opencl_draw(
		Enesim_Renderer_Compound_Layer *l, Enesim_Surface *s,
		const Eina_Rectangle *area, int x, int y)
{
	Eina_Rectangle final;

	/* transform the surface area to renderer coordinates */
	final = *area;
	final.x -= x;
	final.y -= y;

	if (!eina_rectangle_intersection(&final, &l->destination_bounds))
		return;
	/* only draw the layer clipped to the renderer bounds */
	final.x += x;
	final.y += y;

	enesim_renderer_opencl_draw(l->r, s, l->rop, &final, x, y);
}
#endif

static inline void _compound_span_layer_draw(Enesim_Renderer_Compound *thiz, int x, int y, int len, void *ddata)
{
	Eina_List *ll;
	Eina_Rectangle span;

	eina_rectangle_coords_from(&span, x, y, len, 1);
	/* first the background */
	if (thiz->background_enabled)
	{
		_compound_layer_span_draw(&thiz->background, &span, ddata);
	}
	/* now the layers */
	for (ll = thiz->visible_layers; ll; ll = eina_list_next(ll))
	{
		Enesim_Renderer_Compound_Layer *l;

		l = eina_list_data_get(ll);
		_compound_layer_span_draw(l, &span, ddata);
	}
}

/* whenever the compound needs to fill, we need to zeros the whole destination buffer */
static void _compound_fill_span_blend_layer(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Compound *thiz;

	thiz = ENESIM_RENDERER_COMPOUND(r);

	/* we might need to add this memset in case the layers for this span dont fill the whole area
	 * TODO we can do this smarter and just fill the areas that the renderers did not draw
	 */
	memset(ddata, 0, len * sizeof(uint32_t));
	_compound_span_layer_draw(thiz, x, y, len, ddata);
}

/* whenever the compound needs to blend, we only need to draw the area of each layer */
static void _compound_blend_span_blend_layer(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Compound *thiz;

	thiz = ENESIM_RENDERER_COMPOUND(r);

	/* we should only do one memset, here instead of per each layer */
	_compound_span_layer_draw(thiz, x, y, len, ddata);
}

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _compound_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "compound";
}

static void _compound_sw_hints(Enesim_Renderer *r, Enesim_Rop rop,
		Enesim_Renderer_Sw_Hint *hints)
{
	Enesim_Renderer_Compound *thiz;
	Enesim_Renderer_Sw_Hint h = 0xffffffff;
	Eina_Bool same_rop = EINA_TRUE;
	Eina_List *ll;

	thiz = ENESIM_RENDERER_COMPOUND(r);
	if (!thiz->layers)
	{
		*hints = 0;
		return;
	}

	/* the background too */
	if (thiz->background_enabled)
	{
		_compound_layer_sw_hints_merge(&thiz->background, rop, &same_rop, &h);
	}

	/* TODO we need to find an heuristic to set the colorize/rop flag
	 * that reduces the number of raster operations we have to do
	 * (i.e a passthrough)
	 * - if every renderer has the same rop as the compound then
	 * there is no need to do a post composition of the result. but how to
	 * handle the colorize?
	 */
	for (ll = thiz->layers; ll; ll = eina_list_next(ll))
	{
		Enesim_Renderer_Compound_Layer *l = eina_list_data_get(ll);
		_compound_layer_sw_hints_merge(l, rop, &same_rop, &h);
	}
	if (same_rop)
		h |= ENESIM_RENDERER_SW_HINT_ROP;
	else
		h &= ~ENESIM_RENDERER_SW_HINT_ROP;
	*hints = h;
}


static Eina_Bool _compound_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **log)
{
	Enesim_Renderer_Compound *thiz;

	thiz = ENESIM_RENDERER_COMPOUND(r);
	if (!_compound_state_setup(thiz, r, s, rop, log))
		return EINA_FALSE;

	if (rop == ENESIM_ROP_FILL)
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

	thiz = ENESIM_RENDERER_COMPOUND(r);
	_compound_state_cleanup(thiz, s);
}

static Eina_Bool _compound_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *rect, Enesim_Log **log EINA_UNUSED)
{
	Enesim_Renderer_Compound *thiz;
	Enesim_Renderer_Compound_Layer *l;
	Eina_Bool added = EINA_FALSE;
	Eina_Bool layers = EINA_FALSE;
	Eina_List *ll;
	int x1 = INT_MAX;
	int y1 = INT_MAX;
	int x2 = -INT_MAX;
	int y2 = -INT_MAX;

	thiz = ENESIM_RENDERER_COMPOUND(r);
	if (!thiz->added)
		goto no_added;

	added = EINA_TRUE;

	/* first the just added layers */
	EINA_LIST_FOREACH (thiz->added, ll, l)
	{
		Enesim_Renderer *lr = l->r;
		int nx1, ny1, nx2, ny2;
		Eina_Rectangle tmp;

		enesim_renderer_destination_bounds_get(lr, &tmp, 0, 0, NULL);
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
no_added:
	if (!thiz->layers)
		goto no_layers;

	layers = EINA_TRUE;

	/* now the already there layers */
	EINA_LIST_FOREACH (thiz->layers, ll, l)
	{
		Enesim_Renderer *lr = l->r;
		int nx1, ny1, nx2, ny2;
		Eina_Rectangle tmp;

		enesim_renderer_destination_bounds_get(lr, &tmp, 0, 0, NULL);
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
no_layers:
	/* empty bounds */
	if (!layers && !added)
	{
		enesim_rectangle_coords_from(rect, 0, 0, 0, 0);
		return EINA_FALSE;
	}
	else
	{
		enesim_rectangle_coords_from(rect, x1, y1, x2 - x1, y2 - y1);
		return EINA_TRUE;
	}
}

static void _compound_features_get(Enesim_Renderer *r EINA_UNUSED,
		int *features)
{
	/* TODO here we should only support the destination formats of the surfaces */
	*features = 0;
}

static Eina_Bool _compound_is_inside(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Compound *thiz;
	Eina_List *ll;
	Eina_Bool is_inside = EINA_FALSE;

	thiz = ENESIM_RENDERER_COMPOUND(r);
	for (ll = thiz->layers; ll; ll = eina_list_next(ll))
	{
		Enesim_Renderer_Compound_Layer *l = eina_list_data_get(ll);
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

	thiz = ENESIM_RENDERER_COMPOUND(r);
	ret = thiz->changed;
	if (!ret)
	{
		Enesim_Renderer_Compound_Layer *l;
		Eina_List *ll;

		EINA_LIST_FOREACH(thiz->layers, ll, l)
		{
			if ((ret = enesim_renderer_has_changed(l->r)))
			{
				DBG("The child %s has changed",
						enesim_renderer_name_get(l->r));
				break;
			}
		}
	}
	return ret;
}

static Eina_Bool _compound_damage(Enesim_Renderer *r,
		const Eina_Rectangle *old_bounds,
		Enesim_Renderer_Damage cb, void *data)
{
	Enesim_Renderer_Compound *thiz;
	Enesim_Renderer_Compound_Layer *l;
	Eina_List *ll;
	Eina_Bool ret = EINA_FALSE;

	thiz = ENESIM_RENDERER_COMPOUND(r);
	/* in case the backround has changed, send again the previous bounds */
	if (enesim_renderer_has_changed(thiz->background.r))
	{
		DBG("Background has changed, sending old and new bounds");
		goto full_bounds;
	}
	/* given that we do support the visibility, color, rop, we need to take into
	 * account such change
	 */
	if (enesim_renderer_state_has_changed(r))
	{
		DBG("Common properties have changed");
		goto full_bounds;
	}
	/* if some layer has been added or removed after the last draw
	 * we need to inform of those areas even if those have damages
	 * or not
	 */
	else if (thiz->changed)
	{
		EINA_LIST_FOREACH(thiz->removed, ll, l)
		{
			DBG("Sending deleted layer '%s' bounds",
					l->r->name);
			cb(l->r, &l->destination_bounds, EINA_FALSE, data);
		}
		EINA_LIST_FOREACH(thiz->added, ll, l)
		{
			Eina_Rectangle db;

			DBG("Sending added layer '%s' bounds",
					l->r->name);
			enesim_renderer_destination_bounds_get(l->r, &db, 0, 0, NULL);
			cb(l->r, &db, EINA_FALSE, data);
		}
		ret = EINA_TRUE;
	}
	/* now for every layer send the damage */
	EINA_LIST_FOREACH(thiz->layers, ll, l)
	{
		ret = ret | enesim_renderer_damages_get(l->r, cb, data);
	}
	return ret;

full_bounds:
	{
		Eina_Rectangle current_bounds;

		enesim_renderer_destination_bounds_get(r, &current_bounds, 0, 0, NULL);
		cb(r, old_bounds, EINA_TRUE, data);
		cb(r, &current_bounds, EINA_FALSE, data);
		return EINA_TRUE;
	}
}

#if BUILD_OPENGL
static Eina_Bool _compound_opengl_initialize(Enesim_Renderer *r EINA_UNUSED,
		int *num_programs EINA_UNUSED,
		Enesim_Renderer_OpenGL_Program ***programs EINA_UNUSED)
{
	*num_programs = 1;
	*programs = _compound_programs;

	return EINA_TRUE;
}

static Eina_Bool _compound_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l)
{
	Enesim_Renderer_Compound *thiz;

 	thiz = ENESIM_RENDERER_COMPOUND(r);
	if (!_compound_state_setup(thiz, r, s, rop, l)) return EINA_FALSE;

	*draw = _compound_opengl_draw;
	return EINA_TRUE;
}

static void _compound_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Compound *thiz;

 	thiz = ENESIM_RENDERER_COMPOUND(r);
	_compound_state_cleanup(thiz, s);
}
#endif

#if BUILD_OPENCL
static Eina_Bool _compound_opencl_kernel_get(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		const char **program_name, const char **program_source,
		size_t *program_length, const char **kernel_name)
{
	*program_name = "compound";
	*program_source =
	"#include \"enesim_opencl.h\"\n" 
	#include "enesim_renderer_compound.cl"
	*program_length = strlen(*program_source);
	*kernel_name = "compound";
	return EINA_TRUE;
}

static Eina_Bool _compound_opencl_kernel_setup(Enesim_Renderer *r,
		Enesim_Surface *s, int argc,
		Enesim_Renderer_OpenCL_Kernel_Mode *mode)
{
	Enesim_Renderer_Compound *thiz;
	Enesim_Renderer_OpenCL_Data *rdata;

	thiz = ENESIM_RENDERER_COMPOUND(r);
	/* create the surface */
	return EINA_TRUE;
}

static void _compound_opencl_kernel_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Compound *thiz;

	thiz = ENESIM_RENDERER_COMPOUND(r);
	if (thiz->opencl_s)
	{
		enesim_surface_unref(thiz->opencl_s);
		thiz->opencl_s = NULL;
	}
}

static Eina_Bool _compound_opencl_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Log **l)
{
	Enesim_Renderer_Compound *thiz;

 	thiz = ENESIM_RENDERER_COMPOUND(r);
	if (!_compound_state_setup(thiz, r, s, rop, l))
		return EINA_FALSE;
	return enesim_renderer_opencl_setup_default(r, s, rop, l);
}

static void _compound_opencl_cleanup(Enesim_Renderer *r,
		Enesim_Surface *s)
{
	Enesim_Renderer_Compound *thiz;

 	thiz = ENESIM_RENDERER_COMPOUND(r);
	_compound_state_cleanup(thiz, s);
	enesim_renderer_opencl_cleanup_default(r, s);
}

static void _compound_opencl_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, const Eina_Rectangle *area, int x, int y)
{
	Enesim_Renderer_Compound *thiz;
	Enesim_Renderer_OpenCL_Data *rdata;
	Eina_Rectangle larea;
	Eina_List *ll;

	thiz = ENESIM_RENDERER_COMPOUND(r);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENCL);
	/* create our temporary surface */
	{
		Enesim_Pool *pool;
		Enesim_Buffer_OpenCL_Data *sf_data;
		int w, h;
		cl_mem cl_s = NULL;

		pool = enesim_surface_pool_get(s);
		enesim_surface_size_get(s, &w, &h);
		thiz->opencl_s = enesim_surface_new_pool_from(
				ENESIM_FORMAT_ARGB8888, w, h, pool);
		sf_data = enesim_surface_backend_data_get(thiz->opencl_s);
		cl_s = sf_data->mem[0];
		enesim_pool_unref(pool);

		clSetKernelArg(rdata->kernel, 2, sizeof(cl_mem),
				&cl_s);
	}
	/* draw every layer */
	/* first the background */
	eina_rectangle_coords_from(&larea, 0, 0, area->w, area->h);
	if (thiz->background_enabled)
	{
		Enesim_Renderer_Compound_Layer *l = &thiz->background;
		_compound_layer_opencl_draw(l, thiz->opencl_s, &larea, -(area->x - x), -(area->y - y));
	}
	/* now the layers */
	for (ll = thiz->visible_layers; ll; ll = eina_list_next(ll))
	{
		Enesim_Renderer_Compound_Layer *l;

		l = eina_list_data_get(ll);
		_compound_layer_opencl_draw(l, thiz->opencl_s, &larea, -(area->x - x), -(area->y - y));
	}
	enesim_renderer_opencl_draw_default(r, s, rop, area, x, y);
}
#endif
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Compound, Enesim_Renderer_Compound_Class,
		enesim_renderer_compound);

static void _enesim_renderer_compound_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _compound_name;
	klass->bounds_get = _compound_bounds_get;
	klass->features_get = _compound_features_get;
	klass->is_inside = _compound_is_inside;
	klass->damages_get = _compound_damage;
	klass->has_changed = _compound_has_changed;
	klass->sw_hints_get = _compound_sw_hints;
	klass->sw_setup = _compound_sw_setup;
	klass->sw_cleanup = _compound_sw_cleanup;
#if BUILD_OPENGL
	klass->opengl_initialize = _compound_opengl_initialize;
	klass->opengl_setup = _compound_opengl_setup;
	klass->opengl_cleanup = _compound_opengl_cleanup;
#endif
#if BUILD_OPENCL
	klass->opencl_setup = _compound_opencl_setup;
	klass->opencl_cleanup = _compound_opencl_cleanup;
	klass->opencl_kernel_get = _compound_opencl_kernel_get;
	klass->opencl_kernel_setup = _compound_opencl_kernel_setup;
	klass->opencl_kernel_cleanup = _compound_opencl_kernel_cleanup;
	klass->opencl_draw = _compound_opencl_draw;
#endif
}

static void _enesim_renderer_compound_instance_init(void *o)
{
	Enesim_Renderer_Compound *thiz;
	Enesim_Renderer *r;

	thiz = ENESIM_RENDERER_COMPOUND(o);
	r = enesim_renderer_background_new();
	thiz->background.r = r;
	thiz->background.rop = ENESIM_ROP_FILL;
}

static void _enesim_renderer_compound_instance_deinit(void *o)
{
	Enesim_Renderer_Compound *thiz;
	Enesim_Renderer *r;
	Enesim_Renderer_Compound_Layer *l;

	thiz = ENESIM_RENDERER_COMPOUND(o);
	if (thiz->background.r)
	{
		enesim_renderer_unref(thiz->background.r);
		thiz->background.r = NULL;
	}
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
		enesim_renderer_compound_layer_unref(l);
	}

	r = ENESIM_RENDERER(o);
	enesim_renderer_compound_layer_clear(r);
	/* finally unref every removed layer */
	EINA_LIST_FREE(thiz->removed, l)
	{
		/* now is safe to destroy the layer */
		l->owner = NULL;
		enesim_renderer_compound_layer_unref(l);
	}
#if BUILD_OPENGL
	if (thiz->opengl_s)
	{
		enesim_surface_unref(thiz->opengl_s);
	}
#endif
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Creates a compound renderer layer
 * @return The new layer
 */
EAPI Enesim_Renderer_Compound_Layer * enesim_renderer_compound_layer_new(void)
{
	Enesim_Renderer_Compound_Layer *l;
	l = calloc(1, sizeof(Enesim_Renderer_Compound_Layer));
	l->ref = 1;
	return l;
}

/**
 * @brief Increase the reference counter of a layer
 * @param[in] l The layer
 * @return The input parameter @a l for programming convenience
 */
EAPI Enesim_Renderer_Compound_Layer * enesim_renderer_compound_layer_ref(
		Enesim_Renderer_Compound_Layer *l)
{
	if (!l) return NULL;
	l->ref++;
	return l;
}

/**
 * @brief Decrease the reference counter of a layer
 * @param[in] l The layer
 */
EAPI void enesim_renderer_compound_layer_unref(Enesim_Renderer_Compound_Layer *l)
{
	if (!l) return;

	l->ref--;
	if (!l->ref)
	{
		/* wrong */
		if (l->owner)
		{

		}

		if (l->r)
		{
			enesim_renderer_unref(l->r);
			l->r = NULL;
		}
		free(l);
	}
}

/**
 * @brief Sets the renderer of a layer
 * @ender_prop{renderer}
 * @param[in] l The layer to set the renderer on
 * @param[in] r The renderer to set on the layer @ender_transfer{full}
 */
EAPI void enesim_renderer_compound_layer_renderer_set(
		Enesim_Renderer_Compound_Layer *l, Enesim_Renderer *r)
{
	if (!l) return;
	if (l->r)
	{
		enesim_renderer_unref(l->r);
		l->r = NULL;
	}
	l->r = r;
	if (l->owner)
	{
		Enesim_Renderer_Compound *thiz = ENESIM_RENDERER_COMPOUND(l->owner);
		thiz->changed = EINA_TRUE;
	}
} 

/**
 * @brief Gets the renderer of a layer
 * @ender_prop{renderer}
 * @param[in] l The layer to get the renderer from
 * @return The renderer of the layer @ender_transfer{none}
 */
EAPI Enesim_Renderer * enesim_renderer_compound_layer_renderer_get(
		Enesim_Renderer_Compound_Layer *l)
{
	if (!l) return NULL;
	if (!l->r) return NULL;

	return enesim_renderer_ref(l->r);
} 

/**
 * @brief Sets the raster operation of a layer
 * @ender_prop{rop}
 * @param[in] l The layer to set the raster operation on
 * @param[in] rop The raster operation to set on the layer
 */
EAPI void enesim_renderer_compound_layer_rop_set(
		Enesim_Renderer_Compound_Layer *l, Enesim_Rop rop)
{
	if (!l) return;
	l->rop = rop;
	if (l->owner)
	{
		Enesim_Renderer_Compound *thiz = ENESIM_RENDERER_COMPOUND(l->owner);
		thiz->changed = EINA_TRUE;
	}
}

/**
 * @brief Creates a compound renderer
 * @return The new renderer
 */
EAPI Enesim_Renderer * enesim_renderer_compound_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_compound);
	return r;
}

/**
 * Adds a layer
 * @param[in] r The compound renderer
 * @param[in] layer The layer to add @ender_transfer{full}
 */
EAPI void enesim_renderer_compound_layer_add(Enesim_Renderer *r,
		Enesim_Renderer_Compound_Layer *layer)
{
	Enesim_Renderer_Compound *thiz;

	if (!layer) return;
	if (layer->owner)
	{
		WRN("Trying to add a layer which belongs to another compound");
		enesim_renderer_compound_layer_unref(layer);
		return;
	}
	thiz = ENESIM_RENDERER_COMPOUND(r);
	thiz->added = eina_list_append(thiz->added, layer);
	thiz->changed = EINA_TRUE;
}

/**
 * Prepends a layer
 * @param[in] r The compound renderer
 * @param[in] layer The layer to prepend @ender_transfer{full}
 */
EAPI void enesim_renderer_compound_layer_prepend(Enesim_Renderer *r,
		Enesim_Renderer_Compound_Layer *layer)
{
	Enesim_Renderer_Compound *thiz;

	if (!layer) return;
	if (layer->owner)
	{
		WRN("Trying to add a layer which belongs to another compound");
		enesim_renderer_compound_layer_unref(layer);
		return;
	}
	thiz = ENESIM_RENDERER_COMPOUND(r);
	thiz->added = eina_list_prepend(thiz->added, layer);
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Removes a layer
 * @param[in] r The compound renderer
 * @param[in] layer The layer to remove @ender_transfer{full}
 */
EAPI void enesim_renderer_compound_layer_remove(Enesim_Renderer *r,
		Enesim_Renderer_Compound_Layer *layer)
{
	Enesim_Renderer_Compound *thiz;
	Enesim_Renderer_Compound_Layer *owned;
	Eina_List *l;
	Eina_List *l_next;
	Eina_Bool found = EINA_FALSE;

	if (!layer) return;
	if (layer->owner != r)
	{
		WRN("Trying to remove a layer that do not belong to use");
		goto done;
	}

	thiz = ENESIM_RENDERER_COMPOUND(r);
	/* check if the layer exists on the active layers list */
	EINA_LIST_FOREACH_SAFE(thiz->layers, l, l_next, owned)
	{
		if (owned == layer)
		{
			_compound_layer_remove(thiz, owned);
 			thiz->layers = eina_list_remove_list(thiz->layers, l);
			found = EINA_TRUE;
			break;
		}
	}
	if (found) goto done;
	/* check if the layer exists on the recent added list */
	EINA_LIST_FOREACH_SAFE(thiz->added, l, l_next, owned)
	{
		if (owned == layer)
		{
			_compound_layer_remove(thiz, owned);
 			thiz->added = eina_list_remove_list(thiz->added, l);
			found = EINA_TRUE;
			break;
		}
	}
done:
	enesim_renderer_compound_layer_unref(layer);
}

/**
 * @brief Clears up all the layers
 * @param[in] r The compound renderer
 */
EAPI void enesim_renderer_compound_layer_clear(Enesim_Renderer *r)
{
	Enesim_Renderer_Compound *thiz;
	Enesim_Renderer_Compound_Layer *layer;
	Eina_List *l;
	Eina_List *l_next;

	thiz = ENESIM_RENDERER_COMPOUND(r);
	/* remove the current layers */
	EINA_LIST_FOREACH_SAFE(thiz->layers, l, l_next, layer)
	{
		_compound_layer_remove(thiz, layer);
 		thiz->layers = eina_list_remove_list(thiz->layers, l);
	}
	/* remove the added layers */
	EINA_LIST_FOREACH_SAFE(thiz->added, l, l_next, layer)
	{
		_compound_layer_remove(thiz, layer);
 		thiz->added = eina_list_remove_list(thiz->added, l);
	}
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Iterates over all the layers of a compound renderer
 * @param[in] r The compound renderer
 * @param[in] cb The function to call on every layer
 * @param[in] user_data User provided data
 */
EAPI void enesim_renderer_compound_layer_foreach(Enesim_Renderer *r,
		Enesim_Renderer_Compund_Foreach_Layer cb, void *user_data)
{
	Enesim_Renderer_Compound *thiz;
	Enesim_Renderer_Compound_Layer *layer;
	Eina_List *l;

	thiz = ENESIM_RENDERER_COMPOUND(r);
	EINA_LIST_FOREACH(thiz->layers, l, layer)
	{
		if (!cb(r, layer, user_data))
			break;
	}
	EINA_LIST_FOREACH(thiz->added, l, layer)
	{
		if (!cb(r, layer, user_data))
			break;
	}
}

/**
 * @brief Iterates over all the layers of a compound renderer in reverse order
 * @param[in] r The compound renderer
 * @param[in] cb The function to call on every layer
 * @param[in] user_data User provided data
 */
EAPI void enesim_renderer_compound_layer_reverse_foreach(Enesim_Renderer *r,
		Enesim_Renderer_Compund_Foreach_Layer cb, void *user_data)
{
	Enesim_Renderer_Compound *thiz;
	Enesim_Renderer_Compound_Layer *layer;
	Eina_List *l;

	thiz = ENESIM_RENDERER_COMPOUND(r);
	EINA_LIST_REVERSE_FOREACH(thiz->added, l, layer)
	{
		if (!cb(r, layer, user_data))
			break;
	}
	EINA_LIST_REVERSE_FOREACH(thiz->layers, l, layer)
	{
		if (!cb(r, layer, user_data))
			break;
	}
}

/**
 * @brief Enables or disables the background on the compound renderer
 * @ender_prop{background_enable}
 * @param[in] r The compound renderer
 * @param[in] enable @c EINA_TRUE to enable, @c EINA_FALSE to disable
 */
EAPI void enesim_renderer_compound_background_enable_set(Enesim_Renderer *r, Eina_Bool enable)
{
	Enesim_Renderer_Compound *thiz;

	thiz = ENESIM_RENDERER_COMPOUND(r);
	enesim_renderer_visibility_set(r, enable);
	thiz->background_enabled = enable;
}

/**
 * @brief Gets the enable flag on the background
 * @ender_prop{background_enable}
 * @param[in] r The compound renderer
 * @return @c EINA_TRUE if the background is enabled, @c EINA_FALSE otherwise
 */
EAPI Eina_Bool enesim_renderer_compound_background_enable_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Compound *thiz;

	thiz = ENESIM_RENDERER_COMPOUND(r);
	return thiz->background_enabled;
}

/**
 * @brief Sets the background color of the compound renderer
 * @ender_prop{background_color}
 * @param[in] r The compound renderer
 * @param[in] c The color to set
 */
EAPI void enesim_renderer_compound_background_color_set(Enesim_Renderer *r, Enesim_Color c)
{
	Enesim_Renderer_Compound *thiz;

	thiz = ENESIM_RENDERER_COMPOUND(r);
	enesim_renderer_background_color_set(thiz->background.r, c);
}

/**
 * @brief Gets the background color of the compound renderer
 * @ender_prop{background_color}
 * @param[in] r The compound renderer
 * @return The color
 */
EAPI Enesim_Color enesim_renderer_compound_background_color_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Compound *thiz;

	thiz = ENESIM_RENDERER_COMPOUND(r);
	return enesim_renderer_background_color_get(thiz->background.r);
}
