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

#include "enesim_text.h"
#include "enesim_text_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
struct _Enesim_Text_Buffer
{
	void *data;
	Enesim_Text_Buffer_Descriptor *descriptor;
};

typedef struct _Enesim_Text_Buffer_Default
{
	char *string;
	size_t length; /* string length */
	size_t bytes; /* alloced length */
} Enesim_Text_Buffer_Default;

static void _default_string_set(void *data, const char *string, int length)
{
	Enesim_Text_Buffer_Default *thiz = data;

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

static int _default_string_insert(void *data, const char *string, int length, ssize_t offset)
{
	Enesim_Text_Buffer_Default *thiz = data;
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

static int _default_string_delete(void *data, int length EINA_UNUSED, ssize_t offset EINA_UNUSED)
{
	Enesim_Text_Buffer_Default *thiz = data;

	return 0;
}

static const char * _default_string_get(void *data)
{
	Enesim_Text_Buffer_Default *thiz = data;
	return thiz->string;
}

static int _default_string_length(void *data)
{
	Enesim_Text_Buffer_Default *thiz = data;
	return thiz->length;
}

static void _default_free(void *data)
{
	Enesim_Text_Buffer_Default *thiz = data;

	free(thiz->string);
	free(thiz);
}

/* the default buffer */
Enesim_Text_Buffer_Descriptor _enesim_text_buffer_default = {
	/* .string_get = */ _default_string_get,
	/* .string_set = */ _default_string_set,
	/* .string_insert = */ _default_string_insert,
	/* .string_delete = */ _default_string_delete,
	/* .string_length = */ _default_string_length,
	/* .free = */ _default_free,
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
EAPI Enesim_Text_Buffer * enesim_text_buffer_new(int initial_length)
{
	Enesim_Text_Buffer_Default *bd;

	bd = calloc(1, sizeof(Enesim_Text_Buffer_Default));
	if (initial_length <= 0)
		initial_length = PATH_MAX;
	bd->string = calloc(initial_length, sizeof(char));
	bd->bytes = initial_length;
	bd->length = 0;

	return enesim_text_buffer_new_from_descriptor(&_enesim_text_buffer_default, bd);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Text_Buffer * enesim_text_buffer_new_from_descriptor(Enesim_Text_Buffer_Descriptor *descriptor, void *data)
{
	Enesim_Text_Buffer *b;

	if (!descriptor) return NULL;

	b = calloc(1, sizeof(Enesim_Text_Buffer));
	b->descriptor = descriptor;
	b->data = data;

	return b;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_text_buffer_delete(Enesim_Text_Buffer *b)
{
	if (!b) return;

	if (b->descriptor->free)
		b->descriptor->free(b->data);
	free(b);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_text_buffer_string_set(Enesim_Text_Buffer *b, const char *string, int length)
{
	if (!length) return;
	if (!b) return;

	if (b->descriptor->string_set)
		b->descriptor->string_set(b->data, string, length);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI int enesim_text_buffer_string_insert(Enesim_Text_Buffer *b, const char *string, int length, ssize_t offset)
{
	if (!length) return length;

	if (b->descriptor->string_insert)
		return b->descriptor->string_insert(b->data, string, length, offset);
	return 0;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI const char * enesim_text_buffer_string_get(Enesim_Text_Buffer *b)
{
	if (b->descriptor->string_get)
		return b->descriptor->string_get(b->data);
	return NULL;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI int enesim_text_buffer_string_delete(Enesim_Text_Buffer *b, int length, ssize_t offset)
{
	if (!length) return length;

	if (b->descriptor->string_delete)
		return b->descriptor->string_delete(b->data, length, offset);
	return 0;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI int enesim_text_buffer_string_length(Enesim_Text_Buffer *b)
{
	if (!b) return 0;
	if (b->descriptor->string_length)
		return b->descriptor->string_length(b->data);
	return 0;
}
