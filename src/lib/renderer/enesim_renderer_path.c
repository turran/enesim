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

/*
 * The idea of the path/figure renderers is the following:
 * Basically both at the end need to rasterize the resulting polygon, right
 * now the path renderer is using the figure renderer so it is a good option
 * to split the rasterizer algorigthm on the figure and put it into another
 * abstraction (rasterizer maybe?) and then make both renderers use that
 * rasterizer which should operate on list of polygons
 * In order to do the path/figure stroking we should also share the same
 * algorithm to create such offset curves. The simplest approach is:
 * 1. create parallel edges to the original edges
 * 2. when doing the offset curve, if it is convex then do curves
 * (or squares or whatever join type) from the offset edge points to the other
 * offset edge points. if it is concave just intersect both offset edges or do
 * a line from one offset edge point to the original edge point and then another
 * from it to the other offset edge point. If we are doing an inset curve we do
 * the opposite approach.
 * 3. Once the above is done we end with two new polygons, one for the offset curve
 * and one for the inset curve. Then we should pass such polygons to the rasterizer
 * with some special winding algorithm. A positive winding for the offset and a negative
 * winding for the inset.
 */

/*
 * Some formulas found on the research process:
 * l1 = Ax + By + C
 * l2 || l1 => l2 = Ax + By + C'
 * C' = C + d * hypot(A, B);
 */

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef void (*Enesim_Renderer_Path_Vertex_Add)(double x, double y, void *data);
typedef void (*Enesim_Renderer_Path_Polygon_Add)(void *data);

typedef struct _Enesim_Renderer_Command_State
{
	Enesim_Renderer_Path_Vertex_Add vertex_add;
	Enesim_Renderer_Path_Polygon_Add polygon_add;
	double last_x;
	double last_y;
	double last_ctrl_x;
	double last_ctrl_y;
	void *data;
} Enesim_Renderer_Command_State;

typedef struct _Enesim_Renderer_Path
{
	/* properties */
	Eina_List *commands;
	/* private */
	Eina_List *polygons;

	Enesim_Renderer_Sw_Fill fill;
	Enesim_Renderer *line;
	Enesim_Renderer *figure; /* FOR NOW */
	Enesim_Renderer *final_r;
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
static void _path_vertex_add(double x, double y, void *data)
{
	Enesim_Renderer_Path *thiz = data;
	Enesim_Polygon *p;
	Enesim_Point pt;
	Eina_List *last;

	last = eina_list_last(thiz->polygons);
	p = eina_list_data_get(last);
	pt.x = x;
	pt.y = y;
	enesim_polygon_point_append(p, &pt);
}

static void _path_polygon_add(void *data)
{
	Enesim_Renderer_Path *thiz = data;
	Enesim_Polygon *p;

	p = enesim_polygon_new();
	thiz->polygons = eina_list_append(thiz->polygons, p);
}

static void _path_polygon_close(void *data)
{

}
/*----------------------------------------------------------------------------*
 *                                 Commands                                   *
 *----------------------------------------------------------------------------*/
static void _move_to(Enesim_Renderer_Command_State *state,
		double x, double y)
{
	state->polygon_add(state->data);
	state->vertex_add(x, y, state->data);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = x;
	state->last_ctrl_y = y;
}

static void _line_to(Enesim_Renderer_Command_State *state,
		double x, double y)
{
	state->vertex_add(x, y, state->data);
	state->last_ctrl_x = state->last_x;
	state->last_ctrl_y = state->last_y;
	state->last_x = x;
	state->last_y = y;
}

/* these subdiv approximations need to be done more carefully */
static void _quadratic_to(Enesim_Renderer_Command_State *state,
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

static void _squadratic_to(Enesim_Renderer_Command_State *state,
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

static void _cubic_to(Enesim_Renderer_Command_State *state,
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


static void _scubic_to(Enesim_Renderer_Command_State *state,
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

static void _arc_to(Enesim_Renderer_Command_State *state, double rx, double ry, double angle,
                   unsigned char large, unsigned char sweep, double x, double y)
{

}

static void _close(Enesim_Renderer_Command_State *state)
{

}

static void _path_generate_vertices(Eina_List *commands,
		Enesim_Renderer_Path_Vertex_Add vertex_add,
		Enesim_Renderer_Path_Polygon_Add polygon_add,
		void *data)
{
	Eina_List *l;
	Enesim_Renderer_Command_State state;
	Enesim_Renderer_Path_Command *cmd;

	state.vertex_add = vertex_add;
	state.polygon_add = polygon_add;
	state.last_x = 0;
	state.last_y = 0;
	state.last_ctrl_x = 0;
	state.last_ctrl_y = 0;
	state.data = data;

	EINA_LIST_FOREACH(commands, l, cmd)
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

static void _span(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Path *thiz;

	thiz = _path_get(r);
	thiz->fill(thiz->final_r, x, y, len, ddata);
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
	Enesim_Renderer *final_r = NULL;
	Enesim_Rasterizer *rz;
	int npols;
	int fverts;

	Enesim_Color stroke_color;
	Enesim_Renderer *stroke_renderer;
	Enesim_Color fill_color;
	Enesim_Renderer *fill_renderer;
	Enesim_Shape_Draw_Mode draw_mode;
	double stroke_weight;

	thiz = _path_get(r);

	/* TODO in the future the generation of polygons might depend also on the matrix used */
	/* generate the list of points/polygons */
	if (thiz->changed)
	{
		Enesim_Polygon *p;
		/* first remove the polygon/points */
		EINA_LIST_FREE(thiz->polygons, p)
		{
			Enesim_Point *pt;
			EINA_LIST_FREE(p->points, pt)
				free(pt);
			free(p);
		}
	}
	_path_generate_vertices(thiz->commands, _path_vertex_add, _path_polygon_add, thiz);
	/* check for the simplest case, a line (1 polygon, 2 points) */
	npols = eina_list_count(thiz->polygons);
	fverts = 0;
	if (npols)
	{
		Enesim_Polygon *p;

		p = eina_list_data_get(thiz->polygons);
		fverts = eina_list_count(p->points);
	}
	/* use the line renderer */
	if (npols == 1 && fverts < 3)
	{
		Enesim_Polygon *poly;
		Enesim_Point *p;
		Enesim_Point pts[2] = {{0, 0}, {0, 0}};
		Enesim_Point *pt = pts;
		Eina_List *l;

		final_r = thiz->line;
		/* set the two coordinates */
		poly = eina_list_data_get(thiz->polygons);
		EINA_LIST_FOREACH(poly->points, l, p)
		{
			*pt = *p;
			pt++;
		}
		enesim_renderer_line_x0_set(final_r, pts[0].x);
		enesim_renderer_line_y0_set(final_r, pts[0].y);
		enesim_renderer_line_x1_set(final_r, pts[1].x);
		enesim_renderer_line_y1_set(final_r, pts[1].y);
	}
	/* FIXME for now we use the figure renderer */
	else
	{
		Enesim_Polygon *p;
		Eina_List *l1;

		/* decide what rasterizer to use */
		final_r = thiz->figure;
		/* set the points */
		EINA_LIST_FOREACH(thiz->polygons, l1, p)
		{
			Enesim_Point *pt;
			Eina_List *l2;

			enesim_renderer_figure_polygon_add(final_r);
			EINA_LIST_FOREACH(p->points, l2, pt)
			{
				enesim_renderer_figure_polygon_vertex_add(final_r, pt->x, pt->y);
			}
		}
	}

	/* FIXME given that we now pass the state, there's no need to gt/set every property
	 * just pass the state or set the values
	 */
	enesim_renderer_shape_stroke_weight_get(r, &stroke_weight);
	enesim_renderer_shape_draw_mode_get(r, &draw_mode);

	enesim_renderer_shape_stroke_weight_set(final_r, stroke_weight);

	enesim_renderer_shape_stroke_color_get(r, &stroke_color);
	enesim_renderer_shape_stroke_color_set(final_r, stroke_color);

	enesim_renderer_shape_stroke_renderer_get(r, &stroke_renderer);
	enesim_renderer_shape_stroke_renderer_set(final_r, stroke_renderer);

	enesim_renderer_shape_fill_color_get(r, &fill_color);
	enesim_renderer_shape_fill_color_set(final_r, fill_color);

	enesim_renderer_shape_fill_renderer_get(r, &fill_renderer);
	enesim_renderer_shape_fill_renderer_set(final_r, fill_renderer);

	enesim_renderer_shape_draw_mode_set(final_r, draw_mode);

	enesim_renderer_origin_set(final_r, state->ox, state->oy);
	enesim_renderer_transformation_set(final_r, &state->transformation);

	if (!enesim_renderer_setup(final_r, s, error))
	{
		return EINA_FALSE;
	}

	thiz->fill = enesim_renderer_sw_fill_get(final_r);
	if (!thiz->fill)
	{
		return EINA_FALSE;
	}
	*fill = _span;
	thiz->final_r = final_r;

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

	r = enesim_renderer_line_new();
	thiz->line = r;

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
	Enesim_Renderer_Path_Command *c;

	thiz = _path_get(r);
	EINA_LIST_FREE(thiz->commands, c)
	{
		free(c);
	}
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
