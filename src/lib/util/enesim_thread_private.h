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
#ifndef _ENESIM_THREAD_PRIVATE_H
#define _ENESIM_THREAD_PRIVATE_H

#ifdef BUILD_THREAD
#ifdef _WIN32
typedef HANDLE Enesim_Thread;
Eina_Bool enesim_thread_win32_new(HANDLE *thread, LPTHREAD_START_ROUTINE callback, void *data);

#define enesim_thread_new(thread, cb, data) enesim_thread_win32_new(thread, cb, data)
#define enesim_thread_free(thread) CloseHandle(thread)
#define enesim_thread_affinity_set(thread, cpunum) SetThreadAffinityMask(thread, 1 << (cpunum))

#else /* _WIN32 */
typedef pthread_t Enesim_Thread;
Eina_Bool enesim_thread_posix_new(pthread_t *thread, void *(*callback)(void *d), void *data);
void enesim_thread_posix_affinity_set(pthread_t thread, int cpunum);

#define enesim_thread_new(thread, cb, data) enesim_thread_posix_new(thread, cb, data)
#define enesim_thread_free(thread) pthread_join(thread, NULL)
#define enesim_thread_affinity_set(thread, cpunum) enesim_thread_posix_affinity_set(thread, cpunum)

#endif /* _WIN32 */
#endif /* BUILD_THREAD */
#endif /* _ENESIM_THREAD_PRIVATE_H */
