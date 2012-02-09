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
#ifndef ENESIM_RENDERER_PATH_H_
#define ENESIM_RENDERER_PATH_H_

/**
 * @}
 * @defgroup Enesim_Renderer_Path_Group Path
 * @ingroup Enesim_Renderer_Shape_Group
 * @{
 */

typedef enum _Enesim_Renderer_Path_Command_Type
{
	ENESIM_COMMAND_MOVE_TO,
	ENESIM_COMMAND_LINE_TO,
	ENESIM_COMMAND_QUADRATIC_TO,
	ENESIM_COMMAND_SQUADRATIC_TO,
	ENESIM_COMMAND_CUBIC_TO,
	ENESIM_COMMAND_SCUBIC_TO,
	ENESIM_COMMAND_ARC_TO,
	ENESIM_COMMAND_CLOSE,
	ENESIM_COMMAND_TYPES,
} Enesim_Renderer_Path_Command_Type;

typedef struct _Enesim_Renderer_Command_Move_To
{
	double x;
	double y;
} Enesim_Renderer_Command_Move_To;

typedef struct _Enesim_Renderer_Command_Line_To
{
	double x;
	double y;
} Enesim_Renderer_Command_Line_To;

typedef struct _Enesim_Renderer_Command_Squadratic_To
{
	double x;
	double y;
} Enesim_Renderer_Command_Squadratic_To;

typedef struct _Enesim_Renderer_Command_Quadratic_To
{
	double x;
	double y;
	double ctrl_x;
	double ctrl_y;
} Enesim_Renderer_Command_Quadratic_To;

typedef struct _Enesim_Renderer_Command_Cubic_To
{
	double x;
	double y;
	double ctrl_x0;
	double ctrl_y0;
	double ctrl_x1;
	double ctrl_y1;
} Enesim_Renderer_Command_Cubic_To;

typedef struct _Enesim_Renderer_Command_Scubic_To
{
	double x;
	double y;
	double ctrl_x;
	double ctrl_y;
} Enesim_Renderer_Command_Scubic_To;

typedef struct _Enesim_Renderer_Command_Arc_To
{
	double rx;
	double ry;
	double angle;
	double x;
	double y;
	Eina_Bool large;
	Eina_Bool sweep;
} Enesim_Renderer_Command_Arc_To;

typedef struct _Enesim_Renderer_Command_Close
{
	Eina_Bool close;
} Enesim_Renderer_Command_Close;

typedef struct _Enesim_Renderer_Path_Command
{
	Enesim_Renderer_Path_Command_Type type;
	union {
		Enesim_Renderer_Command_Move_To move_to;
		Enesim_Renderer_Command_Line_To line_to;
		Enesim_Renderer_Command_Squadratic_To squadratic_to;
		Enesim_Renderer_Command_Quadratic_To quadratic_to;
		Enesim_Renderer_Command_Scubic_To scubic_to;
		Enesim_Renderer_Command_Cubic_To cubic_to;
		Enesim_Renderer_Command_Arc_To arc_to;
		Enesim_Renderer_Command_Close close;
	} definition;
} Enesim_Renderer_Path_Command;

EAPI Enesim_Renderer * enesim_renderer_path_new(void);

EAPI void enesim_renderer_path_command_clear(Enesim_Renderer *r);
EAPI void enesim_renderer_path_command_add(Enesim_Renderer *r, Enesim_Renderer_Path_Command *cmd);
EAPI void enesim_renderer_path_command_set(Enesim_Renderer *r, Eina_List *l);

EAPI void enesim_renderer_path_move_to(Enesim_Renderer *r, double x, double y);
EAPI void enesim_renderer_path_line_to(Enesim_Renderer *r, double x, double y);
EAPI void enesim_renderer_path_squadratic_to(Enesim_Renderer *r, double x, double y);
EAPI void enesim_renderer_path_quadratic_to(Enesim_Renderer *r, double ctrl_x,
		double ctrl_y, double x, double y);
EAPI void enesim_renderer_path_cubic_to(Enesim_Renderer *r, double ctrl_x0,
		double ctrl_y0, double ctrl_x, double ctrl_y, double x,
		double y);
EAPI void enesim_renderer_path_scubic_to(Enesim_Renderer *r, double ctrl_x, double ctrl_y,
		double x, double y);
EAPI void enesim_renderer_path_arc_to(Enesim_Renderer *r, double rx, double ry, double angle,
                   unsigned char large, unsigned char sweep, double x, double y);
EAPI void enesim_renderer_path_close(Enesim_Renderer *r, Eina_Bool close);

/**
 * @}
 */

#endif
