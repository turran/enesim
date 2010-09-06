/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2008 Jorge Luis Zapata
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
#ifndef _BUFFER_H
#define _BUFFER_H

#define ENESIM_MAGIC_BUFFER 0xe7e51402
#define ENESIM_MAGIC_CHECK_BUFFER(d)\
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_MAGIC_BUFFER))\
			EINA_MAGIC_FAIL(d, ENESIM_MAGIC_BUFFER);\
	} while(0)

struct _Enesim_Buffer
{
	EINA_MAGIC;
	uint32_t w;
	uint32_t h;
	Enesim_Buffer_Data data;
	Enesim_Format format;
	Enesim_Backend backend;
	Enesim_Pool *pool;
	void *user; /* user provided data */
};

Eina_Bool enesim_buffer_setup(Enesim_Buffer *buf, Enesim_Backend b,
		Enesim_Buffer_Format fmt, uint32_t w, uint32_t h,
		Enesim_Buffer_Data *data, Enesim_Pool *pool);

#endif
