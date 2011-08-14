#include "Enesim.h"
#include "Enesim_OpenCL.h"

int main(int argc, char **argv)
{
	Enesim_Pool *pool;
	Enesim_Surface *surface;

	enesim_init();

	pool = enesim_pool_opencl_new();
	if (!pool)
	{
		printf("Failed to create the pool\n");
		return 2;
	}

	surface = enesim_surface_new_pool_from(ENESIM_FORMAT_ARGB8888, 320, 240, pool);
	if (!surface)
	{
		printf("Failed to create the surface\n");
		return 3;
	}
	enesim_surface_unref(surface);
	enesim_pool_delete(pool);

	enesim_shutdown();

	return 0;
}

