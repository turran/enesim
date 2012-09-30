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
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Mempool_Aligned
{
	size_t alignment;
} Enesim_Mempool_Aligned;
static int _init = 0;

/* TODO parse the options */
static void * _aligned_init(const char *context, const char *options, va_list args)
{
	Enesim_Mempool_Aligned *thiz;

	thiz = calloc(1, sizeof(Enesim_Mempool_Aligned));
	/* TODO try to get the cache line size automatically */
	thiz->alignment = 64;
	return thiz;
}

static void _aligned_free(void *data, void *element)
{
	free(element);
}

static void * _aligned_alloc(void *data, unsigned int size)
{
	Enesim_Mempool_Aligned *thiz = data;
	int ret;
	void *element = NULL;

	/* FIXME use the correct version in windows */
	ret = posix_memalign(&element, thiz->alignment, size);
	if (ret)
		return NULL;
	return element;
}

static void _aligned_shutdown(void *data)
{
	Enesim_Mempool_Aligned *thiz = data;
	free(thiz);
}

static Eina_Mempool_Backend _mempool_aligned = {
	/* .name 		= */ "aligned",
	/* .init 		= */ _aligned_init,
	/* .free 		= */ _aligned_free,
	/* .alloc		= */ _aligned_alloc,
	/* .realloc		= */ NULL,
	/* .garbage_collect 	= */ NULL,
	/* .statistics		= */ NULL,
	/* .shutdown		= */ _aligned_shutdown,
	/* .repack		= */ NULL,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
int enesim_mempool_aligned_init(void)
{
	if (!_init++)
	{
		eina_mempool_register(&_mempool_aligned);
	}
	return _init;
}

int enesim_mempool_aligned_shutdown(void)
{
	if (--_init != 0)
		return _init;
	eina_mempool_unregister(&_mempool_aligned);
	return _init;
}

Eina_Mempool * enesim_mempool_aligned_get(void)
{
	static Eina_Mempool *m = NULL;

	if (!m)
	{
		m = eina_mempool_add("aligned", NULL, NULL);
	}
	return m;
}
