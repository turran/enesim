#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <stdio.h>

#include "Enesim.h"

int end = 0;

static void help(void)
{
	printf("enesim_image_example [load | save] [async | sync ] FILE\n");
}

static void async_load_cb(Enesim_Buffer *b EINA_UNUSED, void *data, int error)
{
	char *file = data;

	if (!error)
		printf("Image %s loaded async successfully\n", file);
	else
		printf("Image %s loaded async with error: %s\n", file, eina_error_msg_get(error));
	end = 1;
}

static void async_save_cb(Enesim_Buffer *b, void *data, int error)
{
	char *file = data;

	if (!error)
		printf("Image %s saved async successfully\n", file);
	else
		printf("Image %s saved async with error: %s\n", file, eina_error_msg_get(error));
	enesim_buffer_unref(b);
	end = 1;
}

int main(int argc, char **argv)
{
	char *file;
	Enesim_Buffer *b = NULL;
	int async = 0;
	int save = 0;

	if (argc < 4)
	{
		help();
		return -1;
	}
	if (!strcmp(argv[1], "save"))
		save = 1;
	else if (!strcmp(argv[1], "load"))
		save = 0;
	else
	{
		help();
		return -2;
	}

	if (!strcmp(argv[2], "sync"))
		async = 0;
	else if (!strcmp(argv[2], "async"))
		async = 1;
	else
	{
		help();
		return -3;
	}
	file = argv[3];

	enesim_init();
	if (!async)
		end = 1;

	if (save)
	{
		Enesim_Renderer *r;
		Enesim_Surface *s;

		/* generate a simple pattern with enesim */
		s = enesim_surface_new(ENESIM_FORMAT_ARGB8888, 256, 256);
		r = enesim_renderer_checker_new();
		enesim_renderer_checker_even_color_set(r, 0xffffff00);
		enesim_renderer_checker_odd_color_set(r, 0xff000000);
		enesim_renderer_checker_width_set(r, 20);
		enesim_renderer_checker_height_set(r, 20);
		enesim_renderer_draw(r, s, ENESIM_ROP_FILL, NULL, 0, 0, NULL);
		enesim_renderer_unref(r);

		b = enesim_surface_buffer_get(s);
		if (async)
		{
			enesim_image_file_save_async(file, b, async_save_cb, file, NULL);
		}
		else
		{
			if (enesim_image_file_save(file, b, NULL, NULL))
				printf("Image %s saved sync successfully\n", file);
			else
			{
				Eina_Error err;

				err = eina_error_get();
				printf("Image %s saved sync with error: %s\n", file, eina_error_msg_get(err));
			}
			enesim_buffer_unref(b);
		}

	}
	else
	{
		if (async)
		{
			enesim_image_file_load_async(file, b, NULL, async_load_cb, file, NULL);
		}
		else
		{
			if (enesim_image_file_load(file, &b, NULL, NULL, NULL))
				printf("Image %s loaded sync successfully\n", file);
			else
			{
				Eina_Error err;

				err = eina_error_get();
				printf("Image %s loaded sync with error: %s\n", file, eina_error_msg_get(err));
			}
		}
	}
	while (!end)
	{
		enesim_image_dispatch();
	}
	enesim_shutdown();
	return 0;
}

