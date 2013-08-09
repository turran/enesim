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

#include "enesim_pool_private.h"
#include "enesim_buffer_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_pool

typedef struct _Enesim_Eina_Pool
{
	Eina_Mempool *mp;
} Enesim_Eina_Pool;

static Eina_Bool _data_alloc(void *prv,
		Enesim_Backend *backend,
		void **backend_data,
		Enesim_Buffer_Format fmt, uint32_t w, uint32_t h)
{
	Enesim_Buffer_Sw_Data *data;
	Enesim_Eina_Pool *thiz = prv;
	void *alloc_data;
	size_t bytes;

	data = malloc(sizeof(Enesim_Buffer_Sw_Data));
	*backend = ENESIM_BACKEND_SOFTWARE;
	*backend_data = data;
	bytes = enesim_buffer_format_size_get(fmt, w, h);
	alloc_data = eina_mempool_malloc(thiz->mp, bytes);
	switch (fmt)
	{
		case ENESIM_BUFFER_FORMAT_ARGB8888:
		data->argb8888.plane0 = alloc_data;
		data->argb8888.plane0_stride = w * 4;
		break;

		case ENESIM_BUFFER_FORMAT_ARGB8888_PRE:
		data->argb8888_pre.plane0 = alloc_data;
		data->argb8888_pre.plane0_stride = w * 4;
		break;

		case ENESIM_BUFFER_FORMAT_RGB565:
		data->rgb565.plane0 = alloc_data;
		data->rgb565.plane0_stride = w * 2;
		break;

		case ENESIM_BUFFER_FORMAT_RGB888:
		data->rgb888.plane0 = alloc_data;
		data->rgb888.plane0_stride = w * 3;
		break;

		case ENESIM_BUFFER_FORMAT_A8:
		case ENESIM_BUFFER_FORMAT_GRAY:
		default:
		ERR("Unsupported format %d", fmt);
		break;
	}
	return EINA_TRUE;
}

static void _data_free(void *prv,
		void *backend_data,
		Enesim_Buffer_Format fmt,
		Eina_Bool external_allocated EINA_UNUSED)
{
	Enesim_Eina_Pool *thiz = prv;
	Enesim_Buffer_Sw_Data *data = backend_data;

	switch (fmt)
	{
		case ENESIM_BUFFER_FORMAT_ARGB8888:
		eina_mempool_free(thiz->mp, data->argb8888_pre.plane0);
		break;

		case ENESIM_BUFFER_FORMAT_ARGB8888_PRE:
		eina_mempool_free(thiz->mp, data->argb8888_pre.plane0);
		break;

		case ENESIM_BUFFER_FORMAT_RGB565:
		case ENESIM_BUFFER_FORMAT_RGB888:
		case ENESIM_BUFFER_FORMAT_A8:
		case ENESIM_BUFFER_FORMAT_GRAY:
		default:
		printf("TODO\n");
		break;
	}
	free(data);
}

static Eina_Bool _data_get(void *prv EINA_UNUSED,
		void *backend_data,
		Enesim_Buffer_Format fmt EINA_UNUSED,
		uint32_t w EINA_UNUSED, uint32_t h EINA_UNUSED,
		Enesim_Buffer_Sw_Data *dst)
{
	Enesim_Buffer_Sw_Data *data = backend_data;

	*dst = *data;

	return EINA_TRUE;
}

static void _free(void *prv)
{
	Enesim_Eina_Pool *thiz = prv;

	eina_mempool_del(thiz->mp);
	free(thiz);
}

static Enesim_Pool_Descriptor _descriptor = {
	/* .data_alloc = */ _data_alloc,
	/* .data_free =  */ _data_free,
	/* .data_from =  */ NULL,
	/* .data_get =   */ _data_get,
	/* .free =       */ _free,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Pool * enesim_pool_eina_new(Eina_Mempool *mp)
{
	Enesim_Eina_Pool *thiz;
	Enesim_Pool *p;

	thiz = calloc(1, sizeof(Enesim_Eina_Pool));
	thiz->mp = mp;

	p = enesim_pool_new(&_descriptor, thiz);
	if (!p)
	{
		free(thiz);
		return NULL;
	}

	return p;
}
