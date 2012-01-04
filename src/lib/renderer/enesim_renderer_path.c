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

typedef void (*Enesim_Renderer_Path_Vertex_Add)(double x, double y, void *data);
typedef void (*Enesim_Renderer_Path_Polygon_Add)(void *data);
typedef void (*Enesim_Renderer_Path_Polygon_Close)(Eina_Bool close, void *data);
typedef void (*Enesim_Renderer_Path_Done)(void *data);

typedef struct _Enesim_Renderer_Command_State
{
	Enesim_Renderer_Path_Vertex_Add vertex_add;
	Enesim_Renderer_Path_Polygon_Add polygon_add;
	Enesim_Renderer_Path_Polygon_Close polygon_close;
	Enesim_Renderer_Path_Done path_done;
	double last_x;
	double last_y;
	double last_ctrl_x;
	double last_ctrl_y;
	void *data;
} Enesim_Renderer_Command_State;

typedef struct _Enesim_Renderer_Path_Strokeless_State
{
	Enesim_Figure *fill_figure;
} Enesim_Renderer_Path_Strokeless_State;

typedef struct _Enesim_Renderer_Path_Stroke_State
{
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

	printf("n = %g %g\n", dy, dx);
}

static void _stroke_path_vertex_add(double x, double y, void *data)
{
	Enesim_Renderer_Path_Stroke_State *thiz = data;
	Enesim_Polygon *fill;
	Enesim_Polygon *stroke;
	Enesim_Point o0, o1;
	Enesim_Point i0, i1;
	Eina_List *last;
	int c;
	double ox;
	double oy;

	last = eina_list_last(thiz->fill_figure->polygons);
	fill = eina_list_data_get(last);

	last = eina_list_last(thiz->stroke_figure->polygons);
	stroke = eina_list_data_get(last);

	/* just store the first point */
	if (thiz->count < 2)
	{
		switch (thiz->count)
		{
			case 0:
			thiz->first.x = thiz->p0.x = x;
			thiz->first.y = thiz->p0.y = y;
			thiz->count++;
			printf("first %g %g\n", thiz->p0.x, thiz->p0.y);
			return;

			case 1:
			thiz->p1.x = x;
			thiz->p1.y = y;
			_do_normal(&thiz->n01, &thiz->p0, &thiz->p1);

			ox = thiz->r * thiz->n01.x;
			oy = thiz->r * thiz->n01.y;

			o0.x = thiz->p0.x + ox;
			o0.y = thiz->p0.y + oy;
			enesim_polygon_point_append_from_coords(stroke, o0.x, o0.y);

			o1.x = thiz->p1.x + ox;
			o1.y = thiz->p1.y + oy;
			enesim_polygon_point_append_from_coords(stroke, o1.x, o1.y);

#if 0
			i0.x = thiz->p0.x - ox;
			i0.y = thiz->p0.y - oy;
			enesim_polygon_point_append_from_coords(fill, i0.x, i0.y);

			i1.x = thiz->p1.x - ox;
			i1.y = thiz->p1.y - oy;
			enesim_polygon_point_append_from_coords(fill, i1.x, i1.y);

			printf("inverse %g %g %g %g\n", i0.x, i0.y, i1.x, i1.y);
#endif
			enesim_polygon_point_append_from_coords(fill, thiz->p0.x, thiz->p0.y);
			enesim_polygon_point_append_from_coords(fill, thiz->p1.x, thiz->p1.y);
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

	/* add the vertices of the new edge */
	/* check if the previous edge and this one to see the concave/convex thing */
	/* dot product
	 * = 1 pointing same direction
	 * > 0 concave
	 * = 0 orthogonal
	 * < 0 convex
	 * = -1 pointing opposite direction
	 */

	c = (thiz->n01.x * thiz->n12.x) + (thiz->n01.y * thiz->n12.y);
	if (c <= 0)
	{
		/* TODO do the curve on the offset */
		enesim_polygon_point_prepend_from_coords(fill, thiz->p1.x, thiz->p1.y);
	}
	else
	{
		/* TODO do the curve on the inset */
		enesim_polygon_point_append_from_coords(stroke, thiz->p1.x, thiz->p1.y);
	}
	ox = thiz->r * thiz->n12.x;
	oy = thiz->r * thiz->n12.y;

	o0.x = thiz->p1.x + ox;
	o0.y = thiz->p1.y + oy;
	enesim_polygon_point_append_from_coords(stroke, o0.x, o0.y);

	o1.x = thiz->p2.x + ox;
	o1.y = thiz->p2.y + oy;
	enesim_polygon_point_append_from_coords(stroke, o1.x, o1.y);

#if 0
	i0.x = thiz->p1.x - ox;
	i0.y = thiz->p1.y - oy;
	enesim_polygon_point_prepend_from_coords(fill, i0.x, i0.y);

	i1.x = thiz->p2.x - ox;
	i1.y = thiz->p2.y - oy;
	enesim_polygon_point_prepend_from_coords(fill, i1.x, i1.y);
#endif
	enesim_polygon_point_prepend_from_coords(fill, thiz->p0.x, thiz->p0.y);
	enesim_polygon_point_prepend_from_coords(fill, thiz->p1.x, thiz->p1.y);

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

	p = enesim_polygon_new();
	enesim_figure_polygon_append(thiz->stroke_figure, p);
}

static void _stroke_path_polygon_close(Eina_Bool close, void *data)
{
        Enesim_Renderer_Path_Stroke_State *thiz = data;
	Enesim_Polygon *p;
	Eina_List *last;

	last = eina_list_last(thiz->fill_figure->polygons);
	p = eina_list_data_get(last);
	p->closed = close;

	last = eina_list_last(thiz->stroke_figure->polygons);
	p = eina_list_data_get(last);
	p->closed = close;
}

static void _stroke_path_done(void *data)
{
        Enesim_Renderer_Path_Stroke_State *thiz = data;
	Enesim_Polygon *fill;
	Enesim_Polygon *stroke;
	Eina_List *last;

	last = eina_list_last(thiz->fill_figure->polygons);
	fill = eina_list_data_get(last);

	last = eina_list_last(thiz->stroke_figure->polygons);
	stroke = eina_list_data_get(last);

	/* FOR NOW */
	enesim_polygon_point_prepend_from_coords(fill, thiz->p1.x, thiz->p1.y);
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
	state->polygon_add(state->data);
	state->vertex_add(x, y, state->data);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = x;
	state->last_ctrl_y = y;
}

static void _enesim_line_to(Enesim_Renderer_Command_State *state,
		double x, double y)
{
	state->vertex_add(x, y, state->data);
	state->last_ctrl_x = state->last_x;
	state->last_ctrl_y = state->last_y;
	state->last_x = x;
	state->last_y = y;
}

/* these subdiv approximations need to be done more carefully */
static void _enesim_quadratic_to(Enesim_Renderer_Command_State *state,
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

	_enesim_quadratic_to(state, x0, y0, x01, y01);
	state->last_x = x01;
	state->last_y = y01;
	state->last_ctrl_x = x0;
	state->last_ctrl_y = y0;

	_enesim_quadratic_to(state, x1, y1, x, y);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = x1;
	state->last_ctrl_y = y1;
}

static void _enesim_squadratic_to(Enesim_Renderer_Command_State *state,
		double x, double y)
{
	double x0, y0, cx0, cy0;

	x0 = state->last_x;
	y0 = state->last_y;
	cx0 = state->last_ctrl_x;
	cy0 = state->last_ctrl_y;
	cx0 = (2 * x0) - cx0;
	cy0 = (2 * y0) - cy0;

	_enesim_quadratic_to(state, cx0, cy0, x, y);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = cx0;
	state->last_ctrl_y = cy0;
}

static void _enesim_cubic_to(Enesim_Renderer_Command_State *state,
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

	_enesim_cubic_to(state, x01, y01, xa, ya, xc, yc);
	state->last_x = xc;
	state->last_y = yc;
	state->last_ctrl_x = xa;
	state->last_ctrl_y = ya;

	_enesim_cubic_to(state, xb, yb, x23, y23, x, y);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = x23;
	state->last_ctrl_y = y23;
}


static void _enesim_scubic_to(Enesim_Renderer_Command_State *state,
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

	_enesim_cubic_to(state, cx0, cy0, ctrl_x, ctrl_y, x, y);
	state->last_x = x;
	state->last_y = y;
	state->last_ctrl_x = ctrl_x;
	state->last_ctrl_y = ctrl_y;
}

/* code adapted from the moonlight sources
 * we need to fix it to match c89
 */
static void _enesim_arc_to(Enesim_Renderer_Command_State *state, double rx, double ry, double angle,
                   unsigned char large, unsigned char sweep, double x, double y)
{
	double sx, sy;
	double cos_phi, sin_phi;
	double dx2, dy2;
	double x1p, y1p;
	double x1p2, y1p2;
	double rx2, ry2;
	double lambda;

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
		_enesim_line_to(state, x, y);
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

		_enesim_cubic_to(state, c1x, c1y, c2x, c2y, ex, ey);

		// next start point is the current end point (same for angle)
		sx = ex;
		sy = ey;
		theta1 = theta2;
		// avoid recomputations
		cos_theta1 = cos_theta2;
		sin_theta1 = sin_theta2;
	}
}

static void _enesim_close(Enesim_Renderer_Command_State *state, Eina_Bool close)
{
	state->polygon_close(close, state->data);
}

static void _enesim_path_generate_vertices(Eina_List *commands,
		Enesim_Renderer_Path_Vertex_Add vertex_add,
		Enesim_Renderer_Path_Polygon_Add polygon_add,
		Enesim_Renderer_Path_Polygon_Close polygon_close,
		Enesim_Renderer_Path_Done path_done,
		double scale_x, double scale_y, void *data)
{
	Eina_List *l;
	Enesim_Renderer_Command_State state;
	Enesim_Renderer_Path_Command *cmd;

	state.vertex_add = vertex_add;
	state.polygon_add = polygon_add;
	state.polygon_close = polygon_close;
	state.path_done = path_done;
	state.last_x = 0;
	state.last_y = 0;
	state.last_ctrl_x = 0;
	state.last_ctrl_y = 0;
	state.data = data;

	EINA_LIST_FOREACH(commands, l, cmd)
	{
		double x, y;
		/* send the new vertex to the figure renderer */
		switch (cmd->type)
		{
			case ENESIM_COMMAND_MOVE_TO:
			x = scale_x * cmd->definition.move_to.x;
			y = scale_y * cmd->definition.move_to.y;
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			_enesim_move_to(&state, x, y);
			break;

			case ENESIM_COMMAND_LINE_TO:
			x = scale_x * cmd->definition.line_to.x;
			y = scale_y * cmd->definition.line_to.y;
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			_enesim_line_to(&state, x, y);
			break;

			case ENESIM_COMMAND_QUADRATIC_TO:
			x = scale_x * cmd->definition.quadratic_to.x;
			y = scale_y * cmd->definition.quadratic_to.y;
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			_enesim_quadratic_to(&state, scale_x * cmd->definition.quadratic_to.ctrl_x,
					scale_y * cmd->definition.quadratic_to.ctrl_y, x, y);
			break;

			case ENESIM_COMMAND_SQUADRATIC_TO:
			x = scale_x * cmd->definition.squadratic_to.x;
			y = scale_y * cmd->definition.squadratic_to.y;
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			_enesim_squadratic_to(&state, x, y);
			break;

			case ENESIM_COMMAND_CUBIC_TO:
			x = scale_x * cmd->definition.cubic_to.x;
			y = scale_y * cmd->definition.cubic_to.y;
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			_enesim_cubic_to(&state, scale_x * cmd->definition.cubic_to.ctrl_x0,
					scale_y * cmd->definition.cubic_to.ctrl_y0,
					scale_x * cmd->definition.cubic_to.ctrl_x1,
					scale_y * cmd->definition.cubic_to.ctrl_y1,
					x, y);
			break;

			case ENESIM_COMMAND_SCUBIC_TO:
			x = scale_x * cmd->definition.scubic_to.x;
			y = scale_y * cmd->definition.scubic_to.y;
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			_enesim_scubic_to(&state, scale_x * cmd->definition.scubic_to.ctrl_x,
					scale_y * cmd->definition.scubic_to.ctrl_y,
					x, y);
			break;

			case ENESIM_COMMAND_ARC_TO:
			x = scale_x * cmd->definition.arc_to.x;
			y = scale_y * cmd->definition.arc_to.y;
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			_enesim_arc_to(&state, scale_x * cmd->definition.arc_to.rx,
					scale_y * cmd->definition.arc_to.ry,
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
static const char * _enesim_path_name(Enesim_Renderer *r)
{
	return "path";
}

static Eina_Bool _enesim_state_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
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
					 state->sx, state->sy, &st);
			/* TODO this fill/stroke figure depends on the definition of the vertices being
			 * given that the normal might change by defining the polygon CW or CCW
			 * Maybe we should check the graham scan algorithm to know what normal to take
			 * depending on the edge direction?
			 */
			enesim_rasterizer_figure_set(thiz->bifigure, thiz->stroke_figure);
			enesim_rasterizer_bifigure_over_figure_set(thiz->bifigure, thiz->fill_figure);
		} 
		else
		{
			Enesim_Renderer_Path_Strokeless_State st;

			st.fill_figure = thiz->fill_figure;

			_enesim_path_generate_vertices(thiz->commands, _strokeless_path_vertex_add,
					_strokeless_path_polygon_add,
					_strokeless_path_polygon_close,
					NULL,
					state->sx, state->sy, &st);
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

static void _enesim_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Path *thiz;

	thiz = _enesim_path_get(r);
	enesim_renderer_cleanup(thiz->bifigure, s);
	thiz->changed = EINA_FALSE;
}

static void _enesim_path_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
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

static Enesim_Renderer_Descriptor _path_descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _enesim_path_name,
	/* .free = 			*/ NULL,
	/* .boundings = 		*/ NULL, // _boundings,
	/* .destination_transform = 	*/ NULL,
	/* .flags = 			*/ NULL,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ NULL,
	/* .sw_setup = 			*/ _enesim_state_setup,
	/* .sw_cleanup = 		*/ _enesim_state_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
	/* .opengl_setup =          	*/ NULL,
	/* .opengl_shader_setup = 	*/ NULL,
	/* .opengl_cleanup =        	*/ NULL
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
