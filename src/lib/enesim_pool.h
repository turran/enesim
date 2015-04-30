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
 * @file
 * @ender_group{Enesim_Pool}
 * @ender_group{Enesim_Pool_Sw}
 * @ender_group{Enesim_Pool_Eina}
 */

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
EAPI Eina_Bool enesim_pool_type_get(Enesim_Pool *thiz, const char **lib,
		const char **name);

/**
 * @}
 * @defgroup Enesim_Pool_Sw Sw Pool
 * @ingroup Enesim_Pool
 * @brief Generic Sw based pool
 * @{
 */

EAPI Enesim_Pool * enesim_pool_sw_new(void);

/**
 * @}
 * @defgroup Enesim_Pool_Eina Eina Pool
 * @ingroup Enesim_Pool
 * @brief Enesim pool based on an Eina Mempool
 * @{
 */

EAPI Enesim_Pool * enesim_pool_eina_new(Eina_Mempool *mp);

/** @} */

#endif
