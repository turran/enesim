#include "enesim_renderer_example.h"

/**
 * @example enesim_renderer_path01.c
 * Example usage of a path renderer
 * @image html enesim_renderer_path01.png
 */
static Enesim_Renderer * enesim_renderer_path01(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer *r2;
	Enesim_Renderer_Gradient_Stop stop;
	Enesim_Path *p;

	p = enesim_path_new();
	enesim_path_move_to(p, 2*12, 2*27);
	enesim_path_cubic_to(p, 2*7, 2*37, 2*18, 2*50, 2*18, 2*60);
	enesim_path_scubic_to(p, 2*0, 2*80, 2*10, 2*94);
	enesim_path_scubic_to(p, 2*40, 2*74, 2*50, 2*78);
	enesim_path_scubic_to(p, 2*60, 2*99, 2*76, 2*95);
	enesim_path_scubic_to(p, 2*72, 2*70, 2*75, 2*65);
	enesim_path_scubic_to(p, 2*95, 2*55, 2*95, 2*42);
	enesim_path_scubic_to(p, 2*69, 2*37, 2*66, 2*32);
	enesim_path_scubic_to(p, 2*67, 2*2, 2*53, 2*7);
	enesim_path_scubic_to(p, 2*43, 2*17, 2*35, 2*22);
	enesim_path_scubic_to(p, 2*17, 2*17, 2*12, 2*27);

	r2 = enesim_renderer_gradient_linear_new();
	enesim_renderer_gradient_linear_position_set(r2, 0, 0, 256, 256);
	stop.argb = 0xffff0000;
	stop.pos = 0;
	enesim_renderer_gradient_stop_add(r2, &stop);
	stop.argb = 0xff00ff00;
	stop.pos = 1;
	enesim_renderer_gradient_stop_add(r2, &stop);

	r = enesim_renderer_path_new();
	enesim_renderer_path_path_set(r, p);
	enesim_renderer_shape_stroke_weight_set(r, 18);
	enesim_renderer_shape_stroke_color_set(r, 0xffffff00);
	enesim_renderer_shape_fill_renderer_set(r, r2);
	enesim_renderer_shape_draw_mode_set(r, ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE_FILL);
	enesim_renderer_shape_stroke_dash_add_simple(r, 50, 20);
	enesim_renderer_shape_stroke_cap_set(r, ENESIM_RENDERER_SHAPE_STROKE_CAP_ROUND);

	return r;
}

EXAMPLE(enesim_renderer_path01)
