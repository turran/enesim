#include "enesim_example_renderer.h"

/**
 * @example enesim_renderer_path03.c
 * Example usage of a path renderer
 * @image html enesim_renderer_path03.png
 */
Enesim_Renderer * enesim_example_renderer_renderer_get(Enesim_Example_Renderer_Options *options EINA_UNUSED)
{
	Enesim_Renderer *r;
	Enesim_Path *p;

	p = enesim_path_new();
	enesim_path_move_to(p, 123.2, 10.5);
	enesim_path_line_to(p, 102.5, 243.8);
	enesim_path_line_to(p, 240.6, 80.3);
	enesim_path_line_to(p, 10.4, 90.6);
	enesim_path_line_to(p, 230.3, 230.1);
	enesim_path_line_to(p, 123.2, 10.5);

	r = enesim_renderer_path_new();
	enesim_renderer_path_inner_path_set(r, p);
	enesim_renderer_shape_stroke_weight_set(r, 18);
	enesim_renderer_shape_stroke_color_set(r, 0xffff0000);
	enesim_renderer_shape_fill_color_set(r, 0xff000000);
	enesim_renderer_shape_draw_mode_set(r, ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE_FILL);
	enesim_renderer_shape_fill_rule_set(r, ENESIM_RENDERER_SHAPE_FILL_RULE_EVEN_ODD);
	return r;
}
