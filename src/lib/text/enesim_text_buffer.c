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
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"

#include "enesim_text.h"
#include "enesim_text_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
struct _Enesim_Text_Buffer
{
	int ref;
	void *data;
	Enesim_Text_Buffer_Descriptor *descriptor;
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Text_Buffer * enesim_text_buffer_new_from_descriptor(Enesim_Text_Buffer_Descriptor *descriptor, void *data)
{
	Enesim_Text_Buffer *thiz;

	if (!descriptor) return NULL;

	thiz = calloc(1, sizeof(Enesim_Text_Buffer));
	thiz->ref = 1;
	thiz->descriptor = descriptor;
	thiz->data = data;

	return thiz;
}

void * enesim_text_buffer_data_get(Enesim_Text_Buffer *thiz)
{
	if (!thiz) return NULL;
	return thiz->data;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Text_Buffer * enesim_text_buffer_ref(Enesim_Text_Buffer *thiz)
{
	if (!thiz) return thiz;

	thiz->ref++;
	return thiz;
}

EAPI void enesim_text_buffer_unref(Enesim_Text_Buffer *thiz)
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

EAPI void enesim_text_buffer_string_set(Enesim_Text_Buffer *thiz, const char *string, int length)
{
	if (!length) return;
	if (!thiz) return;

	if (thiz->descriptor->string_set)
		thiz->descriptor->string_set(thiz->data, string, length);
}

EAPI int enesim_text_buffer_string_insert(Enesim_Text_Buffer *thiz, const char *string, int length, ssize_t offset)
{
	if (!length) return length;

	if (thiz->descriptor->string_insert)
		return thiz->descriptor->string_insert(thiz->data, string, length, offset);
	return 0;
}

EAPI const char * enesim_text_buffer_string_get(Enesim_Text_Buffer *thiz)
{
	if (thiz->descriptor->string_get)
		return thiz->descriptor->string_get(thiz->data);
	return NULL;
}

EAPI int enesim_text_buffer_string_delete(Enesim_Text_Buffer *thiz, int length, ssize_t offset)
{
	if (!length) return length;

	if (thiz->descriptor->string_delete)
		return thiz->descriptor->string_delete(thiz->data, length, offset);
	return 0;
}

EAPI int enesim_text_buffer_string_length(Enesim_Text_Buffer *thiz)
{
	if (!thiz) return 0;
	if (thiz->descriptor->string_length)
		return thiz->descriptor->string_length(thiz->data);
	return 0;
}
