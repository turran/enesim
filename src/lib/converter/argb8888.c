#include "Enesim.h"
#include "enesim_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static void _2d_argb8888_none_argb8888(Enesim_Buffer_Sw_Data *data, uint32_t dw, uint32_t dh,
		void *sdata, uint32_t sw, uint32_t sh,
		size_t spitch)
{
	uint8_t *src = sdata;
	uint8_t *dst = (uint8_t *)data->argb8888.plane0;
	size_t dpitch = data->argb8888.plane0_stride;

	while (dh--)
	{
		uint32_t *ddst = (uint32_t *)dst;
		uint32_t *ssrc = (uint32_t *)src;
		uint32_t ddw = dw;
		while (ddw--)
		{
			uint8_t pa;

			pa = (*ssrc >> 24);
			if ((pa > 0) && (pa < 255))
			{
				*ddst = (pa << 24)|
					(((argb8888_red_get(*ssrc) * 255) / pa) << 16) |
					(((argb8888_green_get(*ssrc) * 255) / pa) << 8) |
					((argb8888_blue_get(*ssrc) * 255) / pa);
			}
			else
			{
				*ddst = *ssrc;
			}
			ssrc++;
			ddst++;
		}
		dst += dpitch;
		src += spitch;
	}
}
static void _1d_argb8888_none_argb8888(Enesim_Buffer_Sw_Data *data, uint32_t len, void *sdata)
{
	uint32_t *dst = data->argb8888.plane0;
	uint32_t *src = sdata;

	while (len--)
	{
		uint8_t pa;

		pa = (*src >> 24);
		if ((pa > 0) && (pa < 255))
		{
			*dst = (pa << 24)|
				(((argb8888_red_get(*src) * 255) / pa) << 16) |
				(((argb8888_green_get(*src) * 255) / pa) << 8) |
				((argb8888_blue_get(*src) * 255) / pa);
		}
		else
		{
			*dst = *src;
		}
		dst++;
		src++;
	}
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_converter_argb8888_init(void)
{
	enesim_converter_surface_register(
			ENESIM_CONVERTER_2D(_2d_argb8888_none_argb8888),
			ENESIM_CONVERTER_ARGB8888,
			ENESIM_ANGLE_0,
			ENESIM_FORMAT_ARGB8888);
	enesim_converter_span_register(
			ENESIM_CONVERTER_1D(_1d_argb8888_none_argb8888),
			ENESIM_CONVERTER_ARGB8888,
			ENESIM_ANGLE_0,
			ENESIM_FORMAT_ARGB8888);
}
