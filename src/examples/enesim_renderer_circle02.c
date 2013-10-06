#include "enesim_renderer_example.h"

/**
 * @example enesim_renderer_circle02.c
 * Example usage of a circle renderer
 * @image html enesim_renderer_circle02.png
 */
static Enesim_Renderer * enesim_renderer_circle02(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer *shape_background;
	Enesim_Renderer *shape_compound;
	Enesim_Renderer *shape_circle;
	Enesim_Renderer *highlight_background;
	Enesim_Renderer *highlight_shape;
	Enesim_Renderer *highlight_clip;
	Enesim_Renderer_Compound_Layer *l;
	Enesim_Matrix t;
	double radius;
	double sa, ca;

	/* default values */
	radius = 128;

	/* a two-color stripe pattern */
	/* we'll use it as a simple gradient background via a transform */
	r = enesim_renderer_stripes_new();
	enesim_renderer_stripes_even_color_set(r, 0xff0077ff);
	enesim_renderer_stripes_odd_color_set(r, 0xff00ffff);
	enesim_renderer_stripes_even_thickness_set(r, 1);
	enesim_renderer_stripes_odd_thickness_set(r, 1);
	shape_background = r;

	/* another two-color stripe pattern */
	/* this will be our highlight gradient */
	r = enesim_renderer_stripes_new();
	enesim_renderer_stripes_even_color_set(r, 0);
	enesim_renderer_stripes_odd_color_set(r, 0xffffffff);
	enesim_renderer_stripes_even_thickness_set(r, 1);
	enesim_renderer_stripes_odd_thickness_set(r, 1);
	highlight_background = r;

	/* a circle we'll fill with the highlight */
	r = enesim_renderer_circle_new();
	enesim_renderer_circle_center_set(r, 0, 0);
	enesim_renderer_shape_fill_renderer_set(r, highlight_background);
	enesim_renderer_shape_draw_mode_set(r, ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL);
	enesim_renderer_circle_radius_set(r, 1.2 * radius);
	highlight_shape = r;

	/* a circle we'll fill with above highlight shape */
	/* equivalent to clipping the above highlight shape by it */
	/* this highlight clip is what gives our real highlight shape */
	r = enesim_renderer_circle_new();
	enesim_renderer_shape_fill_renderer_set(r, highlight_shape);
	enesim_renderer_shape_draw_mode_set(r, ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL);
	enesim_renderer_circle_center_set(r, radius, radius);
	enesim_renderer_circle_radius_set(r, (0.925 * radius) - 2);
	highlight_clip = r;

	/* a composite paint consisting of two layers: */
	/* the base layer is the background gradient, */
	/* and on top of that is the highlight real shape */
	r = enesim_renderer_compound_new();

	l = enesim_renderer_compound_layer_new();
	enesim_renderer_compound_layer_renderer_set(l, shape_background);
	enesim_renderer_compound_layer_rop_set(l, ENESIM_ROP_FILL);
	enesim_renderer_compound_layer_add(r, l);

	l = enesim_renderer_compound_layer_new();
	enesim_renderer_compound_layer_renderer_set(l, highlight_clip);
	enesim_renderer_compound_layer_rop_set(l, ENESIM_ROP_BLEND);
	enesim_renderer_compound_layer_add(r, l);

	shape_compound = r;

	/* a circle we'll fill with the composition above */
	/* this is the object we'll draw. */
	r = enesim_renderer_circle_new();

	enesim_renderer_shape_stroke_weight_set(r, 1);
	enesim_renderer_shape_stroke_color_set(r, 0xff777777);
	enesim_renderer_shape_fill_renderer_set(r, shape_compound);
	enesim_renderer_shape_draw_mode_set(r, ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE_FILL);
	enesim_renderer_circle_center_set(r, radius, radius);
	enesim_renderer_circle_radius_set(r, radius);
	shape_circle = r;

	r = shape_background;
	sa = sin(-30 * M_PI / 180.0);
	ca = cos(-30 * M_PI / 180.0);
	t.xx = ca;               t.xy = sa;               t.xz = 0;
	t.yx = -sa / (2 * radius);    t.yy = ca / (2 * radius);     t.yz = 0;
	t.zx = 0;                t.zy = 0;                t.zz = 1;
	enesim_matrix_inverse(&t, &t);
	enesim_renderer_transformation_set(r, &t);

	r = highlight_background;
	sa = sin(120 * M_PI / 180.0);
	ca = cos(120 * M_PI / 180.0);
	t.xx = ca;                t.xy = sa;               t.xz = 0;
	t.yx = -sa / (1.2 * radius);   t.yy = ca / (1.2 * radius);   t.yz = 0;
	t.zx = 0;                 t.zy = 0;                t.zz = 1;
	enesim_matrix_inverse(&t, &t);
	enesim_renderer_transformation_set(r, &t);

	return shape_circle;
}
EXAMPLE(enesim_renderer_circle02)
