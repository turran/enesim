#include "enesim_example_renderer.h"

/**
 * @example enesim_renderer_text_span02.c
 * Example usage of a text span renderer
 * @image html enesim_renderer_text_span02.png
 */
Enesim_Renderer * enesim_example_renderer_renderer_get(Enesim_Example_Renderer_Options *options EINA_UNUSED)
{
	Enesim_Renderer *r;
	Enesim_Text_Font *f;
	Enesim_Text_Engine *e;
	Enesim_Rectangle geom;

	/* Create an arial font */
	e = enesim_text_engine_default_get();
	if (!e) return NULL;

	f = enesim_text_font_new_description_from(
			e, "arial", 120);
	enesim_text_engine_unref(e);

	if (!f) return NULL;

	r = enesim_renderer_text_span_new();
	enesim_renderer_shape_draw_mode_set(r, ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE_FILL);
	enesim_renderer_shape_fill_color_set(r, 0xffff0000);
	enesim_renderer_shape_stroke_color_set(r, 0xffffcccc);
	enesim_renderer_shape_stroke_weight_set(r, 3);
	enesim_renderer_shape_stroke_join_set(r, ENESIM_RENDERER_SHAPE_STROKE_JOIN_ROUND);
	enesim_renderer_text_span_text_set(r, "Ají");
	enesim_renderer_text_span_font_set(r, f);
	/* Center the text. For that we use the destination geometry
	 * We could use the enesim_renderer_shape_geometry_get instead
	 * given that we dont use any transformation matrix
	 */
	enesim_renderer_shape_destination_geometry_get(r, &geom);
	enesim_renderer_origin_set(r, (256 - geom.w) / 2, (256 - geom.h) / 2);

	return r;
}

