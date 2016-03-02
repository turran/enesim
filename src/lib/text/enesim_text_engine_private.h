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

#define ENESIM_TEXT_ENGINE_DESCRIPTOR enesim_text_engine_descriptor_get()
#define ENESIM_TEXT_ENGINE_CLASS(k) ENESIM_OBJECT_CLASS_CHECK(k, 		\
		Enesim_Text_Engine_Class, ENESIM_TEXT_ENGINE_DESCRIPTOR)
#define ENESIM_TEXT_ENGINE_CLASS_GET(o) ENESIM_TEXT_ENGINE_CLASS(		\
		(ENESIM_OBJECT_INSTANCE(o))->klass)
#define ENESIM_TEXT_ENGINE(o) ENESIM_OBJECT_INSTANCE_CHECK(o, 			\
		Enesim_Text_Engine, ENESIM_TEXT_ENGINE_DESCRIPTOR)

struct _Enesim_Text_Engine
{
	Enesim_Object_Instance parent;
	Eina_Hash *fonts;
	int ref;
};

typedef struct _Enesim_Text_Engine_Class
{
	Enesim_Object_Class parent;
	const char * (*type_get)(void);
	/* font interface */
	Enesim_Text_Font * (*font_load)(Enesim_Text_Engine *thiz,
			const char *name, int index, int size);
} Enesim_Text_Engine_Class;

Enesim_Object_Descriptor * enesim_text_engine_descriptor_get(void);
Enesim_Text_Font * enesim_text_engine_font_load(
		Enesim_Text_Engine *thiz, const char *file, int index, int size);
void enesim_text_engine_font_cache(Enesim_Text_Engine *thiz,
		Enesim_Text_Font *f);
void enesim_text_engine_font_uncache(Enesim_Text_Engine *thiz,
		Enesim_Text_Font *f);

#endif
