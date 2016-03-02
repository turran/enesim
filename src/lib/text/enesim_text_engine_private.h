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

#ifndef _ENESIM_TEXT_ENGINE_PRIVATE_H
#define _ENESIM_TEXT_ENGINE_PRIVATE_H

#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_text_font_private.h"

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
	Eina_Bool (*glyph_load)(void *data, void *fdata, Enesim_Text_Glyph *g, int formats);
} Enesim_Text_Engine_Descriptor;

Enesim_Text_Engine * enesim_text_engine_new(Enesim_Text_Engine_Descriptor *d);
void enesim_text_engine_free(Enesim_Text_Engine *thiz);
Enesim_Text_Font * enesim_text_engine_font_new(
		Enesim_Text_Engine *thiz, const char *file, int index, int size);
void enesim_text_engine_font_delete(Enesim_Text_Engine *thiz,
		Enesim_Text_Font *f);
int enesim_text_engine_font_max_ascent_get(Enesim_Text_Engine *thiz,
		Enesim_Text_Font *f);
int enesim_text_engine_font_max_descent_get(Enesim_Text_Engine *thiz,
		Enesim_Text_Font *f);
Enesim_Text_Glyph * enesim_text_engine_font_glyph_get(Enesim_Text_Engine *thiz,
		Enesim_Text_Font *f, Eina_Unicode c);
Eina_Bool enesim_text_engine_glyph_load(Enesim_Text_Engine *thiz,
		Enesim_Text_Glyph *g, int formats);

struct _Enesim_Text_Engine
{
	Enesim_Object_Instance parent;
	Enesim_Text_Engine_Descriptor *d;
	Eina_Hash *fonts;
	int ref;
	void *data;
};

typedef struct _Enesim_Text_Engine_Class
{
	Enesim_Object_Class parent;
} Enesim_Text_Engine_Class;

#endif
