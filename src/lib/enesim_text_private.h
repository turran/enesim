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
#include "enesim_path.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"

/* freetype engine */
void enesim_text_engine_freetype_init(void);
void enesim_text_engine_freetype_shutdown(void);

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

/* font interface */
struct _Enesim_Text_Font
{
	Enesim_Text_Engine *engine;
	void *data;
	Eina_Hash *glyphs;
	char *key;
	int ref;
};

typedef struct _Enesim_Text_Glyph Enesim_Text_Glyph;
typedef struct _Enesim_Text_Glyph_Position Enesim_Text_Glyph_Position;

struct _Enesim_Text_Glyph
{
	/* the font associated with this glyph */
	Enesim_Text_Font *font;
	/* the surface associated with the glyph */
	Enesim_Surface *surface;
	/* the path associated with the glyph */
	Enesim_Path *path;
	/* the unicode char for this glyph */
	Eina_Unicode code;
	/* temporary */
	int origin;
	int x_advance;
	int ref;
	int cache;
};

struct _Enesim_Text_Glyph_Position
{
	int index; /**< The index on the string */
	int distance; /**< Number of pixels from the origin to the glyph */
	Enesim_Text_Glyph *glyph; /**< The glyph at this position */
};

typedef enum _Enesim_Text_Glyph_Format
{
	ENESIM_TEXT_GLYPH_FORMAT_SURFACE,
	ENESIM_TEXT_GLYPH_FORMAT_PATH,
} Enesim_Text_Glyph_Format;

Enesim_Text_Glyph * enesim_text_glyph_new(Enesim_Text_Font *f, Eina_Unicode c);
void enesim_text_glyph_ref(Enesim_Text_Glyph *thiz);
void enesim_text_glyph_unref(Enesim_Text_Glyph *thiz);
Eina_Bool enesim_text_glyph_load(Enesim_Text_Glyph *thiz, int formats);
void enesim_text_glyph_cache(Enesim_Text_Glyph *thiz);
void enesim_text_glyph_uncache(Enesim_Text_Glyph *thiz);

Enesim_Text_Glyph * enesim_text_font_glyph_get(Enesim_Text_Font *f, Eina_Unicode c);
void enesim_text_font_glyph_cache(Enesim_Text_Font *thiz, Enesim_Text_Glyph *g);
void enesim_text_font_glyph_uncache(Enesim_Text_Font *thiz, Enesim_Text_Glyph *g);

void enesim_text_font_dump(Enesim_Text_Font *f, const char *path);

/* engine interface */
typedef struct _Enesim_Text_Engine_Descriptor
{
	/* main interface */
	void *(*init)(void);
	void (*shutdown)(void *data);
	const char * (*type_get)(void);
	/* font interface */
	void *(*font_load)(void *data, const char *name, int index, int size);
	void (*font_delete)(void *data, void *fdata);
	int (*font_max_ascent_get)(void *data, void *fdata);
	int (*font_max_descent_get)(void *data, void *fdata);
	Enesim_Text_Glyph * (*font_glyph_get)(void *data, void *fdata, Eina_Unicode c);
} Enesim_Text_Engine_Descriptor;

Enesim_Text_Engine * enesim_text_engine_new(Enesim_Text_Engine_Descriptor *d);
void enesim_text_engine_free(Enesim_Text_Engine *thiz);
Enesim_Text_Font * enesim_text_engine_font_new(
		Enesim_Text_Engine *thiz, const char *file, int index, int size);
void enesim_text_engine_font_delete(Enesim_Text_Engine *thiz,
		Enesim_Text_Font *f);

/* main interface */
struct _Enesim_Text_Engine
{
	Enesim_Text_Engine_Descriptor *d;
	Eina_Hash *fonts;
	int ref;
	void *data;
};

void enesim_text_init(void);
void enesim_text_shutdown(void);

#endif
