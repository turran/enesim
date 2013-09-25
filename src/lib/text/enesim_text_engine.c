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
#include "enesim_surface.h"
#include "enesim_stream.h"
#include "enesim_image.h"

#include "enesim_text.h"
#include "enesim_text_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_text

static void * _enesim_text_engine_font_load(Enesim_Text_Engine *thiz,
		const char *file, int index, int size)
{
	if (!thiz->d->font_load) return NULL;
	return thiz->d->font_load(thiz->data, file, index, size);
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Text_Engine * enesim_text_engine_new(Enesim_Text_Engine_Descriptor *d)
{
	Enesim_Text_Engine *thiz;

	thiz = calloc(1, sizeof(Enesim_Text_Engine));
	thiz->d = d;
	thiz->data = thiz->d->init();
	thiz->fonts = eina_hash_string_superfast_new(NULL);
	thiz->ref = 1;

	return thiz;
}

void enesim_text_engine_free(Enesim_Text_Engine *thiz)
{
	thiz->d->shutdown(thiz->data);
	eina_hash_free(thiz->fonts);
	free(thiz);
}

Enesim_Text_Font * enesim_text_engine_font_new(
		Enesim_Text_Engine *thiz, const char *file, int index, int size)
{
	Enesim_Text_Font *f;
	void *data;
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
	data = _enesim_text_engine_font_load(thiz, file, index, size);
	if (!data)
	{
		free(key);
		return NULL;
	}

	f = calloc(1, sizeof(Enesim_Text_Font));
	f->engine = enesim_text_engine_ref(thiz);
	f->data = data;
	f->key = key;
	f->glyphs = eina_hash_int32_new(NULL);
	eina_hash_add(thiz->fonts, key, f);
	enesim_text_font_ref(f);

	return f;
}

void enesim_text_engine_font_delete(Enesim_Text_Engine *thiz,
		Enesim_Text_Font *f)
{
	thiz->d->font_delete(thiz->data, f->data);
	eina_hash_del(thiz->fonts, f->key, f);
	enesim_text_engine_unref(thiz);
	free(f);
}

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
		thiz->d->shutdown(thiz->data);
	}
}

Enesim_Text_Engine * enesim_text_engine_default_get(void)
{
	return enesim_text_engine_freetype_get();
}
