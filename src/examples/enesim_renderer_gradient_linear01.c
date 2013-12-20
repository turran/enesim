#include "enesim_example_renderer.h"

/**
 * @example enesim_renderer_gradient_linear01.c
 * Example usage of a gradient_linear renderer
 * @image html enesim_renderer_gradient_linear01.png
 */
Enesim_Renderer * enesim_example_renderer_renderer_get(Enesim_Example_Renderer_Options *options EINA_UNUSED)
{
	Enesim_Renderer *r;
	Enesim_Matrix m;
	Enesim_Renderer_Gradient_Stop stop;

	r = enesim_renderer_gradient_linear_new();
	enesim_matrix_scale(&m, 64, 64);
	enesim_renderer_transformation_set(r, &m);

	enesim_renderer_gradient_linear_position_set(r, 1, 1, 3, 3);
	stop.argb = 0xffff0000;
	stop.pos = 0;
	enesim_renderer_gradient_stop_add(r, &stop);
	stop.argb = 0xff00ff00;
	stop.pos = 1;
	enesim_renderer_gradient_stop_add(r, &stop);

	return r;
}

