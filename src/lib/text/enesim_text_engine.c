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
	return thiz;
}

void enesim_text_engine_free(Enesim_Text_Engine *thiz)
{
	thiz->d->shutdown(thiz->data);
	eina_hash_free(thiz->fonts);
	free(thiz);
}

Enesim_Text_Engine_Font_Data enesim_text_engine_font_load(
		Enesim_Text_Engine *thiz, const char *name, int size)
{
	return thiz->d->font_load(thiz->data, name, size);
}

void enesim_text_engine_font_delete(Enesim_Text_Engine *thiz,
		Enesim_Text_Engine_Font_Data fdata)
{
	return thiz->d->font_delete(thiz->data, fdata);
}

int enesim_text_engine_font_max_ascent_get(Enesim_Text_Engine *thiz,
		Enesim_Text_Engine_Font_Data fdata)
{
	return thiz->d->font_max_ascent_get(thiz->data, fdata);
}

int enesim_text_engine_font_max_descent_get(Enesim_Text_Engine *thiz,
		Enesim_Text_Engine_Font_Data fdata)
{
	return thiz->d->font_max_descent_get(thiz->data, fdata);
}

void enesim_text_engine_font_glyph_get(Enesim_Text_Engine *thiz,
		Enesim_Text_Engine_Font_Data fdata, char c, Enesim_Text_Glyph *g)
{
	thiz->d->font_glyph_get(thiz->data, fdata, c, g);
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
#if 0
EAPI Enesim_Text_Font * enesim_text_engine_font_load_from_descriptrion(Enesim_Text_Engine *e, const char *description)
{
	/* TODO put here the fontconfig stuff */
}

EAPI Enesim_Text_Font * enesim_text_engine_font_load_from_file(Enesim_Text_Engine *e, const char *file, int size)
{
	/* TODO our old code */
}

#endif
