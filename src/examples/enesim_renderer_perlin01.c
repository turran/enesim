#include <Eina_Extra.h>
#include "Enesim.h"
#include "enesim_renderer_example.h"

static Enesim_Renderer * enesim_renderer_perlin01(void)
{
	Enesim_Renderer *r;

	r = enesim_renderer_perlin_new();
	enesim_renderer_color_set(r, 0xffff0000);
	enesim_renderer_perlin_octaves_set(r, 6);
	enesim_renderer_perlin_persistence_set(r, 1);
	enesim_renderer_perlin_xfrequency_set(r, 0.05);
	enesim_renderer_perlin_yfrequency_set(r, 0.05);

	return r;
}
EXAMPLE(enesim_renderer_perlin01)

