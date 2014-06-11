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

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
#if 0
void enesim_text_cache_glyph_add(Enesim_Text_Glyph)
{

}

void enesim_text_cache_clear()
{

}

#endif

void enesim_text_init(void)
{
	enesim_text_engine_freetype_init();
#ifdef HAVE_FONTCONFIG
	FcInit();
#endif
}

void enesim_text_shutdown(void)
{
#ifdef HAVE_FONTCONFIG
	FcFini();
#endif
	enesim_text_engine_freetype_shutdown();
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
