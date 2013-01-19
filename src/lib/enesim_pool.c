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

#include "enesim_pool_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_pool
/*----------------------------------------------------------------------------*
 *                        The Enesim's pool interface                         *
 *----------------------------------------------------------------------------*/
static Eina_Bool _data_alloc(void *prv EINA_UNUSED,
		Enesim_Backend *backend,
		void **backend_data,
		Enesim_Buffer_Format fmt, uint32_t w, uint32_t h)
{
	Enesim_Buffer_Sw_Data *data;
	size_t bytes;
	void *alloc_data;

	*backend = ENESIM_BACKEND_SOFTWARE;
	data = malloc(sizeof(Enesim_Buffer_Sw_Data));
	*backend_data = data;
	bytes = enesim_buffer_format_size_get(fmt, w, h);
	alloc_data = calloc(bytes, sizeof(char));
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

		case ENESIM_BUFFER_FORMAT_BGR888:
		data->bgr888.plane0 = alloc_data;
		data->bgr888.plane0_stride = w * 3;
		break;

		case ENESIM_BUFFER_FORMAT_RGB888:
		data->rgb888.plane0 = alloc_data;
		data->rgb888.plane0_stride = w * 3;
		break;

		case ENESIM_BUFFER_FORMAT_A8:
		data->a8.plane0 = alloc_data;
		data->a8.plane0_stride = w;
		break;

		case ENESIM_BUFFER_FORMAT_CMYK:
		case ENESIM_BUFFER_FORMAT_CMYK_ADOBE:
		data->cmyk.plane0 = alloc_data;
		data->cmyk.plane0_stride = w * 4;
		break;

		case ENESIM_BUFFER_FORMAT_GRAY:
		default:
		ERR("Unsupported format %d", fmt);
		break;
	}
	return EINA_TRUE;
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
	if (external_allocated) goto end;
	switch (fmt)
	{
		case ENESIM_BUFFER_FORMAT_ARGB8888:
		free(data->argb8888.plane0);
		break;

		case ENESIM_BUFFER_FORMAT_ARGB8888_PRE:
		free(data->argb8888_pre.plane0);
		break;

		case ENESIM_BUFFER_FORMAT_RGB565:
		free(data->rgb565.plane0);
		break;

		case ENESIM_BUFFER_FORMAT_RGB888:
		free(data->rgb888.plane0);
		break;

		case ENESIM_BUFFER_FORMAT_BGR888:
		free(data->bgr888.plane0);
		break;

		case ENESIM_BUFFER_FORMAT_A8:
		free(data->a8.plane0);
		break;

		case ENESIM_BUFFER_FORMAT_GRAY:
		default:
		ERR("Unsupported format %d", fmt);
		break;
	}
end:
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

static Enesim_Pool_Descriptor _default_descriptor = {
	/* .data_alloc = */ _data_alloc,
	/* .data_free =  */ _data_free,
	/* .data_from =  */ _data_from,
	/* .data_get =   */ _data_get,
	/* .free =       */ NULL
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Pool * enesim_pool_new(Enesim_Pool_Descriptor *descriptor, void *data)
{
	Enesim_Pool *p;

	p = calloc(1, sizeof(Enesim_Pool));
	p->descriptor = descriptor;
	p->data = data;

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

void enesim_pool_data_free(Enesim_Pool *p, void *data,
		Enesim_Buffer_Format fmt,
		Eina_Bool external_allocated)
{
	if (!p) return;
	if (!p->descriptor) return;
	if (!p->descriptor->data_free) return;

	p->descriptor->data_free(p->data, data, fmt, external_allocated);
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Pool * enesim_pool_default_get(void)
{
	static Enesim_Pool *p = NULL;

	if (!p)
	{
		/* FIXME for later */
#if 0
		Eina_Mempool *m;
		m = enesim_mempool_aligned_get();
		p = enesim_pool_eina_new(m);
#else
		p = enesim_pool_new(&_default_descriptor, NULL);
#endif
	}
	return p;
}

EAPI void enesim_pool_delete(Enesim_Pool *p)
{
	if (!p) return;
	if (!p->descriptor) return;
	if (!p->descriptor->free) return;

	p->descriptor->free(p->data);
	free(p);
}
