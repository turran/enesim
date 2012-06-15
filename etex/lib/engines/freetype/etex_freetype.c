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

#include "Etex.h"
#include "etex_private.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Etex_Freetype_Glyph
{
	FT_GlyphSlot glyph;
	Enesim_Buffer_Sw_Data *data;
} Etex_Freetype_Glyph;


static void * _etex_freetype_init(void)
{
	static FT_Library library = NULL;

	if (!library) FT_Init_FreeType(&library);

	return library;
}

static void _etex_freetype_shutdown(void *data)
{
	FT_Library library = data;

	FT_Done_FreeType(library);
}

static Etex_Engine_Font_Data _etex_freetype_font_load(Etex_Engine_Data data, const char *name, int size)
{
	FT_Library library = data;
	FT_Face face;
	FT_Error error;

	error = FT_New_Face(library, name, 0, &face);
	if (error) return NULL;
	FT_Set_Pixel_Sizes(face, size, size);

	return face;
}

static void _etex_freetype_font_delete(Etex_Engine_Data data, Etex_Engine_Font_Data fdata)
{

}

static int _etex_freetype_font_max_ascent_get(Etex_Engine_Data data, Etex_Engine_Font_Data fdata)
{
	FT_Face face = fdata;
	int asc;
	int dv;

	asc = face->bbox.yMax;
	if (face->units_per_EM == 0) return asc;

	dv = 2048;
	asc = (asc * face->size->metrics.y_scale) / (dv * dv);

	return asc;
}

static int _etex_freetype_font_max_descent_get(Etex_Engine_Data data, Etex_Engine_Font_Data fdata)
{
	FT_Face face = fdata;
	int desc;
	int dv;

	desc = -face->bbox.yMin;
	if (face->units_per_EM == 0) return desc;

	dv = 2048;
	desc = (desc * face->size->metrics.y_scale) / (dv * dv);

	return desc;
}

static void _raster_callback(const int y,
               const int count,
               const FT_Span * const spans,
               void * const user)
{
	Etex_Freetype_Glyph *efg = (Etex_Freetype_Glyph *)user;
	int width;
	int height;
	int i;
	int ry;
	uint32_t *yptr;

	/* get the real y */
	width = efg->glyph->metrics.width >> 6;
	height = efg->glyph->metrics.height >> 6;

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

static void _etex_freetype_glyph_get(Etex_Engine_Data data, Etex_Engine_Font_Data fdata, char c, Etex_Glyph *g)
{
	FT_UInt gindex;
	FT_Face face = fdata;
	FT_Library library = data;

	gindex = FT_Get_Char_Index(face, c);
	if (FT_Load_Glyph(face, gindex, FT_LOAD_NO_BITMAP) == 0)
	{
		if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
		{
			Etex_Freetype_Glyph efg;
			FT_Raster_Params params;
			unsigned int width, height;
			Enesim_Buffer_Sw_Data data;

			width = face->glyph->metrics.width >> 6;
			height = face->glyph->metrics.height >> 6;
			if (!width || !height) goto no_surface;
			data.argb8888_pre.plane0 = calloc(width * height, sizeof(uint32_t));
			data.argb8888_pre.plane0_stride = width * 4;

			efg.data = &data;
			efg.glyph = face->glyph;

			memset(&params, 0, sizeof(params));
			params.flags = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
			params.gray_spans = _raster_callback;
			params.user = &efg;
			//printf("getting glyph %c of size %d %d\n", c, width, height);
			FT_Outline_Render(library, &face->glyph->outline, &params);
			g->surface = enesim_surface_new_data_from(ENESIM_FORMAT_ARGB8888, width, height, EINA_FALSE, &data);
no_surface:
			g->origin = (face->glyph->metrics.horiBearingY >> 6);
			g->x_advance = (face->glyph->metrics.horiAdvance >> 6);
		}
	}
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Etex_Engine etex_freetype  = {
	.init = _etex_freetype_init,
	.shutdown = _etex_freetype_shutdown,
	.font_load = _etex_freetype_font_load,
	.font_delete = _etex_freetype_font_delete,
	.font_max_ascent_get = _etex_freetype_font_max_ascent_get,
	.font_max_descent_get = _etex_freetype_font_max_descent_get,
	.font_glyph_get = _etex_freetype_glyph_get,
};
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
