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
#include "enesim_private.h"

#include "enesim_main.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_image.h"
#include "enesim_image_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Image_Data_Buffer
{
	/* passed in */
	void *buffer;
	size_t len;
	/* current position */
	void *curr;
	ssize_t off;
} Enesim_Image_Data_Buffer;
/*----------------------------------------------------------------------------*
 *                        The Emage Data interface                            *
 *----------------------------------------------------------------------------*/
static ssize_t _enesim_image_data_buffer_read(void *data, void *buffer, size_t len)
{
	Enesim_Image_Data_Buffer *thiz = data;

	if (thiz->off + len > thiz->len)
		len = thiz->len - thiz->off;

	memcpy(thiz->curr, buffer, len);
	thiz->off += len;
	thiz->curr = ((char *)thiz->curr) + len;

	return len;
}

static ssize_t _enesim_image_data_buffer_write(void *data, void *buffer, size_t len)
{
	Enesim_Image_Data_Buffer *thiz = data;

	if (thiz->off + len > thiz->len)
		len = thiz->len - thiz->off;

	memcpy(buffer, thiz->curr, len);
	thiz->off += len;
	thiz->curr = ((char *)thiz->curr) + len;

	return len;
}

static void * _enesim_image_data_buffer_mmap(void *data, size_t *size)
{
	Enesim_Image_Data_Buffer *thiz = data;
	*size = thiz->len;
	return thiz->buffer;
}

static size_t _enesim_image_data_buffer_length(void *data)
{
	Enesim_Image_Data_Buffer *thiz = data;
	return thiz->len;
}

static void _enesim_image_data_buffer_reset(void *data)
{
	Enesim_Image_Data_Buffer *thiz = data;
	thiz->off = 0;
	thiz->curr = thiz->buffer;
}

static void _enesim_image_data_buffer_free(void *data)
{
	Enesim_Image_Data_Buffer *thiz = data;
	free(thiz);
}

static Enesim_Image_Data_Descriptor _enesim_image_data_buffer_descriptor = {
	/* .read	= */ _enesim_image_data_buffer_read,
	/* .write	= */ _enesim_image_data_buffer_write,
	/* .mmap	= */ _enesim_image_data_buffer_mmap,
	/* .munmap	= */ NULL,
	/* .reset	= */ _enesim_image_data_buffer_reset,
	/* .length	= */ _enesim_image_data_buffer_length,
	/* .location 	= */ NULL,
	/* .free	= */ _enesim_image_data_buffer_free,
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Image_Data * enesim_image_data_buffer_new(void *buffer, size_t len)
{
	Enesim_Image_Data_Buffer *thiz;

	thiz = calloc(1, sizeof(Enesim_Image_Data_Buffer));
	thiz->buffer = thiz->curr = buffer;
	thiz->len = len;
	thiz->off = 0;

	return enesim_image_data_new(&_enesim_image_data_buffer_descriptor, thiz);
}

