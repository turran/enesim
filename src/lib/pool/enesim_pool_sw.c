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
/** @cond internal */
#define ENESIM_LOG_DEFAULT enesim_log_pool

static Enesim_Pool *_sw_pool = NULL;

static void _data_free_cb(void *data, void *user_data EINA_UNUSED)
{
	free(data);
}

/*----------------------------------------------------------------------------*
 *                        The Enesim's pool interface                         *
 *----------------------------------------------------------------------------*/
static const char * _type_get(void)
{
	return "enesim.pool";
}

static Eina_Bool _data_alloc(void *prv EINA_UNUSED,
		Enesim_Backend *backend,
		void **backend_data,
		Enesim_Buffer_Format fmt, uint32_t w, uint32_t h)
{
	Enesim_Buffer_Sw_Data *data;
	Eina_Bool ret;

	data = malloc(sizeof(Enesim_Buffer_Sw_Data));
	*backend = ENESIM_BACKEND_SOFTWARE;
	*backend_data = data;
	ret = enesim_buffer_sw_data_alloc(data, fmt, w, h);
	if (!ret)
	{
		free(data);
	}
	return ret;
}

static Eina_Bool _data_from(void *prv EINA_UNUSED,
		Enesim_Backend *backend,
		void **backend_data,
		Enesim_Buffer_Format fmt EINA_UNUSED,
		uint32_t w EINA_UNUSED, uint32_t h EINA_UNUSED,
		Eina_Bool copy,
		Enesim_Buffer_Sw_Data *src)
{
	if (copy)
	{
		ERR("Can't copy data TODO");
		return EINA_FALSE;
	}
	else
	{
		Enesim_Buffer_Sw_Data *data;

		*backend = ENESIM_BACKEND_SOFTWARE;
		data = malloc(sizeof(Enesim_Buffer_Sw_Data));
		*backend_data = data;
		*data = *src;

		return EINA_TRUE;
	}
}

static void _data_free(void *prv EINA_UNUSED, void *backend_data,
		Enesim_Buffer_Format fmt,
		Eina_Bool external_allocated)
{
	Enesim_Buffer_Sw_Data *data = backend_data;
	if (!external_allocated)
		enesim_buffer_sw_data_free(data, fmt, _data_free_cb, NULL);
	free(data);
}

static Eina_Bool _data_get(void *prv EINA_UNUSED, void *backend_data,
		Enesim_Buffer_Format fmt EINA_UNUSED,
		uint32_t w EINA_UNUSED, uint32_t h EINA_UNUSED,
		Enesim_Buffer_Sw_Data *dst)
{
	Enesim_Buffer_Sw_Data *data = backend_data;
	*dst = *data;

	return EINA_TRUE;
}

static Enesim_Pool_Descriptor _sw_descriptor = {
	/* .type_get =   */ _type_get,
	/* .data_alloc = */ _data_alloc,
	/* .data_free =  */ _data_free,
	/* .data_from =  */ _data_from,
	/* .data_get =   */ _data_get,
	/* .data_put =   */ NULL,
	/* .free =       */ NULL
};

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/

void enesim_pool_sw_init(void)
{
	_sw_pool = enesim_pool_new(&_sw_descriptor, NULL);
}

void enesim_pool_sw_shutdown(void)
{
	/* destroy the default pool */
	enesim_pool_unref(_sw_pool);
}

/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Create a generic Sw based pool
 * @return The newly allocated Sw pool
 */
EAPI Enesim_Pool * enesim_pool_sw_new(void)
{
	return enesim_pool_ref(_sw_pool);
}

