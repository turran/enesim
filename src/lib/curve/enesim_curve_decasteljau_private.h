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
#ifndef ENESIM_CURVE_DECASTELJAU_PRIVATE_H
#define ENESIM_CURVE_DECASTELJAU_PRIVATE_H

void enesim_curve_decasteljau_cubic_at(const Enesim_Path_Cubic *c, double t,
		Enesim_Path_Cubic *left, Enesim_Path_Cubic *right);
void enesim_curve_decasteljau_cubic_mid(const Enesim_Path_Cubic *c,
		Enesim_Path_Cubic *left, Enesim_Path_Cubic *right);
void enesim_curve_decasteljau_quadratic_at(const Enesim_Path_Quadratic *c, double t,
		Enesim_Path_Quadratic *left, Enesim_Path_Quadratic *right);
void enesim_curve_decasteljau_quadratic_mid(const Enesim_Path_Quadratic *c,
		Enesim_Path_Quadratic *left, Enesim_Path_Quadratic *right);

#endif

