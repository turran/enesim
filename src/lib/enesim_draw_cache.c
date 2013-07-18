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
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_color.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#if BUILD_OPENGL
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
#endif

#include "enesim_renderer_private.h"
#include "enesim_draw_cache_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
struct _Enesim_Draw_Cache
{
	Enesim_Renderer *r;
	Eina_Rectangle bounds;
	Eina_Bool changed;

	Enesim_Surface *s;

	/* on the tiler we keep the area that needs to be redrawn from the
	 * renderer */
	Eina_Tiler *tiler;
	/* given that the tiler is not lock free we need to lock it */
	Eina_Lock tlock;
	int tw, th;
};

static Eina_Bool _damage_cb(Enesim_Renderer *r EINA_UNUSED,
		const Eina_Rectangle *area, Eina_Bool past EINA_UNUSED,
		void *data)
{
	Enesim_Draw_Cache *thiz = data;
	Eina_Rectangle tiler_rect = *area;

	/* get the real offset based on the geometry of the renderer */
	tiler_rect.x -= thiz->bounds.x;
	tiler_rect.y -= thiz->bounds.y;
	eina_tiler_rect_add(thiz->tiler, &tiler_rect);
	return EINA_TRUE;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Draw_Cache * enesim_draw_cache_new(void)
{
	Enesim_Draw_Cache *thiz;
	thiz = calloc(1, sizeof(Enesim_Draw_Cache));
   	eina_lock_new(&thiz->tlock);
	return thiz;
}

void enesim_draw_cache_free(Enesim_Draw_Cache *thiz)
{
	if (thiz->r)
	{
		enesim_renderer_unref(thiz->r);
		thiz->r = NULL;
	}

	if (thiz->s)
	{
		enesim_surface_unref(thiz->s);
		thiz->s = NULL;
	}

	if (thiz->tiler)
	{
		eina_tiler_free(thiz->tiler);
		thiz->tiler = NULL;
	}
	eina_lock_free(&thiz->tlock);
	free(thiz);
}

void enesim_draw_cache_renderer_set(Enesim_Draw_Cache *thiz,
		Enesim_Renderer *r)
{
	if (thiz->r)
	{
		enesim_renderer_unref(thiz->r);
		thiz->r = NULL;
	}
	thiz->r = r;
	thiz->changed = EINA_TRUE;
}

void enesim_draw_cache_renderer_get(Enesim_Draw_Cache *thiz,
		Enesim_Renderer **r)
{
	if (thiz->r)
		*r = enesim_renderer_ref(thiz->r);
	else
		*r = NULL;
}

Eina_Bool enesim_draw_cache_geometry_get(Enesim_Draw_Cache *thiz,
		Eina_Rectangle *g)
{
	if (!thiz->r) return EINA_FALSE;

	enesim_renderer_destination_bounds(thiz->r, &thiz->bounds, 0, 0);
	*g = thiz->bounds;

	return EINA_TRUE;
}

Eina_Bool enesim_draw_cache_setup_sw(Enesim_Draw_Cache *thiz,
		Enesim_Format f, Enesim_Pool *p)
{
	if (!thiz->r) return EINA_FALSE;
	/* TODO check what happens if the format is different? */
	/* in case the renderer has changed our damaged/clear rectangles
	 * has to be invalidated
	 */
	if (enesim_renderer_has_changed(thiz->r) || (!thiz->tiler) || (thiz->changed))
	{
		Eina_Bool full = EINA_FALSE;

		if (thiz->changed)
		{
			thiz->changed = EINA_FALSE;
			if (thiz->tiler)
				eina_tiler_clear(thiz->tiler);
			full = EINA_TRUE;
		}

		/* in case the size of the renderer has changed be sure
		 * to destroy the tiler
		 */
		enesim_renderer_destination_bounds(thiz->r, &thiz->bounds, 0, 0);
		if (thiz->tw != thiz->bounds.w || thiz->th != thiz->bounds.h)
		{
			if (thiz->tiler)
			{
				eina_tiler_free(thiz->tiler);
				thiz->tiler = NULL;
			}
			full = EINA_TRUE;
		}

		/* create a new tiler in case we dont have one */
		if (!thiz->tiler)
		{
			thiz->tiler = eina_tiler_new(thiz->bounds.w, thiz->bounds.h);
			eina_tiler_tile_size_set(thiz->tiler, 1, 1);
			thiz->tw = thiz->bounds.w;
			thiz->th = thiz->bounds.h;
		}

		/* create the surface if we dont have one already */
		if (thiz->s)
		{
			int sw, sh;

			enesim_surface_size_get(thiz->s, &sw, &sh);
			if (sw != thiz->bounds.w || sh != thiz->bounds.h)
			{
				enesim_surface_unref(thiz->s);
				thiz->s = NULL;
				full = EINA_TRUE;
			}
		}

		if (!thiz->s)
		{
			thiz->s = enesim_surface_new_pool_from(f,
					thiz->bounds.w, thiz->bounds.h, p);
		}
		/* finally make the whole surface to be invalidated or pick
		 * up the damages
		 */
		if (full)
		{
			Eina_Rectangle complete;
			eina_rectangle_coords_from(&complete, 0, 0, thiz->bounds.w, thiz->bounds.h);
			eina_tiler_rect_add(thiz->tiler, &complete);
		}
		else
		{
			enesim_renderer_damages_get(thiz->r, _damage_cb, thiz);
		}
	}
	return EINA_TRUE;
}

/* The area is in surface coordinates 0,0 -> renderer geometry width x renderer geometry height */
Eina_Bool enesim_draw_cache_map_sw(Enesim_Draw_Cache *thiz,
		Eina_Rectangle *area, Enesim_Buffer_Sw_Data *mapped)
{
	Enesim_Buffer *buffer;
	Eina_Iterator *it;
	Eina_Tiler *area_tiler;
	Eina_Rectangle *rect;
	Eina_Rectangle real_area;
	Eina_Rectangle s_area;
	Eina_Bool ret;

	if (!thiz->r) return EINA_FALSE;

	/* TODO to minimize the impact of the lock, split this function into a setup/cleanup/map */
	eina_lock_take(&thiz->tlock);
	//printf("requesting %" EINA_EXTRA_RECTANGLE_FORMAT "\n", EINA_EXTRA_RECTANGLE_ARGS(area));

	/* create our own requested area tiler, so we can know what areas
	 * should be redrawn and what areas should not
	 */
	if (!area)
	{
		eina_rectangle_coords_from(&real_area, 0, 0, thiz->tw, thiz->th);
	}
	else
	{
		real_area = *area;
	}
	area_tiler = eina_tiler_new(real_area.w, real_area.h);
	eina_tiler_tile_size_set(area_tiler, 1, 1);

	/* get the mapped pointer */	
	buffer = enesim_surface_buffer_get(thiz->s);
	ret = enesim_buffer_data_get(buffer, mapped);
	enesim_buffer_unref(buffer);

	/* check if the requested area has been already cached or not, ie
	 * remove rectangles from the area tiler
	 */
	it = eina_tiler_iterator_new(thiz->tiler);
	EINA_ITERATOR_FOREACH(it, rect)
	{
		Eina_Rectangle add = *rect;

		/* convert the rect from surface to area coordinates */
		add.x -= real_area.x;
		add.y -= real_area.y;
		eina_tiler_rect_add(area_tiler, &add);
		//printf("adding %" EINA_EXTRA_RECTANGLE_FORMAT "\n", EINA_EXTRA_RECTANGLE_ARGS(&add));
	}
	eina_iterator_free(it);

	/* ok, we now finally can get the damaged rectangles and draw */
	eina_rectangle_coords_from(&s_area, 0, 0, thiz->tw, thiz->th);
	it = eina_tiler_iterator_new(area_tiler);
	EINA_ITERATOR_FOREACH(it, rect)
	{
		Eina_Rectangle redraw;
		uint8_t *dst;
		int y, maxy;

		redraw = *rect;
		/* convert the rect from area to surface coordinates */
		redraw.x += real_area.x;
		redraw.y += real_area.y;

		if (!eina_rectangle_intersection(&redraw, &s_area))
			continue;
		dst = (uint8_t *)argb8888_at(mapped->argb8888.plane0,
				mapped->argb8888.plane0_stride,
				redraw.x, redraw.y);
		//printf("redrawing into %d %d %d %d from %d %d\n", redraw.x, redraw.y, redraw.w, redraw.h,
		//	redraw.x + thiz->bounds.x, redraw.y + thiz->bounds.y);
		y = redraw.y;
		maxy = y + redraw.h;
		while (y < maxy)
		{
			enesim_renderer_sw_draw(thiz->r, redraw.x + thiz->bounds.x,
					y + thiz->bounds.y, redraw.w, (uint32_t *)dst);
			dst += mapped->argb8888.plane0_stride;
			y++;
		}
		eina_tiler_rect_del(thiz->tiler, &redraw);
	}
	eina_iterator_free(it);
	eina_tiler_free(area_tiler);
	eina_lock_release(&thiz->tlock);

	return ret;
}

#if 0
void enesim_draw_cache_map_gl(Enesim_Draw_Cache *thiz,
		Eina_Rectangle *area, Enesim_Buffer_OpenGL_Data *mapped)
{

}
#endif
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
