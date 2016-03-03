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
typedef struct _Enesim_Text_Engine_Freetype
{
	Enesim_Text_Engine parent;
	FT_Library library;
	Eina_Lock lock;
} Enesim_Text_Engine_Freetype;

typedef struct _Enesim_Text_Engine_Freetype_Class
{
	Enesim_Text_Engine_Class parent;
} Enesim_Text_Engine_Freetype_Class;

#define ENESIM_TEXT_ENGINE_FREETYPE(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Text_Engine_Freetype,					\
		enesim_text_engine_freetype_descriptor_get())

static Enesim_Text_Engine *_engine = NULL;

/*----------------------------------------------------------------------------*
 *                         The text engine interface                          *
 *----------------------------------------------------------------------------*/
static const char * _enesim_text_engine_freetype_type_get(void)
{
	return "enesim.text.engine.freetype";
}

static Enesim_Text_Font * _enesim_text_engine_freetype_font_load(
		Enesim_Text_Engine *e, const char *name, int index, int size)
{
	Enesim_Text_Engine_Freetype *thiz;
	Enesim_Text_Font *f;
	FT_Face face;
	FT_Error error;

 	thiz = ENESIM_TEXT_ENGINE_FREETYPE(e);
	eina_lock_take(&thiz->lock);
	error = FT_New_Face(thiz->library, name, index, &face);
	if (error)
	{
		ERR("Error %d loading font '%s' with size %d", error, name, size);
		eina_lock_release(&thiz->lock);
		return NULL;
	}
	FT_Set_Pixel_Sizes(face, size, size);
	eina_lock_release(&thiz->lock);

	f = enesim_text_font_freetype_new(face);

	return f;
}

/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_TEXT_ENGINE_DESCRIPTOR,
		Enesim_Text_Engine_Freetype, Enesim_Text_Engine_Freetype_Class,
		enesim_text_engine_freetype);

static void _enesim_text_engine_freetype_class_init(void *k)
{
	Enesim_Text_Engine_Class *klass = k;
	klass->font_load = _enesim_text_engine_freetype_font_load;
	klass->type_get = _enesim_text_engine_freetype_type_get;
}

static void _enesim_text_engine_freetype_instance_init(void *o)
{
	Enesim_Text_Engine_Freetype *thiz = o;

	FT_Init_FreeType(&thiz->library);
	eina_lock_new(&thiz->lock);
}

static void _enesim_text_engine_freetype_instance_deinit(void *o)
{
	Enesim_Text_Engine_Freetype *thiz = o;

	FT_Done_FreeType(thiz->library);
	eina_lock_free(&thiz->lock);
}
#endif
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_text_engine_freetype_init(void)
{
#if HAVE_FREETYPE
	_engine = ENESIM_OBJECT_INSTANCE_NEW(enesim_text_engine_freetype);
#endif
}

void enesim_text_engine_freetype_shutdown(void)
{
#if HAVE_FREETYPE
	enesim_text_engine_unref(_engine);
#endif
}

void enesim_text_engine_freetype_lock(Enesim_Text_Engine *e)
{
	Enesim_Text_Engine_Freetype *thiz;

	thiz = ENESIM_TEXT_ENGINE_FREETYPE(e);
	eina_lock_take(&thiz->lock);
}

void enesim_text_engine_freetype_unlock(Enesim_Text_Engine *e)
{
	Enesim_Text_Engine_Freetype *thiz;

	thiz = ENESIM_TEXT_ENGINE_FREETYPE(e);
	eina_lock_release(&thiz->lock);
}

FT_Library enesim_text_engine_freetype_lib_get(Enesim_Text_Engine *e)
{
	Enesim_Text_Engine_Freetype *thiz;

	thiz = ENESIM_TEXT_ENGINE_FREETYPE(e);
	return thiz->library;
}

/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Enesim_Text_Engine * enesim_text_engine_freetype_new(void)
{
#if HAVE_FREETYPE
	return enesim_text_engine_ref(_engine);
#else
	return NULL;
#endif
}
