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
#ifndef ENESIM_CURVE_LOOP_BLINN_PRIVATE_H
#define ENESIM_CURVE_LOOP_BLINN_PRIVATE_H

typedef enum _Enesim_Curve_Loop_Blinn_Type
{
	ENESIM_CURVE_LOOP_BLINN_POINT,
	ENESIM_CURVE_LOOP_BLINN_LINE,
	ENESIM_CURVE_LOOP_BLINN_QUADRATIC,
	ENESIM_CURVE_LOOP_BLINN_CUSP,
	ENESIM_CURVE_LOOP_BLINN_LOOP,
	ENESIM_CURVE_LOOP_BLINN_SERPENTINE,
	ENESIM_CURVE_LOOP_BLINN_UNKNOWN,
} Enesim_Curve_Loop_Blinn_Type;

typedef struct _Enesim_Curve_Loop_Blinn_Classification
{
	Enesim_Curve_Loop_Blinn_Type type;
	double d1, d2, d3;
} Enesim_Curve_Loop_Blinn_Classification;

void enesim_curve_loop_blinn_classify(Enesim_Path_Cubic *c, double err,
		Enesim_Curve_Loop_Blinn_Classification *clas);

#endif
