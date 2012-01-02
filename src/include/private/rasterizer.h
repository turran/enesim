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

typedef Eina_Bool (*Enesim_Rasterizer_Setup)(Enesim_Renderer *r, Enesim_Polygon *p);
typedef void (*Enesim_Rasterizer_Process)(Enesim_Renderer *r, int x, int y, unsigned int len, uint32_t *dst);
typedef void (*Enesim_Rasterizer_Cleanup)(Enesim_Renderer *r);

typedef struct _Enesim_Rasterizer_Descriptor
{
	Enesim_Rasterizer_Setup setup;
	Enesim_Rasterizer_Process process;
	Enesim_Rasterizer_Cleanup cleanup;
} Enesim_Rasterizer_Descriptor;


Enesim_Renderer * enesim_rasterizer_new(Enesim_Rasterizer_Descriptor *d, void *data);
void enesim_rasterizier_polygon_set(Enesim_Renderer *r, const Enesim_Polygon *p);

#endif
