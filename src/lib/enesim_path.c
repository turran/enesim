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
#include "enesim_private.h"

#include "enesim_main.h"
#include "enesim_path.h"

#include "enesim_path_private.h"

/* TODO we now can get a point at a distance X, get the length of the path
 * etc, etc. But first we need to modify the enesim path renderer to
 * normalize the path and store it on the enesim path directly
 * also, add the genertad_flag here and a distance variable on the generator
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static void _cubic_flatten(double x0, double y0, double ctrl_x0,
		double ctrl_y0, double ctrl_x, double ctrl_y,
		double x, double y, double tolerance,
		Enesim_Path_Vertex_Add vertex_add, void *data)
{
	double x01, y01, x23, y23;
	double xa, ya, xb, yb, xc, yc;

	x01 = (x0 + x) / 2;
	y01 = (y0 + y) / 2;
	x23 = (ctrl_x0 + ctrl_x) / 2;
	y23 = (ctrl_y0 + ctrl_y) / 2;

	if ((((x01 - x23) * (x01 - x23)) + ((y01 - y23) * (y01 - y23))) <= tolerance)
	{
		vertex_add(x, y, data);
		return;
	}

	x01 = (x0 + ctrl_x0) / 2;
	y01 = (y0 + ctrl_y0) / 2;
	xc = x23;
	yc = y23;
	x23 = (x + ctrl_x) / 2;
	y23 = (y + ctrl_y) / 2;
	xa = (x01 + xc) / 2;
	ya = (y01 + yc) / 2;
	xb = (x23 + xc) / 2;
	yb = (y23 + yc) / 2;
	xc = (xa + xb) / 2;
	yc = (ya + yb) / 2;

	_cubic_flatten(x0, y0, x01, y01, xa, ya, xc, yc, tolerance, vertex_add, data);
	_cubic_flatten(xc, yc, xb, yb, x23, y23, x, y, tolerance, vertex_add, data);
}

static void _quadratic_flatten(double x0, double y0, double ctrl_x,
		double ctrl_y, double x, double y, double tolerance,
		Enesim_Path_Vertex_Add vertex_add, void *data)
{
	double x1, y1, x01, y01;

	/* TODO should we check here that the x,y == ctrl_x, ctrl_y? or on the caller */
	x01 = (x0 + x) / 2;
	y01 = (y0 + y) / 2;
	if ((((x01 - ctrl_x) * (x01 - ctrl_x)) + ((y01 - ctrl_y) * (y01
			- ctrl_y))) <= tolerance)
	{
		vertex_add(x, y, data);
		return;
	}

	x0 = (ctrl_x + x0) / 2;
	y0 = (ctrl_y + y0) / 2;
	x1 = (ctrl_x + x) / 2;
	y1 = (ctrl_y + y) / 2;
	x01 = (x0 + x1) / 2;
	y01 = (y0 + y1) / 2;

	_quadratic_flatten(x, y, x0, y0, x01, y01, tolerance, vertex_add, data);
	_quadratic_flatten(x01, y01, x1, y1, x, y, tolerance, vertex_add, data);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_path_quadratic_cubic_to(Enesim_Path_Quadratic *q,
		Enesim_Path_Cubic *c)
{
	/* same start and end points */
	c->end_x = q->end_x;
	c->start_x = q->start_x;
	c->end_y = q->end_y;
	c->start_y = q->start_y;
	/* create the new control points */
	c->ctrl_x0 = q->start_x + (2.0/3.0 * (q->ctrl_x - q->start_x));
	c->ctrl_x1 = q->end_x + (2.0/3.0 * (q->ctrl_x - q->end_x));

	c->ctrl_y0 = q->start_y + (2.0/3.0 * (q->ctrl_y - q->start_y));
	c->ctrl_y1 = q->end_y + (2.0/3.0 * (q->ctrl_y - q->end_y));
}

void enesim_path_quadratic_flatten(Enesim_Path_Quadratic *thiz,
		double tolerance, Enesim_Path_Vertex_Add vertex_add,
		void *data)
{
	_quadratic_flatten(thiz->start_x, thiz->start_y, thiz->ctrl_x,
			thiz->ctrl_y, thiz->end_x, thiz->end_y,
			tolerance, vertex_add, data);
}

void enesim_path_cubic_flatten(Enesim_Path_Cubic *thiz,
		double tolerance, Enesim_Path_Vertex_Add vertex_add,
		void *data)
{
	/* force one initial subdivision */
	double x01 = (thiz->start_x + thiz->ctrl_x0) / 2;
	double y01 = (thiz->start_y + thiz->ctrl_y0) / 2;
	double xc = (thiz->ctrl_x0 + thiz->ctrl_x1) / 2;
	double yc = (thiz->ctrl_y0 + thiz->ctrl_y1) / 2;
	double x23 = (thiz->end_x + thiz->ctrl_x1) / 2;
	double y23 = (thiz->end_y + thiz->ctrl_y1) / 2;
	double xa = (x01 + xc) / 2;
	double ya = (y01 + yc) / 2;
	double xb = (x23 + xc) / 2;
	double yb = (y23 + yc) / 2;

	xc = (xa + xb) / 2;
	yc = (ya + yb) / 2;
	_cubic_flatten(thiz->start_x, thiz->start_y, x01, y01, xa, ya, xc, yc,
			tolerance, vertex_add, data);
	_cubic_flatten(xc, yc, xb, yb, x23, y23, thiz->end_x, thiz->end_y,
			tolerance, vertex_add, data);
}

void enesim_path_command_set(Enesim_Path *thiz,
		Eina_List *list)
{
	Enesim_Path_Command *cmd;
	Eina_List *l;

	enesim_path_command_clear(thiz);
	EINA_LIST_FOREACH(list, l, cmd)
	{
		enesim_path_command_add(thiz, cmd);
	}
}

void enesim_path_command_get(Enesim_Path *thiz,
		Eina_List **list)
{
	Enesim_Path_Command *cmd;
	Eina_List *l;

	EINA_LIST_FOREACH(thiz->commands, l, cmd)
	{
		Enesim_Path_Command *new_cmd;
		new_cmd = calloc(1, sizeof(Enesim_Path_Command));
		*new_cmd = *cmd;
		*list = eina_list_append(*list, new_cmd);
	}
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Creates a new path
 * @return The newly created path
 */
EAPI Enesim_Path * enesim_path_new(void)
{
	Enesim_Path *thiz;

	thiz = calloc(1, sizeof(Enesim_Path));
	thiz->ref = 1;
	return thiz;
}

/**
 * @brief Increase the reference counter of a path
 * @param[in] thiz The path
 * @return The input parameter @a thiz for programming convenience
 */
EAPI Enesim_Path * enesim_path_ref(Enesim_Path *thiz)
{
	if (!thiz) return NULL;
	thiz->ref++;
	return thiz;
}

/**
 * @brief Decrease the reference counter of a path
 * @param[in] thiz The path
 */
EAPI void enesim_path_unref(Enesim_Path *thiz)
{
	thiz->ref--;
	if (!thiz->ref)
	{
		enesim_path_command_clear(thiz);
		free(thiz);
	}
}

/**
 * Clear the command list of a path
 * @param[in] thiz The path to clear
 */
EAPI void enesim_path_command_clear(Enesim_Path *thiz)
{
	Enesim_Path_Command *c;

	EINA_LIST_FREE(thiz->commands, c)
	{
		free(c);
	}
	thiz->commands = eina_list_free(thiz->commands);
}

/**
 * Add a command to a path
 * @param[in] thiz The path to add the command to
 * @param[in] cmd The command to add
 */
EAPI void enesim_path_command_add(Enesim_Path *thiz, Enesim_Path_Command *cmd)
{
	Enesim_Path_Command *new_command;

	/* do not allow a move to command after another move to, just simplfiy them */
	if (cmd->type == ENESIM_PATH_COMMAND_MOVE_TO)
	{
		Enesim_Path_Command *last_command;
		Eina_List *last;

		last = eina_list_last(thiz->commands);
		last_command = eina_list_data_get(last);
		if (last_command && last_command->type == ENESIM_PATH_COMMAND_MOVE_TO)
		{
			last_command->definition.move_to.x = cmd->definition.move_to.x;
			last_command->definition.move_to.y = cmd->definition.move_to.y;
			return;
		}
	}

	new_command = malloc(sizeof(Enesim_Path_Command));
	*new_command = *cmd;
	thiz->commands = eina_list_append(thiz->commands, new_command);
	thiz->changed = EINA_TRUE;
}

/**
 * Add a move to command to a path
 * @param[in] thiz The path to add the command to
 * @param[in] x The X coordinate of the move to command
 * @param[in] y The Y coordinate of the move to command
 */
EAPI void enesim_path_move_to(Enesim_Path *thiz, double x, double y)
{
	Enesim_Path_Command cmd;

	cmd.type = ENESIM_PATH_COMMAND_MOVE_TO;
	cmd.definition.move_to.x = x;
	cmd.definition.move_to.y = y;
	enesim_path_command_add(thiz, &cmd);
}

/**
 * Add a line to command to a path
 * @param[in] thiz The path to add the command to
 * @param[in] x The X coordinate of the line to command
 * @param[in] y The Y coordinate of the line to command
 */
EAPI void enesim_path_line_to(Enesim_Path *thiz, double x, double y)
{
	Enesim_Path_Command cmd;

	cmd.type = ENESIM_PATH_COMMAND_LINE_TO;
	cmd.definition.line_to.x = x;
	cmd.definition.line_to.y = y;
	enesim_path_command_add(thiz, &cmd);
}

/**
 * Add a smooth quadratic to command to a path
 * @param[in] thiz The path to add the command to
 * @param[in] x The X destination coordinate
 * @param[in] y The Y destination coordinate
 */
EAPI void enesim_path_squadratic_to(Enesim_Path *thiz, double x,
		double y)
{
	Enesim_Path_Command cmd;

	cmd.type = ENESIM_PATH_COMMAND_SQUADRATIC_TO;
	cmd.definition.squadratic_to.x = x;
	cmd.definition.squadratic_to.y = y;
	enesim_path_command_add(thiz, &cmd);
}

/**
 * Add a quadratic to command to a path
 * @param[in] thiz The path to add the command to
 * @param[in] ctrl_x The control point X coordinate
 * @param[in] ctrl_y The control point Y coordinate 
 * @param[in] x The X destination coordinate
 * @param[in] y The Y destination coordinate
 */
EAPI void enesim_path_quadratic_to(Enesim_Path *thiz, double ctrl_x,
		double ctrl_y, double x, double y)
{
	Enesim_Path_Command cmd;

	cmd.type = ENESIM_PATH_COMMAND_QUADRATIC_TO;
	cmd.definition.quadratic_to.x = x;
	cmd.definition.quadratic_to.y = y;
	cmd.definition.quadratic_to.ctrl_x = ctrl_x;
	cmd.definition.quadratic_to.ctrl_y = ctrl_y;
	enesim_path_command_add(thiz, &cmd);
}

/**
 * Add a cubic to command to a path
 * @param[in] thiz The path to add the command to
 * @param[in] ctrl_x0 The first control point X coordinate
 * @param[in] ctrl_y0 The first control point Y coordinate 
 * @param[in] ctrl_x The second control point X coordinate
 * @param[in] ctrl_y The second control point Y coordinate 
 * @param[in] x The X destination coordinate
 * @param[in] y The Y destination coordinate
 */
EAPI void enesim_path_cubic_to(Enesim_Path *thiz, double ctrl_x0,
		double ctrl_y0, double ctrl_x, double ctrl_y, double x, double y)
{
	Enesim_Path_Command cmd;

	cmd.type = ENESIM_PATH_COMMAND_CUBIC_TO;
	cmd.definition.cubic_to.x = x;
	cmd.definition.cubic_to.y = y;
	cmd.definition.cubic_to.ctrl_x0 = ctrl_x0;
	cmd.definition.cubic_to.ctrl_y0 = ctrl_y0;
	cmd.definition.cubic_to.ctrl_x1 = ctrl_x;
	cmd.definition.cubic_to.ctrl_y1 = ctrl_y;
	enesim_path_command_add(thiz, &cmd);
}

/**
 * Add a smooth cubic to command to a path
 * @param[in] thiz The path to add the command to
 * @param[in] ctrl_x The control point X coordinate
 * @param[in] ctrl_y The control point Y coordinate 
 * @param[in] x The X destination coordinate
 * @param[in] y The Y destination coordinate
 */
EAPI void enesim_path_scubic_to(Enesim_Path *thiz, double ctrl_x,
		double ctrl_y, double x, double y)
{
	Enesim_Path_Command cmd;

	cmd.type = ENESIM_PATH_COMMAND_SCUBIC_TO;
	cmd.definition.scubic_to.x = x;
	cmd.definition.scubic_to.y = y;
	cmd.definition.scubic_to.ctrl_x = ctrl_x;
	cmd.definition.scubic_to.ctrl_y = ctrl_y;
	enesim_path_command_add(thiz, &cmd);
}


/**
 * Add a an arc to command to a path
 * @param[in] thiz The path to add the command to
 * @param[in] rx The radius in X
 * @param[in] ry The radius in Y
 * @param[in] angle The rotation on the X coordinate
 * @param[in] large If Eina_True the large arc will be chosen
 * @param[in] sweep If Eina_True the arc in positive-angle will be chosen
 * @param[in] x The X destination coordinate
 * @param[in] y The Y destination coordinate
 */
EAPI void enesim_path_arc_to(Enesim_Path *thiz, double rx, double ry, double angle,
                   unsigned char large, unsigned char sweep, double x, double y)
{
	Enesim_Path_Command cmd;

	cmd.type = ENESIM_PATH_COMMAND_ARC_TO;
	cmd.definition.arc_to.x = x;
	cmd.definition.arc_to.y = y;
	cmd.definition.arc_to.rx = rx;
	cmd.definition.arc_to.ry = ry;
	cmd.definition.arc_to.angle = angle;
	cmd.definition.arc_to.large = large;
	cmd.definition.arc_to.sweep = sweep;
	enesim_path_command_add(thiz, &cmd);
}

/**
 * Close a path
 * @param[in] thiz The path to close
 */
EAPI void enesim_path_close(Enesim_Path *thiz)
{
	Enesim_Path_Command cmd;

	cmd.type = ENESIM_PATH_COMMAND_CLOSE;
	cmd.definition.close.close = EINA_TRUE;
	enesim_path_command_add(thiz, &cmd);
}
