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
	enesim_renderer_map_quad_vertex_position_set(r, 0, 64, 64);
	enesim_renderer_map_quad_vertex_position_set(r, 1, 192, 64);
	enesim_renderer_map_quad_vertex_position_set(r, 2, 192, 192);
	enesim_renderer_map_quad_vertex_position_set(r, 3, 64, 256);

	enesim_renderer_map_quad_vertex_color_set(r, 0, 0xffff0000);
	enesim_renderer_map_quad_vertex_color_set(r, 1, 0xffffff00);
	enesim_renderer_map_quad_vertex_color_set(r, 2, 0xffffffff);
	enesim_renderer_map_quad_vertex_color_set(r, 3, 0xff000000);
	
	return r;
}

