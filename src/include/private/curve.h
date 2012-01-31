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
#ifndef _ENESIM_CURVE_H
#define _ENESIM_CURVE_H

typedef void (*Enesim_Curve_Vertex_Add)(double x, double y, void *data);

typedef struct _Enesim_Curve_State
{
	Enesim_Curve_Vertex_Add vertex_add;
	double last_x;
	double last_y;
	double last_ctrl_x;
	double last_ctrl_y;
	double threshold;
	void *data;
} Enesim_Curve_State;

void enesim_curve_line_to(Enesim_Curve_State *state,
		double x, double y);
void enesim_curve_quadratic_to(Enesim_Curve_State *state,
		double ctrl_x, double ctrl_y,
		double x, double y);
void enesim_curve_squadratic_to(Enesim_Curve_State *state,
		double x, double y);
void enesim_curve_cubic_to(Enesim_Curve_State *state,
		double ctrl_x0, double ctrl_y0,
		double ctrl_x, double ctrl_y,
		double x, double y);
void enesim_curve_scubic_to(Enesim_Curve_State *state,
		double ctrl_x, double ctrl_y,
		double x, double y);
void enesim_curve_arc_to(Enesim_Curve_State *state,
		double rx, double ry, double angle,
                unsigned char large, unsigned char sweep,
		double x, double y);

#endif
