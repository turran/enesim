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

#ifdef HAVE_FONTCONFIG
#include <fontconfig/fontconfig.h>
#endif
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
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

ENESIM_OBJECT_ABSTRACT_BOILERPLATE(ENESIM_OBJECT_DESCRIPTOR, Enesim_Text_Font,
		Enesim_Text_Font_Class, enesim_text_font);
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
static void _enesim_text_font_class_init(void *k EINA_UNUSED)
{
}

static void _enesim_text_font_instance_init(void *o)
{
	Enesim_Text_Font *thiz = o;

	thiz->glyphs = eina_hash_int32_new(NULL);
	thiz->ref = 1;
}

static void _enesim_text_font_instance_deinit(void *o)
{
	Enesim_Text_Font *thiz = o;

	enesim_text_engine_unref(thiz->engine);
	eina_hash_free(thiz->glyphs);
	free(thiz->key);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Text_Glyph * enesim_text_font_glyph_get(Enesim_Text_Font *thiz, Eina_Unicode c)
{
	Enesim_Text_Glyph *g;
	Enesim_Text_Font_Class *klass;

	if (!thiz)
		return NULL;
	g = eina_hash_find(thiz->glyphs, &c);
	if (g)
		return enesim_text_glyph_ref(g);
	klass = ENESIM_TEXT_FONT_CLASS_GET(thiz);
	if (klass->glyph_get)
	{
		g = klass->glyph_get(thiz, c);
		g->font = enesim_text_font_ref(thiz);
		g->code = c;
	}

	return g;
}

void enesim_text_font_cache(Enesim_Text_Font *thiz)
{
	if (!thiz->cache)
		enesim_text_engine_font_cache(thiz->engine,
				enesim_text_font_ref(thiz));
	enesim_text_font_unref(thiz);
	thiz->cache++;
}

void enesim_text_font_uncache(Enesim_Text_Font *thiz)
{
	thiz->cache--;
	if (!thiz->cache)
		enesim_text_engine_font_uncache(thiz->engine, thiz);
	enesim_text_font_unref(thiz);
}

void enesim_text_font_glyph_cache(Enesim_Text_Font *thiz, Enesim_Text_Glyph *g)
{
	eina_hash_add(thiz->glyphs, &g->code, g);
}

void enesim_text_font_glyph_uncache(Enesim_Text_Font *thiz, Enesim_Text_Glyph *g)
{
	eina_hash_del(thiz->glyphs, &g->code, g);
}

void enesim_text_font_dump(Enesim_Text_Font *thiz, const char *path)
{
	eina_hash_foreach(thiz->glyphs, _dump, path);
}
/** @endcond */
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

	font = enesim_text_engine_font_load(e, file, index, size);
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
	return enesim_text_engine_font_load(e, file, index, size);
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
		enesim_object_instance_free(ENESIM_OBJECT_INSTANCE(thiz));
	}
}

EAPI int enesim_text_font_max_ascent_get(Enesim_Text_Font *thiz)
{
	Enesim_Text_Font_Class *klass;

	klass = ENESIM_TEXT_FONT_CLASS_GET(thiz);
	if (klass->max_ascent_get)
		return klass->max_ascent_get(thiz);
	else
		return 0;
}

EAPI int enesim_text_font_max_descent_get(Enesim_Text_Font *thiz)
{
	Enesim_Text_Font_Class *klass;

	klass = ENESIM_TEXT_FONT_CLASS_GET(thiz);
	if (klass->max_descent_get)
		return klass->max_descent_get(thiz);
	else
		return 0;
}
