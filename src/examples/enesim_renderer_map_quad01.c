#include "enesim_example_renderer.h"

/**
 * @example enesim_renderer_map_quad01.c
 * Example usage of a map_quad renderer
 * @image html enesim_renderer_map_quad01.png
 */
Enesim_Renderer * enesim_example_renderer_renderer_get(Enesim_Example_Renderer_Options *options EINA_UNUSED)
{
	Enesim_Renderer *r;

	r = enesim_renderer_map_quad_new();
	enesim_renderer_map_quad_vertex_set(r, 64, 64, 0);
	enesim_renderer_map_quad_vertex_set(r, 192, 64, 1);
	enesim_renderer_map_quad_vertex_set(r, 192, 192, 2);
	enesim_renderer_map_quad_vertex_set(r, 64, 256, 3);

	enesim_renderer_map_quad_vertex_color_set(r, 0xffff0000, 0);
	enesim_renderer_map_quad_vertex_color_set(r, 0xffffff00, 1);
	enesim_renderer_map_quad_vertex_color_set(r, 0xffffffff, 2);
	enesim_renderer_map_quad_vertex_color_set(r, 0xff000000, 2);
	
	return r;
}

