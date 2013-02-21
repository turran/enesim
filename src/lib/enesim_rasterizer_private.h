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

#include "enesim_renderer_shape_private.h"

typedef void (*Enesim_Rasterizer_Figure_Set)(Enesim_Renderer *r, const Enesim_Figure *figure);

typedef struct _Enesim_Rasterizer_Descriptor
{
	/* inherited from the renderer */
	Enesim_Renderer_Base_Name_Get_Cb base_name_get;
	Enesim_Renderer_Delete_Cb free;
	Enesim_Renderer_Flags_Get_Cb flags_get;
	/* software based functions */
	Enesim_Renderer_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	/* inherited from the shape */
	Enesim_Renderer_Shape_Feature_Get_Cb feature_get;
	/* our own interface */
	Enesim_Rasterizer_Figure_Set figure_set;
} Enesim_Rasterizer_Descriptor;

Enesim_Renderer * enesim_rasterizer_new(Enesim_Rasterizer_Descriptor *d, void *data);
void * enesim_rasterizer_data_get(Enesim_Renderer *r);
void enesim_rasterizer_figure_set(Enesim_Renderer *r, const Enesim_Figure *f);

Enesim_Renderer * enesim_rasterizer_basic_new(void);
void enesim_rasterizer_basic_vectors_get(Enesim_Renderer *r, int *nvectors,
		Enesim_F16p16_Vector **vectors);

Enesim_Renderer * enesim_rasterizer_bifigure_new(void);
void enesim_rasterizer_bifigure_over_figure_set(Enesim_Renderer *r, const Enesim_Figure *figure);

#endif
