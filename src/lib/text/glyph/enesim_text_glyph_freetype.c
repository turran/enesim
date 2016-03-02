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
#define ENESIM_TEXT_GLYPH_FREETYPE(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Text_Glyph_Freetype,					\
		enesim_text_glyph_freetype_descriptor_get())

typedef struct _Enesim_Text_Glyph_Freetype
{
	Enesim_Text_Glyph parent;
	FT_UInt index;
} Enesim_Text_Glyph_Freetype;

typedef struct _Enesim_Text_Glyph_Freetype_Class
{
	Enesim_Text_Glyph_Class parent;
} Enesim_Text_Glyph_Freetype_Class;

typedef struct _Enesim_Text_Glyph_Freetype_Load_Data
{
	FT_GlyphSlot glyph;
	Enesim_Buffer_Sw_Data *data;
} Enesim_Text_Glyph_Freetype_Load_Data;

static void _raster_callback(const int y,
               const int count,
               const FT_Span * const spans,
               void * const user)
{
	Enesim_Text_Glyph_Freetype_Load_Data *efg = (Enesim_Text_Glyph_Freetype_Load_Data *)user;
	int i;
	int ry;
	uint32_t *yptr;

#if 0
	int width;
	int height;
	/* get the real y */
	width = efg->glyph->metrics.width >> 6;
	height = efg->glyph->metrics.height >> 6;
#endif

	ry = (efg->glyph->metrics.horiBearingY >> 6) - y - 1;
	yptr = (uint32_t *)((uint8_t *)efg->data->argb8888_pre.plane0 + (ry * efg->data->argb8888_pre.plane0_stride));
	for (i = 0; i < count; i++)
	{
		int x;
		int rx;
		uint32_t *xptr;
		uint8_t a;

		/* get the real x */
		rx = spans[i].x - (efg->glyph->metrics.horiBearingX >> 6);
		a = spans[i].coverage;
		xptr = yptr + rx;
		for (x = 0; x < spans[i].len; x++)
		{
			*xptr = a << 24 | a << 16 | a << 8 | a;
			xptr++;
		}
	}
}
/*----------------------------------------------------------------------------*
 *                          The text glyph interface                          *
 *----------------------------------------------------------------------------*/
static Eina_Bool _enesim_text_glyph_freetype_load(Enesim_Text_Glyph *g,
		int formats)
{
	Enesim_Text_Glyph_Freetype *thiz;
	FT_Face face;
	FT_Library lib;

	enesim_text_engine_freetype_lock(g->font->engine);
	lib = enesim_text_engine_freetype_lib_get(g->font->engine);
	face = enesim_text_font_freetype_face_get(g->font);

	thiz = ENESIM_TEXT_GLYPH_FREETYPE(g);
	if (!FT_Load_Glyph(face, thiz->index, FT_LOAD_NO_BITMAP))
	{
		if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
		{
			Enesim_Text_Glyph_Freetype_Load_Data efg;
			Enesim_Buffer_Sw_Data sdata;
			FT_Raster_Params params;
			unsigned int width, height;
			void *gdata;

			width = face->glyph->metrics.width >> 6;
			height = face->glyph->metrics.height >> 6;
			if (!width || !height) goto no_surface;
			gdata = calloc(width * height, sizeof(uint32_t));

			sdata.argb8888_pre.plane0 = gdata;
			sdata.argb8888_pre.plane0_stride = width * 4;

			efg.data = &sdata;
			efg.glyph = face->glyph;

			memset(&params, 0, sizeof(params));
			params.flags = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
			params.gray_spans = _raster_callback;
			params.user = &efg;

			//printf("getting glyph %c of size %d %d\n", c, width, height);
			FT_Outline_Render(lib, &face->glyph->outline, &params);
			g->surface = enesim_surface_new_data_from(
					ENESIM_FORMAT_ARGB8888, width, height,
					EINA_FALSE, gdata, width * 4, NULL,
					NULL);
no_surface:
			g->origin = (face->glyph->metrics.horiBearingY >> 6);
			g->x_advance = (face->glyph->metrics.horiAdvance >> 6);
		}
	}
	enesim_text_engine_freetype_unlock(g->font->engine);
	return EINA_TRUE;
}

/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_TEXT_GLYPH_DESCRIPTOR,
		Enesim_Text_Glyph_Freetype, Enesim_Text_Glyph_Freetype_Class,
		enesim_text_glyph_freetype);

static void _enesim_text_glyph_freetype_class_init(void *k)
{
	Enesim_Text_Glyph_Class *klass = k;
	klass->load = _enesim_text_glyph_freetype_load;
}

static void _enesim_text_glyph_freetype_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_text_glyph_freetype_instance_deinit(void *o EINA_UNUSED)
{
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Text_Glyph * enesim_text_glyph_freetype_new(FT_UInt index)
{
	Enesim_Text_Glyph_Freetype *thiz;
	Enesim_Text_Glyph *g;

	g = ENESIM_OBJECT_INSTANCE_NEW(enesim_text_glyph_freetype);
	thiz = ENESIM_TEXT_GLYPH_FREETYPE(g);
	thiz->index = index;

	return g;
}
#endif
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
