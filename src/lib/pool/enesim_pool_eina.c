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

static void _data_free_cb(void *data, void *user_data)
{
	Enesim_Eina_Pool *thiz = user_data;
	eina_mempool_free(thiz->mp, data);
}
/*----------------------------------------------------------------------------*
 *                        The Enesim's pool interface                         *
 *----------------------------------------------------------------------------*/
static Eina_Bool _data_alloc(void *prv,
		Enesim_Backend *backend,
		void **backend_data,
		Enesim_Buffer_Format fmt, uint32_t w, uint32_t h)
{
	Enesim_Buffer_Sw_Data *data;
	Enesim_Eina_Pool *thiz = prv;
	size_t bytes;
	int stride;
	void *alloc_data;

	data = malloc(sizeof(Enesim_Buffer_Sw_Data));
	*backend = ENESIM_BACKEND_SOFTWARE;
	*backend_data = data;

	bytes = enesim_buffer_format_size_get(fmt, w, h);
	stride = enesim_buffer_format_size_get(fmt, w, 1);
	alloc_data = eina_mempool_malloc(thiz->mp, bytes);

	enesim_buffer_sw_data_set(data, fmt, alloc_data, stride);
	return EINA_TRUE;
}

static Eina_Bool _data_sub(void *prv EINA_UNUSED,
		Enesim_Backend backend EINA_UNUSED,
		void **backend_data,
		void *original_data,
		Enesim_Buffer_Format original_fmt,
		const Eina_Rectangle *area)
{
	Enesim_Buffer_Sw_Data *orig_data = original_data;
	Enesim_Buffer_Sw_Data *data;

	data = malloc(sizeof(Enesim_Buffer_Sw_Data));
	*backend_data = data;

	enesim_buffer_sw_data_sub(orig_data, data, original_fmt, area);
	return EINA_TRUE;
}

static void _data_free(void *prv,
		void *backend_data,
		Enesim_Buffer_Format fmt,
		Eina_Bool external_allocated EINA_UNUSED)
{
	Enesim_Eina_Pool *thiz = prv;
	Enesim_Buffer_Sw_Data *data = backend_data;

	if (!external_allocated)
		enesim_buffer_sw_data_free(data, fmt, _data_free_cb, thiz);
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
	/* .data_sub =   */ _data_sub,
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
