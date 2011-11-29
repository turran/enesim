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
#include "Enesim.h"
#include "enesim_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/*
  very simple path struct..
  should maybe keep path commands
  so that we can defer subdiv approx
  to the setup function.
*/
typedef struct _Enesim_Renderer_Path
{
	Enesim_Renderer_Sw_Fill fill;
	Enesim_Renderer *figure;
	Eina_List *commands;
	Eina_Bool changed : 1;
	double last_x;
	double last_y;
	double last_ctrl_x;
	double last_ctrl_y;
} Enesim_Renderer_Path;

static inline Enesim_Renderer_Path * _path_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	return thiz;
}

static void _move_to(Enesim_Renderer_Path *thiz, double x, double y)
{
	enesim_renderer_figure_polygon_add(thiz->figure);
	enesim_renderer_figure_polygon_vertex_add(thiz->figure, x, y);
	thiz->last_x = x;
	thiz->last_y = y;
	thiz->last_ctrl_x = x;
	thiz->last_ctrl_y = y;
}

static void _line_to(Enesim_Renderer_Path *thiz, double x, double y)
{
	enesim_renderer_figure_polygon_vertex_add(thiz->figure, x, y);
	thiz->last_ctrl_x = thiz->last_x;
	thiz->last_ctrl_y = thiz->last_y;
	thiz->last_x = x;
	thiz->last_y = y;
}

/* these subdiv approximations need to be done more carefully */
static void _quadratic_to(Enesim_Renderer_Path *thiz, double ctrl_x, double ctrl_y,
		double x, double y)
{
	double x0, y0, x1, y1, x01, y01;
	double sm = 1 / 92.0;

	x0 = thiz->last_x;
	y0 = thiz->last_y;
	x01 = (x0 + x) / 2;
	y01 = (y0 + y) / 2;
	if ((((x01 - ctrl_x) * (x01 - ctrl_x)) + ((y01 - ctrl_y) * (y01
			- ctrl_y))) <= sm)
	{
		enesim_renderer_figure_polygon_vertex_add(thiz->figure, x, y);
		thiz->last_x = x;
		thiz->last_y = y;
		thiz->last_ctrl_x = ctrl_x;
		thiz->last_ctrl_y = ctrl_y;
		return;
	}

	x0 = (ctrl_x + x0) / 2;
	y0 = (ctrl_y + y0) / 2;
	x1 = (ctrl_x + x) / 2;
	y1 = (ctrl_y + y) / 2;
	x01 = (x0 + x1) / 2;
	y01 = (y0 + y1) / 2;

	_quadratic_to(thiz, x0, y0, x01, y01);
	thiz->last_x = x01;
	thiz->last_y = y01;
	thiz->last_ctrl_x = x0;
	thiz->last_ctrl_y = y0;

	_quadratic_to(thiz, x1, y1, x, y);
	thiz->last_x = x;
	thiz->last_y = y;
	thiz->last_ctrl_x = x1;
	thiz->last_ctrl_y = y1;
}

static void _squadratic_to(Enesim_Renderer_Path *thiz, double x, double y)
{
	double x0, y0, cx0, cy0;

	x0 = thiz->last_x;
	y0 = thiz->last_y;
	cx0 = thiz->last_ctrl_x;
	cy0 = thiz->last_ctrl_y;
	cx0 = (2 * x0) - cx0;
	cy0 = (2 * y0) - cy0;

	_quadratic_to(thiz, cx0, cy0, x, y);
	thiz->last_x = x;
	thiz->last_y = y;
	thiz->last_ctrl_x = cx0;
	thiz->last_ctrl_y = cy0;
}

static void _cubic_to(Enesim_Renderer_Path *thiz, double ctrl_x0, double ctrl_y0,
		double ctrl_x, double ctrl_y, double x, double y)
{
	double x0, y0, x01, y01, x23, y23;
	double xa, ya, xb, yb, xc, yc;
	double sm = 1 / 64.0;

	x0 = thiz->last_x;
	y0 = thiz->last_y;
	x01 = (x0 + x) / 2;
	y01 = (y0 + y) / 2;
	x23 = (ctrl_x0 + ctrl_x) / 2;
	y23 = (ctrl_y0 + ctrl_y) / 2;

	if ((((x01 - x23) * (x01 - x23)) + ((y01 - y23) * (y01 - y23))) <= sm)
	{
		enesim_renderer_figure_polygon_vertex_add(thiz->figure, x, y);
		thiz->last_x = x;
		thiz->last_y = y;
		thiz->last_ctrl_x = ctrl_x;
		thiz->last_ctrl_y = ctrl_y;
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

	_cubic_to(thiz, x01, y01, xa, ya, xc, yc);
	thiz->last_x = xc;
	thiz->last_y = yc;
	thiz->last_ctrl_x = xa;
	thiz->last_ctrl_y = ya;

	_cubic_to(thiz, xb, yb, x23, y23, x, y);
	thiz->last_x = x;
	thiz->last_y = y;
	thiz->last_ctrl_x = x23;
	thiz->last_ctrl_y = y23;
}


static void _scubic_to(Enesim_Renderer_Path *thiz, double ctrl_x, double ctrl_y, double x,
		double y)
{
	double x0, y0, cx0, cy0;

	x0 = thiz->last_x;
	y0 = thiz->last_y;
	cx0 = thiz->last_ctrl_x;
	cy0 = thiz->last_ctrl_y;
	cx0 = (2 * x0) - cx0;
	cy0 = (2 * y0) - cy0;

	_cubic_to(thiz, cx0, cy0, ctrl_x, ctrl_y, x, y);
	thiz->last_x = x;
	thiz->last_y = y;
	thiz->last_ctrl_x = ctrl_x;
	thiz->last_ctrl_y = ctrl_y;
}

static void _arc_to(Enesim_Renderer_Path *thiz, double rx, double ry, double angle,
                   unsigned char large, unsigned char sweep, double x, double y)
{

}

static void _close(Enesim_Renderer_Path *thiz)
{

}

static void _path_generate_vertices(Enesim_Renderer_Path *thiz)
{
	Eina_List *l;
	Enesim_Renderer_Path_Command *cmd;

	enesim_renderer_figure_clear(thiz->figure);
	EINA_LIST_FOREACH(thiz->commands, l, cmd)
	{
		/* send the new vertex to the figure renderer */
		switch (cmd->type)
		{
			case ENESIM_COMMAND_MOVE_TO:
			_move_to(thiz, cmd->definition.move_to.x, cmd->definition.move_to.y);
			break;

			case ENESIM_COMMAND_LINE_TO:
			_line_to(thiz, cmd->definition.line_to.x, cmd->definition.line_to.y);
			break;

			case ENESIM_COMMAND_QUADRATIC_TO:
			_quadratic_to(thiz, cmd->definition.quadratic_to.ctrl_x,
					cmd->definition.quadratic_to.ctrl_y,
					cmd->definition.quadratic_to.x,
					cmd->definition.quadratic_to.y);
			break;

			case ENESIM_COMMAND_SQUADRATIC_TO:
			_squadratic_to(thiz, cmd->definition.squadratic_to.x,
					cmd->definition.squadratic_to.y);
			break;

			case ENESIM_COMMAND_CUBIC_TO:
			_cubic_to(thiz, cmd->definition.cubic_to.ctrl_x0,
					cmd->definition.cubic_to.ctrl_y0,
					cmd->definition.cubic_to.ctrl_x1,
					cmd->definition.cubic_to.ctrl_y1,
					cmd->definition.cubic_to.x,
					cmd->definition.cubic_to.y);
			break;

			case ENESIM_COMMAND_SCUBIC_TO:
			_scubic_to(thiz, cmd->definition.scubic_to.ctrl_x,
					cmd->definition.scubic_to.ctrl_y,
					cmd->definition.scubic_to.x,
					cmd->definition.scubic_to.y);
			break;

			case ENESIM_COMMAND_ARC_TO:
			_arc_to(thiz, cmd->definition.arc_to.rx,
					cmd->definition.arc_to.ry,
					cmd->definition.arc_to.angle,
					cmd->definition.arc_to.large,
					cmd->definition.arc_to.sweep,
					cmd->definition.arc_to.x,
					cmd->definition.arc_to.y);
			break;

			case ENESIM_COMMAND_CLOSE:
			_close(thiz);
			break;

			default:
			break;
		}
	}
}

static void _span(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Path *thiz;

	thiz = _path_get(r);
	thiz->fill(thiz->figure, x, y, len, ddata);
}

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _path_name(Enesim_Renderer *r)
{
	return "path";
}

static Eina_Bool _state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Color stroke_color;
	Enesim_Renderer *stroke_renderer;
	Enesim_Color fill_color;
	Enesim_Renderer *fill_renderer;
	Enesim_Shape_Draw_Mode draw_mode;
	double stroke_weight;

	thiz = _path_get(r);

	/* iterate over the list of commands */
	if (thiz->changed)
	{
		_path_generate_vertices(thiz);
	}

	/* FIXME given that we now pass the state, there's no need to gt/set every property
	 * just pass the state or set the values
	 */

	enesim_renderer_shape_stroke_weight_get(r, &stroke_weight);
	enesim_renderer_shape_stroke_weight_set(thiz->figure, stroke_weight);

	enesim_renderer_shape_stroke_color_get(r, &stroke_color);
	enesim_renderer_shape_stroke_color_set(thiz->figure, stroke_color);

	enesim_renderer_shape_stroke_renderer_get(r, &stroke_renderer);
	enesim_renderer_shape_stroke_renderer_set(thiz->figure, stroke_renderer);

	enesim_renderer_shape_fill_color_get(r, &fill_color);
	enesim_renderer_shape_fill_color_set(thiz->figure, fill_color);

	enesim_renderer_shape_fill_renderer_get(r, &fill_renderer);
	enesim_renderer_shape_fill_renderer_set(thiz->figure, fill_renderer);

	enesim_renderer_shape_draw_mode_get(thiz->figure, &draw_mode);
	enesim_renderer_shape_draw_mode_set(thiz->figure, draw_mode);

	enesim_renderer_origin_set(thiz->figure, state->ox, state->oy);
	enesim_renderer_transformation_set(thiz->figure, &state->transformation);

	if (!enesim_renderer_setup(thiz->figure, s, error))
	{
		return EINA_FALSE;
	}

	thiz->fill = enesim_renderer_sw_fill_get(thiz->figure);
	if (!thiz->fill)
	{
		return EINA_FALSE;
	}

	*fill = _span;

	return EINA_TRUE;
}

static void _state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Path *thiz;

	thiz = _path_get(r);
	enesim_renderer_cleanup(thiz->figure, s);
	thiz->changed = EINA_FALSE;
}

static void _boundings(Enesim_Renderer *r, Enesim_Rectangle *boundings)
{
	Enesim_Renderer_Path *thiz;

	thiz = _path_get(r);

	/* FIXME fix this */
	if (!thiz->figure) return;
	enesim_renderer_boundings(thiz->figure, boundings);
}

static Enesim_Renderer_Descriptor _path_descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _path_name,
	/* .free = 			*/ NULL,
	/* .boundings = 		*/ NULL, // _boundings,
	/* .destination_transform = 	*/ NULL,
	/* .flags = 			*/ NULL,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ NULL,
	/* .sw_setup = 			*/ _state_setup,
	/* .sw_cleanup = 		*/ _state_cleanup
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Renderer * enesim_renderer_path_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Path *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Path));
	if (!thiz) return NULL;
	r = enesim_renderer_figure_new();
	if (!r) goto err_figure;
	thiz->figure = r;
	
	r = enesim_renderer_shape_new(&_path_descriptor, thiz);
	return r;

err_figure:
	free(thiz);
	return NULL;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_command_clear(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;

	thiz = _path_get(r);

	thiz->last_x = 0;
	thiz->last_y = 0;
	thiz->last_ctrl_x = 0;
	thiz->last_ctrl_y = 0;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_command_add(Enesim_Renderer *r, Enesim_Renderer_Path_Command *cmd)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Renderer_Path_Command *new_command;

	thiz = _path_get(r);

	new_command = malloc(sizeof(Enesim_Renderer_Path_Command));
	*new_command = *cmd;
	thiz->commands = eina_list_append(thiz->commands, new_command);
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_move_to(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Path_Command cmd;

	cmd.type = ENESIM_COMMAND_MOVE_TO;
	cmd.definition.move_to.x = ((int) (2* x + 0.5)) / 2.0;
	cmd.definition.move_to.y = ((int) (2* y + 0.5)) / 2.0;
	enesim_renderer_path_command_add(r, &cmd);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_line_to(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Path_Command cmd;

	cmd.type = ENESIM_COMMAND_LINE_TO;
	cmd.definition.line_to.x = ((int) (2* x + 0.5)) / 2.0;
	cmd.definition.line_to.y = ((int) (2* y + 0.5)) / 2.0;
	enesim_renderer_path_command_add(r, &cmd);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_squadratic_to(Enesim_Renderer *r, double x,
		double y)
{
	Enesim_Renderer_Path_Command cmd;

	cmd.type = ENESIM_COMMAND_SQUADRATIC_TO;
	cmd.definition.squadratic_to.x = ((int) (2* x + 0.5)) / 2.0;
	cmd.definition.squadratic_to.y = ((int) (2* y + 0.5)) / 2.0;
	enesim_renderer_path_command_add(r, &cmd);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_quadratic_to(Enesim_Renderer *r, double ctrl_x,
		double ctrl_y, double x, double y)
{
	Enesim_Renderer_Path_Command cmd;

	cmd.type = ENESIM_COMMAND_QUADRATIC_TO;
	cmd.definition.quadratic_to.x = ((int) (2* x + 0.5)) / 2.0;
	cmd.definition.quadratic_to.y = ((int) (2* y + 0.5)) / 2.0;
	cmd.definition.quadratic_to.ctrl_x = ctrl_x;
	cmd.definition.quadratic_to.ctrl_y = ctrl_y;
	enesim_renderer_path_command_add(r, &cmd);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_cubic_to(Enesim_Renderer *r, double ctrl_x0,
		double ctrl_y0, double ctrl_x, double ctrl_y, double x, double y)
{
	Enesim_Renderer_Path_Command cmd;

	cmd.type = ENESIM_COMMAND_CUBIC_TO;
	cmd.definition.cubic_to.x = ((int) (2* x + 0.5)) / 2.0;
	cmd.definition.cubic_to.y = ((int) (2* y + 0.5)) / 2.0;
	cmd.definition.cubic_to.ctrl_x0 = ctrl_x0;
	cmd.definition.cubic_to.ctrl_y0 = ctrl_y0;
	cmd.definition.cubic_to.ctrl_x1 = ctrl_x;
	cmd.definition.cubic_to.ctrl_y1 = ctrl_y;
	enesim_renderer_path_command_add(r, &cmd);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_scubic_to(Enesim_Renderer *r, double ctrl_x,
		double ctrl_y, double x, double y)
{
	Enesim_Renderer_Path_Command cmd;

	cmd.type = ENESIM_COMMAND_SCUBIC_TO;
	cmd.definition.scubic_to.x = ((int) (2* x + 0.5)) / 2.0;
	cmd.definition.scubic_to.y = ((int) (2* y + 0.5)) / 2.0;
	cmd.definition.scubic_to.ctrl_x = ctrl_x;
	cmd.definition.scubic_to.ctrl_y = ctrl_y;
	enesim_renderer_path_command_add(r, &cmd);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_arc_to(Enesim_Renderer *r, double rx, double ry, double angle,
                   unsigned char large, unsigned char sweep, double x, double y)
{
	Enesim_Renderer_Path_Command cmd;

	cmd.type = ENESIM_COMMAND_ARC_TO;
	cmd.definition.arc_to.x = ((int) (2* x + 0.5)) / 2.0;
	cmd.definition.arc_to.y = ((int) (2* y + 0.5)) / 2.0;
	cmd.definition.arc_to.rx = rx;
	cmd.definition.arc_to.ry = ry;
	cmd.definition.arc_to.angle = angle;
	cmd.definition.arc_to.large = large;
	cmd.definition.arc_to.sweep = sweep;
	enesim_renderer_path_command_add(r, &cmd);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_close(Enesim_Renderer *r, Eina_Bool close)
{
	Enesim_Renderer_Path_Command cmd;

	cmd.type = ENESIM_COMMAND_CLOSE;
	enesim_renderer_path_command_add(r, &cmd);
}
