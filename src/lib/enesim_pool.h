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
#ifndef ENESIM_POOL_H_
#define ENESIM_POOL_H_

/**
 * @defgroup Enesim_Pool Pool
 * @brief Buffer and surface pixel data allocator
 * @{
 */
typedef struct _Enesim_Pool Enesim_Pool; /**< Pool Handle */

EAPI Enesim_Pool * enesim_pool_default_get(void);
EAPI void enesim_pool_default_set(Enesim_Pool *thiz);

EAPI Enesim_Pool * enesim_pool_ref(Enesim_Pool *thiz);
EAPI void enesim_pool_unref(Enesim_Pool *thiz);

EAPI Enesim_Pool * enesim_pool_eina_new(Eina_Mempool *mp);

/** @} */

#endif
