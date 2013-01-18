/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2011 Jorge Luis Zapata
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
static void _2d_rgb565_none_argb8888(Enesim_Buffer_Sw_Data *data, uint32_t dw, uint32_t dh,
		void *sdata, uint32_t sw, uint32_t sh,
		size_t spitch)
{
	uint16_t *dst = data->rgb565.plane0;
	uint32_t *src = sdata;
	size_t dpitch = data->rgb565.plane0_stride;

	while (dh--)
	{
		uint16_t *ddst = dst;
		uint32_t *ssrc = src;
		uint32_t ddw = dw;
		while (ddw--)
		{
			*dst = ((*src & 0xf80000) >> 8) | ((*src & 0xfc00) >> 5)
					| ((*src & 0xf8) >> 3);
			ssrc++;
			ddst++;
		}
		dst += dpitch;
		src += spitch;
	}
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_converter_rgb565_init(void)
{
	/* TODO check if the cpu is the host */
	enesim_converter_surface_register(
			ENESIM_CONVERTER_2D(_2d_rgb565_none_argb8888),
			ENESIM_BUFFER_FORMAT_RGB565,
			ENESIM_ANGLE_0,
			ENESIM_FORMAT_ARGB8888);
}

