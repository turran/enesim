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
#ifndef _SURFACE_H
#define _SURFACE_H

#define ENESIM_MAGIC_SURFACE 0xe7e51410
#define ENESIM_MAGIC_CHECK_SURFACE(d)\
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_MAGIC_SURFACE))\
			EINA_MAGIC_FAIL(d, ENESIM_MAGIC_SURFACE);\
	} while(0)

struct _Enesim_Surface
{
	EINA_MAGIC
	int ref;
	Enesim_Buffer *buffer;
	Enesim_Buffer_Free free_func;
	void *free_func_data;
	Enesim_Format format;
	void *user; /* user provided data */
};
	
void * enesim_surface_backend_data_get(Enesim_Surface *s);

#endif
