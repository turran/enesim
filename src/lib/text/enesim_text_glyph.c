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
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_LOG_DEFAULT enesim_log_text

ENESIM_OBJECT_ABSTRACT_BOILERPLATE(ENESIM_OBJECT_DESCRIPTOR, Enesim_Text_Glyph,
		Enesim_Text_Glyph_Class, enesim_text_glyph);
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
static void _enesim_text_glyph_class_init(void *k EINA_UNUSED)
{
}

static void _enesim_text_glyph_instance_init(void *o)
{
	Enesim_Text_Glyph *thiz = o;

	thiz->ref = 1;
}

static void _enesim_text_glyph_instance_deinit(void *o EINA_UNUSED)
{
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Text_Glyph * enesim_text_glyph_ref(Enesim_Text_Glyph *thiz)
{
	if (!thiz)
		return NULL;
	thiz->ref++;
	return thiz;
}

void enesim_text_glyph_unref(Enesim_Text_Glyph *thiz)
{
	if (!thiz)
		return;
	thiz->ref--;
	if (!thiz->ref)
	{
		enesim_text_font_unref(thiz->font);
		if (thiz->surface)
		{
			enesim_surface_unref(thiz->surface);
			thiz->surface = NULL;
		}
		if (thiz->path)
		{
			enesim_path_unref(thiz->path);
			thiz->path = NULL;
		}
		free(thiz);
	}
}

void enesim_text_glyph_cache(Enesim_Text_Glyph *thiz)
{
	if (!thiz->cache)
		enesim_text_font_glyph_cache(thiz->font,
				enesim_text_glyph_ref(thiz));
	enesim_text_glyph_unref(thiz);
	thiz->cache++;
}

void enesim_text_glyph_uncache(Enesim_Text_Glyph *thiz)
{
	thiz->cache--;
	if (!thiz->cache)
		enesim_text_font_glyph_uncache(thiz->font, thiz);
	enesim_text_glyph_unref(thiz);
}

Eina_Bool enesim_text_glyph_load(Enesim_Text_Glyph *thiz,
		int formats)
{
	Enesim_Text_Glyph_Class *klass;

	if (thiz->path && (formats & ENESIM_TEXT_GLYPH_FORMAT_PATH))
		formats &= ~ENESIM_TEXT_GLYPH_FORMAT_PATH;
	if (thiz->surface && (formats & ENESIM_TEXT_GLYPH_FORMAT_SURFACE))
		formats &= ~ENESIM_TEXT_GLYPH_FORMAT_SURFACE;
	if (!formats)
		return EINA_TRUE;
	klass = ENESIM_TEXT_GLYPH_CLASS_GET(thiz);
	if (klass->load)
		return klass->load(thiz, formats);
	return EINA_FALSE;
}

double enesim_text_glyph_kerning_get(Enesim_Text_Glyph *thiz, Enesim_Text_Glyph *prev)
{
	Enesim_Text_Glyph_Class *klass;

	if (!thiz || !prev)
		return 0;
	klass = ENESIM_TEXT_GLYPH_CLASS_GET(thiz);
	if (klass->kerning_get)
		return klass->kerning_get(thiz, prev);
	return 0;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
