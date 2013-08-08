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
#ifndef _ENESIM_BARRIER_PRIVATE_H
#define _ENESIM_BARRIER_PRIVATE_H

#ifdef BUILD_THREAD
#ifdef _WIN32

typedef struct _Enesim_Barrier Enesim_Barrier;
struct _Enesim_Barrier
{
   int needed, called;
   Eina_Lock cond_lock;
   Eina_Condition cond;
};

Eina_Bool enesim_barrier_new(Enesim_Barrier *barrier, int needed);
void enesim_barrier_free(Enesim_Barrier *barrier);
Eina_Bool enesim_barrier_wait(Enesim_Barrier *barrier);

#else /* _WIN32 */

typedef pthread_barrier_t Enesim_Barrier;
#define enesim_barrier_new(barrier, needed) pthread_barrier_init(barrier, NULL, needed)
#define enesim_barrier_free(barrier) pthread_barrier_destroy(barrier)
#define enesim_barrier_wait(barrier) pthread_barrier_wait(barrier)

#endif /* _WIN32 */

#endif /* BUILD_THREAD */

#endif /* _ENESIM_BARRIER_PRIVATE_H */
