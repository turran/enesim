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
#ifndef _ENESIM_PATH_NORMALIZER_PRIVATE_H
#define _ENESIM_PATH_NORMALIZER_PRIVATE_H

typedef void (*Enesim_Path_Normalizer_Move_To_Cb)(Enesim_Path_Command_Move_To move_to,
		void *data);
typedef void (*Enesim_Path_Normalizer_Line_To_Cb)(Enesim_Path_Command_Line_To line_to,
		void *data);
typedef void (*Enesim_Path_Normalizer_Cubic_To_Cb)(Enesim_Path_Command_Cubic_To cubic_to,
		void *data);
typedef void (*Enesim_Path_Normalizer_Close_Cb)(Enesim_Path_Command_Close close,
		void *data);

typedef struct _Enesim_Path_Normalizer_Path_Descriptor {
	Enesim_Path_Normalizer_Move_To_Cb move_to;
	Enesim_Path_Normalizer_Line_To_Cb line_to;
	Enesim_Path_Normalizer_Cubic_To_Cb cubic_to;
	Enesim_Path_Normalizer_Close_Cb close;
} Enesim_Path_Normalizer_Path_Descriptor;

Enesim_Path_Normalizer * enesim_path_normalizer_path_new(
		Enesim_Path_Normalizer_Path *descriptor,
		void *data);

typedef void (*Enesim_Path_Normalizer_Figure_Vertex_Add)(double x, double y, void *data);
typedef void (*Enesim_Path_Normalizer_Figure_Polygon_Add)(void *data);
typedef void (*Enesim_Path_Normalizer_Figure_Polygon_Close)(Eina_Bool close, void *data);

typedef struct _Enesim_Path_Normalizer_Figure_Descriptor {
	Enesim_Path_Normalizer_Figure_Vertex_Add vertex_add;
	Enesim_Path_Normalizer_Figure_Polygon_Add polygon_add;
	Enesim_Path_Normalizer_Figure_Polygon_Close polygon_close;
} Enesim_Path_Normalizer_Figure_Descriptor;

Enesim_Path_Normalizer * enesim_path_normalizer_figure_new(
		Enesim_Path_Normalizer_Figure_Descriptor *descriptor,
		void *data);

void enesim_path_normalizer_normalize(Enesim_Path_Normalizer *thiz,
		Enesim_Path_Command *cmd);

#endif
