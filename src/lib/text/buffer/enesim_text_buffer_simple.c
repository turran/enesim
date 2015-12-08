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
/** @cond internal */
typedef struct _Enesim_Text_Buffer_Simple
{
	char *string;
	size_t length; /* string length */
	size_t blength; /* string length in bytes */
	size_t bytes; /* alloced length */
} Enesim_Text_Buffer_Simple;

static void _utf8_strlen(const char *string, int *length, size_t *bytes)
{
	int count = 0;
	int idx = 0;

	if (*length < 0)
	{
		while (eina_unicode_utf8_next_get(string, &idx))
			count++;
		*length = count;
	}
	else
	{
		while (count < *length && eina_unicode_utf8_next_get(string, &idx))
			count++;
	}
	*bytes = idx;
}

/*----------------------------------------------------------------------------*
 *                           Text buffer interface                            *
 *----------------------------------------------------------------------------*/
static const char * _simple_type_get(void)
{
	return "enesim.text.buffer.simple";
}

static void _simple_string_set(void *data, const char *string, int length)
{
	Enesim_Text_Buffer_Simple *thiz = data;
	size_t bytes;

	/* first create the needed space */
	_utf8_strlen(string, &length, &bytes);
	if (bytes + 1 > thiz->bytes)
	{
		thiz->string = realloc(thiz->string, sizeof(char) * bytes + 1);
		thiz->bytes = bytes + 1;
	}
	/* now set */
	strncpy(thiz->string, string, bytes);
	thiz->string[bytes] = '\0';
	thiz->length = length;
	thiz->blength = bytes;
}

static int _simple_string_insert(void *data, const char *string, int length, ssize_t offset)
{
	Enesim_Text_Buffer_Simple *thiz = data;
	size_t bytes;
	size_t new_blength;
	size_t new_length;
	int to_move;
	int i;
	size_t boffset = 0;

	/* first create the needed space */
	_utf8_strlen(string, &length, &bytes);
	/* sanitize the offset */
	if (offset < 0)
		offset = thiz->length;
	else if ((unsigned int)offset > thiz->length)
		offset = thiz->length;

	new_blength = bytes + thiz->blength;
	new_length = length + thiz->length;
	if (new_blength + 1 > thiz->bytes)
	{
		thiz->string = realloc(thiz->string, sizeof(char) * new_blength + 1);
		thiz->bytes = new_blength + 1;
	}
	/* now insert */
	/* make the needed space */
	_utf8_strlen(thiz->string, (int *)&offset, &boffset);
	to_move = thiz->blength - boffset;
	/* ok we found the byte offset to start moving */
	for (i = 0; i < to_move; i++)
	{
		thiz->string[new_blength - i - 1] = thiz->string[boffset + to_move - i - 1];
	}
	/* insert */
	strncpy(thiz->string + boffset, string, bytes);
	thiz->string[new_blength] = '\0';
	thiz->length = new_length;
	thiz->blength = new_blength;

	return length;
}

static int _simple_string_delete(void *data, int length, ssize_t offset)
{
	Enesim_Text_Buffer_Simple *thiz = data;

	if (length <= 0)
		return 0;

	if (offset < 0)
	{
		if (!thiz->length)
			return 0;
		offset = thiz->length - length;
	}

	if ((unsigned int)offset >= thiz->length)
		return 0;

	/* simple case */
	if ((unsigned int)(offset + length) >= thiz->length)
	{
		int del;
		size_t boffset;

		del = thiz->length - offset;
		_utf8_strlen(thiz->string, (int *)&offset, &boffset);
		thiz->string[boffset] = '\0';
		thiz->length = offset;
		thiz->blength = boffset;

		return del;
	}
	else
	{
		int i;
		int j = offset;
		size_t boffset;
		size_t blength;

		/* shift the chars */
		_utf8_strlen(thiz->string, (int *)&offset, &boffset);
		_utf8_strlen(thiz->string + boffset, &length, &blength);
		for (i = boffset + blength; (unsigned int)i < thiz->blength; i++, j++)
			thiz->string[j] = thiz->string[i];
		thiz->blength -= blength;
		thiz->length -= length;
		thiz->string[thiz->blength] = '\0';
		return length;
	}
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
	/* .type_get 		= */ _simple_type_get,
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
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Text_Buffer * enesim_text_buffer_simple_new(int initial_length)
{
	Enesim_Text_Buffer_Simple *thiz;

	thiz = calloc(1, sizeof(Enesim_Text_Buffer_Simple));
	if (initial_length <= 0)
		initial_length = PATH_MAX;
	/* worst case */
	thiz->string = calloc(initial_length, sizeof(Eina_Unicode));
	thiz->bytes = initial_length * sizeof(Eina_Unicode);
	thiz->length = thiz->blength = 0;

	return enesim_text_buffer_new_from_descriptor(&_enesim_text_buffer_simple, thiz);
}
