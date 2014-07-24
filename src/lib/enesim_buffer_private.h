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
#ifndef _ENESIM_BUFFER_PRIVATE_H
#define _ENESIM_BUFFER_PRIVATE_H

#define ENESIM_MAGIC_BUFFER 0xe7e51430
#define ENESIM_MAGIC_CHECK_BUFFER(d)\
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_MAGIC_BUFFER))\
			EINA_MAGIC_FAIL(d, ENESIM_MAGIC_BUFFER);\
	} while(0)

struct _Enesim_Buffer
{
	EINA_MAGIC
	int ref;
	uint32_t w;
	uint32_t h;
	Enesim_Buffer_Format format;
	Enesim_Backend backend;
	void *backend_data;
	Enesim_Pool *pool;
	/* callback to use whenever the data is provided
	 * by the user. This is only used whenever the
	 * external_allocated flag is set
	 */
	Enesim_Buffer_Free free_func;
	void *free_func_data;
	/* whenever the user wants a buffer by not copying
	 * a sw buffer pointer this is true otherwise false
	 */
	Eina_Bool external_allocated;
	/* FIXME make this conditional for windows */
	Eina_RWLock lock;
	/* in case this is a subbuffer */
	Enesim_Buffer *owner;
	void *user; /* user provided data */
};

void * enesim_buffer_backend_data_get(Enesim_Buffer *b);
void enesim_buffer_sw_data_set(Enesim_Buffer_Sw_Data *data,
		Enesim_Buffer_Format fmt, void *content0, int stride0);
void enesim_buffer_sw_data_free(Enesim_Buffer_Sw_Data *data,
		Enesim_Buffer_Format fmt,
		Enesim_Buffer_Free free_func,
		void *free_func_data);

#endif
