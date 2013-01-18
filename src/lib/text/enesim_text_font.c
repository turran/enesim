/* ETEX - Enesim's Text Renderer
 * Copyright (C) 2010 Jorge Luis Zapata
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

#if HAVE_CONFIG_H
# include <config.h>
#endif

#if HAVE_EMAGE
#include <Emage.h>
#endif

#include "Enesim_Text.h"
#include "enesim_text_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
struct _Enesim_Text_Font
{
	Etex *etex;
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
	f->etex->engine->font_delete(f->etex->data, f->data);
	eina_hash_del(f->etex->fonts, f->key, f);
}

static void _font_unref(Enesim_Text_Font *f)
{
	if (f->ref <= 0)
	{
		printf("unreffing error\n");
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

#if HAVE_EMAGE
static Eina_Bool _dump(const Eina_Hash *hash, const void *key, void *data, void *fdata)
{
	Enesim_Text_Glyph *g = (Enesim_Text_Glyph *)data;
	char c = *(int *)key;
	char fout[PATH_MAX];
	char *path = fdata;

	snprintf(fout, PATH_MAX, "%s/%c.png", path, c);
	enesim_image_file_save(fout, g->surface, NULL);
	return EINA_TRUE;
}
#endif
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Text_Font * enesim_text_font_load(Etex *e, const char *name, int size)
{
	Enesim_Text_Font *f;
	Enesim_Text_Engine_Font_Data data;
	char *key;
	size_t len;

	if (!name) return NULL;

	len = strlen(name) + 4;
	key = malloc(len); // 4 numbers
	snprintf(key, len, "%s%d", name, size);
	f = eina_hash_find(e->fonts, name);
	if (f)
	{
		_font_ref(f);
		free(key);
		return f;
	};
	data = e->engine->font_load(e->data, name, size);
	if (!data)
	{
		free(key);
		return NULL;
	}

	f = calloc(1, sizeof(Enesim_Text_Font));
	f->etex = e;
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
	return f->etex->engine->font_max_ascent_get(f->etex->data, f->data);
}

int enesim_text_font_max_descent_get(Enesim_Text_Font *f)
{
	if (!f) return 0;
	return f->etex->engine->font_max_descent_get(f->etex->data, f->data);
}

Enesim_Text_Glyph * enesim_text_font_glyph_get(Enesim_Text_Font *f, char c)
{
	Enesim_Text_Glyph *g;

	if (!f) return NULL;
	g = calloc(1, sizeof(Enesim_Text_Glyph));
	f->etex->engine->font_glyph_get(f->etex->data, f->data, c, g);

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
#if HAVE_EMAGE
	eina_hash_foreach(f->glyphs, _dump, path);
#endif
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

