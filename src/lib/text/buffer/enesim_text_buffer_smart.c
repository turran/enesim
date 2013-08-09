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
#include "enesim_surface.h"

#include "enesim_text.h"
#include "enesim_text_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Text_Buffer_Smart
{
	Enesim_Text_Buffer *real;
	Eina_Bool dirty;
} Enesim_Text_Buffer_Smart;
/*----------------------------------------------------------------------------*
 *                           Text buffer interface                            *
 *----------------------------------------------------------------------------*/

static void _smart_string_set(void *data, const char *string, int length)
{
	Enesim_Text_Buffer_Smart *thiz = data;
	if (thiz->real)
	{
		thiz->dirty = EINA_TRUE;
		enesim_text_buffer_string_set(thiz->real, string, length);
	}
}

static int _smart_string_insert(void *data, const char *string, int length, ssize_t offset)
{
	Enesim_Text_Buffer_Smart *thiz = data;
	if (thiz->real)
	{
		thiz->dirty = EINA_TRUE;
		return enesim_text_buffer_string_insert(thiz->real, string, length, offset);
	}
	return 0;
}

static int _smart_string_delete(void *data, int length, ssize_t offset)
{
	Enesim_Text_Buffer_Smart *thiz = data;
	if (thiz->real)
	{
		thiz->dirty = EINA_TRUE;
		return enesim_text_buffer_string_delete(thiz->real, length, offset);
	}
	return 0;
}

static const char * _smart_string_get(void *data)
{
	Enesim_Text_Buffer_Smart *thiz = data;
	if (thiz->real)
		return enesim_text_buffer_string_get(thiz->real);
	return NULL;
}

static int _smart_string_length(void *data)
{
	Enesim_Text_Buffer_Smart *thiz = data;
	if (thiz->real)
		return enesim_text_buffer_string_length(thiz->real);
	return 0;
}

static void _smart_free(void *data)
{
	Enesim_Text_Buffer_Smart *thiz = data;

	enesim_text_buffer_unref(thiz->real);
	free(thiz);
}

/* the smart buffer */
static Enesim_Text_Buffer_Descriptor _enesim_text_buffer_smart = {
	/* .string_get 		= */ _smart_string_get,
	/* .string_set 		= */ _smart_string_set,
	/* .string_insert 	= */ _smart_string_insert,
	/* .string_delete 	= */ _smart_string_delete,
	/* .string_length 	= */ _smart_string_length,
	/* .free 		= */ _smart_free,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Text_Buffer * enesim_text_buffer_smart_new(Enesim_Text_Buffer *real)
{
	Enesim_Text_Buffer_Smart *thiz;

	thiz = calloc(1, sizeof(Enesim_Text_Buffer_Smart));
	thiz->real = real;
	return enesim_text_buffer_new_from_descriptor(&_enesim_text_buffer_smart, thiz);
}

EAPI void enesim_text_buffer_smart_real_get(Enesim_Text_Buffer *b,
		Enesim_Text_Buffer **real)
{
	Enesim_Text_Buffer_Smart *thiz;

	if (!real) return;

	thiz = enesim_text_buffer_data_get(b);
	if (thiz->real) *real = enesim_text_buffer_ref(thiz->real);
	else *real = NULL;
}

EAPI void enesim_text_buffer_smart_real_set(Enesim_Text_Buffer *b,
		Enesim_Text_Buffer *real)
{
	Enesim_Text_Buffer_Smart *thiz;

	thiz = enesim_text_buffer_data_get(b);
	if (thiz->real)
	{
		enesim_text_buffer_unref(thiz->real);
		thiz->real = NULL;
	}
	thiz->real = real;
	thiz->dirty = EINA_TRUE;
}

EAPI void enesim_text_buffer_smart_dirty(Enesim_Text_Buffer *b)
{
	Enesim_Text_Buffer_Smart *thiz;

	thiz = enesim_text_buffer_data_get(b);
	thiz->dirty = EINA_TRUE;
}

EAPI void enesim_text_buffer_smart_clear(Enesim_Text_Buffer *b)
{
	Enesim_Text_Buffer_Smart *thiz;

	thiz = enesim_text_buffer_data_get(b);
	thiz->dirty = EINA_FALSE;
}

EAPI Eina_Bool enesim_text_buffer_smart_is_dirty(Enesim_Text_Buffer *b)
{
	Enesim_Text_Buffer_Smart *thiz;

	thiz = enesim_text_buffer_data_get(b);
	return thiz->dirty;
}
