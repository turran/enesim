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
#ifndef ENESIM_PATH_H_
#define ENESIM_PATH_H_

/**
 * @defgroup Enesim_Path Paths
 * @brief Path definition
 * @{
 */

/**
 * Enumaration of the different command types
 */
typedef enum _Enesim_Path_Command_Type
{
	ENESIM_PATH_COMMAND_MOVE_TO, /**< A move command type */
	ENESIM_PATH_COMMAND_LINE_TO, /**< A line command type */
	ENESIM_PATH_COMMAND_QUADRATIC_TO, /**< A quadratic command type */
	ENESIM_PATH_COMMAND_SQUADRATIC_TO, /**< A smooth quadratic command type */
	ENESIM_PATH_COMMAND_CUBIC_TO, /**< A cubic command type */
	ENESIM_PATH_COMMAND_SCUBIC_TO, /**< A smooth cubic command type */
	ENESIM_PATH_COMMAND_ARC_TO, /**< An arc command type */
	ENESIM_PATH_COMMAND_CLOSE, /**< A close command type */
	ENESIM_PATH_COMMAND_TYPES, /**< The number command types */
} Enesim_Path_Command_Type;

/**
 * Definition of a move command
 */
typedef struct _Enesim_Path_Command_Move_To
{
	double x; /**< The X destination coordinate */
	double y; /**< The Y destination coordinate */
} Enesim_Path_Command_Move_To;

/**
 * Definition of a line command
 */
typedef struct _Enesim_Path_Command_Line_To
{
	double x; /**< The X destination coordinate */
	double y; /**< The Y destination coordinate */
} Enesim_Path_Command_Line_To;

/**
 * Definition of a smooth quadratic command
 */
typedef struct _Enesim_Path_Command_Squadratic_To
{
	double x; /**< The X destination coordinate */
	double y; /**< The Y destination coordinate */
} Enesim_Path_Command_Squadratic_To;

/**
 * Definition of a quadratic command
 */
typedef struct _Enesim_Path_Command_Quadratic_To
{
	double x; /**< The X destination coordinate */
	double y; /**< The Y destination coordinate */
	double ctrl_x; /**< The control point X coordinate */
	double ctrl_y; /**< The control point Y coordinate */
} Enesim_Path_Command_Quadratic_To;

/**
 * Definition of a cubic command
 */
typedef struct _Enesim_Path_Command_Cubic_To
{
	double x; /**< The X destination coordinate */
	double y; /**< The Y destination coordinate */
	double ctrl_x0; /**< The first control point X coordinate */
	double ctrl_y0; /**< The first control point Y coordinate */
	double ctrl_x1; /**< The second control point X coordinate */
	double ctrl_y1; /**< The second control point Y coordinate */
} Enesim_Path_Command_Cubic_To;

/**
 * Definition of a smooth cubic command
 */
typedef struct _Enesim_Path_Command_Scubic_To
{
	double x; /**< The X destination coordinate */
	double y; /**< The Y destination coordinate */
	double ctrl_x; /**< The control point X coordinate */
	double ctrl_y; /**< The control point Y coordinate */
} Enesim_Path_Command_Scubic_To;

/**
 * Definition of an arc command
 */
typedef struct _Enesim_Path_Command_Arc_To
{
	double rx; /**< The radius in X */
	double ry; /**< The radius in Y */
	double angle; /**< The rotation on the X coordinate */
	double x; /**< The X destination coordinate */
	double y; /**< The Y destination coordinate */
	Eina_Bool large; /**< If Eina_True the large arc will be chosen */
	Eina_Bool sweep; /**< If Eina_True the arc in positive-angle will be chosen */
} Enesim_Path_Command_Arc_To;

/**
 * Definition of close command
 */
typedef struct _Enesim_Path_Command_Close
{
	Eina_Bool close; /**< Set to Eina_True for an actual close */
} Enesim_Path_Command_Close;

/**
 * Union of the possible types of commands
 */
typedef union _Enesim_Path_Command_Definition
{
	Enesim_Path_Command_Move_To move_to; /**< The move to values */
	Enesim_Path_Command_Line_To line_to; /**< The line to values */
	Enesim_Path_Command_Squadratic_To squadratic_to; /**< The smooth quadratic to values */
	Enesim_Path_Command_Quadratic_To quadratic_to; /**< The quadratic to values */
	Enesim_Path_Command_Scubic_To scubic_to; /**< The smooth cubic to values */
	Enesim_Path_Command_Cubic_To cubic_to; /**< The cubic to values */
	Enesim_Path_Command_Arc_To arc_to; /**< The arc to values */
	Enesim_Path_Command_Close close; /**< The close value */
} Enesim_Path_Command_Definition;

/**
 * Definition of a path command
 */
typedef struct _Enesim_Path_Command
{
	Enesim_Path_Command_Type type; /**< The type of the command */
	Enesim_Path_Command_Definition definition; /**< The values of the definition */
} Enesim_Path_Command;

typedef struct _Enesim_Path Enesim_Path; /**< A path handle */

EAPI Enesim_Path * enesim_path_new(void);
EAPI Enesim_Path * enesim_path_ref(Enesim_Path *thiz);
EAPI void enesim_path_unref(Enesim_Path *thiz);

EAPI void enesim_path_command_clear(Enesim_Path *thiz);
EAPI void enesim_path_command_add(Enesim_Path *thiz, Enesim_Path_Command *cmd);

EAPI void enesim_path_move_to(Enesim_Path *thiz, double x, double y);
EAPI void enesim_path_line_to(Enesim_Path *thiz, double x, double y);
EAPI void enesim_path_squadratic_to(Enesim_Path *thiz, double x, double y);
EAPI void enesim_path_quadratic_to(Enesim_Path *thiz, double ctrl_x,
		double ctrl_y, double x, double y);
EAPI void enesim_path_cubic_to(Enesim_Path *thiz, double ctrl_x0,
		double ctrl_y0, double ctrl_x, double ctrl_y, double x,
		double y);
EAPI void enesim_path_scubic_to(Enesim_Path *thiz, double ctrl_x, double ctrl_y,
		double x, double y);
EAPI void enesim_path_arc_to(Enesim_Path *thiz, double rx, double ry, double angle,
                   unsigned char large, unsigned char sweep, double x, double y);
EAPI void enesim_path_close(Enesim_Path *thiz);

/**
 * @}
 */

#endif
