#include "Enesim.h"
#include "enesim_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static void _2d_rgb888_none_argb8888(Enesim_Buffer_Sw_Data *data, uint32_t dw, uint32_t dh,
		uint32_t *src, uint32_t sw, uint32_t sh,
		uint32_t spitch)
{
	uint8_t *dst = data->rgb888.plane0;
	size_t dpitch = data->rgb888.plane0_stride;

	while (dh--)
	{
		uint8_t *ddst = dst;
		uint32_t *ssrc = src;
		uint32_t ddw = dw;
		while (ddw--)
		{
			*ddst++ = (*ssrc >> 16) & 0xff;
			*ddst++ = (*ssrc >> 8) & 0xff;
			*ddst++ = *ssrc & 0xff;
			//printf("%02x%02x%02x\n", *(ddst - 3), *(ddst - 2), *(ddst - 1));
			ssrc++;
		}
		dst += dpitch;
		src += spitch;
	}
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_converter_rgb888_init(void)
{
	/* TODO check if the cpu is the host */
	enesim_converter_surface_register(
			ENESIM_CONVERTER_2D(_2d_rgb888_none_argb8888),
			ENESIM_CONVERTER_RGB888,
			ENESIM_ANGLE_0,
			ENESIM_FORMAT_ARGB8888);
}

