#include "enesim_example_renderer.h"

/**
 * @example enesim_renderer_text_span03.c
 * Example usage of a text span renderer
 * @image html enesim_renderer_text_span03.png
 */

typedef struct _Point_At_Data
{
	Enesim_Renderer *c;
	Enesim_Text_Font *f;
	Enesim_Renderer *r;
	double adv;
	int i;
	double d;
} Point_At_Data;

static char *str = "My first text to path example";

static double _point_at(const Enesim_Figure *thiz EINA_UNUSED, double x, double y,
		double n, void *data)
{
	Point_At_Data *at = data;
	Enesim_Renderer *r;
	Enesim_Renderer_Compound_Layer *l;
	char *s;

	r = enesim_renderer_text_span_new();
	enesim_renderer_shape_draw_mode_set(r, ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL);
	enesim_renderer_shape_fill_color_set(r, 0xff000000);

	s = strndup(str + at->i, 1);
	printf("inside %s %d %g %g\n", s, at->i, x, y);
	enesim_renderer_text_span_text_set(r, s);
	free(s);

	enesim_renderer_text_span_font_set(r, enesim_text_font_ref(at->f));
	enesim_renderer_text_span_position_set(r, x, y);

	l = enesim_renderer_compound_layer_new();
	enesim_renderer_compound_layer_renderer_set(l, r);
	enesim_renderer_compound_layer_add(at->c, l);

	if (strlen(str) <= (unsigned int)at->i)
	{
		return -1;
	}
	else
	{
		/* advance in x */
		at->i++;
		at->d += 10;
		return at->d;
	}
}

Enesim_Renderer * enesim_example_renderer_renderer_get(Enesim_Example_Renderer_Options *options EINA_UNUSED)
{
	Enesim_Renderer *r1, *r2;
	Enesim_Renderer_Compound_Layer *l;
	Enesim_Text_Font *f;
	Enesim_Text_Engine *e;
	Enesim_Figure *fig;
	Enesim_Path *p;
	Point_At_Data at;

	/* Create an arial font */
	e = enesim_text_engine_default_get();
	if (!e) return NULL;

	f = enesim_text_font_new_description_from(
			e, "arial", 20);
	enesim_text_engine_unref(e);
	if (!f) return NULL;

	/* Create the compound renderer */
	r1 = enesim_renderer_compound_new();
	/* Create the path for the text-to-path */
	r2 = enesim_renderer_path_new();
	p = enesim_path_new();
	enesim_path_move_to(p, 10, 100);
	enesim_path_scubic_to(p, 42, 50, 74, 100);
	enesim_path_line_to(p, 192, 50);
	enesim_path_line_to(p, 210, 128);
	/* get the figure to calculate the glyph positions */
	fig = enesim_path_flatten(p);
	enesim_renderer_path_inner_path_set(r2, p);
	enesim_renderer_shape_draw_mode_set(r2, ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE);
	enesim_renderer_shape_stroke_color_set(r2, 0xff00ff00);
	enesim_renderer_shape_stroke_weight_set(r2, 3);
	l = enesim_renderer_compound_layer_new();
	enesim_renderer_compound_layer_renderer_set(l, r2);
	enesim_renderer_compound_layer_add(r1, l);

	at.c = r1;
	at.i = 0;
	at.d = 0;
	at.f = f;
	enesim_figure_point_at(fig, 0, _point_at, &at);
	enesim_text_font_unref(f);
	enesim_figure_unref(fig);

	return r1;
}
