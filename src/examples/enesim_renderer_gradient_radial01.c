#include "enesim_example_renderer.h"

/**
 * @example enesim_renderer_gradient_radial01.c
 * Example usage of a gradient_radial renderer
 * @image html enesim_renderer_gradient_radial01.png
 */
Enesim_Renderer * enesim_example_renderer_renderer_get(Enesim_Example_Renderer_Options *options EINA_UNUSED)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Gradient_Stop stop;

	r = enesim_renderer_gradient_radial_new();

	enesim_renderer_gradient_radial_center_set(r, 128, 128);
	enesim_renderer_gradient_radial_focus_set(r, 128, 128);
	enesim_renderer_gradient_radial_radius_set(r, 64);
	enesim_renderer_gradient_repeat_mode_set(r, ENESIM_REPEAT_MODE_REFLECT);
	stop.argb = 0xffff0000;
	stop.pos = 0;
	enesim_renderer_gradient_stop_add(r, &stop);
	stop.argb = 0xff00ff00;
	stop.pos = 1;
	enesim_renderer_gradient_stop_add(r, &stop);

	return r;
}

