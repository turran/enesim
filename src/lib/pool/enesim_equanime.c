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
#include "Equanime.h"

typedef struct _Enesim_Equanime_Pool
{
	Enesim_Pool pool;
	Equanime *equ;
	Equ_Host *host;
} Enesim_Equanime_Pool;
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static void * _surface_new(Enesim_Pool *p,
		Enesim_Backend be, Enesim_Format f,
		int w, int h)
{
	Enesim_Equanime_Pool *pool = (Enesim_Eina_Pool *)p;
	Equ_Format fmt;
	Equ_Surface *s;

	s = equ_host_surface_get(pool->equ, pool->host, w, h, fmt, EQU_SURFACE_SHARED);

	return s;
}

static void _surface_free(Enesim_Pool *p,
		void *data)
{
	Enesim_Equanime_Pool *pool = (Enesim_Eina_Pool *)p;

}

static void _free(Enesim_Pool *p)
{
	Enesim_Equanime_Pool *pool = (Enesim_Equanime_Pool *)p;

	free(pool);
}

EAPI Enesim_Pool * enesim_pool_equanime_get(Equanime *eq, Equ_Host *h)
{
	Enesim_Equanime_Pool *pool;

	pool = calloc(1, sizeof(Enesim_Equanime_Pool));
	pool->equ = eq;
	pool->host = h;

	return &pool->pool;
}
