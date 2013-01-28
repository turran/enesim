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

#ifndef _ENESIM_TEXT_H
#define _ENESIM_TEXT_H

/**
 * @todo
 * - Split the renderer into different types of renderers:
 *   - span renderer
 * - Move the actual font/face loader into a submodule that should fill a common structure
 *   - freetype
 *   - move our own loader from efl-research here, for legacy code
 * - The possible cache we can have is:
 *   - FT_Face from memory (shared memory)
 *   - FT_Glyphs from memory?
 *   - Some kind of already rendered glyphs on a buffer
 *   - We can have a table per glyphs already renderered with a refcount on them
 */



/**
 * @defgroup Enesim_Text_Main_Group Main
 * @{
 */

EAPI int enesim_text_init(void);
EAPI int enesim_text_shutdown(void);

typedef struct _Enesim_Text_Engine Enesim_Text_Engine;
EAPI Enesim_Text_Engine * enesim_text_engine_default_get(void);
EAPI Enesim_Text_Engine * enesim_text_freetype_get(void);
EAPI void enesim_text_engine_delete(Enesim_Text_Engine *e);

typedef enum _Enesim_Text_Direction
{
	ENESIM_TEXT_DIRECTION_LTR,
	ENESIM_TEXT_DIRECTION_RTL,
} Enesim_Text_Direction;

/**
 * @}
 * @defgroup Enesim_Text_Font_Group Enesim Text Font
 * @{
 */

typedef struct _Enesim_Text_Font Enesim_Text_Font;
/* TODO export more font/glyph functions */

/**
 * @}
 * @defgroup Enesim_Text_Buffer_Group Enexim Text Buffer
 * @{
 */

typedef struct _Enesim_Text_Buffer Enesim_Text_Buffer;

typedef void (*Enesim_Text_Buffer_String_Set)(void *data, const char *string, int length);
typedef const char * (*Enesim_Text_Buffer_String_Get)(void *data);
typedef int (*Enesim_Text_Buffer_String_Insert)(void *data, const char *string, int length, ssize_t offset);
typedef int (*Enesim_Text_Buffer_String_Delete)(void *data, int length, ssize_t offset);
typedef int (*Enesim_Text_Buffer_String_Length)(void *data);
typedef void (*Enesim_Text_Buffer_Free)(void *data);

typedef struct _Enesim_Text_Buffer_Descriptor
{
	Enesim_Text_Buffer_String_Get string_get;
	Enesim_Text_Buffer_String_Set string_set;
	Enesim_Text_Buffer_String_Insert string_insert;
	Enesim_Text_Buffer_String_Delete string_delete;
	Enesim_Text_Buffer_String_Length string_length;
	Enesim_Text_Buffer_Free free;
} Enesim_Text_Buffer_Descriptor;

EAPI Enesim_Text_Buffer * enesim_text_buffer_new(int initial_length);
EAPI Enesim_Text_Buffer * enesim_text_buffer_new_from_descriptor(Enesim_Text_Buffer_Descriptor *descriptor, void *data);
EAPI void enesim_text_buffer_string_set(Enesim_Text_Buffer *b, const char *string, int length);
EAPI const char * enesim_text_buffer_string_get(Enesim_Text_Buffer *b);
EAPI int enesim_text_buffer_string_insert(Enesim_Text_Buffer *b, const char *string, int length, ssize_t offset);
EAPI int enesim_text_buffer_string_delete(Enesim_Text_Buffer *b, int length, ssize_t offset);
EAPI void enesim_text_buffer_delete(Enesim_Text_Buffer *b);
EAPI int enesim_text_buffer_string_length(Enesim_Text_Buffer *b);

/**
 * @}
 */

#endif /*_ENESIM_TEXT_H*/
