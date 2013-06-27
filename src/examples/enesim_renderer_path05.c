#include <Eina_Extra.h>
#include "Enesim.h"
#include "enesim_renderer_example.h"

static Enesim_Renderer * enesim_renderer_path05(void)
{
	Enesim_Renderer *r;

	r = enesim_renderer_path_new();
	enesim_renderer_path_move_to(r, 20, 20);
	enesim_renderer_path_line_to(r, 220, 20);
	enesim_renderer_path_line_to(r, 220, 40);
	enesim_renderer_path_line_to(r, 20, 40);
	enesim_renderer_path_line_to(r, 20, 20);
	enesim_renderer_shape_fill_color_set(r, 0xffff0000);
	enesim_renderer_shape_draw_mode_set(r, ENESIM_SHAPE_DRAW_MODE_FILL);

	return r;
}

EXAMPLE(enesim_renderer_path05)

