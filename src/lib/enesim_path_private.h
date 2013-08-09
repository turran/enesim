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

#ifndef ENESIM_PATH_PRIVATE_H_
#define ENESIM_PATH_PRIVATE_H_

typedef struct _Enesim_Path_Quadratic
{
	double start_x;
	double start_y;
	double end_x;
	double end_y;
	double ctrl_x;
	double ctrl_y;
} Enesim_Path_Quadratic;

typedef struct _Enesim_Path_Cubic
{
	double start_x;
	double start_y;
	double end_x;
	double end_y;
	double ctrl_x0;
	double ctrl_y0;
	double ctrl_x1;
	double ctrl_y1;
} Enesim_Path_Cubic;

typedef void (*Enesim_Path_Vertex_Add)(double x, double y, void *data);

void enesim_path_quadratic_cubic_to(Enesim_Path_Quadratic *q,
		Enesim_Path_Cubic *c);
void enesim_path_quadratic_flatten(Enesim_Path_Quadratic *thiz,
		double tolerance, Enesim_Path_Vertex_Add vertex_add,
		void *data);
void enesim_path_cubic_flatten(Enesim_Path_Cubic *thiz,
		double tolerance, Enesim_Path_Vertex_Add vertex_add,
		void *data);

static inline void enesim_path_command_arc_to_values_from(
		Enesim_Path_Command_Arc_To *thiz,
		double rx, double ry, double angle,
		double x, double y, Eina_Bool large,
		Eina_Bool sweep)
{
	thiz->rx = rx;
	thiz->ry = ry;
	thiz->angle = angle;
	thiz->x = x;
	thiz->y = y;
	thiz->large = large;
	thiz->sweep = sweep;
}

static inline void enesim_path_command_arc_to_values_to(
		Enesim_Path_Command_Arc_To *thiz,
		double *rx, double *ry, double *angle,
		double *x, double *y, Eina_Bool *large,
		Eina_Bool *sweep)
{
	*rx = thiz->rx;
	*ry = thiz->ry;
	*angle = thiz->angle;
	*x = thiz->x;
	*y = thiz->y;
	*large = thiz->large;
	*sweep = thiz->sweep;
}

static inline void enesim_path_command_squadratic_to_values_to(
		Enesim_Path_Command_Squadratic_To *thiz,
		double *x, double *y)
{
	*x = thiz->x;
	*y = thiz->y;
}

static inline void enesim_path_command_squadratic_to_values_from(
		Enesim_Path_Command_Squadratic_To *thiz,
		double x, double y)
{
	thiz->x = x;
	thiz->y = y;
}

static inline void enesim_path_command_quadratic_to_values_from(
		Enesim_Path_Command_Quadratic_To *thiz,
		double x, double y, double ctrl_x, double ctrl_y)
{
	thiz->x = x;
	thiz->y = y;
	thiz->ctrl_x = ctrl_x;
	thiz->ctrl_y = ctrl_y;
}

static inline void enesim_path_command_quadratic_to_values_to(
		Enesim_Path_Command_Quadratic_To *thiz,
		double *x, double *y, double *ctrl_x, double *ctrl_y)
{
	*x = thiz->x;
	*y = thiz->y;
	*ctrl_x = thiz->ctrl_x;
	*ctrl_y = thiz->ctrl_y;
}

static inline void enesim_path_command_scubic_to_values_from(
		Enesim_Path_Command_Scubic_To *thiz,
		double x, double y, double ctrl_x, double ctrl_y)
{
	thiz->x = x;
	thiz->y = y;
	thiz->ctrl_x = ctrl_x;
	thiz->ctrl_y = ctrl_y;
}

static inline void enesim_path_command_scubic_to_values_to(
		Enesim_Path_Command_Scubic_To *thiz,
		double *x, double *y, double *ctrl_x, double *ctrl_y)
{
	*x = thiz->x;
	*y = thiz->y;
	*ctrl_x = thiz->ctrl_x;
	*ctrl_y = thiz->ctrl_y;
}

static inline void enesim_path_command_cubic_to_values_from(
		Enesim_Path_Command_Cubic_To *thiz,
		double x, double y, double ctrl_x0, double ctrl_y0,
		double ctrl_x1, double ctrl_y1)
{
	thiz->x = x;
	thiz->y = y;
	thiz->ctrl_x0 = ctrl_x0;
	thiz->ctrl_y0 = ctrl_y0;
	thiz->ctrl_x1 = ctrl_x1;
	thiz->ctrl_y1 = ctrl_y1;
}

static inline void enesim_path_command_cubic_to_values_to(
		Enesim_Path_Command_Cubic_To *thiz,
		double *x, double *y, double *ctrl_x0, double *ctrl_y0,
		double *ctrl_x1, double *ctrl_y1)
{
	*x = thiz->x;
	*y = thiz->y;
	*ctrl_x0 = thiz->ctrl_x0;
	*ctrl_y0 = thiz->ctrl_y0;
	*ctrl_x1 = thiz->ctrl_x1;
	*ctrl_y1 = thiz->ctrl_y1;
}

static inline void enesim_path_command_line_to_values_to(
		Enesim_Path_Command_Line_To *thiz,
		double *x, double *y)
{
	*x = thiz->x;
	*y = thiz->y;
}

static inline void enesim_path_command_line_to_values_from(
		Enesim_Path_Command_Line_To *thiz,
		double x, double y)
{
	thiz->x = x;
	thiz->y = y;
}

static inline void enesim_path_command_move_to_values_to(
		Enesim_Path_Command_Move_To *thiz,
		double *x, double *y)
{
	*x = thiz->x;
	*y = thiz->y;
}

static inline void enesim_path_command_move_to_values_from(
		Enesim_Path_Command_Move_To *thiz,
		double x, double y)
{
	thiz->x = x;
	thiz->y = y;
}

struct _Enesim_Path {
	/* this is to know whenever a command has been added/removed */
	Eina_Bool changed;
	/* the actual list of commands */
	Eina_List *commands;
	/* the refcounting */
	int ref;
};

void enesim_path_command_set(Enesim_Path *thiz, Eina_List *l);
void enesim_path_command_get(Enesim_Path *thiz, Eina_List **list);

#endif
