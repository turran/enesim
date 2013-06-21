#include "Enesim.h"
#include "enesim_renderer_example.h"

static Enesim_Renderer * _create_circle(double x, double y,
		Enesim_Color color)
{
	Enesim_Renderer *proxy;
	Enesim_Renderer *circle;

	circle = enesim_renderer_circle_new();
	enesim_renderer_circle_center_set(circle, x, y);
	enesim_renderer_circle_radius_set(circle, 60);
	enesim_renderer_shape_fill_color_set(circle, color);
	enesim_renderer_shape_draw_mode_set(circle, ENESIM_SHAPE_DRAW_MODE_FILL);
	enesim_renderer_rop_set(circle, ENESIM_BLEND);

	proxy = enesim_renderer_proxy_new();
	enesim_renderer_proxy_proxied_set(proxy, circle);
	enesim_renderer_rop_set(proxy, ENESIM_BLEND);

	return proxy;
}

static Enesim_Renderer * enesim_renderer_hints01(void)
{
	Enesim_Renderer *compound;
	Enesim_Renderer *background;
	Enesim_Color colors[] = { 0xffff0000, 0xff00ff00, 0xff0000ff };
	unsigned int i;
	double x = 60;
	double y = 60;

	background = enesim_renderer_background_new();
	enesim_renderer_background_color_set(background, 0xff888888);
	enesim_renderer_rop_set(background, ENESIM_FILL);

	compound = enesim_renderer_compound_new();
	enesim_renderer_compound_layer_add(compound, background);

	for (i = 0; i < sizeof(colors)/sizeof(Enesim_Color); i++)
	{
		Enesim_Renderer *shape;

		shape = _create_circle(x, y, colors[i]);
		enesim_renderer_compound_layer_add(compound, shape);
		x += 60;
	}
	return compound;
}

EXAMPLE(enesim_renderer_hints01)
