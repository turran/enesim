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
#if 0
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
#endif
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_converter_a8_init(void)
{
}
