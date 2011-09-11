#include "Enesim.h"

int main(int argc, char **argv)
{
	Enesim_Renderer *r;
	Enesim_Surface *s;

	enesim_init();
	r = enesim_renderer_rectangle_new();
	enesim_renderer_shape_fill_color_set(r, 0xffffffff);
	enesim_renderer_rectangle_width_set(r, 100);
	enesim_renderer_rectangle_height_set(r, 100);

	s = enesim_surface_new(ENESIM_FORMAT_ARGB8888, 320, 240);

	enesim_renderer_draw(r, s, NULL, 0, 0, NULL);
	enesim_shutdown();

	return 0;
}
