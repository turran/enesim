#include "enesim_example_renderer.h"
#include <unistd.h>

#if BUILD_OPENCL

static Eina_Bool opencl_image_setup(
		Enesim_Example_Renderer_Options *options EINA_UNUSED)
{
	return EINA_TRUE;
}

static void opencl_image_cleanup(void)
{
}

static void opencl_image_run(Enesim_Renderer *r,
		Enesim_Example_Renderer_Options *options)
{
	Enesim_Pool *pool;
	Enesim_Buffer *b;
	Enesim_Surface *s;
	char *real_file;
	char *output_dir;
	int ret;

	pool = enesim_pool_opencl_new();
	printf("pool %p\n", pool);
	s = enesim_surface_new_pool_from(ENESIM_FORMAT_ARGB8888,
			options->width, options->height, pool);
	enesim_example_renderer_draw(r, s, options);

	b = enesim_surface_buffer_get(s);
	output_dir = getcwd(NULL, 0);
	ret = asprintf(&real_file, "%s/%s.png", output_dir, options->name);
	free(output_dir);
	if (ret && real_file)
	{
		enesim_image_file_save(real_file, b, NULL, NULL);
		free(real_file);
	}
	enesim_buffer_unref(b);
	enesim_surface_unref(s);
}

Enesim_Example_Renderer_Backend_Interface
enesim_example_renderer_backend_opencl_image = {
	/* .setup 	= */ opencl_image_setup,
	/* .cleanup 	= */ opencl_image_cleanup,
	/* .run 	= */ opencl_image_run,
};

#endif
