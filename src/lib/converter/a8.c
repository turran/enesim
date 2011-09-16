#include "Enesim.h"
#include "enesim_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static void _1d_a8_none_argb8888(Enesim_Buffer_Sw_Data *data,
		uint32_t len, void *sdata)
{
	uint8_t *dst = data->a8.plane0;
	uint8_t *src = sdata;

	while (len--)
	{
		*dst = *src >> 24;

		dst++;
		src++;
	}
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_converter_a8_init(void)
{
	enesim_converter_span_register(
			ENESIM_CONVERTER_1D(_1d_a8_none_argb8888),
			ENESIM_CONVERTER_A8,
			ENESIM_ANGLE_0,
			ENESIM_FORMAT_ARGB8888);
}
