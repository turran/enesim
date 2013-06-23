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
#include "enesim_text.h"

#include "enesim_text_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_text

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
#if HAVE_FREETYPE
extern Enesim_Text_Engine_Descriptor enesim_text_freetype;
#endif

#if 0
/* TODO we need this for later, to create the cache for example */
void enesim_text_init(void)
{

}

void enesim_text_shutdown(void)
{

}
#endif
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To be documented
 * FIXME: To be fixed
 * TODO change this to _get()
 * add a refcount and on deletion check that
 */
EAPI Enesim_Text_Engine * enesim_text_freetype_get(void)
{
#if HAVE_FREETYPE
	static Enesim_Text_Engine *e = NULL;

	if (!e)
	{
		e = enesim_text_engine_new(&enesim_text_freetype);
	}
	return e;
#else
	return NULL;
#endif
}

/**
 * To be documented
 * FIXME: To be fixed
 * FIXME: remove this!
 */
EAPI void enesim_text_engine_delete(Enesim_Text_Engine *e)
{
#if HAVE_FREETYPE
	e->d->shutdown(e->data);
#endif
}

/**
 *
 */
Enesim_Text_Engine * enesim_text_engine_default_get(void)
{
#if HAVE_FREETYPE
	return enesim_text_freetype_get();
#endif
	return NULL;
}
