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
#ifndef RASTERIZER_H_
#define RASTERIZER_H_

#include "enesim_renderer_shape_private.h"

#define ENESIM_RASTERIZER_DESCRIPTOR 					\
		enesim_rasterizer_descriptor_get()
#define ENESIM_RASTERIZER_CLASS(k) ENESIM_OBJECT_CLASS_CHECK(k, 	\
		Enesim_Rasterizer_Class,				\
		ENESIM_RASTERIZER_DESCRIPTOR())
#define ENESIM_RASTERIZER_CLASS_GET(o)					\
		ENESIM_RASTERIZER_CLASS(				\
		ENESIM_OBJECT_INSTANCE_CLASS(o));
#define ENESIM_RASTERIZER(o) ENESIM_OBJECT_INSTANCE_CHECK(o, 		\
		Enesim_Rasterizer,					\
		ENESIM_RASTERIZER_DESCRIPTOR)

typedef struct _Enesim_Rasterizer
{
	Enesim_Renderer_Shape parent;
	/* private */
} Enesim_Rasterizer;

/* FIXME we should not inherit from the shape, but use a new interface that only has
 * a subset of the properties. For example we dont need the dash list, or the stroke
 * width. A rasterizer should only rasterize a figure
 */

typedef void (*Enesim_Rasterizer_Figure_Set)(Enesim_Renderer *r, const Enesim_Figure *f);

typedef struct _Enesim_Rasterizer_Class
{
	Enesim_Renderer_Shape_Class parent;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	Enesim_Rasterizer_Figure_Set figure_set;
} Enesim_Rasterizer_Class;

Enesim_Object_Descriptor * enesim_rasterizer_descriptor_get(void);

Eina_Bool enesim_rasterizer_has_changed(Enesim_Renderer *r);
void enesim_rasterizer_figure_set(Enesim_Renderer *r, const Enesim_Figure *f);

Enesim_Renderer * enesim_rasterizer_basic_new(void);
const Enesim_F16p16_Vector * enesim_rasterizer_basic_figure_vectors_get(
		Enesim_Renderer *r, int *nvectors);

Enesim_Renderer * enesim_rasterizer_bifigure_new(void);
void enesim_rasterizer_bifigure_over_figure_set(Enesim_Renderer *r, const Enesim_Figure *figure);

#endif
