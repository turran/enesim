/* ENESIM - Drawing Library
 * Copyright (C) 2007-2013 Jorge Luis Zapata
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library.
 * If not, see <http://www.gnu.org/licenses/>.
 */
#include "enesim_private.h"

#include "enesim_main.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"

#include "enesim_converter_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static void _2d_bgr888_none_argb8888_pre(Enesim_Buffer_Sw_Data *data, uint32_t dw, uint32_t dh,
		Enesim_Buffer_Sw_Data *sdata, uint32_t sw EINA_UNUSED, uint32_t sh EINA_UNUSED)
{
	uint8_t *dst = data->bgr888.plane0;
	uint8_t *src = (uint8_t *)sdata->argb8888_pre.plane0;
	size_t dstride = data->bgr888.plane0_stride;
	size_t sstride = data->argb8888_pre.plane0_stride;

	while (dh--)
	{
		uint8_t *ddst = dst;
		uint32_t *ssrc = (uint32_t *)src;
		uint32_t ddw = dw;
		while (ddw--)
		{
			*ddst++ = *ssrc & 0xff;
			*ddst++ = (*ssrc >> 8) & 0xff;
			*ddst++ = (*ssrc >> 16) & 0xff;
			//printf("%02x%02x%02x\n", *(ddst - 3), *(ddst - 2), *(ddst - 1));
			ssrc++;
		}
		dst += dstride;
		src += sstride;
	}
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_converter_bgr888_init(void)
{
	enesim_converter_surface_register(
			ENESIM_CONVERTER_2D(_2d_bgr888_none_argb8888_pre),
			ENESIM_BUFFER_FORMAT_BGR888,
			ENESIM_ANGLE_0,
			ENESIM_BUFFER_FORMAT_ARGB8888_PRE);
}
