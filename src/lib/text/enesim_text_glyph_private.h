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

#ifndef _ENESIM_TEXT_GLYPH_PRIVATE_H
#define _ENESIM_TEXT_GLYPH_PRIVATE_H

#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_path.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_text_font_private.h"

#define ENESIM_TEXT_GLYPH_DESCRIPTOR enesim_text_glyph_descriptor_get()
#define ENESIM_TEXT_GLYPH_CLASS(k) ENESIM_OBJECT_CLASS_CHECK(k, 		\
		Enesim_Text_Glyph_Class, ENESIM_TEXT_GLYPH_DESCRIPTOR)
#define ENESIM_TEXT_GLYPH_CLASS_GET(o) ENESIM_TEXT_GLYPH_CLASS(		\
		(ENESIM_OBJECT_INSTANCE(o))->klass)
#define ENESIM_TEXT_GLYPH(o) ENESIM_OBJECT_INSTANCE_CHECK(o, 		\
		Enesim_Text_Glyph, ENESIM_TEXT_GLYPH_DESCRIPTOR)

/* forward declarations */
typedef struct _Enesim_Text_Font Enesim_Text_Font;

typedef struct _Enesim_Text_Glyph
{
	Enesim_Object_Instance parent;
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
} Enesim_Text_Glyph;

typedef struct _Enesim_Text_Glyph_Class
{
	Enesim_Object_Class parent;
	/* load glyph */
	Eina_Bool (*load)(Enesim_Text_Glyph *thiz, int formats);
	double (*kerning_get)(Enesim_Text_Glyph *thiz, Enesim_Text_Glyph *prev);
} Enesim_Text_Glyph_Class;

typedef struct _Enesim_Text_Glyph_Position
{
	int index; /**< The index on the string */
	int distance; /**< Number of pixels from the origin to the glyph */
	Enesim_Text_Glyph *glyph; /**< The glyph at this position */
} Enesim_Text_Glyph_Position;

typedef enum _Enesim_Text_Glyph_Format
{
	ENESIM_TEXT_GLYPH_FORMAT_SURFACE = 1,
	ENESIM_TEXT_GLYPH_FORMAT_PATH    = 2,
} Enesim_Text_Glyph_Format;

Enesim_Object_Descriptor * enesim_text_glyph_descriptor_get(void);
Enesim_Text_Glyph * enesim_text_glyph_ref(Enesim_Text_Glyph *thiz);
double enesim_text_glyph_kerning_get(Enesim_Text_Glyph *thiz, Enesim_Text_Glyph *prev);
void enesim_text_glyph_unref(Enesim_Text_Glyph *thiz);
Eina_Bool enesim_text_glyph_load(Enesim_Text_Glyph *thiz, int formats);
void enesim_text_glyph_cache(Enesim_Text_Glyph *thiz);
void enesim_text_glyph_uncache(Enesim_Text_Glyph *thiz);

#endif
