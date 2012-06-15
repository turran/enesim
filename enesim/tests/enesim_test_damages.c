#include "Enesim.h"

typedef struct _Enesim_Test_Damage
{
	Enesim_Renderer *layer1;
	Enesim_Renderer *layer2;
	Enesim_Renderer *fill_r;
	Enesim_Renderer *r;
	Enesim_Surface *s;
} Enesim_Test_Damage;

static Eina_Bool _damages(Enesim_Renderer *r, Eina_Rectangle *damage, Eina_Bool past, void *data)
{
	printf("(%d) destination damage recevied %d %d %d %d\n", past, damage->x, damage->y, damage->w, damage->h);
	return EINA_TRUE;
}

static void test1(Enesim_Test_Damage *thiz)
{
	printf("Test 1: Current state\n");
	/* before drawing check the damages */
	enesim_renderer_damages_get(thiz->r, _damages, NULL);
	enesim_renderer_draw(thiz->r, thiz->s, NULL, 0, 0, NULL);
	/* check again, we should not have anything */
	enesim_renderer_damages_get(thiz->r, _damages, NULL);
}

/* only modify the fill renderer of the shape */
static void test2(Enesim_Test_Damage *thiz)
{
	printf("Test 2: Changing the fill renderer only\n");
	enesim_renderer_color_set(thiz->fill_r, 0xffff0000);
	enesim_renderer_damages_get(thiz->r, _damages, NULL);
	enesim_renderer_draw(thiz->r, thiz->s, NULL, 0, 0, NULL);
}

/* modify the renderer itself */
static void test3(Enesim_Test_Damage *thiz)
{
	printf("Test 3: Changing the current renderer only\n");
	enesim_renderer_rectangle_width_set(thiz->r, 120.0);
	enesim_renderer_damages_get(thiz->r, _damages, NULL);
	enesim_renderer_draw(thiz->r, thiz->s, NULL, 0, 0, NULL);
}

/* modify a layer */
static void test4(Enesim_Test_Damage *thiz)
{
	printf("Test 4: Changing a layer only\n");
	enesim_renderer_shape_fill_color_set(thiz->layer1, 0xffff00ff);
	enesim_renderer_damages_get(thiz->r, _damages, NULL);
	enesim_renderer_draw(thiz->r, thiz->s, NULL, 0, 0, NULL);
}

/* modify the renderer and the fill renderer */
static void test5(Enesim_Test_Damage *thiz)
{
	printf("Test 5: Changing the current renderer and the layer\n");
	enesim_renderer_rectangle_width_set(thiz->r, 150.0);
	enesim_renderer_damages_get(thiz->r, _damages, NULL);
	enesim_renderer_draw(thiz->r, thiz->s, NULL, 0, 0, NULL);
}

int main(int argc, char **argv)
{
	Enesim_Test_Damage thiz;
	Enesim_Renderer *r;
	Enesim_Surface *s;

	enesim_init();

	/* we draw a rectangle with two rectangles inside but are being drawn
	 * through a compound which is set as the fill renderer on the rectangle
	 */
	r = enesim_renderer_rectangle_new();
	enesim_renderer_shape_fill_color_set(r, 0xffff0000);
	enesim_renderer_rectangle_x_set(r, 0);
	enesim_renderer_rectangle_y_set(r, 0);
	enesim_renderer_rectangle_width_set(r, 50.0);
	enesim_renderer_rectangle_height_set(r, 50.0);
	thiz.layer1 = r;

	r = enesim_renderer_rectangle_new();
	enesim_renderer_shape_fill_color_set(r, 0xff0000ff);
	enesim_renderer_origin_set(r, 100, 0);
	enesim_renderer_rectangle_width_set(r, 50.0);
	enesim_renderer_rectangle_height_set(r, 50.0);
	thiz.layer2 = r;

	r = enesim_renderer_compound_new();
	enesim_renderer_compound_layer_add(r, thiz.layer1);
	enesim_renderer_compound_layer_add(r, thiz.layer2);
	thiz.fill_r = r;

	r = enesim_renderer_rectangle_new();
	enesim_renderer_shape_fill_renderer_set(r, thiz.fill_r);
	enesim_renderer_shape_draw_mode_set(r, ENESIM_SHAPE_DRAW_MODE_FILL);
	enesim_renderer_rectangle_x_set(r, 0);
	enesim_renderer_rectangle_y_set(r, 0);
	enesim_renderer_rectangle_width_set(r, 150.0);
	enesim_renderer_rectangle_height_set(r, 150.0);
	thiz.r = r;

	thiz.s = enesim_surface_new(ENESIM_FORMAT_ARGB8888, 320, 240);

	test1(&thiz);
	test2(&thiz);
	test3(&thiz);
	test4(&thiz);
	test5(&thiz);

	enesim_shutdown();

	return 0;
}

