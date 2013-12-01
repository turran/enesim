#include "enesim_example_renderer.h"

/**
 * @example enesim_renderer_perlin01.c
 * Example usage of a perlin renderer
 * @image html enesim_renderer_perlin01.png
 */
Enesim_Renderer * enesim_example_renderer_renderer_get(Enesim_Example_Renderer_Options *options EINA_UNUSED)
{
	Enesim_Renderer *r;

	r = enesim_renderer_perlin_new();
	enesim_renderer_color_set(r, 0xffff0000);
	enesim_renderer_perlin_octaves_set(r, 6);
	enesim_renderer_perlin_persistence_set(r, 1);
	enesim_renderer_perlin_xfrequency_set(r, 0.05);
	enesim_renderer_perlin_yfrequency_set(r, 0.05);

	return r;
}
