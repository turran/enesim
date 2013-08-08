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

/* eina includes sched.h for us, but does not set the _GNU_SOURCE
 * flag, so basically we cant use CPU_ZERO, CPU_SET, etc
 */
#include "enesim_private.h"
#include "enesim_barrier_private.h" 
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
#ifdef BUILD_THREAD
#ifdef _WIN32
static Eina_Bool enesim_thread_win32_new(HANDLE *thread, LPTHREAD_START_ROUTINE callback, void *data)
{
	*thread = CreateThread(NULL, 0, callback, data, 0, NULL);
	return *thread != NULL;
}
#endif

#ifndef _WIN32
Eina_Bool enesim_thread_posix_new(pthread_t *thread, void *(*callback)(void *d), void *data)
{
	pthread_attr_t attr;

	pthread_attr_init(&attr);
	return pthread_create(thread, &attr, callback, data);
}

void enesim_thread_posix_affinity_set(pthread_t thread, int cpunum)
{
	enesim_cpu_set_t cpu;

	CPU_ZERO(&cpu);
	CPU_SET(cpunum, &cpu);
	pthread_setaffinity_np(thread, sizeof(enesim_cpu_set_t), &cpu);
}

#endif
#endif
