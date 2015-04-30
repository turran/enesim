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
/** @cond internal */
struct _Enesim_Stream
{
	int ref;
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
	thiz->ref = 1;

	return thiz;
}

void * enesim_stream_data_get(Enesim_Stream *thiz)
{
	return thiz->data;
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Increase the reference counter of a stream
 * @param[in] thiz The stream
 * @return The input parameter @a thiz for programming convenience
 */
EAPI Enesim_Stream * enesim_stream_ref(Enesim_Stream *thiz)
{
	if (!thiz) return thiz;
	thiz->ref++;
	return thiz;
}

/**
 * @brief Decrease the reference counter of a stream
 * @param[in] thiz The stream
 */
EAPI void enesim_stream_unref(Enesim_Stream *thiz)
{
	if (!thiz) return;

	thiz->ref--;
	if (!thiz->ref)
	{
		if (thiz->descriptor->free)
			thiz->descriptor->free(thiz->data);
		free(thiz);
	}
}

/**
 * @brief Reads from a stream
 * @param[in] thiz The stream to read from
 * @param[in] buffer Where to read the data into
 * @param[in] len How many bytes to read
 * @return The number of bytes read
 */
EAPI ssize_t enesim_stream_read(Enesim_Stream *thiz, void *buffer, size_t len)
{
	if (thiz->descriptor->read)
		return thiz->descriptor->read(thiz->data, buffer, len);
	return 0;
}


/**
 * @brief Writes from a stream
 * @param[in] thiz The stream to write to
 * @param[in] buffer The data to write
 * @param[in] len How many bytes to write
 * @return The number of bytes written
 */
EAPI ssize_t enesim_stream_write(Enesim_Stream *thiz, void *buffer, size_t len)
{
	if (thiz->descriptor->write)
		return thiz->descriptor->write(thiz->data, buffer, len);
	return 0;
}

/**
 * @brief Map the stream in memory
 * @param[in] thiz The stream to map
 * @param[out] size The size of the mapped buffer
 * @return The memory mapped pointer
 */
EAPI void * enesim_stream_mmap(Enesim_Stream *thiz, size_t *size)
{
	if (thiz->descriptor->mmap)
		return thiz->descriptor->mmap(thiz->data, size);
	return NULL;
}

/**
 * @brief Unmaps the memory of a stream
 * @param[in] thiz The stream to unmap
 * @param[in] ptr The mapped memory
 */
EAPI void enesim_stream_munmap(Enesim_Stream *thiz, void *ptr)
{
	if (thiz->descriptor->munmap)
		return thiz->descriptor->munmap(thiz->data, ptr);
}

/**
 * @brief Reset the stream reading/writing position
 * @param[in] thiz The stream to reset
 *
 * On every @ref enesim_stream_read or @ref enesim_stream_write
 * the internal position of the stream is advanced, this function
 * resets the position of the stream
 */
EAPI void enesim_stream_reset(Enesim_Stream *thiz)
{
	if (thiz->descriptor->reset)
		thiz->descriptor->reset(thiz->data);
}

/**
 * @brief Gets the length of a stream
 * @param[in] thiz The stream to get the length from
 * @return The length of the stream
 */
EAPI size_t enesim_stream_length(Enesim_Stream *thiz)
{
	if (thiz->descriptor->length)
		return thiz->descriptor->length(thiz->data);
	return 0;
}

/**
 * @brief Get the URI of a stream
 * @param[in] thiz The stream to get the URI from
 * @return The URI of a stream @ender_transfer{none}
 *
 * Every stream has an URI associated that identifies the origin of
 * stream data. This function gets such URI
 */
EAPI const char * enesim_stream_uri_get(Enesim_Stream *thiz)
{
	if (thiz->descriptor->uri_get)
		return thiz->descriptor->uri_get(thiz->data);
	return NULL;
}

/**
 * @brief Get the type of a stream
 * @ender_downcast
 * @param[in] thiz The stream to get the type from
 * @param[out] lib The ender library associated with this stream
 * @param[out] name The ender item name of the renderer
 *
 * This function is needed for ender in order to downcast a stream
 */
EAPI Eina_Bool enesim_stream_type_get(Enesim_Stream *thiz, const char **lib,
		const char **name)
{
	if (!thiz)
		return EINA_FALSE;
	if (lib)
		*lib = "enesim";
	if (name)
		*name = thiz->descriptor->type_get();
	return EINA_TRUE;
}
