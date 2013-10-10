#ifndef _EXAMPLE_H
#define _EXAMPLE_H

#define _GNU_SOURCE 1
#include <stdio.h>

#include "Enesim.h"

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
		enesim_init();							\
		r = name();							\
		run(r, #name ".png", argc, argv);				\
		enesim_shutdown();						\
		return 0;							\
	}

static void help(const char *name)
{
	printf("Usage: %s [(blend|fill) output dir]\n", name);
}

static void run(Enesim_Renderer *r, const char *file, int argc, char **argv)
{
	Enesim_Renderer *c;
	Enesim_Surface *s;
	Enesim_Buffer *b;
	Enesim_Log *error = NULL;
	Enesim_Rop rop = ENESIM_ROP_FILL;
	Eina_Rectangle bounds;
	const char *output_dir = "./";
	const char *exec_name;
	int ret;
	char *real_file;

	if (!r)
	{
		printf("No such renderer, nothing to do");
		return;
	}

	exec_name = argv[0];

	/* handle the parameters */
	if (argc > 1)
	{
		if (argc < 3)
		{
			help(exec_name);
			return;
		}
		/* the rop */
		if (!strcmp(argv[1], "fill"))
			rop = ENESIM_ROP_FILL;
		else if (!strcmp(argv[1], "blend"))
			rop = ENESIM_ROP_BLEND;
		else
		{
			help(exec_name);
			return;
		}
		/* the output dir */
		output_dir = argv[2];
	}

	enesim_renderer_destination_bounds_get(r, &bounds, 0, 0);
	printf("bounds %" EINA_RECTANGLE_FORMAT "\n", 	
			EINA_RECTANGLE_ARGS(&bounds));
	s = enesim_surface_new(ENESIM_FORMAT_ARGB8888,
			256, 256);
	c = enesim_renderer_checker_new();
	enesim_renderer_checker_odd_color_set(c, 0xffcccccc);
	enesim_renderer_checker_even_color_set(c, 0xffaaaaaa);
	enesim_renderer_checker_width_set(c, 10);
	enesim_renderer_checker_height_set(c, 10);
	/* first draw the background */	
	if (!enesim_renderer_draw(c, s, ENESIM_ROP_FILL, NULL, 0, 0,
			&error))
	{
		enesim_log_dump(error);
	}
	/* now the real renderer */
	if (!enesim_renderer_draw(r, s, rop, NULL, 0, 0,
			&error))
	{
		enesim_log_dump(error);
	}
	b = enesim_surface_buffer_get(s);
	ret = asprintf(&real_file, "%s/%s", output_dir, file);
	if (ret && real_file)
	{
		enesim_image_file_save(real_file, b, NULL);
		free(real_file);
	}

	enesim_buffer_unref(b);
	enesim_surface_unref(s);
	enesim_renderer_unref(r);
	enesim_renderer_unref(c);
}

#endif
