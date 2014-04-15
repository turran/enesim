#include "enesim_example_renderer.h"

/**
 * @example enesim_renderer_pattern01.c
 * Example usage of a pattern renderer
 * @image html enesim_renderer_pattern01.png
 */
Enesim_Renderer * enesim_example_renderer_renderer_get(Enesim_Example_Renderer_Options *options EINA_UNUSED)
{
	Enesim_Renderer *r1, *r2;

	r1 = enesim_renderer_circle_new();
	enesim_renderer_circle_center_set(r1, 128, 128);
	enesim_renderer_circle_radius_set(r1, 64);
	enesim_renderer_shape_stroke_weight_set(r1, 5);
	enesim_renderer_shape_stroke_color_set(r1, 0xff0000ff);
	enesim_renderer_shape_fill_color_set(r1, 0xffff0000);
	enesim_renderer_shape_draw_mode_set(r1, ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE_FILL);

	r2 = enesim_renderer_pattern_new();
	enesim_renderer_pattern_source_renderer_set(r2, r1);
	enesim_renderer_pattern_size_set(r2, 256, 256);
	enesim_renderer_pattern_repeat_mode_set(r2, ENESIM_REPEAT);

	return r2;
}

