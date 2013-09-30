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
 * @defgroup Enesim_Stream_Group Stream
 * @brief Stream sources
 * @{
 */
typedef struct _Enesim_Stream Enesim_Stream;

EAPI Enesim_Stream * enesim_stream_ref(Enesim_Stream *thiz);
EAPI void enesim_stream_unref(Enesim_Stream *thiz);
EAPI ssize_t enesim_stream_read(Enesim_Stream *thiz, void *buffer, size_t len);
EAPI ssize_t enesim_stream_write(Enesim_Stream *thiz, void *buffer, size_t len);
EAPI size_t enesim_stream_length(Enesim_Stream *thiz);
EAPI void * enesim_stream_mmap(Enesim_Stream *thiz, size_t *size);
EAPI void enesim_stream_munmap(Enesim_Stream *thiz, void *ptr);
EAPI void enesim_stream_reset(Enesim_Stream *thiz);
EAPI char * enesim_stream_location(Enesim_Stream *thiz);

EAPI Enesim_Stream * enesim_stream_file_new(const char *file, const char *mode);
EAPI Enesim_Stream * enesim_stream_buffer_new(void *buffer, size_t len);
EAPI Enesim_Stream * enesim_stream_base64_new(Enesim_Stream *d);

/**
 * @}
 */

#endif

