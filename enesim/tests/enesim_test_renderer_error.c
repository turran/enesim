#include "Enesim.h"

int main(int argc, char **argv)
{
	Enesim_Renderer *r;
	Enesim_Surface *s;
	Enesim_Error *error = NULL;

	enesim_init();
	r = enesim_renderer_rectangle_new();
	enesim_renderer_shape_fill_color_set(r, 0xffffffff);
	/* we dont set any property to force the error */

	s = enesim_surface_new(ENESIM_FORMAT_ARGB8888, 320, 240);

	if (!enesim_renderer_draw(r, s, NULL, 0, 0, &error))
	{
		enesim_error_dump(error);
		enesim_error_delete(error);
	}

	enesim_shutdown();

	return 0;
}

