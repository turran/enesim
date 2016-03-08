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
#include "enesim_text.h"
#include "enesim_text_private.h"

#if HAVE_FREETYPE
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#endif

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_LOG_DEFAULT enesim_log_text

#if HAVE_FREETYPE
typedef struct _Enesim_Text_Font_Freetype
{
	Enesim_Text_Font parent;
	FT_Face face;
} Enesim_Text_Font_Freetype;

typedef struct _Enesim_Text_Font_Freetype_Class
{
	Enesim_Text_Font_Class parent;
} Enesim_Text_Font_Freetype_Class;

#define ENESIM_TEXT_FONT_FREETYPE(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Text_Font_Freetype,					\
		enesim_text_font_freetype_descriptor_get())

/*----------------------------------------------------------------------------*
 *                          The text font interface                           *
 *----------------------------------------------------------------------------*/
static int _enesim_text_font_freetype_max_ascent_get(Enesim_Text_Font *f)
{
	Enesim_Text_Font_Freetype *thiz;
	int asc;
	int dv;

 	thiz = ENESIM_TEXT_FONT_FREETYPE(f);
	asc = thiz->face->bbox.yMax;
	if (thiz->face->units_per_EM == 0) return asc;

	dv = 2048;
	asc = (asc * thiz->face->size->metrics.y_scale) / (dv * dv);

	return asc;
}

static int _enesim_text_font_freetype_max_descent_get(Enesim_Text_Font *f)
{
	Enesim_Text_Font_Freetype *thiz;
	int desc;
	int dv;

 	thiz = ENESIM_TEXT_FONT_FREETYPE(f);
	desc = -thiz->face->bbox.yMin;
	if (thiz->face->units_per_EM == 0) return desc;

	dv = 2048;
	desc = (desc * thiz->face->size->metrics.y_scale) / (dv * dv);

	return desc;

}

static Enesim_Text_Glyph * _enesim_text_font_freetype_glyph_get(
		Enesim_Text_Font *f, Eina_Unicode c)
{
	Enesim_Text_Font_Freetype *thiz;
	Enesim_Text_Glyph *g = NULL;
	FT_UInt gindex;

 	thiz = ENESIM_TEXT_FONT_FREETYPE(f);
	enesim_text_engine_freetype_lock(f->engine);
	gindex = FT_Get_Char_Index(thiz->face, c);
	enesim_text_engine_freetype_unlock(f->engine);
	if (!gindex)
		return NULL;
	g = enesim_text_glyph_freetype_new(gindex);
	return g;
}

static Eina_Bool _enesim_text_font_freetype_has_kerning(Enesim_Text_Font *f)
{
	Enesim_Text_Font_Freetype *thiz;

 	thiz = ENESIM_TEXT_FONT_FREETYPE(f);
	return FT_HAS_KERNING(thiz->face);
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_TEXT_FONT_DESCRIPTOR,
		Enesim_Text_Font_Freetype, Enesim_Text_Font_Freetype_Class,
		enesim_text_font_freetype);

static void _enesim_text_font_freetype_class_init(void *k)
{
	Enesim_Text_Font_Class *klass = k;
	klass->max_ascent_get = _enesim_text_font_freetype_max_ascent_get;
	klass->max_descent_get = _enesim_text_font_freetype_max_descent_get;
	klass->glyph_get = _enesim_text_font_freetype_glyph_get;
	klass->has_kerning = _enesim_text_font_freetype_has_kerning;
}

static void _enesim_text_font_freetype_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_text_font_freetype_instance_deinit(void *o)
{
	Enesim_Text_Font_Freetype *thiz = o;
	/* TODO lock the engine, check that we still have it */
	FT_Done_Face(thiz->face);
}
#endif
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Text_Font * enesim_text_font_freetype_new(FT_Face face)
{
	Enesim_Text_Font_Freetype *thiz;
	Enesim_Text_Font *f;

	f = ENESIM_OBJECT_INSTANCE_NEW(enesim_text_font_freetype);
	thiz = ENESIM_TEXT_FONT_FREETYPE(f);
	thiz->face = face;

	return f;
}

FT_Face enesim_text_font_freetype_face_get(Enesim_Text_Font *f)
{
	Enesim_Text_Font_Freetype *thiz;

	thiz = ENESIM_TEXT_FONT_FREETYPE(f);
	return thiz->face;
}

/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
