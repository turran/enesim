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

typedef struct _Enesim_Eina_Pool
{
	Enesim_Pool pool;
	Eina_Mempool *mp;
} Enesim_Eina_Pool;
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static void * _surface_new(Enesim_Pool *p,
		Enesim_Backend be, Enesim_Format f,
		int w, int h)
{
	Enesim_Eina_Pool *pool = (Enesim_Eina_Pool *)p;
	void *data;
	size_t bytes;

	if (be != ENESIM_BACKEND_SOFTWARE)
		return NULL;

	bytes = enesim_format_bytes_calc(f, w, h);
	data = eina_mempool_malloc(pool->mp, bytes);

	return data;
}

static void _surface_free(Enesim_Pool *p,
		void *data)
{
	Enesim_Eina_Pool *pool = (Enesim_Eina_Pool *)p;

	eina_mempool_free(pool->mp, data);
}

static void _free(Enesim_Pool *p)
{
	Enesim_Eina_Pool *pool = (Enesim_Eina_Pool *)p;

	eina_mempool_del(pool->mp);
	free(pool);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Pool * enesim_pool_eina_get(Eina_Mempool *mp)
{
	Enesim_Eina_Pool *pool;

	pool = calloc(1, sizeof(Enesim_Eina_Pool));
	pool->mp = mp;

	return &pool->pool;
}
