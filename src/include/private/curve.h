/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2008 Jorge Luis Zapata
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
#ifndef _ENESIM_CURVE_H
#define _ENESIM_CURVE_H

typedef void (*Enesim_Curve_Vertex_Add_Callback)(double x, double y, void *data);

void enesim_curve3_decasteljau_generate(double x1, double y1, double x2, double y2,
		double x3, double y3, Enesim_Curve_Vertex_Add_Callback cb, void *data);

void enesim_curve4_decasteljau_generate(double x1, double y1, double x2, double y2,
		double x3, double y3, double x4, double y4, Enesim_Curve_Vertex_Add_Callback
		cb, void *data);
#endif
