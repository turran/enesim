#include "Enesim.h"

int main(int argc, char **argv)
{
	Enesim_Pool *pool;
	Eina_Mempool *mp;
	Enesim_Surface *surface;

	enesim_init();

	mp = eina_mempool_add("chained_mempool", NULL, NULL);
	if (!mp)
	{
		printf("Impossible to create the mempool\n");
		return 1;
	}

	pool = enesim_pool_eina_new(mp);
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
	enesim_surface_delete(surface);
	enesim_pool_delete(pool);

	enesim_shutdown();

	return 0;
}
