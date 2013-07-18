#include <Eina_Extra.h>
#include "Enesim.h"
#include "enesim_renderer_example.h"

static Enesim_Renderer * enesim_renderer_blur01(void)
{
	Enesim_Renderer *r1, *r2;
	Enesim_Matrix m;

	r1 = enesim_renderer_circle_new();
	enesim_renderer_circle_center_set(r1, 128, 128);
	enesim_renderer_circle_radius_set(r1, 64);
	enesim_renderer_shape_fill_color_set(r1, 0xffff0000);
	enesim_renderer_shape_draw_mode_set(r1, ENESIM_SHAPE_DRAW_MODE_FILL);

	r2 = enesim_renderer_blur_new();
	enesim_renderer_blur_renderer_set(r2, r1);
	enesim_renderer_blur_radius_x_set(r2, 4.5);
	enesim_renderer_blur_radius_y_set(r2, 4.5);

	return r2;
}
EXAMPLE(enesim_renderer_blur01)

