#include "enesim_example_renderer.h"

/**
 * @example enesim_renderer_path07.c
 * Example usage of a path renderer
 * @image html enesim_renderer_path07.png
 */
Enesim_Renderer * enesim_example_renderer_renderer_get(
		Enesim_Example_Renderer_Options *options)
{
	Enesim_Renderer *r;
	Enesim_Matrix m;
	Enesim_Path *p;

	r = enesim_renderer_path_new();
	enesim_renderer_shape_stroke_color_set(r, 0xff000000);
	enesim_renderer_shape_stroke_weight_set(r, 0.1);
	enesim_renderer_shape_draw_mode_set(r, ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE);

	p = enesim_renderer_path_path_get(r);
	enesim_path_move_to(p, 0.3, 0.3);
	enesim_path_cubic_to(p, 0.6, 1.1, 1.0, 0.9, 1.4, 0.3);
	enesim_path_unref(p);

	enesim_matrix_scale(&m, options->width/2, options->height/2);
	enesim_renderer_transformation_set(r, &m);

	return r;
}

