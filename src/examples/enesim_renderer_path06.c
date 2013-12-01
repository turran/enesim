#include "enesim_example_renderer.h"

#if 0
Cubic tests[] = {
	/* sin full wave */
	{
		0.1, 0.5,
		0.1, 1.0,
		0.9, 0.0,
		0.9, 0.5,
	},
	/* sine half wave */
	{
		0.2, 0.1,
		0.1, 0.9,
		0.6, 0.9,
		0.9, 0.1,
	},
	{
		0.1, 0.5,
		0.1, 1.0,
		0.9, 0.1,
		0.5, 0.1,
	},
	/* loop */
	{
		0.1, 0.52,
		0.8, 0.0,
		0.3, 1.0,
		0.1, 0.48,
	},
	/* strictly linear */
	{
		0.1, 0.1,
		0.3, 0.3,
		0.7, 0.7,
		0.9, 0.9,
	},
	/* almost linear */
	{
		0.1, 0.1,
		0.3, 0.3 + 0.00001,
		0.7, 0.7 - 0.00001,
		0.9, 0.9,
	},
	/* circle */
	{
		1.0, 0.0,
		1.0, 0.552,
		0.552, 1.0,
		0.0, 1.0,
	},
	/* ellipse */
	{
		1.0, 0.0 * 0.5,
		1.0, 0.552 * 0.5,
		0.552, 1.0 * 0.5,
		0.0, 1.0 * 0.5,
	},
	{
		0.1, 0.1,
		0.1, 0.9,
		0.5, 0.9,
		0.9, 0.8,
	},
	/* symmetrical parabola */
	{
		0, 0,
		0, 1,
		1, 1,
		1, 0,
	},

	/* near singularity */
	{
		0.3, 0.3,
		0.5, 1.1, // 0.596425
		1.0, 0.9,
		1.4, 0.3,
	},
	/* singularity */
	{
		0.3, 0.3,
		0.6, 1.1, // 0.596425
		1.0, 0.9,
		1.4, 0.3,
	},
	/* singularity */
	{
		0.3, 0.3,
		0.596452, 1.1, // 0.596425
		1.0, 0.9,
		1.4, 0.3,
	}
};
#endif

/**
 * @example enesim_renderer_path06.c
 * Example usage of a path renderer
 * @image html enesim_renderer_path06.png
 */
Enesim_Renderer * enesim_example_renderer_renderer_get(
		Enesim_Example_Renderer_Options *options)
{
	Enesim_Renderer *r;
	Enesim_Matrix m;
	Enesim_Path *p;

	r = enesim_renderer_path_new();
	enesim_renderer_shape_stroke_color_set(r, 0xff000000);
	enesim_renderer_shape_stroke_weight_set(r, 3);
	enesim_renderer_shape_draw_mode_set(r, ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE);

	p = enesim_renderer_path_path_get(r);
	enesim_path_move_to(p, 0.1, 0.1);
	enesim_path_cubic_to(p, 0.1, 0.9, 0.5, 0.9, 0.9, 0.8);
	enesim_path_unref(p);

	enesim_matrix_scale(&m, options->width, options->height);
	enesim_renderer_transformation_set(r, &m);

	return r;
}
