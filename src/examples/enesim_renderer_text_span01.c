#include "enesim_renderer_example.h"

/**
 * @example enesim_renderer_text_span01.c
 * Example usage of a text span renderer
 * @image html enesim_renderer_text_span01.png
 */
static Enesim_Renderer * enesim_renderer_text_span01(void)
{
	Enesim_Renderer *r;
	Enesim_Text_Font *f;
	Enesim_Text_Engine *e;
	Enesim_Rectangle geom;

	/* Create an arial font */
	e = enesim_text_engine_default_get();
	if (!e) return NULL;

	f = enesim_text_font_new_description_from(
			e, "arial", 16);
	enesim_text_engine_unref(e);

	if (!f) return NULL;

	r = enesim_renderer_text_span_new();
	/* Draw black */
	enesim_renderer_color_set(r, 0xff000000);
	enesim_renderer_text_span_text_set(r, "Hello World!");
	enesim_renderer_text_span_font_set(r, f);
	/* Center the text. For that we use the destination geometry
	 * We could use the enesim_renderer_shape_geometry_get instead
	 * given that we dont use any transformation matrix
	 */
	enesim_renderer_shape_destination_geometry_get(r, &geom);
	enesim_renderer_origin_set(r, (256 - geom.w) / 2, (256 - geom.h) / 2);

	return r;
}
EXAMPLE(enesim_renderer_text_span01)

