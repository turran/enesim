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
Eina_Bool enesim_barrier_new(Enesim_Barrier *barrier, int needed)
{
   barrier->needed = needed;
   barrier->called = 0;
   if (!eina_lock_new(&(barrier->cond_lock)))
     return EINA_FALSE;
   if (!eina_condition_new(&(barrier->cond), &(barrier->cond_lock)))
     return EINA_FALSE;
   return EINA_TRUE;
}

void enesim_barrier_free(Enesim_Barrier *barrier)
{
   eina_condition_free(&(barrier->cond));
   eina_lock_free(&(barrier->cond_lock));
   barrier->needed = 0;
   barrier->called = 0;
}

Eina_Bool enesim_barrier_wait(Enesim_Barrier *barrier)
{
   eina_lock_take(&(barrier->cond_lock));
   barrier->called++;
   if (barrier->called == barrier->needed)
     {
        barrier->called = 0;
        eina_condition_broadcast(&(barrier->cond));
     }
   else
     eina_condition_wait(&(barrier->cond));
   eina_lock_release(&(barrier->cond_lock));
   return EINA_TRUE;
}
#endif
#endif
