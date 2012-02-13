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
#ifndef GRADIENT_H_
#define GRADIENT_H_

/* TODO also move the different _color_get functions (repeat, pad, whatever) here and the macros */
/* common gradient renderer functions */
typedef struct _Enesim_Renderer_Gradient_State
{
	Enesim_Repeat_Mode mode;
	Eina_List *stops;
} Enesim_Renderer_Gradient_State;

typedef Eina_F16p16 (*Enesim_Renderer_Gradient_Distance)(Enesim_Renderer *r, Eina_F16p16 x, Eina_F16p16 y);
typedef int (*Enesim_Renderer_Gradient_Length)(Enesim_Renderer *r);

typedef struct _Enesim_Renderer_Gradient_Descriptor
{
	Enesim_Renderer_Gradient_Distance distance;
	Enesim_Renderer_Gradient_Length length;
	Enesim_Renderer_Name name;
	Enesim_Renderer_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	Enesim_Renderer_Delete free;
	Enesim_Renderer_Boundings boundings;
	Enesim_Renderer_Destination_Boundings destination_boundings;
	Enesim_Renderer_Inside is_inside;
	Enesim_Renderer_Has_Changed has_changed;
} Enesim_Renderer_Gradient_Descriptor;

Enesim_Renderer * enesim_renderer_gradient_new(Enesim_Renderer_Gradient_Descriptor *gdescriptor, void *data);
void * enesim_renderer_gradient_data_get(Enesim_Renderer *r);
Enesim_Color enesim_renderer_gradient_color_get(Enesim_Renderer *r, Eina_F16p16 pos);

#endif
