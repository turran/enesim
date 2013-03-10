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
#include "enesim_private.h"

#include "enesim_main.h"
#include "enesim_eina.h"
#include "enesim_path.h"

#include "enesim_path_private.h"
#include "enesim_path_normalizer_private.h"

typedef struct _Enesim_Path_Normalizer_State
{
	double last_x;
	double last_y;
	double last_ctrl_x;
	double last_ctrl_y;
} Enesim_Path_Normalizer_State;

typedef void (*Enesim_Path_Normalizer_Move_To_Cb)(Enesim_Path_Command_Move_To *move_to,
		Enesim_Path_Normalizer_State *state, void *data);
typedef void (*Enesim_Path_Normalizer_Line_To_Cb)(Enesim_Path_Command_Line_To *line_to,
		Enesim_Path_Normalizer_State *state, void *data);
typedef void (*Enesim_Path_Normalizer_Cubic_To_Cb)(Enesim_Path_Command_Cubic_To *cubic_to,
		Enesim_Path_Normalizer_State *state, void *data);
typedef void (*Enesim_Path_Normalizer_Close_Cb)(Enesim_Path_Command_Close *close,
		Enesim_Path_Normalizer_State *state, void *data);
typedef void (*Enesim_Path_Normalizer_Free_Cb)(void *data);

typedef struct _Enesim_Path_Normalizer_Descriptor
{
	Enesim_Path_Normalizer_Move_To_Cb move_to;
	Enesim_Path_Normalizer_Line_To_Cb line_to;
	Enesim_Path_Normalizer_Cubic_To_Cb cubic_to;
	Enesim_Path_Normalizer_Close_Cb close;
	Enesim_Path_Normalizer_Free_Cb free;
} Enesim_Path_Normalizer_Descriptor;

struct _Enesim_Path_Normalizer
{
	Enesim_Path_Normalizer_Descriptor *descriptor;
	Enesim_Path_Normalizer_State state;
	void *data;
};

typedef struct _Enesim_Path_Normalizer_Figure
{
	Enesim_Path_Normalizer_Figure_Descriptor *descriptor;
	void *data;
} Enesim_Path_Normalizer_Figure;

typedef struct _Enesim_Path_Normalizer_Path
{
	Enesim_Path_Normalizer_Path_Descriptor *descriptor;
	void *data;
} Enesim_Path_Normalizer_Path;
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#if 0
/* these subdiv approximations need to be done more carefully */
void enesim_curve_quadratic_to(Enesim_Curve_State *state,
		double ctrl_x, double ctrl_y,
		double x, double y)
{
	_curve_quadratic_to(state, ctrl_x, ctrl_y, x, y);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = ctrl_x;
	state->last_ctrl_y = ctrl_y;
}

void enesim_curve_squadratic_to(Enesim_Curve_State *state,
		double x, double y)
{
	double x0, y0, cx0, cy0;

	x0 = state->last_x;
	y0 = state->last_y;
	cx0 = state->last_ctrl_x;
	cy0 = state->last_ctrl_y;
	cx0 = (2 * x0) - cx0;
	cy0 = (2 * y0) - cy0;

	enesim_curve_quadratic_to(state, cx0, cy0, x, y);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = cx0;
	state->last_ctrl_y = cy0;
}

void enesim_curve_cubic_to(Enesim_Curve_State *state,
		double ctrl_x0, double ctrl_y0,
		double ctrl_x, double ctrl_y,
		double x, double y)
{
	/* force one initial subdivision */
	double x01 = (state->last_x + ctrl_x0) / 2;
	double y01 = (state->last_y + ctrl_y0) / 2;
	double xc = (ctrl_x0 + ctrl_x) / 2;
	double yc = (ctrl_y0 + ctrl_y) / 2;
	double x23 = (x + ctrl_x) / 2;
	double y23 = (y + ctrl_y) / 2;
	double xa = (x01 + xc) / 2;
	double ya = (y01 + yc) / 2;
	double xb = (x23 + xc) / 2;
	double yb = (y23 + yc) / 2;

	xc = (xa + xb) / 2;
	yc = (ya + yb) / 2;
	_curve_cubic_to(state, x01, y01, xa, ya, xc, yc);
	state->last_x = xc;
	state->last_y = yc;
	state->last_ctrl_x = xa;
	state->last_ctrl_y = ya;

	_curve_cubic_to(state, xb, yb, x23, y23, x, y);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = x23;
	state->last_ctrl_y = y23;
}

#endif
/*----------------------------------------------------------------------------*
 *                            Figure normalizer                               *
 *----------------------------------------------------------------------------*/
static void _figure_move_to(Enesim_Path_Command_Move_To *move_to,
		Enesim_Path_Normalizer_State *state EINA_UNUSED, void *data)
{
	Enesim_Path_Normalizer_Figure *thiz = data;
	double x, y;

	enesim_path_command_move_to_values_to(move_to, &x, &y);
	thiz->descriptor->polygon_add(thiz->data);
	thiz->descriptor->vertex_add(x, y, thiz->data);
}

static void _figure_line_to(Enesim_Path_Command_Line_To *line_to,
		Enesim_Path_Normalizer_State *state EINA_UNUSED, void *data)
{
	Enesim_Path_Normalizer_Figure *thiz = data;
	double x, y;

	enesim_path_command_line_to_values_to(line_to, &x, &y);
	thiz->descriptor->vertex_add(x, y, thiz->data);
}

static void _figure_cubic_to(Enesim_Path_Command_Cubic_To *cubic_to,
		Enesim_Path_Normalizer_State *state, void *data)
{
	Enesim_Path_Normalizer_Figure *thiz = data;
	Enesim_Path_Cubic q;

	q.start_x = state->last_x;
	q.start_y = state->last_y;
	q.ctrl_x0 = cubic_to->ctrl_x0;
	q.ctrl_y0 = cubic_to->ctrl_y0;
	q.ctrl_x1 = cubic_to->ctrl_x1;
	q.ctrl_y1 = cubic_to->ctrl_y1;
	q.end_x = cubic_to->x;
	q.end_y = cubic_to->y;
	/* normalize the cubic command */
	/* TODO add a tolerance/quality paramter */
	enesim_path_cubic_flatten(&q, 1/64.0, thiz->descriptor->vertex_add, thiz->data);
}

static void _figure_close(Enesim_Path_Command_Close *close,
		Enesim_Path_Normalizer_State *state EINA_UNUSED, void *data)
{
	Enesim_Path_Normalizer_Figure *thiz = data;
	thiz->descriptor->polygon_close(close->close, thiz->data);
}

static void _figure_free(void *data)
{
	free(data);
}

static Enesim_Path_Normalizer_Descriptor _figure_descriptor = {
	/* .move_to = 	*/ _figure_move_to,
	/* .line_to = 	*/ _figure_line_to,
	/* .cubic_to = 	*/ _figure_cubic_to,
	/* .close = 	*/ _figure_close,
	/* .free = 	*/ _figure_free,
};
/*----------------------------------------------------------------------------*
 *                             Path normalizer                                *
 *----------------------------------------------------------------------------*/
static void _path_move_to(Enesim_Path_Command_Move_To *move_to,
		Enesim_Path_Normalizer_State *state EINA_UNUSED, void *data)
{
	Enesim_Path_Normalizer_Path *thiz = data;
	thiz->descriptor->move_to(move_to, thiz->data);
}

static void _path_line_to(Enesim_Path_Command_Line_To *line_to,
		Enesim_Path_Normalizer_State *state EINA_UNUSED, void *data)
{
	Enesim_Path_Normalizer_Path *thiz = data;
	thiz->descriptor->line_to(line_to, thiz->data);
}

static void _path_cubic_to(Enesim_Path_Command_Cubic_To *cubic_to,
		Enesim_Path_Normalizer_State *state EINA_UNUSED, void *data)
{
	Enesim_Path_Normalizer_Path *thiz = data;
	thiz->descriptor->cubic_to(cubic_to, thiz->data);
}

static void _path_close(Enesim_Path_Command_Close *close,
		Enesim_Path_Normalizer_State *state EINA_UNUSED, void *data)
{
	Enesim_Path_Normalizer_Path *thiz = data;
	thiz->descriptor->close(close, thiz->data);
}

static void _path_free(void *data)
{
	free(data);
}

static Enesim_Path_Normalizer_Descriptor _path_descriptor = {
	/* .move_to = 	*/ _path_move_to,
	/* .line_to = 	*/ _path_line_to,
	/* .cubic_to = 	*/ _path_cubic_to,
	/* .close = 	*/ _path_close,
	/* .free = 	*/ _path_free,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Path_Normalizer * enesim_path_normalizer_new(
		Enesim_Path_Normalizer_Descriptor *descriptor,
		void *data)
{
	Enesim_Path_Normalizer *thiz;

	thiz = calloc(1, sizeof(Enesim_Path_Normalizer));
	thiz->data = data;
	thiz->descriptor = descriptor;
	return thiz;
}

void enesim_path_normalizer_free(Enesim_Path_Normalizer *thiz)
{
	thiz->descriptor->free(thiz->data);
	free(thiz);
}

void enesim_path_normalizer_scubic_to(Enesim_Path_Normalizer *thiz,
		Enesim_Path_Command_Scubic_To *scubic_to)
{
	Enesim_Path_Normalizer_State *state = &thiz->state;
	Enesim_Path_Command_Cubic_To cubic_to;
	double ctrl_x, ctrl_y, x, y;
	double x0, y0, cx0, cy0;

	enesim_path_command_scubic_to_values_to(scubic_to, &x, &y, &ctrl_x, &ctrl_y);
	x0 = state->last_x;
	y0 = state->last_y;
	cx0 = state->last_ctrl_x;
	cy0 = state->last_ctrl_y;
	cx0 = (2 * x0) - cx0;
	cy0 = (2 * y0) - cy0;

	enesim_path_command_cubic_to_values_from(&cubic_to, x, y, cx0, cy0, ctrl_x, ctrl_y);
	enesim_path_normalizer_cubic_to(thiz, &cubic_to);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = ctrl_x;
	state->last_ctrl_y = ctrl_y;
}

/* code adapted from the moonlight sources
 * we need to fix it to match c89
 */
void enesim_path_normalizer_arc_to(Enesim_Path_Normalizer *thiz,
		Enesim_Path_Command_Arc_To *arc)
{
	Enesim_Path_Normalizer_State *state = &thiz->state;
	double rx;
	double ry;
	double angle;
	unsigned char large;
	unsigned char sweep;
	double x;
	double y;
	double sx, sy;
	double cos_phi, sin_phi;
	double dx2, dy2;
	double x1p, y1p;
	double x1p2, y1p2;
	double rx2, ry2;
	double lambda;

	enesim_path_command_arc_to_values_to(arc, &rx, &ry, &angle, &x, &y, &large, &sweep);
	// some helpful stuff is available here:
	// http://www.w3.org/TR/SVG/implnote.html#ArcImplementationNotes

	sx = state->last_x;
	sy = state->last_y;

	// if start and end points are identical, then no arc is drawn
	if ((fabs(x - sx) < (1 / 256.0)) && (fabs(y - sy) < (1 / 256.0)))
		return;

	// Correction of out-of-range radii, see F6.6.1 (step 2)
	rx = fabs(rx);
	ry = fabs(ry);
	if ((rx < 0.5) || (ry < 0.5))
	{
		Enesim_Path_Command_Line_To line_to;

		enesim_path_command_line_to_values_from(&line_to, x, y);
		enesim_path_normalizer_line_to(thiz, &line_to);
		return;
	}

	angle = angle * M_PI / 180.0;
	cos_phi = cos(angle);
	sin_phi = sin(angle);
	dx2 = (sx - x) / 2.0;
	dy2 = (sy - y) / 2.0;
	x1p = cos_phi * dx2 + sin_phi * dy2;
	y1p = cos_phi * dy2 - sin_phi * dx2;
	x1p2 = x1p * x1p;
	y1p2 = y1p * y1p;
	rx2 = rx * rx;
	ry2 = ry * ry;
	lambda = (x1p2 / rx2) + (y1p2 / ry2);

	// Correction of out-of-range radii, see F6.6.2 (step 4)
	if (lambda > 1.0)
	{
		// see F6.6.3
		double lambda_root = sqrt(lambda);

		rx *= lambda_root;
		ry *= lambda_root;
		// update rx2 and ry2
		rx2 = rx * rx;
		ry2 = ry * ry;
	}

	double cxp, cyp, cx, cy;
	double c = (rx2 * ry2) - (rx2 * y1p2) - (ry2 * x1p2);

	// check if there is no possible solution
	// (i.e. we can't do a square root of a negative value)
	if (c < 0.0)
	{
		// scale uniformly until we have a single solution
		// (see F6.2) i.e. when c == 0.0
		double scale = sqrt(1.0 - c / (rx2 * ry2));
		rx *= scale;
		ry *= scale;
		// update rx2 and ry2
		rx2 = rx * rx;
		ry2 = ry * ry;

		// step 2 (F6.5.2) - simplified since c == 0.0
		cxp = 0.0;
		cyp = 0.0;
		// step 3 (F6.5.3 first part) - simplified since cxp and cyp == 0.0
		cx = 0.0;
		cy = 0.0;
	}
	else
	{
		// complete c calculation
		c = sqrt(c / ((rx2 * y1p2) + (ry2 * x1p2)));
		// inverse sign if Fa == Fs
		if (large == sweep)
		   c = -c;

		// step 2 (F6.5.2)
		cxp = c * ( rx * y1p / ry);
		cyp = c * (-ry * x1p / rx);

		// step 3 (F6.5.3 first part)
		cx = cos_phi * cxp - sin_phi * cyp;
		cy = sin_phi * cxp + cos_phi * cyp;
	}

	// step 3 (F6.5.3 second part) we now have the center point of the ellipse
	cx += (sx + x) / 2.0;
	cy += (sy + y) / 2.0;

	// step 4 (F6.5.4)
	// we dont' use arccos (as per w3c doc),
	// see http://www.euclideanspace.com/maths/algebra/vectors/angleBetween/index.htm
	// note: atan2 (0.0, 1.0) == 0.0
	double at = atan2(((y1p - cyp) / ry), ((x1p - cxp) / rx));
	double theta1 = (at < 0.0) ? 2.0 * M_PI + at : at;

	double nat = atan2(((-y1p - cyp) / ry), ((-x1p - cxp) / rx));
	double delta_theta = (nat < at) ? 2.0 * M_PI - at + nat : nat - at;

	if (sweep)
	{
		// ensure delta theta < 0 or else add 360 degrees
		if (delta_theta < 0.0)
			delta_theta += 2.0 * M_PI;
	}
	else
	{
		// ensure delta theta > 0 or else substract 360 degrees
		if (delta_theta > 0.0)
		    delta_theta -= 2.0 * M_PI;
	}

	// add several cubic bezier to approximate the arc
	// (smaller than 90 degrees)
	// we add one extra segment because we want something
	// smaller than 90deg (i.e. not 90 itself)
	int segments = (int) (fabs(delta_theta / M_PI_2)) + 1;
	double delta = delta_theta / segments;

	// http://www.stillhq.com/ctpfaq/2001/comp.text.pdf-faq-2001-04.txt (section 2.13)
	double bcp = 4.0 / 3 * (1 - cos(delta / 2)) / sin(delta / 2);

	double cos_phi_rx = cos_phi * rx;
	double cos_phi_ry = cos_phi * ry;
	double sin_phi_rx = sin_phi * rx;
	double sin_phi_ry = sin_phi * ry;

	double cos_theta1 = cos(theta1);
	double sin_theta1 = sin(theta1);

	int i;
	for (i = 0; i < segments; ++i)
	{
		Enesim_Path_Command_Cubic_To cubic_to;
		// end angle (for this segment) = current + delta
		double theta2 = theta1 + delta;
		double cos_theta2 = cos(theta2);
		double sin_theta2 = sin(theta2);

		// first control point (based on start point sx,sy)
		double c1x = sx - bcp * (cos_phi_rx * sin_theta1 + sin_phi_ry * cos_theta1);
		double c1y = sy + bcp * (cos_phi_ry * cos_theta1 - sin_phi_rx * sin_theta1);

		// end point (for this segment)
		double ex = cx + (cos_phi_rx * cos_theta2 - sin_phi_ry * sin_theta2);
		double ey = cy + (sin_phi_rx * cos_theta2 + cos_phi_ry * sin_theta2);

		// second control point (based on end point ex,ey)
		double c2x = ex + bcp * (cos_phi_rx * sin_theta2 + sin_phi_ry * cos_theta2);
		double c2y = ey + bcp * (sin_phi_rx * sin_theta2 - cos_phi_ry * cos_theta2);

		enesim_path_command_cubic_to_values_from(&cubic_to, ex, ey, c1x, c1y, c2x, c2y);
		enesim_path_normalizer_cubic_to(thiz, &cubic_to);

		// next start point is the current end point (same for angle)
		sx = ex;
		sy = ey;
		theta1 = theta2;
		// avoid recomputations
		cos_theta1 = cos_theta2;
		sin_theta1 = sin_theta2;
	}
}

void enesim_path_normalizer_line_to(Enesim_Path_Normalizer *thiz,
		Enesim_Path_Command_Line_To *line_to)
{
	Enesim_Path_Normalizer_State *state = &thiz->state;
	double x, y;

	enesim_path_command_line_to_values_to(line_to, &x, &y);
	state->last_ctrl_x = state->last_x;
	state->last_ctrl_y = state->last_y;
	state->last_x = x;
	state->last_y = y;
	thiz->descriptor->line_to(line_to, state, thiz->data);
}

void enesim_path_normalizer_squadratic_to(Enesim_Path_Normalizer *thiz,
		Enesim_Path_Command_Squadratic_To *squadratic)
{
	Enesim_Path_Normalizer_State *state = &thiz->state;
	Enesim_Path_Command_Quadratic_To quadratic_to;
	double x, y, x0, y0, cx0, cy0;

	enesim_path_command_squadratic_values_to(squadratic, &x, &y);
	x0 = state->last_x;
	y0 = state->last_y;
	cx0 = state->last_ctrl_x;
	cy0 = state->last_ctrl_y;
	cx0 = (2 * x0) - cx0;
	cy0 = (2 * y0) - cy0;
	enesim_path_command_quadratic_values_from(&quadratic_to,
		x, y, cx0, cy0);
	enesim_path_normalizer_quadratic_to(thiz, &quadratic_to);
}

void enesim_path_normalizer_quadratic_to(Enesim_Path_Normalizer *thiz,
		Enesim_Path_Command_Quadratic_To *quadratic)
{
	Enesim_Path_Normalizer_State *state = &thiz->state;
	Enesim_Path_Command_Cubic_To cubic_to;
	Enesim_Path_Quadratic q;
	Enesim_Path_Cubic c;

	q.start_x = state->last_x;
	q.start_y = state->last_y;
	enesim_path_command_quadratic_values_to(quadratic, &q.end_x, &q.end_y, &q.ctrl_x, &q.ctrl_y);
	enesim_path_quadratic_cubic_to(&q, &c);
	enesim_path_command_cubic_to_values_from(&cubic_to, c.end_x, c.end_y, c.ctrl_x0, c.ctrl_y0, c.ctrl_x1, c.ctrl_y1);
	enesim_path_normalizer_cubic_to(thiz, &cubic_to);
}

void enesim_path_normalizer_cubic_to(Enesim_Path_Normalizer *thiz,
		Enesim_Path_Command_Cubic_To *cubic_to)
{
	Enesim_Path_Normalizer_State *state = &thiz->state;
	double x, y, unused, ctrl_x1, ctrl_y1;

	enesim_path_command_cubic_to_values_to(cubic_to, &x, &y, &unused, &unused, &ctrl_x1, &ctrl_y1);
	state->last_ctrl_x = ctrl_x1;
	state->last_ctrl_y = ctrl_y1;
	state->last_x = x;
	state->last_y = y;
	thiz->descriptor->cubic_to(cubic_to, state, thiz->data);
}

void enesim_path_normalizer_move_to(Enesim_Path_Normalizer *thiz,
		Enesim_Path_Command_Move_To *move_to)
{
	Enesim_Path_Normalizer_State *state = &thiz->state;
	double x, y;

	enesim_path_command_move_to_values_to(move_to, &x, &y);
	state->last_ctrl_x = x;
	state->last_ctrl_y = y;
	state->last_x = x;
	state->last_y = y;
	thiz->descriptor->move_to(move_to, state, thiz->data);
}

void enesim_path_normalizer_close(Enesim_Path_Normalizer *thiz,
		Enesim_Path_Command_Close *close)
{
	Enesim_Path_Normalizer_State *state = &thiz->state;
	thiz->descriptor->close(close, state, thiz->data);
}

void enesim_path_normalizer_normalize(Enesim_Path_Normalizer *thiz,
		Enesim_Path_Command *cmd)
{
	switch (cmd->type)
	{
		case ENESIM_PATH_COMMAND_MOVE_TO:
		enesim_path_normalizer_move_to(thiz, &cmd->definition.move_to);
		break;

		case ENESIM_PATH_COMMAND_LINE_TO:
		enesim_path_normalizer_line_to(thiz, &cmd->definition.line_to);
		break;

		case ENESIM_PATH_COMMAND_QUADRATIC_TO:
		enesim_path_normalizer_quadratic_to(thiz, &cmd->definition.quadratic_to);
		break;

		case ENESIM_PATH_COMMAND_SQUADRATIC_TO:
		enesim_path_normalizer_squadratic_to(thiz, &cmd->definition.squadratic_to);
		break;

		case ENESIM_PATH_COMMAND_CUBIC_TO:
		enesim_path_normalizer_cubic_to(thiz, &cmd->definition.cubic_to);
		break;

		case ENESIM_PATH_COMMAND_SCUBIC_TO:
		enesim_path_normalizer_scubic_to(thiz, &cmd->definition.scubic_to);
		break;

		case ENESIM_PATH_COMMAND_ARC_TO:
		enesim_path_normalizer_arc_to(thiz, &cmd->definition.arc_to);
		break;

		case ENESIM_PATH_COMMAND_CLOSE:
		enesim_path_normalizer_close(thiz, &cmd->definition.close);
		break;

		default:
		break;
	}
}

/* generate a figure only (line, move, close) commands */
Enesim_Path_Normalizer * enesim_path_normalizer_figure_new(
		Enesim_Path_Normalizer_Figure_Descriptor *descriptor,
		void *data)
{
	Enesim_Path_Normalizer_Figure *thiz;

	thiz = calloc(1, sizeof(Enesim_Path_Normalizer_Figure));
	thiz->descriptor = descriptor;
	thiz->data = data;
	return enesim_path_normalizer_new(&_figure_descriptor, thiz);
}

/* generate a path simplifed (line, move, cubic, close) */
Enesim_Path_Normalizer * enesim_path_normalizer_path_new(
		Enesim_Path_Normalizer_Path_Descriptor *descriptor,
		void *data)
{
	Enesim_Path_Normalizer_Path *thiz;

	thiz = calloc(1, sizeof(Enesim_Path_Normalizer_Path));
	thiz->descriptor = descriptor;
	thiz->data = data;
	return enesim_path_normalizer_new(&_path_descriptor, thiz);
}

void enesim_path_normalizer_reset(Enesim_Path_Normalizer *thiz)
{
	thiz->state.last_ctrl_x = 0;
	thiz->state.last_ctrl_y = 0;
	thiz->state.last_x = 0;
	thiz->state.last_y = 0;
}
