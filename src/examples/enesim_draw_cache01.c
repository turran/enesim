#include <Enesim.h>

int main(void)
{
	Enesim_Renderer *r;
	Enesim_Surface *s;
	Enesim_Draw_Cache *cache;
	Enesim_Buffer_Sw_Data sw_data_cache;
	Eina_Rectangle area, geom;
	uint8_t *dst;
	uint8_t *src;
	size_t stride;
	int i;

	enesim_init();
	r = enesim_renderer_circle_new();
	enesim_renderer_circle_x_set(r, 128);
	enesim_renderer_circle_y_set(r, 128);
	enesim_renderer_circle_radius_set(r, 64);
	enesim_renderer_shape_fill_color_set(r, 0xffff0000);
	enesim_renderer_shape_draw_mode_set(r, ENESIM_SHAPE_DRAW_MODE_FILL);

	cache = enesim_draw_cache_new();
	enesim_draw_cache_renderer_set(cache, r);

	/* try to map the area at 0, 0, 64, 64 */
	eina_rectangle_coords_from(&area, 0, 0, 64, 64);
	if (!enesim_draw_cache_map_sw(cache, &area, &sw_data_cache, ENESIM_FORMAT_ARGB8888, NULL))
		goto failed_mapping;

	/* try to map the area at 64, 64, 128, 128 */
	eina_rectangle_coords_from(&area, 64, 64, 128, 128);
	if (!enesim_draw_cache_map_sw(cache, &area, &sw_data_cache, ENESIM_FORMAT_ARGB8888, NULL))
		goto failed_mapping;

	/* now map again the first area, in theory we should not draw anymore */
	eina_rectangle_coords_from(&area, 0, 0, 64, 64);
	if (!enesim_draw_cache_map_sw(cache, &area, &sw_data_cache, ENESIM_FORMAT_ARGB8888, NULL))
		goto failed_mapping;
	src = sw_data_cache.argb8888.plane0;

	/* get the geometry */
	enesim_draw_cache_geometry_get(cache, &geom);
	printf("creating a surface of size %d %d\n", geom.w, geom.h);
	s = enesim_surface_new(ENESIM_FORMAT_ARGB8888,
			geom.w, geom.h);

	enesim_surface_data_get(s, (void **)&dst, &stride);
	for (i = 0; i < 128; i++)
	{
		memcpy(dst, src, stride);
		dst += stride;
		src += stride;
	}


	enesim_image_file_save("enesim_draw_cache01_0x0_64x64.png", s, NULL);
	enesim_surface_unref(s);
failed_mapping:
	enesim_draw_cache_free(cache);
	enesim_shutdown();

	return 0;
}

