/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2008 Jorge Luis Zapata
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
typedef struct _Enesim_Renderer_Path Enesim_Renderer_Path;
struct _Enesim_Renderer_Path
{
	Enesim_Renderer_Sw_Fill fill;
	Enesim_Renderer *figure;
	float   last_x, last_y;
	float   last_ctrl_x, last_ctrl_y;
// ....  geom_transform?
};

static void _quadratic_to(Enesim_Renderer *r, float ctrl_x, float ctrl_y,
		float x, float y);
static void _cubic_to(Enesim_Renderer *r, float ctrl_x0, float ctrl_y0,
		float ctrl_x, float ctrl_y, float x, float y);

static inline Enesim_Renderer_Path * _path_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	return thiz;
}


static void _move_to(Enesim_Renderer *r, float x, float y)
{
	Enesim_Renderer_Path *thiz;

	thiz = _path_get(r);

	enesim_renderer_figure_polygon_add(thiz->figure);
	enesim_renderer_figure_polygon_vertex_add(thiz->figure, x, y);
	thiz->last_x = x;
	thiz->last_y = y;
	thiz->last_ctrl_x = x;
	thiz->last_ctrl_y = y;
}

static void _line_to(Enesim_Renderer *r, float x, float y)
{
	Enesim_Renderer_Path *thiz;

	thiz = _path_get(r);

	enesim_renderer_figure_polygon_vertex_add(thiz->figure, x, y);
	thiz->last_ctrl_x = thiz->last_x;
	thiz->last_ctrl_y = thiz->last_y;
	thiz->last_x = x;
	thiz->last_y = y;
}

/* these subdiv approximations need to be done more carefully */
static void _quadratic_to(Enesim_Renderer *r, float ctrl_x, float ctrl_y,
		float x, float y)
{
	Enesim_Renderer_Path *thiz;
	double x0, y0, x1, y1, x01, y01;
	double sm = 1 / 92.0;

	thiz = _path_get(r);

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

	_quadratic_to(r, x0, y0, x01, y01);
	thiz->last_x = x01;
	thiz->last_y = y01;
	thiz->last_ctrl_x = x0;
	thiz->last_ctrl_y = y0;

	_quadratic_to(r, x1, y1, x, y);
	thiz->last_x = x;
	thiz->last_y = y;
	thiz->last_ctrl_x = x1;
	thiz->last_ctrl_y = y1;
}

static void _squadratic_to(Enesim_Renderer *r, float x, float y)
{
	Enesim_Renderer_Path *thiz;
	double x0, y0, cx0, cy0;

	thiz = _path_get(r);

	x0 = thiz->last_x;
	y0 = thiz->last_y;
	cx0 = thiz->last_ctrl_x;
	cy0 = thiz->last_ctrl_y;
	cx0 = (2 * x0) - cx0;
	cy0 = (2 * y0) - cy0;

	_quadratic_to(r, cx0, cy0, x, y);
	thiz->last_x = x;
	thiz->last_y = y;
	thiz->last_ctrl_x = cx0;
	thiz->last_ctrl_y = cy0;
}

static void _scubic_to(Enesim_Renderer *r, float ctrl_x, float ctrl_y, float x,
		float y)
{
	Enesim_Renderer_Path *thiz;
	double x0, y0, cx0, cy0;

	thiz = _path_get(r);

	x0 = thiz->last_x;
	y0 = thiz->last_y;
	cx0 = thiz->last_ctrl_x;
	cy0 = thiz->last_ctrl_y;
	cx0 = (2 * x0) - cx0;
	cy0 = (2 * y0) - cy0;

	_cubic_to(r, cx0, cy0, ctrl_x, ctrl_y, x, y);
	thiz->last_x = x;
	thiz->last_y = y;
	thiz->last_ctrl_x = ctrl_x;
	thiz->last_ctrl_y = ctrl_y;
}

static void _cubic_to(Enesim_Renderer *r, float ctrl_x0, float ctrl_y0,
		float ctrl_x, float ctrl_y, float x, float y)
{
	Enesim_Renderer_Path *thiz;
	double x0, y0, x01, y01, x23, y23;
	double xa, ya, xb, yb, xc, yc;
	double sm = 1 / 64.0;

	thiz = _path_get(r);

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

	_cubic_to(r, x01, y01, xa, ya, xc, yc);
	thiz->last_x = xc;
	thiz->last_y = yc;
	thiz->last_ctrl_x = xa;
	thiz->last_ctrl_y = ya;

	_cubic_to(r, xb, yb, x23, y23, x, y);
	thiz->last_x = x;
	thiz->last_y = y;
	thiz->last_ctrl_x = x23;
	thiz->last_ctrl_y = y23;
}

static void _span(Enesim_Renderer *r, int x, int y, unsigned int len, uint32_t *dst)
{
	Enesim_Renderer_Path *thiz;

	thiz = _path_get(r);
	thiz->fill(thiz->figure, x, y, len, dst);
}

/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static Eina_Bool _state_setup(Enesim_Renderer *r, Enesim_Renderer_Sw_Fill *fill)
{
	Enesim_Renderer_Path *thiz;
	double stroke_weight;
	Enesim_Color stroke_color;
	Enesim_Renderer *stroke_renderer;
	Enesim_Color fill_color;
	Enesim_Renderer *fill_renderer;
	Enesim_Shape_Draw_Mode draw_mode;

	thiz = _path_get(r);
	if (!thiz)
		return EINA_FALSE;
	if (!thiz->figure)
		return EINA_FALSE; // should just not draw

	enesim_renderer_shape_outline_weight_get(r, &stroke_weight);
	enesim_renderer_shape_outline_weight_set(thiz->figure, stroke_weight);

	enesim_renderer_shape_outline_color_get(r, &stroke_color);
	enesim_renderer_shape_outline_color_set(thiz->figure, stroke_color);

	enesim_renderer_shape_outline_renderer_get(r, &stroke_renderer);
	enesim_renderer_shape_outline_renderer_set(thiz->figure, stroke_renderer);

	enesim_renderer_shape_fill_color_get(r, &fill_color);
	enesim_renderer_shape_fill_color_set(thiz->figure, fill_color);

	enesim_renderer_shape_fill_renderer_get(r, &fill_renderer);
	enesim_renderer_shape_fill_renderer_set(thiz->figure, fill_renderer);

	enesim_renderer_shape_draw_mode_get(thiz->figure, &draw_mode);
	enesim_renderer_shape_draw_mode_set(thiz->figure, draw_mode);

	thiz->figure->matrix = r->matrix;

	if (!enesim_renderer_sw_setup(thiz->figure))
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

static void _state_cleanup(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;

	thiz = _path_get(r);
	enesim_renderer_shape_sw_cleanup(thiz->figure);
}

static void _boundings(Enesim_Renderer *r, Enesim_Rectangle *boundings)
{
	Enesim_Renderer_Path *thiz;

	thiz = _path_get(r);
	if (!thiz->figure) return;
	enesim_renderer_boundings(thiz->figure, boundings);
}

static Enesim_Renderer_Descriptor _path_descriptor = {
	/* .sw_setup =   */ _state_setup,
	/* .sw_cleanup = */ _state_cleanup,
	/* .free =       */ NULL,
	/* .boundings =  */ _boundings,
	/* .flags =      */ NULL,
	/* .is_inside =  */ NULL
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
EAPI void enesim_renderer_path_move_to(Enesim_Renderer *r, float x, float y)
{
	x = ((int) (2* x + 0.5)) / 2.0;
	y = ((int) (2* y + 0.5)) / 2.0;
	_move_to(r, x, y);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_line_to(Enesim_Renderer *r, float x, float y)
{
	x = ((int) (2* x + 0.5)) / 2.0;
	y = ((int) (2* y + 0.5)) / 2.0;
	_line_to(r, x, y);
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_squadratic_to(Enesim_Renderer *r, float x,
		float y)
{
	x = ((int) (2* x + 0.5)) / 2.0;
	y = ((int) (2* y + 0.5)) / 2.0;
	_squadratic_to(r, x, y);
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_quadratic_to(Enesim_Renderer *r, float ctrl_x,
		float ctrl_y, float x, float y)
{
	x = ((int) (2* x + 0.5)) / 2.0;
	y = ((int) (2* y + 0.5)) / 2.0;
	_quadratic_to(r, ctrl_x, ctrl_y, x, y);
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_cubic_to(Enesim_Renderer *r, float ctrl_x0,
		float ctrl_y0, float ctrl_x, float ctrl_y, float x, float y)
{
	x = ((int) (2* x + 0.5)) / 2.0;
	y = ((int) (2* y + 0.5)) / 2.0;
	_cubic_to(r, ctrl_x0, ctrl_y0, ctrl_x, ctrl_y, x, y);
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_scubic_to(Enesim_Renderer *r, float ctrl_x,
		float ctrl_y, float x, float y)
{
	x = ((int) (2* x + 0.5)) / 2.0;
	y = ((int) (2* y + 0.5)) / 2.0;
	_scubic_to(r, ctrl_x, ctrl_y, x, y);
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_clear(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;

	thiz = _path_get(r);

	enesim_renderer_figure_clear(thiz->figure);
	thiz->last_x = 0;
	thiz->last_y = 0;
	thiz->last_ctrl_x = 0;
	thiz->last_ctrl_y = 0;
}
