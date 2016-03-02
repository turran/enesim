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

/* forward declarations */
typedef struct _Enesim_Text_Glyph Enesim_Text_Glyph;

typedef struct _Enesim_Text_Font
{
	Enesim_Object_Instance parent;
	Enesim_Text_Engine *engine;
	void *data;
	Eina_Hash *glyphs;
	char *key;
	int ref;
} Enesim_Text_Font;

typedef struct _Enesim_Text_Font_Class
{
	Enesim_Object_Class parent;
} Enesim_Text_Font_Class;

Enesim_Text_Glyph * enesim_text_font_glyph_get(Enesim_Text_Font *f, Eina_Unicode c);
void enesim_text_font_dump(Enesim_Text_Font *f, const char *path);

#endif
