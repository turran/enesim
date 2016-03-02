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
#include "enesim_private.h"

#include "enesim_main.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_stream.h"
#include "enesim_image.h"

#include "enesim_text.h"
#include "enesim_text_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_LOG_DEFAULT enesim_log_text

ENESIM_OBJECT_ABSTRACT_BOILERPLATE(ENESIM_OBJECT_DESCRIPTOR, Enesim_Text_Engine,
		Enesim_Text_Engine_Class, enesim_text_engine);
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
static void _enesim_text_engine_class_init(void *k EINA_UNUSED)
{
}

static void _enesim_text_engine_instance_init(void *o)
{
	Enesim_Text_Engine *thiz = o;
	thiz->fonts = eina_hash_string_superfast_new(NULL);
	thiz->ref = 1;
}

static void _enesim_text_engine_instance_deinit(void *o)
{
	Enesim_Text_Engine *thiz = o;
	eina_hash_free(thiz->fonts);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Text_Font * enesim_text_engine_font_load(
		Enesim_Text_Engine *thiz, const char *file, int index, int size)
{
	Enesim_Text_Font *f;
	Enesim_Text_Engine_Class *klass;
	char *key;
	int len;

	if (!file) return NULL;

	len = asprintf (&key, "%s-%d-%d", file, index, size);
	if (len <= 0 || !key) return NULL;

	f = eina_hash_find(thiz->fonts, key);
	if (f)
	{
		enesim_text_font_ref(f);
		free(key);
		return f;
	}
	klass = ENESIM_TEXT_ENGINE_CLASS_GET(thiz);
	if (klass->font_load)
	{
		f = klass->font_load(thiz, file, index, size);
		if (f)
		{
			f->engine = enesim_text_engine_ref(thiz);
			f->key = key;
		}
	}

	if (!f)
	{
		free(key);
		return NULL;
	}

	return f;
}

void enesim_text_engine_font_cache(Enesim_Text_Engine *thiz,
		Enesim_Text_Font *f)
{
	eina_hash_add(thiz->fonts, &f->key, f);
}

void enesim_text_engine_font_uncache(Enesim_Text_Engine *thiz,
		Enesim_Text_Font *f)
{
	eina_hash_del(thiz->fonts, &f->key, f);
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Text_Engine * enesim_text_engine_ref(Enesim_Text_Engine *thiz)
{
	if (!thiz) return NULL;
	thiz->ref++;
	return thiz;
}

EAPI void enesim_text_engine_unref(Enesim_Text_Engine *thiz)
{
	if (!thiz) return;
	thiz->ref--;
	if (!thiz->ref)
	{
		enesim_object_instance_free(ENESIM_OBJECT_INSTANCE(thiz));
	}
}

Enesim_Text_Engine * enesim_text_engine_default_get(void)
{
	return enesim_text_engine_freetype_new();
}

/**
 * @brief Get the type of a text engine 
 * @ender_downcast
 * @param[in] thiz The text engine to get the type from
 * @param[out] lib The ender library associated with this text engine
 * @param[out] name The ender item name of the text engine
 * @return EINA_TRUE if the function succeeds, EINA_FALSE otherwise
 *
 * This function is needed for ender in order to downcast a text engine
 */
EAPI Eina_Bool enesim_text_engine_type_get(Enesim_Text_Engine *thiz,
		const char **lib, const char **name)
{
	if (!thiz)
		return EINA_FALSE;
	if (lib)
		*lib = "enesim";
	if (name)
	{
		Enesim_Text_Engine_Class *klass;

		klass = ENESIM_TEXT_ENGINE_CLASS_GET(thiz);
		if (klass->type_get)
			*name = klass->type_get();
	}
	return EINA_TRUE;
}
