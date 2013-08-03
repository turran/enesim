#include <Eina_Extra.h>
#include "Enesim.h"
#include "enesim_renderer_example.h"

static Enesim_Renderer * enesim_renderer_path04(void)
{
	Enesim_Renderer *r1, *r2;
	Enesim_Path *p;
	double x, y, w, h;

	x = 100;
	y = 100;
	w = 60;
	h = 50;

	r1 = enesim_renderer_compound_new();
	r2 = enesim_renderer_background_new();
	enesim_renderer_rop_set(r2, ENESIM_FILL);
	enesim_renderer_color_set(r2, 0Xffffffff);
	enesim_renderer_compound_layer_add(r1, r2);

	r2 = enesim_renderer_ellipse_new();
	enesim_renderer_ellipse_x_set(r2, x);
	enesim_renderer_ellipse_y_set(r2, y);
	enesim_renderer_ellipse_radii_set(r2, w, h);
	enesim_renderer_rop_set(r2, ENESIM_BLEND);
        enesim_renderer_shape_stroke_weight_set(r2, 2);
        enesim_renderer_shape_stroke_color_set(r2, 0xff000000);
        enesim_renderer_shape_draw_mode_set(r2, ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE);
	enesim_renderer_compound_layer_add(r1, r2);

	r2 = enesim_renderer_ellipse_new();
	enesim_renderer_ellipse_x_set(r2, x + w);
	enesim_renderer_ellipse_y_set(r2, y - h);
	enesim_renderer_ellipse_radii_set(r2, w, h);
        enesim_renderer_shape_stroke_weight_set(r2, 2);
        enesim_renderer_shape_stroke_color_set(r2, 0xff000000);
        enesim_renderer_shape_draw_mode_set(r2, ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE);
	enesim_renderer_rop_set(r2, ENESIM_BLEND);
	enesim_renderer_compound_layer_add(r1, r2);

	p = enesim_path_new();
	enesim_path_move_to(p, x, y - h);
	enesim_path_arc_to(p, w, h, 0, EINA_FALSE, EINA_FALSE, x + w, y);

	r2 = enesim_renderer_path_new();
	enesim_renderer_path_path_set(r2, p);
        enesim_renderer_shape_stroke_weight_set(r2, 2);
        enesim_renderer_shape_stroke_color_set(r2, 0xffff0000);
        enesim_renderer_shape_draw_mode_set(r2, ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE);
	enesim_renderer_rop_set(r2, ENESIM_BLEND);
	enesim_renderer_compound_layer_add(r1, r2);

	return r1;
}

EXAMPLE(enesim_renderer_path04)

