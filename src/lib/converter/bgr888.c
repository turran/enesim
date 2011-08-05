#include "Enesim.h"
#include "enesim_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static void _2d_bgr888_none_argb8888(Enesim_Buffer_Sw_Data *data, uint32_t dw, uint32_t dh,
		uint32_t dpitch, uint32_t *src, uint32_t sw, uint32_t sh,
		uint32_t spitch)
{
	uint8_t *dst = data->bgr888.plane0;
	dpitch *= 3;
	while (dh--)
	{
		uint8_t *ddst = dst;
		uint32_t *ssrc = src;
		uint32_t ddw = dw;
		while (ddw--)
		{
			*ddst++ = *ssrc & 0xff;
			*ddst++ = (*ssrc >> 8) & 0xff;
			*ddst++ = (*ssrc >> 16) & 0xff;
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
void enesim_converter_bgr888_init(void)
{
	enesim_converter_surface_register(
			ENESIM_CONVERTER_2D(_2d_bgr888_none_argb8888),
			ENESIM_CONVERTER_BGR888,
			ENESIM_ANGLE_0,
			ENESIM_FORMAT_ARGB8888);
}


