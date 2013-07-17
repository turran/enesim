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
#include "enesim_log.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_color.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"

#if BUILD_OPENGL
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
#endif

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

/* The area is in surface coordinates 0,0 -> renderer geometry width x renderer geometry height */
Eina_Bool enesim_draw_cache_map_sw(Enesim_Draw_Cache *thiz,
		Eina_Rectangle *area, Enesim_Buffer_Sw_Data *mapped,
		Enesim_Format f, Enesim_Pool *p)
{
	Enesim_Buffer *buffer;
	Eina_Iterator *it;
	Eina_Tiler *area_tiler;
	Eina_Rectangle *rect;
	Eina_Rectangle complete, real_area;
	Eina_List *redraws = NULL;
	Eina_Bool ret;

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
			eina_rectangle_coords_from(&complete, 0, 0, thiz->bounds.w, thiz->bounds.h);
			eina_tiler_rect_add(thiz->tiler, &complete);
		}
		else
		{
			enesim_renderer_damages_get(thiz->r, _damage_cb, thiz);
		}
	}

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
	}
	eina_iterator_free(it);

	/* ok, we now finally can get the damaged rectangles and draw */
	it = eina_tiler_iterator_new(area_tiler);
	EINA_ITERATOR_FOREACH(it, rect)
	{
		Eina_Rectangle *redraw;

		redraw = calloc(1, sizeof(Eina_Rectangle));
		*redraw = *rect;
		/* convert the rect from area to surface coordinates */
		redraw->x += real_area.x;
		redraw->y += real_area.y;
		//printf("redrawing %d %d %d %d\n", redraw->x, redraw->y, redraw->w, redraw->h);
		redraws = eina_list_append(redraws, redraw); 
	}
	eina_iterator_free(it);
	eina_tiler_free(area_tiler);

	if (!redraws) goto no_redraws;

	/* finally draw */
	ret = enesim_renderer_draw_list(thiz->r, thiz->s, redraws, thiz->bounds.x, thiz->bounds.y, NULL);
	EINA_LIST_FREE(redraws, rect)
	{
		//printf("removing %d %d %d %d\n", rect->x, rect->y, rect->w, rect->h);
		eina_tiler_rect_del(thiz->tiler, rect);
		free(rect);
	}
	if (!ret) return ret;

	/* get the sw data from the surface */
no_redraws:
	buffer = enesim_surface_buffer_get(thiz->s);
	ret = enesim_buffer_data_get(buffer, mapped);
	enesim_buffer_unref(buffer);

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
