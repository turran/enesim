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
#ifndef ENESIM_RENDERER_SHAPE_SIMPLE_PRIVATE_H_
#define ENESIM_RENDERER_SHAPE_SIMPLE_PRIVATE_H_

typedef Eina_Bool (*Enesim_Renderer_Shape_Simple_Setup)(Enesim_Renderer *r,
		Enesim_Renderer *path);
typedef void (*Enesim_Renderer_Shape_Simple_Cleanup)(Enesim_Renderer *r);
typedef struct _Enesim_Renderer_Shape_Simple_Descriptor
{
	Enesim_Renderer_Name name;
	Enesim_Renderer_Delete free;
	Enesim_Renderer_Has_Changed has_changed;
	Enesim_Renderer_Shape_Feature_Get feature_get;
	Enesim_Renderer_Shape_Bounds bounds;
	Enesim_Renderer_Shape_Destination_Bounds destination_bounds;
	Enesim_Renderer_Shape_Simple_Setup setup;
	Enesim_Renderer_Shape_Simple_Cleanup cleanup;
} Enesim_Renderer_Shape_Simple_Descriptor;

Enesim_Renderer * enesim_renderer_shape_simple_new(
		Enesim_Renderer_Shape_Simple_Descriptor *descriptor, void *data);
void * enesim_renderer_shape_simple_data_get(Enesim_Renderer *r);

#endif
