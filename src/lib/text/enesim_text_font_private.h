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

#ifndef _ENESIM_TEXT_FONT_PRIVATE_H
#define _ENESIM_TEXT_FONT_PRIVATE_H

#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_text_glyph_private.h"

#define ENESIM_TEXT_FONT_DESCRIPTOR enesim_text_font_descriptor_get()
#define ENESIM_TEXT_FONT_CLASS(k) ENESIM_OBJECT_CLASS_CHECK(k, 		\
		Enesim_Text_Font_Class, ENESIM_TEXT_FONT_DESCRIPTOR)
#define ENESIM_TEXT_FONT_CLASS_GET(o) ENESIM_TEXT_FONT_CLASS(		\
		(ENESIM_OBJECT_INSTANCE(o))->klass)
#define ENESIM_TEXT_FONT(o) ENESIM_OBJECT_INSTANCE_CHECK(o, 		\
		Enesim_Text_Font, ENESIM_TEXT_FONT_DESCRIPTOR)

/* forward declarations */
typedef struct _Enesim_Text_Glyph Enesim_Text_Glyph;

typedef struct _Enesim_Text_Font
{
	Enesim_Object_Instance parent;
	Enesim_Text_Engine *engine;
	Eina_Hash *glyphs;
	char *key;
	int ref;
	int cache;
} Enesim_Text_Font;

typedef struct _Enesim_Text_Font_Class
{
	Enesim_Object_Class parent;
	int (*max_ascent_get)(Enesim_Text_Font *thiz);
	int (*max_descent_get)(Enesim_Text_Font *thiz);
	Enesim_Text_Glyph * (*glyph_get)(Enesim_Text_Font *thiz, Eina_Unicode c);
	Eina_Bool (*has_kerning)(Enesim_Text_Font *thiz);
} Enesim_Text_Font_Class;

Enesim_Object_Descriptor * enesim_text_font_descriptor_get(void);
Enesim_Text_Glyph * enesim_text_font_glyph_get(Enesim_Text_Font *f, Eina_Unicode c);
Eina_Bool enesim_text_font_has_kerning(Enesim_Text_Font *f);
void enesim_text_font_glyph_cache(Enesim_Text_Font *thiz, Enesim_Text_Glyph *g);
void enesim_text_font_glyph_uncache(Enesim_Text_Font *thiz, Enesim_Text_Glyph *g);
void enesim_text_font_dump(Enesim_Text_Font *f, const char *path);

#endif
