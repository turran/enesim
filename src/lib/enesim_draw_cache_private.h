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
#ifndef ENESIM_DRAW_CACHE_PRIVATE_H_
#define ENESIM_DRAW_CACHE_PRIVATE_H_

typedef struct _Enesim_Draw_Cache Enesim_Draw_Cache; /**< Draw cache handle */

Enesim_Draw_Cache * enesim_draw_cache_new(void);
void enesim_draw_cache_free(Enesim_Draw_Cache *thiz);

void enesim_draw_cache_renderer_set(Enesim_Draw_Cache *thiz,
		Enesim_Renderer *r);
void enesim_draw_cache_renderer_get(Enesim_Draw_Cache *thiz,
		Enesim_Renderer **r);

Eina_Bool enesim_draw_cache_geometry_get(Enesim_Draw_Cache *thiz,
		Eina_Rectangle *g);
Eina_Bool enesim_draw_cache_setup_sw(Enesim_Draw_Cache *thiz,
		Enesim_Format f, Enesim_Pool *p);
Eina_Bool enesim_draw_cache_map_sw(Enesim_Draw_Cache *thiz,
		Eina_Rectangle *area, Enesim_Buffer_Sw_Data *mapped);

#endif /*ENESIM_DRAW_CACHE_PRIVATE_H_*/

