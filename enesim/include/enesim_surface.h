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
#ifndef ENESIM_SURFACE_H_
#define ENESIM_SURFACE_H_

/**
 * @defgroup Enesim_Surface_Group Surface
 * @{
 *
 * @todo
 * - Add a pitch, this is good for different planes, still the width and height
 * are good things to have so the pitch
 * - Add a data provider: as a parameter to the new, so the destruction is
 * handled by the library itself, same for data get, etc.
 * - Add a surface iterator
 * - Normalize the occurences of argb, colors, etc. Always premul or flat
 * argb8888?
 */
typedef struct _Enesim_Surface 	Enesim_Surface; /**< Surface Handler */

typedef enum _Enesim_Surface_Flag
{
	ENESIM_SURFACE_FLAG_ALPHA,
	ENESIM_SURFACE_FLAG_SPARSE_ALPHA,
	ENESIM_SURFACE_FLAG_NO_ALPHA,
	ENESIM_SURFACE_FLAGS
} Enesim_Surface_Flag;

EAPI Enesim_Surface * enesim_surface_new(Enesim_Format f, uint32_t w, uint32_t h);
EAPI Enesim_Surface * enesim_surface_new_data_from(Enesim_Format f, uint32_t w, uint32_t h, Eina_Bool copy, Enesim_Buffer_Sw_Data *data);
EAPI Enesim_Surface * enesim_surface_new_pool_from(Enesim_Format f, uint32_t w, uint32_t h, Enesim_Pool *p);
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

EAPI Eina_Bool enesim_surface_data_get(Enesim_Surface *s, void **data, size_t *stride);

EAPI void enesim_surface_lock(Enesim_Surface *s, Eina_Bool write);
EAPI void enesim_surface_unlock(Enesim_Surface *s);

#ifdef ENESIM_EXTENSION
EAPI void * enesim_surface_backend_data_get(Enesim_Surface *s);
#endif

/** @} */ //End of Enesim_Surface_Group

#endif /*ENESIM_SURFACE_H_*/
