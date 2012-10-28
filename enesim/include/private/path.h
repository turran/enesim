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
#ifndef _PATH_H
#define _PATH_H

typedef void (*Enesim_Path_Polygon_Add)(void *data);
typedef void (*Enesim_Path_Polygon_Close)(Eina_Bool close, void *data);
typedef void (*Enesim_Path_Done)(void *data);

typedef struct _Enesim_Path_Descriptor
{
	Enesim_Curve_Vertex_Add vertex_add;
	Enesim_Path_Polygon_Add polygon_add;
	Enesim_Path_Polygon_Close polygon_close;
	Enesim_Path_Done path_done;
} Enesim_Path_Descriptor;

typedef struct _Enesim_Path
{
	Enesim_Path_Descriptor *descriptor;
	const Enesim_Matrix *gm;
	double scale_x;
	double scale_y;
	void *data;
} Enesim_Path;

Enesim_Path * enesim_path_new(Enesim_Path_Descriptor *descriptor, void *data);
Enesim_Path * enesim_path_strokeless_new(void);
Enesim_Path * enesim_path_full_new(void);
void * enesim_path_data_get(Enesim_Path *thiz);
void enesim_path_generate(Enesim_Path *thiz, Eina_List *commands);

#endif
