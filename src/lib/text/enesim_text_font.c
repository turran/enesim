/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2011 Jorge Luis Zapata
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
#include "enesim_image.h"

#include "enesim_text.h"
#include "enesim_text_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_text

struct _Enesim_Text_Font
{
	Enesim_Text_Engine *engine;
	Enesim_Text_Engine_Font_Data data;
	Eina_Hash *glyphs;
	char *key;
	int ref;
};

static void _font_ref(Enesim_Text_Font *f)
{
	f->ref++;
}

static void _font_unload(Enesim_Text_Font *f)
{
	f->engine->d->font_delete(f->engine->data, f->data);
	eina_hash_del(f->engine->fonts, f->key, f);
}

static void _font_unref(Enesim_Text_Font *f)
{
	if (f->ref <= 0)
	{
		ERR("Unreffing error");
		return;
	}

	f->ref--;
	if (!f->ref)
	{
		_font_unload(f);
		free(f->key);
		free(f);
	}
}

static Eina_Bool _dump(const Eina_Hash *hash EINA_UNUSED, const void *key, void *data, void *fdata)
{
	Enesim_Text_Glyph *g = (Enesim_Text_Glyph *)data;
	Enesim_Buffer *b;
	char c = *(int *)key;
	char fout[PATH_MAX];
	char *path = fdata;

	b = enesim_surface_buffer_get(g->surface);
	snprintf(fout, PATH_MAX, "%s/%c.png", path, c);
	enesim_image_file_save(fout, b, NULL);
	enesim_buffer_unref(b);

	return EINA_TRUE;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Text_Font * enesim_text_font_load(Enesim_Text_Engine *e, const char *name, int size)
{
	Enesim_Text_Font *f;
	Enesim_Text_Engine_Font_Data data;
	char *key;
	size_t len;

	if (!name) return NULL;

	len = strlen(name) + 4;
	key = malloc(len); // 4 numbers
	snprintf(key, len, "%s%d", name, size);
	f = eina_hash_find(e->fonts, key);
	if (f)
	{
		_font_ref(f);
		free(key);
		return f;
	}
	data = e->d->font_load(e->data, name, size);
	if (!data)
	{
		free(key);
		return NULL;
	}

	f = calloc(1, sizeof(Enesim_Text_Font));
	f->engine = e;
	f->data = data;
	f->key = key;
	f->glyphs = eina_hash_int32_new(NULL);
	eina_hash_add(e->fonts, key, f);
	_font_ref(f);

	return f;
}

void enesim_text_font_unref(Enesim_Text_Font *f)
{
	if (!f) return;
	_font_unref(f);
}

Enesim_Text_Font * enesim_text_font_ref(Enesim_Text_Font *f)
{
	if (!f) return NULL;
	_font_ref(f);
	return f;
}

int enesim_text_font_max_ascent_get(Enesim_Text_Font *f)
{
	if (!f) return 0;
	return f->engine->d->font_max_ascent_get(f->engine->data, f->data);
}

int enesim_text_font_max_descent_get(Enesim_Text_Font *f)
{
	if (!f) return 0;
	return f->engine->d->font_max_descent_get(f->engine->data, f->data);
}

Enesim_Text_Glyph * enesim_text_font_glyph_get(Enesim_Text_Font *f, char c)
{
	Enesim_Text_Glyph *g;

	if (!f) return NULL;
	g = calloc(1, sizeof(Enesim_Text_Glyph));
	f->engine->d->font_glyph_get(f->engine->data, f->data, c, g);

	return g;
}

Enesim_Text_Glyph * enesim_text_font_glyph_load(Enesim_Text_Font *f, char c)
{
	Enesim_Text_Glyph *g;
	int key;

	key = c;
	g = eina_hash_find(f->glyphs, &key);
	if (!g)
	{
		g = enesim_text_font_glyph_get(f, c);
		if (!g) goto end;
		eina_hash_add(f->glyphs, &key, g);
	}
	g->ref++;
end:
	return g;
}

void enesim_text_font_glyph_unload(Enesim_Text_Font *f, char c)
{
	Enesim_Text_Glyph *g;
	int key;

	key = c;
	g = eina_hash_find(f->glyphs, &key);
	if (g)
	{
		g->ref--;
		if (!g->ref)
		{
			eina_hash_del(f->glyphs, &key, g);
			free(g);
		}
	}
}

void enesim_text_font_dump(Enesim_Text_Font *f, const char *path)
{
	eina_hash_foreach(f->glyphs, _dump, path);
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
#if 0

EAPI Enesim_Text_Font * enesim_text_engine_font_ref(Enesim_Text_Font *thiz)
{

}

EAPI void enesim_text_engine_font_unref(Enesim_Text_Font *thiz)
{

}

#endif
