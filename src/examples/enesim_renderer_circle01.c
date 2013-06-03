#include "Enesim.h"
#include "enesim_renderer_example.h"

static Enesim_Renderer * enesim_renderer_circle01(void)
{
	Enesim_Renderer *r;
	Enesim_Matrix m;

	r = enesim_renderer_circle_new();
	enesim_renderer_circle_center_set(r, 0.5, 0.5);
	enesim_renderer_circle_radius_set(r, 0.1);

	enesim_renderer_shape_fill_color_set(r, 0xffff0000);
	enesim_renderer_shape_draw_mode_set(r, ENESIM_SHAPE_DRAW_MODE_FILL);

	enesim_matrix_scale(&m, 128, 128);
	enesim_renderer_transformation_set(r, &m);

	return r;
}
EXAMPLE(enesim_renderer_circle01)

