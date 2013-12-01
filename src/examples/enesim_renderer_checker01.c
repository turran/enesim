#include "enesim_example_renderer.h"

/**
 * @example enesim_renderer_checker01.c
 * Example usage of a checker renderer
 * @image html enesim_renderer_checker01.png
 */
Enesim_Renderer * enesim_example_renderer_renderer_get(Enesim_Example_Renderer_Options *options EINA_UNUSED)
{
	Enesim_Renderer *r;

	r = enesim_renderer_checker_new();
	enesim_renderer_checker_odd_color_set(r, 0x00000000);
	enesim_renderer_checker_even_color_set(r, 0xffff0000);
	enesim_renderer_checker_width_set(r, 5);
	enesim_renderer_checker_height_set(r, 5);

	return r;
}
