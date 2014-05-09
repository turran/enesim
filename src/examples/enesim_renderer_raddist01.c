#include "enesim_example_renderer.h"

/**
 * @example enesim_renderer_raddist01.c
 * Example usage of a raddist renderer
 * @image html enesim_renderer_raddist01.png
 */
Enesim_Renderer * enesim_example_renderer_renderer_get(Enesim_Example_Renderer_Options *options EINA_UNUSED)
{
	Enesim_Renderer *r, *rsrc;
	Enesim_Surface *ssrc;
	Enesim_Log *error = NULL;

	ssrc = enesim_surface_new(ENESIM_FORMAT_ARGB8888, 200, 200);
	rsrc = enesim_renderer_checker_new();
	enesim_renderer_checker_odd_color_set(rsrc, 0xff000000);
	enesim_renderer_checker_even_color_set(rsrc, 0xffffffff);
	enesim_renderer_checker_width_set(rsrc, 20);
	enesim_renderer_checker_height_set(rsrc, 20);
	if (!enesim_renderer_draw(rsrc, ssrc, ENESIM_ROP_FILL, NULL, 0, 0,
			&error))
	{
		enesim_log_dump(error);
		return NULL;
	}
	enesim_renderer_unref(rsrc);

	r = enesim_renderer_raddist_new();
	enesim_renderer_origin_set(r, 28, 28);
	enesim_renderer_raddist_radius_set(r, 100);
	enesim_renderer_raddist_source_surface_set(r, ssrc);
	enesim_renderer_raddist_x_set(r, 100);
	enesim_renderer_raddist_y_set(r, 100);

	return r;
}

