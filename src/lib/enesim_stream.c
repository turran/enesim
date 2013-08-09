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
#include "enesim_private.h"

#include "enesim_main.h"
#include "enesim_stream.h"
#include "enesim_stream_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
struct _Enesim_Stream
{
	Enesim_Stream_Descriptor *descriptor;
	void *data;
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Stream * enesim_stream_new(
		Enesim_Stream_Descriptor *descriptor, void *data)
{
	Enesim_Stream *thiz;

	if (!descriptor)
		return NULL;

	thiz = calloc(1, sizeof(Enesim_Stream));
	thiz->descriptor = descriptor;
	thiz->data = data;

	return thiz;
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI ssize_t enesim_stream_read(Enesim_Stream *thiz, void *buffer, size_t len)
{
	if (thiz->descriptor->read)
		return thiz->descriptor->read(thiz->data, buffer, len);
	return 0;
}


EAPI ssize_t enesim_stream_write(Enesim_Stream *thiz, void *buffer, size_t len)
{
	if (thiz->descriptor->write)
		return thiz->descriptor->write(thiz->data, buffer, len);
	return 0;
}

EAPI void * enesim_stream_mmap(Enesim_Stream *thiz, size_t *size)
{
	if (thiz->descriptor->mmap)
		return thiz->descriptor->mmap(thiz->data, size);
	return NULL;
}

EAPI void enesim_stream_munmap(Enesim_Stream *thiz, void *ptr)
{
	if (thiz->descriptor->munmap)
		return thiz->descriptor->munmap(thiz->data, ptr);
}

EAPI void enesim_stream_reset(Enesim_Stream *thiz)
{
	if (thiz->descriptor->reset)
		thiz->descriptor->reset(thiz->data);
}

EAPI size_t enesim_stream_length(Enesim_Stream *thiz)
{
	if (thiz->descriptor->length)
		return thiz->descriptor->length(thiz->data);
	return 0;
}

EAPI char * enesim_stream_location(Enesim_Stream *thiz)
{
	if (thiz->descriptor->location)
		return thiz->descriptor->location(thiz->data);
	return NULL;
}

EAPI void enesim_stream_free(Enesim_Stream *thiz)
{
	if (thiz->descriptor->free)
		thiz->descriptor->free(thiz->data);
	free(thiz);
}


