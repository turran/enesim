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
#ifndef POOL_H_
#define POOL_H_

typedef Eina_Bool (*Enesim_Pool_Data_Alloc)(void *prv,
		Enesim_Backend *backend,
		void **backend_data,
		Enesim_Buffer_Format fmt, uint32_t w, uint32_t h);

typedef void (*Enesim_Pool_Data_Free)(void *prv,
		void *backend_data,
		Enesim_Buffer_Format fmt,
		Eina_Bool external_allocated);

typedef Eina_Bool (*Enesim_Pool_Data_From)(void *prv,
		Enesim_Backend *backend,
		void **backend_data,
		Enesim_Buffer_Format fmt,
		uint32_t w, uint32_t h,
		Eina_Bool copy,
		Enesim_Buffer_Sw_Data *src);

typedef Eina_Bool (*Enesim_Pool_Data_Sub)(void *prv,
		void **backend_data,
		void *original_data,
		Enesim_Buffer_Format fmt,
		const Eina_Rectangle *area);

typedef Eina_Bool (*Enesim_Pool_Data_Get)(void *prv,
		void *backend_data,
		Enesim_Buffer_Format fmt,
		uint32_t w, uint32_t h,
		Enesim_Buffer_Sw_Data *dst);

typedef void (*Enesim_Pool_Free)(void *prv);

typedef struct _Enesim_Pool_Descriptor
{
	Enesim_Pool_Data_Alloc data_alloc;
	Enesim_Pool_Data_Free data_free;
	Enesim_Pool_Data_From data_from;
	Enesim_Pool_Data_Get data_get;
	Enesim_Pool_Free free;
} Enesim_Pool_Descriptor;

struct _Enesim_Pool
{
	EINA_MAGIC
	Enesim_Pool_Descriptor *descriptor;
	void *data;
	int ref;
};

void enesim_pool_init(void);
void enesim_pool_shutdown(void);

Enesim_Pool * enesim_pool_new(Enesim_Pool_Descriptor *descriptor, void *data);
Eina_Bool enesim_pool_data_alloc(Enesim_Pool *p, Enesim_Backend *backend, void **data,
		Enesim_Buffer_Format fmt, uint32_t w, uint32_t h);
Eina_Bool enesim_pool_data_from(Enesim_Pool *p, Enesim_Backend *backend, void **data,
		Enesim_Buffer_Format fmt, uint32_t w, uint32_t h, Eina_Bool copy,
		Enesim_Buffer_Sw_Data *from);
Eina_Bool enesim_pool_data_get(Enesim_Pool *p, void *data,
		Enesim_Buffer_Format fmt,
		uint32_t w, uint32_t h,
		Enesim_Buffer_Sw_Data *dst);
void enesim_pool_data_free(Enesim_Pool *p, void *data,
		Enesim_Buffer_Format fmt,
		Eina_Bool external_allocated);

#endif
