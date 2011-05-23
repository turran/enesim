/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2008 Jorge Luis Zapata
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
#include "Enesim.h"
#include "enesim_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static Eina_Bool _data_alloc(Enesim_Pool *p, Enesim_Buffer_Data *data,
		Enesim_Backend be, Enesim_Buffer_Format fmt,
		uint32_t w, uint32_t h)
{
	size_t bytes;
	void *alloc_data;

	if (be != ENESIM_BACKEND_SOFTWARE)
		return EINA_FALSE;

	bytes = enesim_buffer_format_size_get(fmt, w, h);
	alloc_data = calloc(bytes, sizeof(char));
	switch (fmt)
	{
		case ENESIM_CONVERTER_ARGB8888:
		data->argb8888.plane0 = alloc_data;
		data->argb8888.plane0_stride = w;
		break;

		case ENESIM_CONVERTER_ARGB8888_PRE:
		data->argb8888_pre.plane0 = alloc_data;
		data->argb8888_pre.plane0_stride = w;
		break;

		case ENESIM_CONVERTER_RGB565:
		data->rgb565.plane0 = alloc_data;
		data->rgb565.plane0_stride = w;
		break;

		case ENESIM_CONVERTER_RGB888:
		data->rgb565.plane0 = alloc_data;
		data->rgb565.plane0_stride = w;
		break;

		case ENESIM_CONVERTER_A8:
		case ENESIM_CONVERTER_GRAY:
		printf("TODO\n");
		break;
	}
	return EINA_TRUE;
}

static void _data_free(Enesim_Pool *p, Enesim_Buffer_Data *data,
		Enesim_Backend be, Enesim_Buffer_Format fmt)
{
	if (be != ENESIM_BACKEND_SOFTWARE)
		return;
	switch (fmt)
	{
		case ENESIM_CONVERTER_ARGB8888:
		free(data->argb8888.plane0);
		break;

		case ENESIM_CONVERTER_ARGB8888_PRE:
		free(data->argb8888_pre.plane0);
		break;

		case ENESIM_CONVERTER_RGB565:
		case ENESIM_CONVERTER_RGB888:
		case ENESIM_CONVERTER_A8:
		case ENESIM_CONVERTER_GRAY:
		printf("TODO\n");
		break;
	}
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/* The main enesim pool */
Enesim_Pool enesim_default_pool = {
	/* EINA_MAGIC =  */ 0,
	/* .data_alloc = */ _data_alloc,
	/* .data_free =  */ _data_free,
	/* .free =       */ NULL
};

Eina_Bool enesim_pool_data_alloc(Enesim_Pool *p, Enesim_Buffer_Data *data,
		Enesim_Backend be, Enesim_Buffer_Format fmt,
		uint32_t w, uint32_t h)
{
	if (p->data_alloc) return p->data_alloc(p, data, be, fmt, w, h);
	return EINA_FALSE;
}

void enesim_pool_data_free(Enesim_Pool *p, Enesim_Buffer_Data *data,
		Enesim_Backend be, Enesim_Buffer_Format fmt)
{
	if (p->data_free) p->data_free(p, data, be, fmt);
}

void enesim_pool_free(Enesim_Pool *p)
{
	if (p->free) p->free(p);
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
