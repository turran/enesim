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
typedef void (*Enesim_Renderer_Path_Vertex_Add)(double x, double y, void *data);
typedef void (*Enesim_Renderer_Path_Polygon_Add)(void *data);

typedef struct _Enesim_Renderer_Path_Command_State
{
	Enesim_Renderer_Path_Vertex_Add vertex_add;
	Enesim_Renderer_Path_Polygon_Add polygon_add;
	double last_x;
	double last_y;
	double last_ctrl_x;
	double last_ctrl_y;
	void *data;
} Enesim_Renderer_Path_Command_State;

typedef struct _Enesim_Renderer_Path_Stroke_State
{
	Eina_F16p16 r;
	Enesim_F16p16_Point p0, p1, p2;
	Eina_F16p16 x01;
	Eina_F16p16 y01;
	Eina_F16p16 x12;
	Eina_F16p16 y12;
	Enesim_F16p16_Line e01, e01_nm, e01_np;
	Enesim_F16p16_Line e12, e12_nm, e12_np;
	int count;
} Enesim_Renderer_Path_Stroke_State;

typedef struct _Enesim_Renderer_Path
{
	/* properties */
	Eina_List *commands;
	/* private */
	Enesim_Renderer_Sw_Fill fill;
	Enesim_Renderer *figure;
	Enesim_Renderer_Path_Stroke_State stroke_state;
	Eina_Bool changed : 1;
} Enesim_Renderer_Path;

static inline Enesim_Renderer_Path * _path_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	return thiz;
}
/*----------------------------------------------------------------------------*
 *                              Without stroke                                *
 *----------------------------------------------------------------------------*/
/* For this case we simply pass the vertex/polygon directly to the figure */
static void _fill_only_path_vertex_add(double x, double y, void *data)
{
	Enesim_Renderer_Path *thiz = data;
	enesim_renderer_figure_polygon_vertex_add(thiz->figure, x, y);
}

static void _fill_only_path_polygon_add(void *data)
{
	Enesim_Renderer_Path *thiz = data;
	enesim_renderer_figure_polygon_add(thiz->figure);
}
/*----------------------------------------------------------------------------*
 *                                With stroke                                 *
 *----------------------------------------------------------------------------*/
static inline void _do_lines(Eina_F16p16 r, Enesim_F16p16_Point *p0, Enesim_F16p16_Point *p1,
		Eina_F16p16 *x01, Eina_F16p16 *y01,
		Enesim_F16p16_Line *offset, Enesim_F16p16_Line *np, Enesim_F16p16_Line *nm)
{
	Eina_F16p16 lx01;
	Eina_F16p16 ly01;

	lx01 = p1->x - p0->x;
	ly01 = p1->y - p0->y;

	enesim_f16p16_line_f16p16_direction_from(offset, p0, lx01, ly01);
	offset->c = eina_f16p16_mul(r, hypot(offset->a, offset->b));
	enesim_f16p16_line_f16p16_direction_from(np, p0, -ly01, lx01);
	enesim_f16p16_line_f16p16_direction_from(nm, p1, ly01, -lx01);
	*x01 = lx01;
	*y01 = ly01;
}

static void _stroke_path_vertex_add(double x, double y, void *data)
{
	Enesim_Renderer_Path *thiz = data;
	Enesim_Renderer_Path_Stroke_State *state = &thiz->stroke_state;
	int c;

	if (state->count < 2)
	{
		/* store the initial vertices */
		switch (state->count)
		{
			case 0:
			state->p0.x = eina_f16p16_double_from(x);
			state->p0.y = eina_f16p16_double_from(y);
			state->count++;
			return;

			case 1:
			state->p1.x = eina_f16p16_double_from(x);
			state->p1.y = eina_f16p16_double_from(y);
			_do_lines(state->r, &state->p0, &state->p1,
					&state->x01, &state->y01,
					&state->e01, &state->e01_np, &state->e01_nm);
			state->count++;
			return;

			default:
			break;
		}
	}
	/* do the calculations */
	state->p2.x = eina_f16p16_double_from(x);
	state->p2.y = eina_f16p16_double_from(y);
	_do_lines(state->r, &state->p1, &state->p2,
			&state->x12, &state->y12,
			&state->e12, &state->e12_np, &state->e12_nm);
	state->count++;

	/* dot product
	 * = 1 pointing same direction
	 * > 0 concave
	 * = 0 orthogonal
	 * < 0 convex
	 * = -1 pointing opposite direction
	 */

	c = eina_f16p16_mul(state->x01, state->x12) + eina_f16p16_mul(state->y01, state->y12);
	/* convex case */
	if (c <= 0)
	{
		double x;
		double y;

		/* intersect the paralel and orthogonal lines */
		/* or calculate the direction vector of the perpendicular lines and multiply by r */
		printf("convex\n");
		//enesim_renderer_figure_polygon_vertex_add(thiz->figure, x, y);
		//enesim_renderer_figure_polygon_vertex_add(thiz->figure, x, y);
	}
	/* concave case */
	else
	{
		double x;
		double y;

		/* intersect the paralel lines */
		/* or calculate the bisector vector and multiply by r */
		printf("concave\n");
		//enesim_renderer_figure_polygon_vertex_add(thiz->figure, x, y);
	}
	/* we do have enough vertices, just swap them */
	state->p0 = state->p1;
	state->p1 = state->p2;
	state->e01 = state->e12;
	state->e01_np = state->e12_np;
	state->e01_nm = state->e12_nm;

}

static void _stroke_path_polygon_add(void *data)
{
	Enesim_Renderer_Path *thiz = data;
	Enesim_Renderer_Path_Stroke_State *state = &thiz->stroke_state;

	/* just reset */
	state->count = 0;
	enesim_renderer_figure_polygon_add(thiz->figure);
}
/*----------------------------------------------------------------------------*
 *                                 Commands                                   *
 *----------------------------------------------------------------------------*/
static void _move_to(Enesim_Renderer_Path_Command_State *state,
		double x, double y)
{
	state->polygon_add(state->data);
	state->vertex_add(x, y, state->data);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = x;
	state->last_ctrl_y = y;
}

static void _line_to(Enesim_Renderer_Path_Command_State *state,
		double x, double y)
{
	state->vertex_add(x, y, state->data);
	state->last_ctrl_x = state->last_x;
	state->last_ctrl_y = state->last_y;
	state->last_x = x;
	state->last_y = y;
}

/* these subdiv approximations need to be done more carefully */
static void _quadratic_to(Enesim_Renderer_Path_Command_State *state,
		double ctrl_x, double ctrl_y,
		double x, double y)
{
	double x0, y0, x1, y1, x01, y01;
	double sm = 1 / 92.0;

	x0 = state->last_x;
	y0 = state->last_y;
	x01 = (x0 + x) / 2;
	y01 = (y0 + y) / 2;
	if ((((x01 - ctrl_x) * (x01 - ctrl_x)) + ((y01 - ctrl_y) * (y01
			- ctrl_y))) <= sm)
	{
		state->vertex_add(x, y, state->data);
		state->last_x = x;
		state->last_y = y;
		state->last_ctrl_x = ctrl_x;
		state->last_ctrl_y = ctrl_y;
		return;
	}

	x0 = (ctrl_x + x0) / 2;
	y0 = (ctrl_y + y0) / 2;
	x1 = (ctrl_x + x) / 2;
	y1 = (ctrl_y + y) / 2;
	x01 = (x0 + x1) / 2;
	y01 = (y0 + y1) / 2;

	_quadratic_to(state, x0, y0, x01, y01);
	state->last_x = x01;
	state->last_y = y01;
	state->last_ctrl_x = x0;
	state->last_ctrl_y = y0;

	_quadratic_to(state, x1, y1, x, y);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = x1;
	state->last_ctrl_y = y1;
}

static void _squadratic_to(Enesim_Renderer_Path_Command_State *state,
		double x, double y)
{
	double x0, y0, cx0, cy0;

	x0 = state->last_x;
	y0 = state->last_y;
	cx0 = state->last_ctrl_x;
	cy0 = state->last_ctrl_y;
	cx0 = (2 * x0) - cx0;
	cy0 = (2 * y0) - cy0;

	_quadratic_to(state, cx0, cy0, x, y);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = cx0;
	state->last_ctrl_y = cy0;
}

static void _cubic_to(Enesim_Renderer_Path_Command_State *state,
		double ctrl_x0, double ctrl_y0,
		double ctrl_x, double ctrl_y,
		double x, double y)
{
	double x0, y0, x01, y01, x23, y23;
	double xa, ya, xb, yb, xc, yc;
	double sm = 1 / 64.0;

	x0 = state->last_x;
	y0 = state->last_y;
	x01 = (x0 + x) / 2;
	y01 = (y0 + y) / 2;
	x23 = (ctrl_x0 + ctrl_x) / 2;
	y23 = (ctrl_y0 + ctrl_y) / 2;

	if ((((x01 - x23) * (x01 - x23)) + ((y01 - y23) * (y01 - y23))) <= sm)
	{
		state->vertex_add(x, y, state->data);
		state->last_x = x;
		state->last_y = y;
		state->last_ctrl_x = ctrl_x;
		state->last_ctrl_y = ctrl_y;
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

	_cubic_to(state, x01, y01, xa, ya, xc, yc);
	state->last_x = xc;
	state->last_y = yc;
	state->last_ctrl_x = xa;
	state->last_ctrl_y = ya;

	_cubic_to(state, xb, yb, x23, y23, x, y);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = x23;
	state->last_ctrl_y = y23;
}


static void _scubic_to(Enesim_Renderer_Path_Command_State *state,
		double ctrl_x, double ctrl_y,
		double x, double y)
{
	double x0, y0, cx0, cy0;

	x0 = state->last_x;
	y0 = state->last_y;
	cx0 = state->last_ctrl_x;
	cy0 = state->last_ctrl_y;
	cx0 = (2 * x0) - cx0;
	cy0 = (2 * y0) - cy0;

	_cubic_to(state, cx0, cy0, ctrl_x, ctrl_y, x, y);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = ctrl_x;
	state->last_ctrl_y = ctrl_y;
}

static void _arc_to(Enesim_Renderer_Path_Command_State *state, double rx, double ry, double angle,
                   unsigned char large, unsigned char sweep, double x, double y)
{

}

static void _close(Enesim_Renderer_Path_Command_State *state)
{

}

static void _path_generate_vertices(Enesim_Renderer_Path *thiz,
		Enesim_Renderer_Path_Vertex_Add vertex_add,
		Enesim_Renderer_Path_Polygon_Add polygon_add)
{
	Eina_List *l;
	Enesim_Renderer_Path_Command_State state;
	Enesim_Renderer_Path_Command *cmd;

	state.vertex_add = vertex_add;
	state.polygon_add = polygon_add;
	state.last_x = 0;
	state.last_y = 0;
	state.last_ctrl_x = 0;
	state.last_ctrl_y = 0;
	state.data = thiz;

	EINA_LIST_FOREACH(thiz->commands, l, cmd)
	{
		/* send the new vertex to the figure renderer */
		switch (cmd->type)
		{
			case ENESIM_COMMAND_MOVE_TO:
			_move_to(&state, cmd->definition.move_to.x, cmd->definition.move_to.y);
			break;

			case ENESIM_COMMAND_LINE_TO:
			_line_to(&state, cmd->definition.line_to.x, cmd->definition.line_to.y);
			break;

			case ENESIM_COMMAND_QUADRATIC_TO:
			_quadratic_to(&state, cmd->definition.quadratic_to.ctrl_x,
					cmd->definition.quadratic_to.ctrl_y,
					cmd->definition.quadratic_to.x,
					cmd->definition.quadratic_to.y);
			break;

			case ENESIM_COMMAND_SQUADRATIC_TO:
			_squadratic_to(&state, cmd->definition.squadratic_to.x,
					cmd->definition.squadratic_to.y);
			break;

			case ENESIM_COMMAND_CUBIC_TO:
			_cubic_to(&state, cmd->definition.cubic_to.ctrl_x0,
					cmd->definition.cubic_to.ctrl_y0,
					cmd->definition.cubic_to.ctrl_x1,
					cmd->definition.cubic_to.ctrl_y1,
					cmd->definition.cubic_to.x,
					cmd->definition.cubic_to.y);
			break;

			case ENESIM_COMMAND_SCUBIC_TO:
			_scubic_to(&state, cmd->definition.scubic_to.ctrl_x,
					cmd->definition.scubic_to.ctrl_y,
					cmd->definition.scubic_to.x,
					cmd->definition.scubic_to.y);
			break;

			case ENESIM_COMMAND_ARC_TO:
			_arc_to(&state, cmd->definition.arc_to.rx,
					cmd->definition.arc_to.ry,
					cmd->definition.arc_to.angle,
					cmd->definition.arc_to.large,
					cmd->definition.arc_to.sweep,
					cmd->definition.arc_to.x,
					cmd->definition.arc_to.y);
			break;

			case ENESIM_COMMAND_CLOSE:
			_close(&state);
			break;

			default:
			break;
		}
	}
}

static void _path_stroke(Enesim_Renderer_Path *thiz, double stroke_weight)
{
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

	enesim_renderer_shape_stroke_weight_get(r, &stroke_weight);
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);
	/* iterate over the list of commands */
	if (thiz->changed)
	{
		enesim_renderer_figure_clear(thiz->figure);
		if (stroke_weight >= 1.0)
		{
			_path_generate_vertices(thiz, _stroke_path_vertex_add, _stroke_path_polygon_add);
			thiz->stroke_state.r = EINA_F16P16_HALF * (stroke_weight + 1);
			_path_stroke(thiz, stroke_weight);
		}
		else
		{
			_path_generate_vertices(thiz, _fill_only_path_vertex_add, _fill_only_path_polygon_add);
		}
	}

	/* FIXME given that we now pass the state, there's no need to gt/set every property
	 * just pass the state or set the values
	 */

	enesim_renderer_shape_stroke_weight_set(thiz->figure, stroke_weight);

	enesim_renderer_shape_stroke_color_get(r, &stroke_color);
	enesim_renderer_shape_stroke_color_set(thiz->figure, stroke_color);

	enesim_renderer_shape_stroke_renderer_get(r, &stroke_renderer);
	enesim_renderer_shape_stroke_renderer_set(thiz->figure, stroke_renderer);

	enesim_renderer_shape_fill_color_get(r, &fill_color);
	enesim_renderer_shape_fill_color_set(thiz->figure, fill_color);

	enesim_renderer_shape_fill_renderer_get(r, &fill_renderer);
	enesim_renderer_shape_fill_renderer_set(thiz->figure, fill_renderer);

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
	/* FIXME clean the command list */
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
