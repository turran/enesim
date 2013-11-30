#include "enesim_example_renderer.h"

/**
 * @example enesim_renderer_circle01.c
 * Example usage of a circle renderer
 * @image html enesim_renderer_circle01.png
 */
Enesim_Renderer * enesim_example_renderer_renderer_get(void)
{
	Enesim_Renderer *r;

	r = enesim_renderer_circle_new();
	/* TODO we have a bug on the rasterizer election. If the stroke weight
	 * is <= 1 but we have a transformation, the simple rasterizer should
	 * not be used
	 */
	enesim_renderer_circle_center_set(r, 128, 128);
	enesim_renderer_circle_radius_set(r, 64);

	enesim_renderer_shape_stroke_color_set(r, 0xffff0000);
	enesim_renderer_shape_stroke_weight_set(r, 5);
	enesim_renderer_shape_fill_color_set(r, 0x55550000);
	enesim_renderer_shape_draw_mode_set(r, ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE_FILL);

	return r;
}
