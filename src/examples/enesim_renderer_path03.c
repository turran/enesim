#include "Enesim.h"
#include "enesim_renderer_example.h"

static Enesim_Renderer * enesim_renderer_path03(void)
{
	Enesim_Renderer *r;
	Enesim_Path *p;

	p = enesim_path_new();
	enesim_path_move_to(p, 50, 50);
	enesim_path_line_to(p, 150, 150);
	enesim_path_line_to(p, 50, 150);
	enesim_path_line_to(p, 150, 50);
	enesim_path_line_to(p, 50, 50);

	r = enesim_renderer_path_new();
	enesim_renderer_path_path_set(r, p);
	enesim_renderer_shape_stroke_weight_set(r, 18);
	enesim_renderer_shape_stroke_color_set(r, 0xffffff00);
	enesim_renderer_shape_fill_color_set(r, 0xffff0000);
	enesim_renderer_shape_draw_mode_set(r, ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE_FILL);
	return r;
}

EXAMPLE(enesim_renderer_path03)

