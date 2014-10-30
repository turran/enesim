#include "enesim_example_renderer.h"

/**
 * @example enesim_renderer_background01.c
 * Example usage of a background renderer
 * @image html enesim_renderer_background01.png
 */
Enesim_Renderer * enesim_example_renderer_renderer_get(Enesim_Example_Renderer_Options *options EINA_UNUSED)
{
	Enesim_Renderer *r;

	r = enesim_renderer_background_new();
	enesim_renderer_background_color_set(r, 0xaaaa0000);

	return r;
}

