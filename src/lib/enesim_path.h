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
#ifndef ENESIM_PATH_H_
#define ENESIM_PATH_H_

typedef enum _Enesim_Path_Command_Type
{
	ENESIM_PATH_COMMAND_MOVE_TO,
	ENESIM_PATH_COMMAND_LINE_TO,
	ENESIM_PATH_COMMAND_QUADRATIC_TO,
	ENESIM_PATH_COMMAND_SQUADRATIC_TO,
	ENESIM_PATH_COMMAND_CUBIC_TO,
	ENESIM_PATH_COMMAND_SCUBIC_TO,
	ENESIM_PATH_COMMAND_ARC_TO,
	ENESIM_PATH_COMMAND_CLOSE,
	ENESIM_PATH_COMMAND_TYPES,
} Enesim_Path_Command_Type;

typedef struct _Enesim_Path_Command_Move_To
{
	double x;
	double y;
} Enesim_Path_Command_Move_To;

typedef struct _Enesim_Path_Command_Line_To
{
	double x;
	double y;
} Enesim_Path_Command_Line_To;

typedef struct _Enesim_Path_Command_Squadratic_To
{
	double x;
	double y;
} Enesim_Path_Command_Squadratic_To;

typedef struct _Enesim_Path_Command_Quadratic_To
{
	double x;
	double y;
	double ctrl_x;
	double ctrl_y;
} Enesim_Path_Command_Quadratic_To;

typedef struct _Enesim_Path_Command_Cubic_To
{
	double x;
	double y;
	double ctrl_x0;
	double ctrl_y0;
	double ctrl_x1;
	double ctrl_y1;
} Enesim_Path_Command_Cubic_To;

typedef struct _Enesim_Path_Command_Scubic_To
{
	double x;
	double y;
	double ctrl_x;
	double ctrl_y;
} Enesim_Path_Command_Scubic_To;

typedef struct _Enesim_Path_Command_Arc_To
{
	double rx;
	double ry;
	double angle;
	double x;
	double y;
	Eina_Bool large;
	Eina_Bool sweep;
} Enesim_Path_Command_Arc_To;

typedef struct _Enesim_Path_Command_Close
{
	Eina_Bool close;
} Enesim_Path_Command_Close;

typedef struct _Enesim_Path_Command
{
	Enesim_Path_Command_Type type;
	union {
		Enesim_Path_Command_Move_To move_to;
		Enesim_Path_Command_Line_To line_to;
		Enesim_Path_Command_Squadratic_To squadratic_to;
		Enesim_Path_Command_Quadratic_To quadratic_to;
		Enesim_Path_Command_Scubic_To scubic_to;
		Enesim_Path_Command_Cubic_To cubic_to;
		Enesim_Path_Command_Arc_To arc_to;
		Enesim_Path_Command_Close close;
	} definition;
} Enesim_Path_Command;

#endif