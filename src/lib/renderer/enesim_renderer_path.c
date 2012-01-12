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
#define ENESIM_RENDERER_PATH_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_PATH_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_PATH_MAGIC);\
	} while(0)

typedef void (*Enesim_Renderer_Path_Polygon_Add)(void *data);
typedef void (*Enesim_Renderer_Path_Polygon_Close)(Eina_Bool close, void *data);
typedef void (*Enesim_Renderer_Path_Done)(void *data);

typedef struct _Enesim_Renderer_Command_State
{
	Enesim_Curve_State st;
	Enesim_Curve_Vertex_Add vertex_add;
	Enesim_Renderer_Path_Polygon_Add polygon_add;
	Enesim_Renderer_Path_Polygon_Close polygon_close;
	Enesim_Renderer_Path_Done path_done;
	void *data;
} Enesim_Renderer_Command_State;

typedef struct _Enesim_Renderer_Path_Strokeless_State
{
	Enesim_Figure *fill_figure;
} Enesim_Renderer_Path_Strokeless_State;

typedef struct _Enesim_Renderer_Path_Stroke_State
{
	Enesim_Polygon *original_polygon;
	Enesim_Polygon *offset_polygon;
	Enesim_Polygon *inset_polygon;
	Enesim_Figure *fill_figure;
	Enesim_Figure *stroke_figure;
	Enesim_Point first;
        Enesim_Point p0, p1, p2;
        Enesim_Point n01, n12;
        double r;
        int count;
} Enesim_Renderer_Path_Stroke_State;

typedef struct _Enesim_Renderer_Path
{
	EINA_MAGIC
	/* properties */
	Eina_List *commands;
	/* private */
	Enesim_Figure *fill_figure;
	Enesim_Figure *stroke_figure;

	Enesim_Renderer_Sw_Fill fill;
	Enesim_Renderer *bifigure;
	Eina_Bool changed : 1;
} Enesim_Renderer_Path;

static inline Enesim_Renderer_Path * _enesim_path_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	ENESIM_RENDERER_PATH_MAGIC_CHECK(thiz);

	return thiz;
}
/*----------------------------------------------------------------------------*
 *                                With stroke                                 *
 *----------------------------------------------------------------------------*/
static void _do_normal(Enesim_Point *n, Enesim_Point *p0, Enesim_Point *p1)
{
	double dx;
	double dy;
	double f;

	dx = p1->x - p0->x;
	dy = p1->y - p0->y;

	/* FIXME check if the point is the same */
	f = 1.0 / hypot(dx, dy);
	n->x = dy * f;
	n->y = -dx * f;
}

static void _stroke_curve_append(double x, double y, void *data)
{
	Enesim_Polygon *p = data;

	enesim_polygon_point_append_from_coords(p, x, y);
}

static void _stroke_curve_prepend(double x, double y, void *data)
{
	Enesim_Polygon *p = data;

	enesim_polygon_point_prepend_from_coords(p, x, y);
}

static void _stroke_path_vertex_add(double x, double y, void *data)
{
	Enesim_Renderer_Path_Stroke_State *thiz = data;
	Enesim_Polygon *inset = thiz->inset_polygon;
	Enesim_Polygon *offset = thiz->offset_polygon;
	Enesim_Point o0, o1;
	Enesim_Point i0, i1;
	Eina_Bool large;
	double ox;
	double oy;
	int c1;
	int c2;

	enesim_polygon_point_append_from_coords(thiz->original_polygon, x, y);

	/* just store the first point */
	if (thiz->count < 2)
	{
		switch (thiz->count)
		{
			case 0:
			thiz->first.x = thiz->p0.x = x;
			thiz->first.y = thiz->p0.y = y;
			thiz->count++;
			return;

			case 1:
			thiz->p1.x = x;
			thiz->p1.y = y;
			_do_normal(&thiz->n01, &thiz->p0, &thiz->p1);

			ox = thiz->r * thiz->n01.x;
			oy = thiz->r * thiz->n01.y;

			o0.x = thiz->p0.x + ox;
			o0.y = thiz->p0.y + oy;
			enesim_polygon_point_append_from_coords(offset, o0.x, o0.y);

			o1.x = thiz->p1.x + ox;
			o1.y = thiz->p1.y + oy;
			enesim_polygon_point_append_from_coords(offset, o1.x, o1.y);

			i0.x = thiz->p0.x - ox;
			i0.y = thiz->p0.y - oy;
			enesim_polygon_point_prepend_from_coords(inset, i0.x, i0.y);

			i1.x = thiz->p1.x - ox;
			i1.y = thiz->p1.y - oy;
			enesim_polygon_point_prepend_from_coords(inset, i1.x, i1.y);

			thiz->count++;
			return;

			default:
			break;
		}
	}

	/* get the normals of the new edge */
	thiz->p2.x = x;
	thiz->p2.y = y;
	_do_normal(&thiz->n12, &thiz->p1, &thiz->p2);
	ox = thiz->r * thiz->n12.x;
	oy = thiz->r * thiz->n12.y;

	o0.x = thiz->p1.x + ox;
	o0.y = thiz->p1.y + oy;

	i0.x = thiz->p1.x - ox;
	i0.y = thiz->p1.y - oy;


	c1 = ((thiz->p2.x - thiz->p1.x) * thiz->n01.x) + ((thiz->p2.y - thiz->p1.y) * thiz->n01.y);
	c2 = (thiz->n01.x * thiz->n12.x) + (thiz->n01.y * thiz->n12.y);
	/* add the vertices of the new edge */
	/* check if the previous edge and this one to see the concave/convex thing */
	/* dot product
	 * = 1 pointing same direction
	 * > 0 concave
	 * = 0 orthogonal
	 * < 0 convex
	 * = -1 pointing opposite direction
	 */
	if (c2 > 0)
		large = EINA_TRUE;
	else
		large = EINA_FALSE;

	/* right side */
	if (c1 >= 0)
	{
		Enesim_Curve_State st;
		Enesim_Point *p;
		double rad = acos(c2) * 180 / M_PI;

		enesim_polygon_point_append_from_coords(offset, o0.x, o0.y);
		/* add an arc to the inset */
		p = eina_list_data_get(inset->points);
		st.vertex_add = _stroke_curve_prepend;
		st.last_x = p->x;
		st.last_y = p->y;
		st.last_ctrl_x = p->x;
		st.last_ctrl_y = p->y;
		st.data = inset;
		enesim_curve_arc_to(&st, thiz->r, thiz->r, rad, large, EINA_FALSE, i0.x, i0.y);
	}
	/* left side */
	else
	{
		Enesim_Curve_State st;
		Enesim_Point *p;
		Eina_List *l;
		double rad = acos(c2) * 180 / M_PI;

		enesim_polygon_point_prepend_from_coords(inset, i0.x, i0.y);
		/* add an arc to the offset */
		l = eina_list_last(offset->points);
		p = eina_list_data_get(l);
		st.vertex_add = _stroke_curve_append;
		st.last_x = p->x;
		st.last_y = p->y;
		st.last_ctrl_x = p->x;
		st.last_ctrl_y = p->y;
		st.data = offset;

		enesim_curve_arc_to(&st, thiz->r, thiz->r, rad, large, EINA_TRUE, o0.x, o0.y);
	}

	o1.x = thiz->p2.x + ox;
	o1.y = thiz->p2.y + oy;
	enesim_polygon_point_append_from_coords(offset, o1.x, o1.y);


	i1.x = thiz->p2.x - ox;
	i1.y = thiz->p2.y - oy;
	enesim_polygon_point_prepend_from_coords(inset, i1.x, i1.y);

	thiz->p0 = thiz->p1;
	thiz->p1 = thiz->p2;
	thiz->n01 = thiz->n12;
	thiz->count++;
	return;
}

static void _stroke_path_polygon_add(void *data)
{
        Enesim_Renderer_Path_Stroke_State *thiz = data;
        Enesim_Polygon *p;

        /* just reset */
        thiz->count = 0;

	p = enesim_polygon_new();
	enesim_figure_polygon_append(thiz->fill_figure, p);
	thiz->original_polygon = p;

	p = enesim_polygon_new();
	enesim_figure_polygon_append(thiz->stroke_figure, p);
	thiz->offset_polygon = p;

	p = enesim_polygon_new();
	enesim_figure_polygon_append(thiz->stroke_figure, p);
	thiz->inset_polygon = p;
}

static void _stroke_path_polygon_close(Eina_Bool close, void *data)
{
        Enesim_Renderer_Path_Stroke_State *thiz = data;
	Enesim_Polygon *inset = thiz->inset_polygon;
	Enesim_Polygon *offset = thiz->offset_polygon;
	Enesim_Point o0, o1;
	Enesim_Point i0, i1;
	double ox;
	double oy;
	double c;

	enesim_polygon_close(thiz->original_polygon, close);

	/* add the missing edge */
	_do_normal(&thiz->n12, &thiz->p1, &thiz->first);

	c = (thiz->n01.x * thiz->n12.x) + (thiz->n01.y * thiz->n12.y);
	if (c <= 0)
	{
		/* TODO do the curve on the offset_polygon */
		enesim_polygon_point_prepend_from_coords(inset, thiz->p1.x, thiz->p1.y);
	}
	else
	{
		/* TODO do the curve on the inset_polygon */
		enesim_polygon_point_append_from_coords(offset, thiz->p1.x, thiz->p1.y);
	}
	ox = thiz->r * thiz->n12.x;
	oy = thiz->r * thiz->n12.y;

	o0.x = thiz->p1.x + ox;
	o0.y = thiz->p1.y + oy;
	enesim_polygon_point_append_from_coords(offset, o0.x, o0.y);

	o1.x = thiz->first.x + ox;
	o1.y = thiz->first.y + oy;
	enesim_polygon_point_append_from_coords(offset, o1.x, o1.y);

	i0.x = thiz->p1.x - ox;
	i0.y = thiz->p1.y - oy;
	enesim_polygon_point_prepend_from_coords(inset, i0.x, i0.y);

	i1.x = thiz->first.x - ox;
	i1.y = thiz->first.y - oy;
	enesim_polygon_point_prepend_from_coords(inset, i1.x, i1.y);
}

static void _stroke_path_done(void *data)
{
        Enesim_Renderer_Path_Stroke_State *thiz = data;

	/* TODO check that we are not closed */
	/* FIXME for now, is not the same to close a polygon through a close
	 * command that closing it like this
	 */
	enesim_polygon_close(thiz->offset_polygon, EINA_TRUE);
	enesim_polygon_close(thiz->inset_polygon, EINA_TRUE);
}
/*----------------------------------------------------------------------------*
 *                              Without stroke                                *
 *----------------------------------------------------------------------------*/
static void _strokeless_path_vertex_add(double x, double y, void *data)
{
	Enesim_Renderer_Path_Strokeless_State *thiz = data;
	Enesim_Polygon *p;
	Eina_List *last;

	last = eina_list_last(thiz->fill_figure->polygons);
	p = eina_list_data_get(last);
	enesim_polygon_point_append_from_coords(p, x, y);
}

static void _strokeless_path_polygon_add(void *data)
{
	Enesim_Renderer_Path_Strokeless_State *thiz = data;
	Enesim_Polygon *p;

	p = enesim_polygon_new();
	enesim_figure_polygon_append(thiz->fill_figure, p);
}

static void _strokeless_path_polygon_close(Eina_Bool close, void *data)
{
	Enesim_Renderer_Path_Strokeless_State *thiz = data;
	Enesim_Polygon *p;
	Eina_List *last;

	last = eina_list_last(thiz->fill_figure->polygons);
	p = eina_list_data_get(last);
	p->closed = close;
}
/*----------------------------------------------------------------------------*
 *                                 Commands                                   *
 *----------------------------------------------------------------------------*/
static void _enesim_move_to(Enesim_Renderer_Command_State *state,
		double x, double y)
{
	/* we have to reset the curve state too */
	state->st.last_x = x;
	state->st.last_y = y;
	state->st.last_ctrl_x = x;
	state->st.last_ctrl_y = y;
	/* now call the interface */
	state->polygon_add(state->data);
	state->vertex_add(x, y, state->data);
}

static void _enesim_close(Enesim_Renderer_Command_State *state, Eina_Bool close)
{
	state->polygon_close(close, state->data);
}

static void _enesim_path_generate_vertices(Eina_List *commands,
		Enesim_Curve_Vertex_Add vertex_add,
		Enesim_Renderer_Path_Polygon_Add polygon_add,
		Enesim_Renderer_Path_Polygon_Close polygon_close,
		Enesim_Renderer_Path_Done path_done,
		double scale_x, double scale_y,
		Enesim_Matrix *gm,
		void *data)
{
	Eina_List *l;
	Enesim_Renderer_Command_State state;
	Enesim_Renderer_Path_Command *cmd;
	double rx;
	double ry;
	double ctrl_x0;
	double ctrl_y0;
	double ctrl_x1;
	double ctrl_y1;

	state.vertex_add = vertex_add;
	state.polygon_add = polygon_add;
	state.polygon_close = polygon_close;
	state.path_done = path_done;
	state.data = data;

	state.st.vertex_add = vertex_add;
	state.st.last_x = 0;
	state.st.last_y = 0;
	state.st.last_ctrl_x = 0;
	state.st.last_ctrl_y = 0;
	state.st.data = data;

	EINA_LIST_FOREACH(commands, l, cmd)
	{
		double x, y;
		/* send the new vertex to the figure renderer */
		switch (cmd->type)
		{
			case ENESIM_COMMAND_MOVE_TO:
			x = scale_x * cmd->definition.move_to.x;
			y = scale_y * cmd->definition.move_to.y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			_enesim_move_to(&state, x, y);
			break;

			case ENESIM_COMMAND_LINE_TO:
			x = scale_x * cmd->definition.line_to.x;
			y = scale_y * cmd->definition.line_to.y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			enesim_curve_line_to(&state.st, x, y);
			break;

			case ENESIM_COMMAND_QUADRATIC_TO:
			x = scale_x * cmd->definition.quadratic_to.x;
			y = scale_y * cmd->definition.quadratic_to.y;
			ctrl_x0 = scale_x * cmd->definition.quadratic_to.ctrl_x;
			ctrl_y0 = scale_y * cmd->definition.quadratic_to.ctrl_y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			enesim_matrix_point_transform(gm, ctrl_x0, ctrl_y0, &ctrl_x0, &ctrl_y0);
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			enesim_curve_quadratic_to(&state.st, ctrl_x0, ctrl_y0, x, y);
			break;

			case ENESIM_COMMAND_SQUADRATIC_TO:
			x = scale_x * cmd->definition.squadratic_to.x;
			y = scale_y * cmd->definition.squadratic_to.y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			enesim_curve_squadratic_to(&state.st, x, y);
			break;

			case ENESIM_COMMAND_CUBIC_TO:
			x = scale_x * cmd->definition.cubic_to.x;
			y = scale_y * cmd->definition.cubic_to.y;
			ctrl_x0 = scale_x * cmd->definition.cubic_to.ctrl_x0;
			ctrl_y0 = scale_y * cmd->definition.cubic_to.ctrl_y0;
			ctrl_x1 = scale_x * cmd->definition.cubic_to.ctrl_x1;
			ctrl_y1 = scale_y * cmd->definition.cubic_to.ctrl_y1;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			enesim_matrix_point_transform(gm, ctrl_x0, ctrl_y0, &ctrl_x0, &ctrl_y0);
			enesim_matrix_point_transform(gm, ctrl_x1, ctrl_y1, &ctrl_x1, &ctrl_y1);
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			enesim_curve_cubic_to(&state.st,
					ctrl_x0, ctrl_y0, ctrl_x1, ctrl_y1,
					x, y);
			break;

			case ENESIM_COMMAND_SCUBIC_TO:
			x = scale_x * cmd->definition.scubic_to.x;
			y = scale_y * cmd->definition.scubic_to.y;
			ctrl_x0 = scale_x * cmd->definition.scubic_to.ctrl_x;
			ctrl_y0 = scale_y * cmd->definition.scubic_to.ctrl_y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			enesim_matrix_point_transform(gm, ctrl_x0, ctrl_y0, &ctrl_x0, &ctrl_y0);
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			enesim_curve_scubic_to(&state.st, ctrl_x0, ctrl_y0,
					x, y);
			break;

			case ENESIM_COMMAND_ARC_TO:
			x = scale_x * cmd->definition.arc_to.x;
			y = scale_y * cmd->definition.arc_to.y;
			rx = scale_x * cmd->definition.arc_to.rx;
			ry = scale_y * cmd->definition.arc_to.ry;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			enesim_matrix_point_transform(gm, rx, ry, &rx, &ry);
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			enesim_curve_arc_to(&state.st,
					rx, ry,
					cmd->definition.arc_to.angle,
					cmd->definition.arc_to.large,
					cmd->definition.arc_to.sweep,
					x, y);
			break;

			case ENESIM_COMMAND_CLOSE:
			_enesim_close(&state, cmd->definition.close.close);
			break;

			default:
			break;
		}
	}
	/* in case we delay the creation of the vertices this triggers that */
	if (state.path_done)
		state.path_done(state.data);
}

static void _enesim_span(Enesim_Renderer *r, int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Path *thiz;

	thiz = _enesim_path_get(r);
	thiz->fill(thiz->bifigure, x, y, len, ddata);
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _path_name(Enesim_Renderer *r)
{
	return "path";
}

static Eina_Bool _path_sw_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Renderer_Path *thiz;

	Enesim_Color color;
	Enesim_Color stroke_color;
	Enesim_Renderer *stroke_renderer;
	Enesim_Color fill_color;
	Enesim_Renderer *fill_renderer;
	Enesim_Shape_Draw_Mode draw_mode;
	double stroke_weight;

	thiz = _enesim_path_get(r);

	enesim_renderer_shape_draw_mode_get(r, &draw_mode);
	enesim_renderer_shape_stroke_weight_get(r, &stroke_weight);

	/* TODO in the future the generation of polygons might depend also on the geometric matrix used */
	/* generate the list of points/polygons */
	if (thiz->changed)
	{
		if (thiz->fill_figure)
			enesim_figure_clear(thiz->fill_figure);
		else
			thiz->fill_figure = enesim_figure_new();

		if (thiz->stroke_figure)
			enesim_figure_clear(thiz->stroke_figure);
		else
			thiz->stroke_figure = enesim_figure_new();
		if ((draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE) && (stroke_weight > 1.0))
		{
			Enesim_Renderer_Path_Stroke_State st;

			st.fill_figure = thiz->fill_figure;
			st.stroke_figure = thiz->stroke_figure;
			st.count = 0;
			st.r = stroke_weight / 2.0;

			_enesim_path_generate_vertices(thiz->commands, _stroke_path_vertex_add,
					_stroke_path_polygon_add,
					_stroke_path_polygon_close,
					_stroke_path_done,
					state->sx, state->sy,
					&state->geometry_transformation,
					&st);
			enesim_rasterizer_figure_set(thiz->bifigure, thiz->fill_figure);
			enesim_rasterizer_bifigure_over_figure_set(thiz->bifigure, thiz->stroke_figure);
		}
		else
		{
			Enesim_Renderer_Path_Strokeless_State st;

			st.fill_figure = thiz->fill_figure;

			_enesim_path_generate_vertices(thiz->commands, _strokeless_path_vertex_add,
					_strokeless_path_polygon_add,
					_strokeless_path_polygon_close,
					NULL,
					state->sx, state->sy,
					&state->geometry_transformation,
					&st);
			enesim_rasterizer_figure_set(thiz->bifigure, thiz->fill_figure);
		}
		/* set the fill figure on the bifigure as its under polys */
		thiz->changed = 0;
	}

	/* FIXME given that we now pass the state, there's no need to gt/set every property
	 * just pass the state or set the values
	 */
	enesim_renderer_shape_draw_mode_set(thiz->bifigure, draw_mode);
	enesim_renderer_shape_stroke_weight_set(thiz->bifigure, stroke_weight);

	enesim_renderer_shape_stroke_color_get(r, &stroke_color);
	enesim_renderer_shape_stroke_color_set(thiz->bifigure, stroke_color);

	enesim_renderer_shape_stroke_renderer_get(r, &stroke_renderer);
	enesim_renderer_shape_stroke_renderer_set(thiz->bifigure, stroke_renderer);

	enesim_renderer_shape_fill_color_get(r, &fill_color);
	enesim_renderer_shape_fill_color_set(thiz->bifigure, fill_color);

	enesim_renderer_shape_fill_renderer_get(r, &fill_renderer);
	enesim_renderer_shape_fill_renderer_set(thiz->bifigure, fill_renderer);

	enesim_renderer_color_get(r, &color);
	enesim_renderer_color_set(thiz->bifigure, color);
	enesim_renderer_origin_set(thiz->bifigure, state->ox, state->oy);
	enesim_renderer_transformation_set(thiz->bifigure, &state->transformation);

	if (!enesim_renderer_setup(thiz->bifigure, s, error))
	{
		return EINA_FALSE;
	}

	thiz->fill = enesim_renderer_sw_fill_get(thiz->bifigure);
	if (!thiz->fill)
	{
		return EINA_FALSE;
	}
	*fill = _enesim_span;

	return EINA_TRUE;
}

static void _path_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Path *thiz;

	thiz = _enesim_path_get(r);
	enesim_renderer_cleanup(thiz->bifigure, s);
	thiz->changed = EINA_FALSE;
}

static void _path_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_ARGB8888 |
			ENESIM_SHAPE_FLAG_FILL_RENDERER;
}

static void _enesim_boundings(Enesim_Renderer *r, Enesim_Rectangle *boundings)
{
	Enesim_Renderer_Path *thiz;

	thiz = _enesim_path_get(r);

	/* FIXME fix this */
	if (!thiz->bifigure) return;
	enesim_renderer_boundings(thiz->bifigure, boundings);
}

#if BUILD_OPENGL
static Eina_Bool _path_opengl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		Enesim_Surface *s,
		int *num_shaders,
		Enesim_Renderer_OpenGL_Shader **shaders,
		Enesim_Error **error)
{
	Enesim_Renderer_Path *thiz;
 	thiz = _enesim_path_get(r);

	/*
	 * On the gl version use the tesselator to generate the triangles and pass them
	 * to the goemetry shader. Only tesselate again whenever something has changed
	 */
#if 0
	tobj = gluNewTess();
	gluTessCallback(tobj, GLU_TESS_VERTEX,
			   (GLvoid (*) ()) &glVertex3dv);
	gluTessCallback(tobj, GLU_TESS_BEGIN,
			   (GLvoid (*) ()) &beginCallback);
	gluTessCallback(tobj, GLU_TESS_END,
			   (GLvoid (*) ()) &endCallback);
	gluTessCallback(tobj, GLU_TESS_ERROR,
			   (GLvoid (*) ()) &errorCallback);

	/*  new callback routines registered by these calls */
	void vertexCallback(GLvoid *vertex)
	{
		const GLdouble *pointer;

		pointer = (GLdouble *) vertex;
		glColor3dv(pointer+3);
		glVertex3dv(vertex);
	}

	void combineCallback(GLdouble coords[3], 
			     GLdouble *vertex_data[4],
			     GLfloat weight[4], GLdouble **dataOut )
	{
		GLdouble *vertex;
		int i;

		vertex = (GLdouble *) malloc(6 * sizeof(GLdouble));
		vertex[0] = coords[0];
		vertex[1] = coords[1];
		vertex[2] = coords[2];
		for (i = 3; i < 7; i++)
		vertex[i] = weight[0] * vertex_data[0][i] 
			  + weight[1] * vertex_data[1][i]
			  + weight[2] * vertex_data[2][i] 
			  + weight[3] * vertex_data[3][i];
		*dataOut = vertex;
	}

	/*  the callback routines registered by gluTessCallback() */

	void beginCallback(GLenum which)
	{
	}

	void endCallback(void)
	{
	}
	// describe non-convex polygon
	gluTessBeginPolygon(tess, user_data);
		// first contour
		gluTessBeginContour(tess);
		gluTessVertex(tess, coords[0], vertex_data);
		...
		gluTessEndContour(tess);

		// second contour
		gluTessBeginContour(tess);
		gluTessVertex(tess, coords[5], vertex_data);
		...
		gluTessEndContour(tess);
		...
	gluTessEndPolygon(tess);

#endif
	/* after tesselating we should have a structure with our triangles that we should
	 * pass to the geometry shader through uniforms
	 * the shader should only emit primitives and vertices
	 */
}

static Eina_Bool _path_opengl_shader_setup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Renderer_OpenGL_Data *rdata;
	int final_color;

 	thiz = _enesim_path_get(r);

	return EINA_TRUE;
}

static void _path_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Path *thiz;

 	thiz = _enesim_path_get(r);
}
#endif

static Enesim_Renderer_Shape_Descriptor _path_descriptor = {
	/* .name = 			*/ _path_name,
	/* .free = 			*/ NULL,
	/* .boundings = 		*/ NULL, // _boundings,
	/* .destination_transform = 	*/ NULL,
	/* .flags = 			*/ _path_flags,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ NULL,
	/* .sw_setup = 			*/ _path_sw_setup,
	/* .sw_cleanup = 		*/ _path_sw_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
#if BUILD_OPENGL
	/* .opengl_setup = 		*/ _path_opengl_setup,
	/* .opengl_shader_setup =	*/ _path_opengl_shader_setup,
	/* .opengl_cleanup =		*/ _path_opengl_cleanup
#else
	/* .opengl_setup = 		*/ NULL,
	/* .opengl_shader_setup =	*/ NULL,
	/* .opengl_cleanup = 		*/ NULL
#endif
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
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_PATH_MAGIC);

	r = enesim_rasterizer_bifigure_new();
	if (!r) goto err_figure;
	thiz->bifigure = r;

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

	thiz = _enesim_path_get(r);
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

	thiz = _enesim_path_get(r);

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
	cmd.definition.move_to.x = x;
	cmd.definition.move_to.y = y;
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
	cmd.definition.line_to.x = x;
	cmd.definition.line_to.y = y;
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
	cmd.definition.squadratic_to.x = x;
	cmd.definition.squadratic_to.y = y;
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
	cmd.definition.quadratic_to.x = x;
	cmd.definition.quadratic_to.y = y;
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
	cmd.definition.cubic_to.x = x;
	cmd.definition.cubic_to.y = y;
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
	cmd.definition.scubic_to.x = x;
	cmd.definition.scubic_to.y = y;
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
	cmd.definition.arc_to.x = x;
	cmd.definition.arc_to.y = y;
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
	cmd.definition.close.close = close;
	enesim_renderer_path_command_add(r, &cmd);
}
