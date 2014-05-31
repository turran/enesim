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
typedef struct _Enesim_Text_Buffer_Simple
{
	char *string;
	size_t length; /* string length */
	size_t bytes; /* alloced length */
} Enesim_Text_Buffer_Simple;
/*----------------------------------------------------------------------------*
 *                           Text buffer interface                            *
 *----------------------------------------------------------------------------*/

static void _simple_string_set(void *data, const char *string, int length)
{
	Enesim_Text_Buffer_Simple *thiz = data;

	/* first create the needed space */
	if (length < 0)
		length = strlen(string);
	if ((unsigned int)length + 1 > thiz->bytes)
	{
		thiz->string = realloc(thiz->string, sizeof(char) * length + 1);
		thiz->bytes = length + 1;
	}
	/* now set */
	strncpy(thiz->string, string, length);
	thiz->string[length] = '\0';
	thiz->length = length;
}

static int _simple_string_insert(void *data, const char *string, int length, ssize_t offset)
{
	Enesim_Text_Buffer_Simple *thiz = data;
	unsigned int new_length;
	int to_move;
	int i;

	/* first create the needed space */
	if (length < 0)
		length = strlen(string);
	if (offset < 0)
		offset = 0;
	else if ((unsigned int)offset > thiz->length)
		offset = thiz->length;

	new_length = length + thiz->length;
	to_move = thiz->length - offset;
	if (new_length + 1 > thiz->bytes)
	{
		thiz->string = realloc(thiz->string, sizeof(char) * new_length + 1);
		thiz->bytes = new_length + 1;
	}
	/* now insert */
	/* make the needed space */
	for (i = 0; i < to_move; i++)
	{
		thiz->string[new_length - i - 1] = thiz->string[offset + to_move - i - 1];
	}
	/* insert */
	strncpy(thiz->string + offset, string, length);
	thiz->string[new_length] = '\0';
	thiz->length = new_length;

	return length;
}

static int _simple_string_delete(void *data EINA_UNUSED, int length EINA_UNUSED, ssize_t offset EINA_UNUSED)
{
	return 0;
}

static const char * _simple_string_get(void *data)
{
	Enesim_Text_Buffer_Simple *thiz = data;
	return thiz->string;
}

static int _simple_string_length(void *data)
{
	Enesim_Text_Buffer_Simple *thiz = data;
	return thiz->length;
}

static void _simple_free(void *data)
{
	Enesim_Text_Buffer_Simple *thiz = data;

	free(thiz->string);
	free(thiz);
}

/* the simple buffer */
static Enesim_Text_Buffer_Descriptor _enesim_text_buffer_simple = {
	/* .string_get 		= */ _simple_string_get,
	/* .string_set 		= */ _simple_string_set,
	/* .string_insert 	= */ _simple_string_insert,
	/* .string_delete 	= */ _simple_string_delete,
	/* .string_length 	= */ _simple_string_length,
	/* .free 		= */ _simple_free,
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Text_Buffer * enesim_text_buffer_simple_new(int initial_length)
{
	Enesim_Text_Buffer_Simple *thiz;

	thiz = calloc(1, sizeof(Enesim_Text_Buffer_Simple));
	if (initial_length <= 0)
		initial_length = PATH_MAX;
	thiz->string = calloc(initial_length, sizeof(char));
	thiz->bytes = initial_length;
	thiz->length = 0;

	return enesim_text_buffer_new_from_descriptor(&_enesim_text_buffer_simple, thiz);
}
