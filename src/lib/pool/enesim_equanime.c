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

#if BUILD_EQUANIME
#include "Equanime.h"
#include "Enesim_Equanime.h"

typedef struct _Enesim_Equanime_Pool
{
	Enesim_Pool pool;
	Equanime *equ;
	Equ_Host *host;
} Enesim_Equanime_Pool;
#endif
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#if BUILD_EQUANIME
static Eina_Hash *_surfaces = NULL;

static Eina_Bool _data_alloc(Enesim_Pool *p, Enesim_Buffer_Data *data,
		Enesim_Backend be, Enesim_Buffer_Format f,
		uint32_t w, uint32_t h)
{
	Enesim_Equanime_Pool *pool = (Enesim_Equanime_Pool *)p;
	Equ_Surface_Data sdata;
	Equ_Format fmt;
	Equ_Surface *s;

	if (!enesim_pool_equanime_buffer_format(f, &fmt))
		return EINA_FALSE;
	s = equ_host_surface_get(pool->equ, pool->host, w, h, fmt, EQU_SURFACE_SYSTEM);
	equ_surface_data_get(s, &sdata);
	switch (fmt)
	{
		case EQU_FORMAT_ARGB8888:
		data->argb8888.plane0 = sdata.data.argb8888.plane0;
		break;

		case EQU_FORMAT_RGB888:
		data->rgb888.plane0 = sdata.data.rgb888.plane0;
		break;
	}
	if (!_surfaces) _surfaces = eina_hash_pointer_new(NULL);
	eina_hash_add(_surfaces, data, s);

	return EINA_TRUE;
}

static void _data_free(Enesim_Pool *p, Enesim_Buffer_Data *data,
		Enesim_Backend be, Enesim_Buffer_Format fmt)
{
	Enesim_Equanime_Pool *pool = (Enesim_Equanime_Pool *)p;
	Equ_Surface *s;

	/* TODO add a surface free on equanime */
	s = eina_hash_find(_surfaces, data);
}

static void _free(Enesim_Pool *p)
{
	Enesim_Equanime_Pool *pool = (Enesim_Equanime_Pool *)p;

	free(pool);
}
#endif
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Pool * enesim_pool_equanime_get(Equanime *eq, Equ_Host *h)
{
#if BUILD_EQUANIME
	Enesim_Equanime_Pool *pool;

	pool = calloc(1, sizeof(Enesim_Equanime_Pool));
	pool->equ = eq;
	pool->host = h;
	pool->pool.data_alloc = _data_alloc;
	pool->pool.data_free = _data_free;
	pool->pool.free = _free;

	return &pool->pool;
#else
	return NULL;
#endif
}

EAPI Eina_Bool enesim_pool_equanime_support(void)
{
#if BUILD_EQUANIME
	return EINA_TRUE;
#else
	return EINA_FALSE;
#endif
}

EAPI Eina_Bool enesim_pool_equanime_buffer_format(Enesim_Buffer_Format fmt, Equ_Format *equ_fmt)
{
#if BUILD_EQUANIME
	switch (fmt)
	{
		case ENESIM_CONVERTER_ARGB8888:
		*equ_fmt = EQU_FORMAT_ARGB8888;
		break;

		case ENESIM_CONVERTER_RGB888:
		*equ_fmt = EQU_FORMAT_RGB888;
		break;

		case ENESIM_CONVERTER_ARGB8888_PRE:
		case ENESIM_CONVERTER_RGB565:
		case ENESIM_CONVERTER_A8:
		case ENESIM_CONVERTER_GRAY:
		default:
		return EINA_FALSE;
	}
	return EINA_TRUE;
#else
	return EINA_FALSE;
#endif
}

#if 0
Enesim_Equanime_Buffer
{
	Enesim_Buffer *buffer;
	Equ_Surface *surface;
} Enesim_Equanime_Buffer;

Enesim_Equanime_Surface
{
	Enesim_Surface *surface;
	Equ_Surface *equ_surface;
}
#endif
