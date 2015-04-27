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
#ifndef ENESIM_STREAM_H
#define ENESIM_STREAM_H

/**
 * @file
 * @ender_group{Enesim_Stream}
 * @ender_group{Enesim_Stream_File}
 * @ender_group{Enesim_Stream_Buffer}
 * @ender_group{Enesim_Stream_Base64}
 */

/**
 * @defgroup Enesim_Stream Stream
 * @brief Stream sources
 * @{
 */

typedef struct _Enesim_Stream Enesim_Stream; /**< Stream handle */

EAPI Enesim_Stream * enesim_stream_ref(Enesim_Stream *thiz);
EAPI void enesim_stream_unref(Enesim_Stream *thiz);
EAPI ssize_t enesim_stream_read(Enesim_Stream *thiz, void *buffer, size_t len);
EAPI ssize_t enesim_stream_write(Enesim_Stream *thiz, void *buffer, size_t len);
EAPI size_t enesim_stream_length(Enesim_Stream *thiz);
EAPI void * enesim_stream_mmap(Enesim_Stream *thiz, size_t *size);
EAPI void enesim_stream_munmap(Enesim_Stream *thiz, void *ptr);
EAPI void enesim_stream_reset(Enesim_Stream *thiz);
EAPI const char * enesim_stream_uri_get(Enesim_Stream *thiz);

/**
 * @}
 * @defgroup Enesim_Stream_File File
 * @ingroup Enesim_Stream
 * @brief File based stream
 * @{
 */
EAPI Enesim_Stream * enesim_stream_file_new(const char *file, const char *mode);

/**
 * @}
 * @defgroup Enesim_Stream_Buffer Buffer
 * @ingroup Enesim_Stream
 * @brief Buffer based stream
 * @{
 */

/**
 * @param b The user provided buffer
 */
typedef void (*Enesim_Stream_Buffer_Free)(void *b);

EAPI Enesim_Stream * enesim_stream_buffer_new(void *buffer, size_t len, Enesim_Stream_Buffer_Free free_cb);
/**
 * @}
 * @defgroup Enesim_Stream_Base64 Base64
 * @ingroup Enesim_Stream
 * @brief Base64 stream
 * @{
 */
EAPI Enesim_Stream * enesim_stream_base64_new(Enesim_Stream *d);

/**
 * @}
 */

#endif

