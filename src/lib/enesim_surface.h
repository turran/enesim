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
#ifndef ENESIM_SURFACE_H_
#define ENESIM_SURFACE_H_

/**
 * @file
 * @ender_group{Enesim_Surface}
 */

/**
 * @defgroup Enesim_Surface Surface
 * @brief Native pixel data holder
 * 
 * All the rendering process of an @ref Enesim_Renderer is done into a
 * surface. A surface is the library's internal way to store pixel data.
 * It is a specification of an @ref Enesim_Buffer but handled with it's own
 * type @ref Enesim_Surface and functions. To get the buffer associated with a
 * surface you can use @ref enesim_surface_buffer_get().
 *
 * A surface can be created by using @ref enesim_surface_new() and all
 * it's variants depending on the need. You can use your own memory
 * to store the pixel data, a specific @ref Enesim_Pool or from
 * an @ref Enesim_Buffer if using @ref enesim_surface_new_buffer_from()
 *
 * 
 * @{
 */
typedef struct _Enesim_Surface Enesim_Surface; /**< Surface Handle */

EAPI Enesim_Surface * enesim_surface_new(Enesim_Format f, uint32_t w,
		uint32_t h);
EAPI Enesim_Surface * enesim_surface_new_data_from(Enesim_Format f,
		uint32_t w, uint32_t h, Eina_Bool copy, void *sw_data,
		size_t stride, Enesim_Buffer_Free free_func, void *free_func_data);
EAPI Enesim_Surface * enesim_surface_new_pool_from(Enesim_Format f,
		uint32_t w, uint32_t h, Enesim_Pool *p);
EAPI Enesim_Surface * enesim_surface_new_pool_and_data_from(Enesim_Format fmt,
		uint32_t w, uint32_t h, Enesim_Pool *p, Eina_Bool copy,
		void *sw_data, size_t stride, Enesim_Buffer_Free free_func,
		void *free_func_data);
EAPI Enesim_Surface * enesim_surface_new_buffer_from(Enesim_Buffer *buffer);
EAPI Enesim_Surface * enesim_surface_ref(Enesim_Surface *s);
EAPI void enesim_surface_unref(Enesim_Surface *s);

EAPI Enesim_Buffer * enesim_surface_buffer_get(Enesim_Surface *s);

EAPI void enesim_surface_size_get(const Enesim_Surface *s, int *w, int *h);
EAPI Enesim_Format enesim_surface_format_get(const Enesim_Surface *s);
EAPI Enesim_Backend enesim_surface_backend_get(const Enesim_Surface *s);
EAPI Enesim_Pool * enesim_surface_pool_get(Enesim_Surface *s);

EAPI void enesim_surface_private_set(Enesim_Surface *s, void *data);
EAPI void * enesim_surface_private_get(Enesim_Surface *s);

EAPI Eina_Bool enesim_surface_sw_data_get(Enesim_Surface *s, void **data, size_t *stride);
EAPI Eina_Bool enesim_surface_map(const Enesim_Surface *s, void **data, size_t *stride);
EAPI Eina_Bool enesim_surface_unmap(const Enesim_Surface *s, void *data, Eina_Bool written);

EAPI void enesim_surface_lock(Enesim_Surface *s, Eina_Bool write);
EAPI void enesim_surface_unlock(Enesim_Surface *s);

EAPI void enesim_surface_alpha_hint_set(Enesim_Surface *s, Enesim_Alpha_Hint hint);
EAPI Enesim_Alpha_Hint enesim_surface_alpha_hint_get(Enesim_Surface *s);

/** @} */ //End of Enesim_Surface

#endif /*ENESIM_SURFACE_H_*/
