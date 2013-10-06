#ifndef _EXAMPLE_H
#define _EXAMPLE_H

/**
 * @example enesim_renderer_example.h
 * Example usage of a renderer shared among every enesim_renderer_*.c
 * example file
 */

#define EXAMPLE(name)								\
	static Enesim_Renderer * name(void);					\
	int main(int argc, char **argv)						\
	{									\
		Enesim_Renderer *r;						\
		r = name();							\
		run(r, #name ".png", argc, argv);				\
		return 0;							\
	}

static void run(Enesim_Renderer *r, const char *file, int argc EINA_UNUSED, char **argv EINA_UNUSED)
{
	Enesim_Renderer *c;
	Enesim_Surface *s;
	Enesim_Buffer *b;
	Enesim_Log *error = NULL;
	Eina_Rectangle bounds;

	enesim_init();

	/* TODO get the output dir */
	enesim_renderer_destination_bounds_get(r, &bounds, 0, 0);
	printf("bounds %" EINA_RECTANGLE_FORMAT "\n", 	
			EINA_RECTANGLE_ARGS(&bounds));
	s = enesim_surface_new(ENESIM_FORMAT_ARGB8888,
			256, 256);
	c = enesim_renderer_checker_new();
	enesim_renderer_checker_odd_color_set(c, 0xff444444);
	enesim_renderer_checker_even_color_set(c, 0xff222222);
	enesim_renderer_checker_width_set(c, 20);
	enesim_renderer_checker_height_set(c, 20);
	/* first draw the background */	
	if (!enesim_renderer_draw(c, s, ENESIM_ROP_FILL, NULL, 0, 0,
			&error))
	{
		enesim_log_dump(error);
	}
	/* now the real renderer */
	if (!enesim_renderer_draw(r, s, ENESIM_ROP_FILL, NULL, 0, 0,
			&error))
	{
		enesim_log_dump(error);
	}
	b = enesim_surface_buffer_get(s);
	enesim_image_file_save(file, b, NULL);
	enesim_buffer_unref(b);
	enesim_surface_unref(s);
	enesim_renderer_unref(r);
	enesim_renderer_unref(c);

	enesim_shutdown();
}

#endif

