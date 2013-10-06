#include "enesim_renderer_example.h"

/**
 * @example enesim_renderer_rectangle01.c
 * Example usage of a rectangle renderer
 * @image html enesim_renderer_rectangle01.png
 */
static Enesim_Renderer * enesim_renderer_rectangle01(void)
{
	Enesim_Renderer *r;

	r = enesim_renderer_rectangle_new();
	enesim_renderer_rectangle_position_set(r, 30, 30);
	enesim_renderer_rectangle_size_set(r, 120, 120);
	enesim_renderer_rectangle_corner_radii_set(r, 15, 15);
	enesim_renderer_rectangle_corners_set(r, EINA_TRUE, EINA_TRUE, EINA_TRUE, EINA_TRUE);

	enesim_renderer_shape_stroke_weight_set(r, 18);
	enesim_renderer_shape_stroke_color_set(r, 0xffffff00);
	enesim_renderer_shape_fill_color_set(r, 0xffff0000);
	enesim_renderer_shape_draw_mode_set(r, ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE_FILL);
	return r;
}
EXAMPLE(enesim_renderer_rectangle01)

