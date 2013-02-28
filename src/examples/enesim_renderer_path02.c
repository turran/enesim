#include "Enesim.h"
#include "enesim_renderer_example.h"

static Enesim_Renderer * enesim_renderer_path02(void)
{
	Enesim_Renderer *r1;
	Enesim_Renderer *r2;
	Enesim_Renderer *c;
	Eina_Rectangle bounds;
	Enesim_Matrix m;

	c = enesim_renderer_compound_new();
	r1 = enesim_renderer_path_new();
	enesim_renderer_path_move_to(r1, 10, 10);
	enesim_renderer_path_line_to(r1, 100, 50);
	enesim_renderer_path_line_to(r1, 60, 100);
	enesim_renderer_path_line_to(r1, 30, 45);
	enesim_renderer_shape_stroke_weight_set(r1, 18);
	enesim_renderer_shape_stroke_color_set(r1, 0xffffff00);
	enesim_renderer_shape_fill_color_set(r1, 0xffff0000);
	enesim_renderer_shape_draw_mode_set(r1, ENESIM_SHAPE_DRAW_MODE_STROKE_FILL);
	enesim_renderer_shape_stroke_join_set(r1, ENESIM_JOIN_ROUND);
	enesim_renderer_shape_stroke_cap_set(r1, ENESIM_CAP_ROUND);
	enesim_renderer_rop_set(r1, ENESIM_BLEND);

	enesim_matrix_identity(&m);
	enesim_matrix_scale(&m, 0.5, 0.5);
	enesim_renderer_transformation_set(r1, &m);
	enesim_renderer_destination_bounds(r1, &bounds, 0, 0);

	r2 = enesim_renderer_rectangle_new();
	enesim_renderer_shape_fill_color_set(r2, 0xff000000);
	enesim_renderer_rectangle_x_set(r2, bounds.x);
	enesim_renderer_rectangle_y_set(r2, bounds.y);
	enesim_renderer_rectangle_size_set(r2, bounds.w, bounds.h);
	enesim_renderer_rop_set(r2, ENESIM_FILL);
	printf("%d %d %d %d\n", bounds.x, bounds.y, bounds.w, bounds.h);

	enesim_renderer_compound_layer_add(c, r2);
	enesim_renderer_compound_layer_add(c, r1);

	return c;
}

EXAMPLE(enesim_renderer_path02)
