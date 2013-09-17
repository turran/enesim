#ifndef _EXAMPLE_H
#define _EXAMPLE_H

/* TODO handle options for the raster operation to use, etc */
#define EXAMPLE(name)								\
	static Enesim_Renderer * name(void);					\
	int main(void)								\
	{									\
		Enesim_Renderer *r, *c;						\
		Enesim_Surface *s;						\
		Enesim_Buffer *b;						\
		Enesim_Log *error = NULL;					\
		Eina_Rectangle bounds;						\
										\
		enesim_init();							\
										\
		r = name();							\
		enesim_renderer_destination_bounds_get(r, &bounds, 0, 0);	\
		printf("bounds %" EINA_RECTANGLE_FORMAT "\n", 		\
				EINA_RECTANGLE_ARGS(&bounds)); 		\
		s = enesim_surface_new(ENESIM_FORMAT_ARGB8888,			\
				256, 256);					\
		c = enesim_renderer_checker_new();				\
		enesim_renderer_checker_odd_color_set(c, 0xff444444);		\
		enesim_renderer_checker_even_color_set(c, 0xff222222);		\
		enesim_renderer_checker_width_set(c, 20);			\
		enesim_renderer_checker_height_set(c, 20);			\
		/* first draw the background */					\
		if (!enesim_renderer_draw(c, s, ENESIM_ROP_FILL, NULL, 0, 0, 	\
				&error))					\
		{								\
			enesim_log_dump(error);					\
		}								\
		/* now the real renderer */					\
		if (!enesim_renderer_draw(r, s, ENESIM_ROP_FILL, NULL, 0, 0, 	\
				&error))					\
		{								\
			enesim_log_dump(error);					\
		}								\
		b = enesim_surface_buffer_get(s);				\
		enesim_image_file_save(#name ".png", b, NULL);			\
		enesim_buffer_unref(b);						\
		enesim_surface_unref(s);					\
		enesim_renderer_unref(r);					\
		enesim_renderer_unref(c);					\
										\
		enesim_shutdown();						\
		return 0;							\
	}

#endif
