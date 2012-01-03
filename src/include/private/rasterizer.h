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
#ifndef RASTERIZER_H_
#define RASTERIZER_H_

typedef Eina_Bool (*Enesim_Rasterizer_Sw_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Figure *figure,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill,
		Enesim_Error **error);
typedef void (*Enesim_Rasterizer_Sw_Cleanup)(Enesim_Renderer *r);

typedef struct _Enesim_Rasterizer_Descriptor
{
	Enesim_Renderer_Name name;
	Enesim_Renderer_Delete free;
	Enesim_Rasterizer_Sw_Setup sw_setup;
	Enesim_Rasterizer_Sw_Cleanup sw_cleanup;
} Enesim_Rasterizer_Descriptor;

Enesim_Renderer * enesim_rasterizer_new(Enesim_Rasterizer_Descriptor *d, void *data);
void * enesim_rasterizer_data_get(Enesim_Renderer *r);
void enesim_rasterizier_figure_set(Enesim_Renderer *r, const Enesim_Figure *f);

Enesim_Renderer * enesim_rasterizer_basic_new(void);
void enesim_rasterizer_basic_vectors_get(Enesim_Renderer *r, int *nvectors,
		Enesim_F16p16_Vector **vectors);

Enesim_Renderer * enesim_rasterizer_bifigure_new(void);
void enesim_renderer_bifigure_over_polygons_set(Enesim_Renderer *r, Eina_List *polygons);
void enesim_renderer_bifigure_under_polygons_set(Enesim_Renderer *r, Eina_List *polygons);

#endif
