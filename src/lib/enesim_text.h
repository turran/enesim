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

#ifndef _ENESIM_TEXT_H
#define _ENESIM_TEXT_H

/**
 * @defgroup Enesim_Text Text
 * @brief Text and font definitions and operations
 * @todo
 * - Move the actual font/face loader into a submodule that should fill a common structure
 *   - freetype
 *   - move our own loader from efl-research here, for legacy code
 * - The possible cache we can have is:
 *   - FT_Face from memory (shared memory)
 *   - FT_Glyphs from memory?
 *   - Some kind of already rendered glyphs on a buffer
 *   - We can have a table per glyphs already renderered with a refcount on them
 * @{
 */

typedef struct _Enesim_Text_Engine Enesim_Text_Engine;
EAPI Enesim_Text_Engine * enesim_text_engine_default_get(void);
EAPI Enesim_Text_Engine * enesim_text_engine_freetype_get(void);
EAPI Enesim_Text_Engine * enesim_text_engine_ref(Enesim_Text_Engine *thiz);
EAPI void enesim_text_engine_unref(Enesim_Text_Engine *thiz);

typedef enum _Enesim_Text_Direction
{
	ENESIM_TEXT_DIRECTION_LTR,
	ENESIM_TEXT_DIRECTION_RTL,
} Enesim_Text_Direction;

/**
 * @}
 * @defgroup Enesim_Text_Font Font
 * @ingroup Enesim_Text
 * @{
 */

typedef struct _Enesim_Text_Font Enesim_Text_Font;
EAPI Enesim_Text_Font * enesim_text_font_new_description_from(
		Enesim_Text_Engine *e, const char *description,
		int size);
EAPI Enesim_Text_Font * enesim_text_font_new_file_from(
		Enesim_Text_Engine *e, const char *file, int index, int size);
EAPI Enesim_Text_Font * enesim_text_font_ref(Enesim_Text_Font *thiz);
EAPI void enesim_text_font_unref(Enesim_Text_Font *thiz);
EAPI int enesim_text_font_max_ascent_get(Enesim_Text_Font *thiz);
EAPI int enesim_text_font_max_descent_get(Enesim_Text_Font *thiz);

/**
 * @}
 * @defgroup Enesim_Text_Buffer Buffer
 * @ingroup Enesim_Text
 * @{
 */

typedef struct _Enesim_Text_Buffer Enesim_Text_Buffer;

EAPI Enesim_Text_Buffer * enesim_text_buffer_ref(Enesim_Text_Buffer *thiz);
EAPI void enesim_text_buffer_unref(Enesim_Text_Buffer *thiz);

EAPI void enesim_text_buffer_string_set(Enesim_Text_Buffer *thiz, const char *string, int length);
EAPI const char * enesim_text_buffer_string_get(Enesim_Text_Buffer *thiz);
EAPI int enesim_text_buffer_string_insert(Enesim_Text_Buffer *thiz, const char *string, int length, ssize_t offset);
EAPI int enesim_text_buffer_string_delete(Enesim_Text_Buffer *thiz, int length, ssize_t offset);
EAPI int enesim_text_buffer_string_length(Enesim_Text_Buffer *thiz);

/**
 * @}
 * @defgroup Enesim_Text_Buffer_Simple Simple Buffer
 * @ingroup Enesim_Text_Buffer
 * @{
 */

EAPI Enesim_Text_Buffer * enesim_text_buffer_simple_new(int initial_length);

/**
 * @}
 * @defgroup Enesim_Text_Buffer_Smart Smart Buffer
 * @ingroup Enesim_Text_Buffer
 * @{
 */

EAPI Enesim_Text_Buffer * enesim_text_buffer_smart_new(Enesim_Text_Buffer *thiz);
EAPI Enesim_Text_Buffer * enesim_text_buffer_smart_real_get(Enesim_Text_Buffer *b);
EAPI void enesim_text_buffer_smart_real_set(Enesim_Text_Buffer *b,
		Enesim_Text_Buffer *real);
EAPI void enesim_text_buffer_smart_dirty(Enesim_Text_Buffer *b);
EAPI void enesim_text_buffer_smart_clear(Enesim_Text_Buffer *b);
EAPI Eina_Bool enesim_text_buffer_smart_is_dirty(Enesim_Text_Buffer *b);

/**
 * @}
 */

#endif /*_ENESIM_TEXT_H*/
