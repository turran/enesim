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

#ifndef ENESIM_STREAM_PRIVATE_H_
#define ENESIM_STREAM_PRIVATE_H_

typedef ssize_t (*Enesim_Stream_Read)(void *data, void *buffer, size_t len);
typedef ssize_t (*Enesim_Stream_Write)(void *data, void *buffer, size_t len);
typedef void * (*Enesim_Stream_Mmap)(void *data, size_t *size);
typedef void (*Enesim_Stream_Munmap)(void *data, void *ptr);
typedef void (*Enesim_Stream_Reset)(void *data);
typedef size_t (*Enesim_Stream_Length)(void *data);
typedef const char * (*Enesim_Stream_Uri_Get)(void *data);
typedef void (*Enesim_Stream_Free)(void *data);

typedef struct _Enesim_Stream_Descriptor
{
	Enesim_Stream_Read read;
	Enesim_Stream_Write write;
	Enesim_Stream_Mmap mmap;
	Enesim_Stream_Munmap munmap;
	Enesim_Stream_Reset reset;
	Enesim_Stream_Length length;
	Enesim_Stream_Uri_Get uri_get;
	Enesim_Stream_Free free;
} Enesim_Stream_Descriptor;

Enesim_Stream * enesim_stream_new(Enesim_Stream_Descriptor *d, void *data);
void * enesim_stream_data_get(Enesim_Stream *thiz);

#endif
