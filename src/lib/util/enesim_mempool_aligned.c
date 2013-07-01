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

#ifdef _WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <windows.h>
# undef WIN32_LEAN_AND_MEAN
#elif (defined (__MACH__) && defined (__APPLE__)) || defined (__FreeBSD__)
# include <sys/sysctl.h>
#elif defined (__linux__)
# include <stdio.h>
# include <unistd.h>
#endif

#if !defined(_WIN32) && !defined(HAVE_POSIX_MEMALIGN)
#include <errno.h>
#endif

#include "enesim_mempool_aligned_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Mempool_Aligned
{
	size_t alignment;
} Enesim_Mempool_Aligned;

static int _init = 0;

/* TODO parse the options */
static void * _enesim_mempool_aligned_init(const char *context EINA_UNUSED, const char *options EINA_UNUSED, va_list args EINA_UNUSED)
{
	Enesim_Mempool_Aligned *thiz;

	thiz = calloc(1, sizeof(Enesim_Mempool_Aligned));
	/* TODO try to get the cache line size automatically */
	/* Remains Solaris and BSD* */
	/* TODO: Merge in Eina later */
#ifdef _WIN32
	{
		DWORD buffer_size = 0;
		DWORD i;
		SYSTEM_LOGICAL_PROCESSOR_INFORMATION *buffer = NULL;

		if (!GetLogicalProcessorInformation(NULL, &buffer_size))
		{
			free(thiz);
			return NULL;
		}
		buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION *)malloc(buffer_size);
		if (!buffer)
		{
			free(thiz);
			return NULL;
		}
		if (!GetLogicalProcessorInformation(&buffer[0], &buffer_size))
		{
			free(buffer);
			free(thiz);
			return NULL;
		}
		for (i = 0; i != buffer_size / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); ++i)
		{
			if ((buffer[i].Relationship == RelationCache) && (buffer[i].Cache.Level == 1))
			{
				thiz->alignment = buffer[i].Cache.LineSize;
				break;
			}
		}
		free(buffer);
		if (thiz->alignment == 0)
		{
			free(thiz);
			return NULL;
		}
	}
#elif (defined (__MACH__) && defined (__APPLE__)) || defined (__FreeBSD__)
	{
		size_t sizeof_line_size = sizeof(thiz->alignment);

		sysctlbyname("hw.cachelinesize", &thiz->alignment, &sizeof_line_size, 0, 0);
		if (thiz->alignment == 0)
		{
			free(thiz);
			return NULL;
		}
	}
#elif defined (__linux__)
	{
# ifdef _SC_LEVEL1_DCACHE_LINESIZE
		thiz->alignment = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
# else
		FILE *p = NULL;

		p = fopen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", "rb");
		if (p)
		{
			fscanf(p, "%d", &thiz->alignment);
			fclose(p);
		}
		if (thiz->alignment == 0)
		{
			free(thiz);
			return NULL;
		}
# endif
	}
#else
	thiz->alignment = 64;
#endif
	return thiz;
}

static void _enesim_mempool_aligned_free(void *data EINA_UNUSED, void *element)
{
	free(element);
}

static void * _enesim_mempool_aligned_alloc(void *data, unsigned int size)
{
	Enesim_Mempool_Aligned *thiz = (Enesim_Mempool_Aligned *)data;
#ifdef HAVE_POSIX_MEMALIGN
	int ret;
#endif
	void *element = NULL;

#ifdef _WIN32
	element = _aligned_malloc(size, thiz->alignment);
	if (!element)
		return NULL;
#else
# ifdef HAVE_POSIX_MEMALIGN
	ret = posix_memalign(&element, thiz->alignment, size);
	if (ret)
		return NULL;
# else
	element = memalign(thiz->alignment, size);
	if (!element) ret = -EINVAL;
# endif
#endif
	return element;
}

static void _enesim_mempool_aligned_shutdown(void *data)
{
	Enesim_Mempool_Aligned *thiz = data;
	free(thiz);
}

static Eina_Mempool_Backend _mempool_aligned = {
	/* .name 		= */ "aligned",
	/* .init 		= */ _enesim_mempool_aligned_init,
	/* .free 		= */ _enesim_mempool_aligned_free,
	/* .alloc		= */ _enesim_mempool_aligned_alloc,
	/* .realloc		= */ NULL,
	/* .garbage_collect 	= */ NULL,
	/* .statistics		= */ NULL,
	/* .shutdown		= */ _enesim_mempool_aligned_shutdown,
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
