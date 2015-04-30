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

static Enesim_Pool *_current_default_pool = NULL;

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Pool * enesim_pool_new(Enesim_Pool_Descriptor *descriptor, void *data)
{
	Enesim_Pool *p;

	p = calloc(1, sizeof(Enesim_Pool));
	p->descriptor = descriptor;
	p->data = data;
	p->ref = 1;

	return p;
}

Eina_Bool enesim_pool_data_alloc(Enesim_Pool *p,
		Enesim_Backend *backend,
		void **data,
		Enesim_Buffer_Format fmt,
		uint32_t w, uint32_t h)
{
	if (!p) return EINA_FALSE;
	if (!p->descriptor) return EINA_FALSE;
	if (!p->descriptor->data_alloc) return EINA_FALSE;

	return p->descriptor->data_alloc(p->data, backend, data, fmt, w, h);
}

Eina_Bool enesim_pool_data_from(Enesim_Pool *p, Enesim_Backend *backend, void **data,
		Enesim_Buffer_Format fmt, uint32_t w, uint32_t h, Eina_Bool copy,
		Enesim_Buffer_Sw_Data *from)
{
	if (!p) return EINA_FALSE;
	if (!p->descriptor) return EINA_FALSE;
	if (!p->descriptor->data_from)
	{
		WRN("No data_from() implementation");
		return EINA_FALSE;
	}

	return p->descriptor->data_from(p->data, backend, data, fmt, w, h, copy, from);
}

Eina_Bool enesim_pool_data_get(Enesim_Pool *p, void *data,
		Enesim_Buffer_Format fmt,
		uint32_t w, uint32_t h,
		Enesim_Buffer_Sw_Data *dst)
{
	if (!p) return EINA_FALSE;
	if (!p->descriptor) return EINA_FALSE;
	if (!p->descriptor->data_get)
	{
		WRN("No data_get() implementation");
		return EINA_FALSE;
	}

	return p->descriptor->data_get(p->data, data, fmt, w, h, dst);
}

Eina_Bool enesim_pool_data_put(Enesim_Pool *p, void *data,
		Enesim_Buffer_Format fmt,
		uint32_t w, uint32_t h,
		Enesim_Buffer_Sw_Data *dst)
{
	if (!p) return EINA_FALSE;
	if (!p->descriptor) return EINA_FALSE;
	if (!p->descriptor->data_put)
	{
		WRN("No data_put() implementation");
		return EINA_FALSE;
	}

	return p->descriptor->data_put(p->data, data, fmt, w, h, dst);
}

void enesim_pool_data_free(Enesim_Pool *p, void *data,
		Enesim_Buffer_Format fmt,
		Eina_Bool external_allocated)
{
	if (!p) return;
	if (!p->descriptor) return;
	if (!p->descriptor->data_free) return;

	p->descriptor->data_free(p->data, data, fmt, external_allocated);
}

void enesim_pool_init(void)
{
	Enesim_Pool *thiz;

	/* init every particular pool */
	enesim_pool_sw_init();
	/* create the default pool */
	thiz = enesim_pool_sw_new();
	enesim_pool_default_set(thiz);
}

void enesim_pool_shutdown(void)
{
	/* destroy the default pool */
	enesim_pool_unref(_current_default_pool);
	/* shutdown every particular pool */
	enesim_pool_sw_shutdown();
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

/**
 * @brief Get the default memory pool
 * @return The default memory pool @ender_transfer{none}
 */
EAPI Enesim_Pool * enesim_pool_default_get(void)
{
	return enesim_pool_ref(_current_default_pool);
}

/**
 * @brief Set the default memory pool
 * @param[in] thiz The newly default memory pool @ender_transfer{full}
 */
EAPI void enesim_pool_default_set(Enesim_Pool *thiz)
{
	if (_current_default_pool)
		enesim_pool_unref(_current_default_pool);
	_current_default_pool = thiz;
}

/**
 * @brief Increase the reference counter of a pool
 * @param[in] thiz The pool
 * @return The input parameter @a thiz for programming convenience
 */
EAPI Enesim_Pool * enesim_pool_ref(Enesim_Pool *thiz)
{
	if (!thiz) return NULL;
	thiz->ref++;
	return thiz;
}

/**
 * @brief Decrease the reference counter of a pool
 * @param[in] thiz The pool
 */
EAPI void enesim_pool_unref(Enesim_Pool *thiz)
{
	if (!thiz) return;
	thiz->ref--;
	if (!thiz->ref)
	{
		if (thiz->descriptor && thiz->descriptor->free)
			thiz->descriptor->free(thiz->data);
		free(thiz);
	}
}

/**
 * @brief Get the type of a pool
 * @ender_downcast
 * @param[in] thiz The pool to get the type from
 * @param[out] lib The ender library associated with this pool
 * @param[out] name The ender item name of the renderer
 *
 * This function is needed for ender in order to downcast a pool
 */
EAPI Eina_Bool enesim_pool_type_get(Enesim_Pool *thiz, const char **lib,
		const char **name)
{
	if (!thiz)
		return EINA_FALSE;
	if (lib)
		*lib = "enesim";
	if (name)
		*name = thiz->descriptor->type_get();
	return EINA_TRUE;
}

