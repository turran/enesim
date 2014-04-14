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

#ifdef HAVE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_text

static Eina_Bool _dump(const Eina_Hash *hash EINA_UNUSED, const void *key, void *data, void *fdata)
{
	Enesim_Text_Glyph *g = (Enesim_Text_Glyph *)data;
	Enesim_Buffer *b;
	char c = *(int *)key;
	char fout[PATH_MAX];
	char *path = fdata;

	b = enesim_surface_buffer_get(g->surface);
	snprintf(fout, PATH_MAX, "%s/%c.png", path, c);
	enesim_image_file_save(fout, b, NULL, NULL);
	enesim_buffer_unref(b);

	return EINA_TRUE;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Text_Glyph * enesim_text_font_glyph_get(Enesim_Text_Font *thiz, char c)
{
	Enesim_Text_Glyph *g;

	if (!thiz) return NULL;
	if (!thiz->engine->d->font_glyph_get) return NULL;

	g = calloc(1, sizeof(Enesim_Text_Glyph));
	thiz->engine->d->font_glyph_get(thiz->engine->data, thiz->data, c, g);

	return g;
}

Enesim_Text_Glyph * enesim_text_font_glyph_load(Enesim_Text_Font *thiz, char c)
{
	Enesim_Text_Glyph *g;
	int key;

	key = c;
	g = eina_hash_find(thiz->glyphs, &key);
	if (!g)
	{
		g = enesim_text_font_glyph_get(thiz, c);
		if (!g) goto end;
		eina_hash_add(thiz->glyphs, &key, g);
	}
	g->ref++;
end:
	return g;
}

void enesim_text_font_glyph_unload(Enesim_Text_Font *thiz, char c)
{
	Enesim_Text_Glyph *g;
	int key;

	key = c;
	g = eina_hash_find(thiz->glyphs, &key);
	if (g)
	{
		g->ref--;
		if (!g->ref)
		{
			eina_hash_del(thiz->glyphs, &key, g);
			free(g);
		}
	}
}

void enesim_text_font_dump(Enesim_Text_Font *thiz, const char *path)
{
	eina_hash_foreach(thiz->glyphs, _dump, path);
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Text_Font * enesim_text_font_new_description_from(
		Enesim_Text_Engine *e, const char *description, int size)
{
#ifdef HAVE_FONTCONFIG
	FcPattern *pattern;
	FcPattern *match;
	FcResult result = FcResultMatch;
	const char *file;
	int index;
	Enesim_Text_Font *font = NULL;

	pattern = FcNameParse((FcChar8 *)description);
	if (!pattern)
		goto no_pattern;
	FcDefaultSubstitute(pattern);
	match = FcFontMatch(NULL, pattern, &result);
	if (result != FcResultMatch)
		goto no_match;
	if (FcPatternGetString(match, FC_FILE, 0, (FcChar8 **)&file) != FcResultMatch)
		goto no_prop;
	if (FcPatternGetInteger(match, FC_INDEX, 0, &index) != FcResultMatch)
		goto no_prop;

	font = enesim_text_engine_font_new(e, file, index, size);
no_prop:
	FcPatternDestroy(match);
no_match:
	FcPatternDestroy(pattern);
no_pattern:
	return font;
#else
	return NULL;
#endif
}

EAPI Enesim_Text_Font * enesim_text_font_new_file_from(
		Enesim_Text_Engine *e, const char *file, int index, int size)
{
	return enesim_text_engine_font_new(e, file, index, size);
}

EAPI Enesim_Text_Font * enesim_text_font_ref(Enesim_Text_Font *thiz)
{
	if (!thiz) return NULL;
	thiz->ref++;
	return thiz;
}

EAPI void enesim_text_font_unref(Enesim_Text_Font *thiz)
{
	if (!thiz) return;
	thiz->ref--;
	if (!thiz->ref)
	{
		enesim_text_engine_font_delete(thiz->engine, thiz);
		free(thiz->key);
		free(thiz);
	}
}

EAPI int enesim_text_font_max_ascent_get(Enesim_Text_Font *thiz)
{
	if (!thiz || !thiz->engine || !thiz->engine->d) return 0;
	if (!thiz->engine->d->font_max_ascent_get) return 0;
	return thiz->engine->d->font_max_ascent_get(thiz->engine->data, thiz->data);
}

EAPI int enesim_text_font_max_descent_get(Enesim_Text_Font *thiz)
{
	if (!thiz || !thiz->engine || !thiz->engine->d) return 0;
	if (!thiz->engine->d->font_max_descent_get) return 0;
	return thiz->engine->d->font_max_descent_get(thiz->engine->data, thiz->data);
}
