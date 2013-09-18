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
	enesim_renderer_shape_draw_mode_set(circle, ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL);

	proxy = enesim_renderer_proxy_new();
	enesim_renderer_proxy_proxied_set(proxy, circle);

	return proxy;
}

static Enesim_Renderer * enesim_renderer_hints01(void)
{
	Enesim_Renderer *compound;
	Enesim_Renderer *background;
	Enesim_Renderer_Compound_Layer *lbackground;
	Enesim_Color colors[] = { 0xffff0000, 0xff00ff00, 0xff0000ff };
	unsigned int i;
	double x = 60;
	double y = 60;

	compound = enesim_renderer_compound_new();

	background = enesim_renderer_background_new();
	enesim_renderer_background_color_set(background, 0xff888888);

	lbackground = enesim_renderer_compound_layer_new();
	enesim_renderer_compound_layer_renderer_set(lbackground, background);
	enesim_renderer_compound_layer_rop_set(lbackground, ENESIM_ROP_FILL);
	enesim_renderer_compound_layer_add(compound, lbackground);

	for (i = 0; i < sizeof(colors)/sizeof(Enesim_Color); i++)
	{
		Enesim_Renderer *shape;
		Enesim_Renderer_Compound_Layer *l;

		shape = _create_circle(x, y, colors[i]);
		l = enesim_renderer_compound_layer_new();
		enesim_renderer_compound_layer_renderer_set(l, shape);
		enesim_renderer_compound_layer_rop_set(l, ENESIM_ROP_BLEND);
		enesim_renderer_compound_layer_add(compound, l);
		x += 60;
	}
	return compound;
}

EXAMPLE(enesim_renderer_hints01)
