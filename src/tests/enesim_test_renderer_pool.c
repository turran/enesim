#include "Enesim.h"
#include "Enesim_OpenCL.h"

int main(int argc, char **argv)
{
	Enesim_Renderer *r;
	Enesim_Surface *s;
	Enesim_Pool *pool;

	enesim_init();
	r = enesim_renderer_background_new();
	enesim_renderer_background_color_set(r, 0xffff0000);

	pool = enesim_pool_opencl_new();
	if (!pool)
	{
		printf("Failed to create the pool\n");
		return 2;
	}

	s = enesim_surface_new_pool_from(ENESIM_FORMAT_ARGB8888, 320, 240, pool);
	
	enesim_renderer_draw(r, s, NULL, 0, 0);
	enesim_shutdown();

	return 0;
}

