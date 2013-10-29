#include "enesim_renderer_example.h"

/**
 * @example enesim_renderer_image01.c
 * Example usage of a image renderer
 * @image html enesim_renderer_image01.png
 */
static Enesim_Renderer * enesim_renderer_image01(void)
{
	Enesim_Renderer *is;
	Enesim_Renderer *r;
	Enesim_Surface *s;
	Enesim_Log *error = NULL;

	/* Create a temporary surface to use as a source
	 * for the image renderer.
	 */
	s = enesim_surface_new(ENESIM_FORMAT_ARGB8888,
			64, 64);
	is = enesim_renderer_checker_new();
	enesim_renderer_checker_odd_color_set(is, 0xffff0000);
	enesim_renderer_checker_even_color_set(is, 0xffaa0000);
	enesim_renderer_checker_width_set(is, 8);
	enesim_renderer_checker_height_set(is, 8);
	if (!enesim_renderer_draw(is, s, ENESIM_ROP_FILL, NULL, 0, 0,
			&error))
	{
		enesim_log_dump(error);
		enesim_surface_unref(s);
		enesim_renderer_unref(is);
		return NULL;
	}
	enesim_renderer_unref(is);

	/* Finally make the image renderer and use the rendered
	 * surface
	 */
	r = enesim_renderer_image_new();
	enesim_renderer_image_source_surface_set(r, s);
	enesim_renderer_image_size_set(r, 64, 64);

	return r;
}
EXAMPLE(enesim_renderer_image01)

