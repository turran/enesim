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

#ifndef _ENESIM_TEXT_PRIVATE_H
#define _ENESIM_TEXT_PRIVATE_H

#include "enesim_text.h"

#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"

#include "enesim_text_engine_private.h"
#include "enesim_text_font_private.h"
#include "enesim_text_glyph_private.h"

#if HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#endif

/* text buffer interface */
typedef const char * (*Enesim_Text_Buffer_Type_Get)(void);
typedef void (*Enesim_Text_Buffer_String_Set)(void *data, const char *string, int length);
typedef const char * (*Enesim_Text_Buffer_String_Get)(void *data);
typedef int (*Enesim_Text_Buffer_String_Insert)(void *data, const char *string, int length, ssize_t offset);
typedef int (*Enesim_Text_Buffer_String_Delete)(void *data, int length, ssize_t offset);
typedef int (*Enesim_Text_Buffer_String_Length)(void *data);
typedef void (*Enesim_Text_Buffer_Free)(void *data);

typedef struct _Enesim_Text_Buffer_Descriptor
{
	Enesim_Text_Buffer_Type_Get type_get;
	Enesim_Text_Buffer_String_Get string_get;
	Enesim_Text_Buffer_String_Set string_set;
	Enesim_Text_Buffer_String_Insert string_insert;
	Enesim_Text_Buffer_String_Delete string_delete;
	Enesim_Text_Buffer_String_Length string_length;
	Enesim_Text_Buffer_Free free;
} Enesim_Text_Buffer_Descriptor;

Enesim_Text_Buffer * enesim_text_buffer_new_from_descriptor(Enesim_Text_Buffer_Descriptor *descriptor, void *data);
void * enesim_text_buffer_data_get(Enesim_Text_Buffer *thiz);

void enesim_text_init(void);
void enesim_text_shutdown(void);


/* freetype engine */
void enesim_text_engine_freetype_init(void);
void enesim_text_engine_freetype_shutdown(void);
#if HAVE_FREETYPE
void enesim_text_engine_freetype_lock(Enesim_Text_Engine *e);
void enesim_text_engine_freetype_unlock(Enesim_Text_Engine *e);
FT_Library enesim_text_engine_freetype_lib_get(Enesim_Text_Engine *e);

Enesim_Text_Font * enesim_text_font_freetype_new(FT_Face face);
FT_Face enesim_text_font_freetype_face_get(Enesim_Text_Font *f);

Enesim_Text_Glyph * enesim_text_glyph_freetype_new(FT_UInt index);
#endif

#endif
