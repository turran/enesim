#include "Enesim.h"

Eina_Bool _damages(Enesim_Renderer *r, Eina_Rectangle *damage, Eina_Bool past, void *data)
{
	printf("(%d) destination damage recevied %d %d %d %d\n", past, damage->x, damage->y, damage->w, damage->h);
	return EINA_TRUE;
}

int main(int argc, char **argv)
{
	Enesim_Renderer *r1, *r2, *r3;
	Enesim_Surface *s;

	enesim_init();

	r3 = enesim_renderer_rectangle_new();
	enesim_renderer_shape_fill_color_set(r3, 0xffffffff);
	enesim_renderer_rectangle_width_set(r3, 50.0);
	enesim_renderer_rectangle_height_set(r3, 50.0);

	r2 = enesim_renderer_rectangle_new();
	enesim_renderer_shape_fill_color_set(r2, 0xffffffff);
	enesim_renderer_origin_set(r2, 5.0, 7.0);
	enesim_renderer_rectangle_width_set(r2, 100.0);
	enesim_renderer_rectangle_height_set(r2, 100.0);

	r1 = enesim_renderer_compound_new();
	enesim_renderer_compound_layer_add(r1, r2);
	enesim_renderer_compound_layer_add(r1, r3);

	/* before drawing check the damages */
	printf("before drawing\n");
	printf("==============\n");
	enesim_renderer_damages_get(r1, _damages, NULL);
	s = enesim_surface_new(ENESIM_FORMAT_ARGB8888, 320, 240);
	enesim_renderer_draw(r1, s, NULL, 0, 0, NULL);
	/* check again, we should not have anything */
	printf("after drawing\n");
	printf("=============\n");
	enesim_renderer_damages_get(r1, _damages, NULL);

	printf("after writing\n");
	printf("=============\n");
	enesim_renderer_rectangle_width_set(r2, 120.0);
	enesim_renderer_damages_get(r1, _damages, NULL);

	enesim_shutdown();

	return 0;
}

