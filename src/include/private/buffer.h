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
#ifndef _BUFFER_H
#define _BUFFER_H

#define ENESIM_MAGIC_BUFFER 0xe7e51430
#define ENESIM_MAGIC_CHECK_BUFFER(d)\
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_MAGIC_BUFFER))\
			EINA_MAGIC_FAIL(d, ENESIM_MAGIC_BUFFER);\
	} while(0)

#if BUILD_OPENCL
typedef struct _Enesim_Buffer_OpenCL_Data
{
	cl_mem mem;
	cl_context context;
	cl_device_id device;
	cl_command_queue queue;
} Enesim_Buffer_OpenCL_Data;
#endif

struct _Enesim_Buffer
{
	EINA_MAGIC
	int ref;
	uint32_t w;
	uint32_t h;
	Enesim_Format format;
	Enesim_Backend backend;
	void *backend_data;
	Enesim_Pool *pool;
	Eina_Bool external_allocated; /* whenever the user wants a buffer by not copying a sw buffer pointer this is true otherwise false */
	void *user; /* user provided data */
};

#endif
