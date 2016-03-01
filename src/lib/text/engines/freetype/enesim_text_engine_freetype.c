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
static Enesim_Text_Engine *_engine = NULL;

#if 0
typedef struct _Enesim_Text_Freetype_Glyph
{
	Enesim_Text_Glyph parent;
} Enesim_Text_Freetype_Glyph;

typedef struct _Enesim_Text_Freetype_Glyph_Class
{
	Enesim_Text_Glyph_Class parent;
	FT_UInt gindex;
	/* load glyph */
} Enesim_Text_Freetype_Glyph_Class;

typedef struct _Enesim_Text_Freetype_Font
{
	Enesim_Text_Font parent;
	FT_Face face;
} Enesim_Text_Freetype_Font;

typedef struct _Enesim_Text_Freetype_Font_Class
{
	Enesim_Text_Font_Class parent;
	/* get glyph */
} Enesim_Text_Freetype_Font_Class;

typedef struct _Enesim_Text_Freetype_Engine
{
	Enesim_Text_Engine parent;
	FT_Library library;
	Eina_Lock lock;
} Enesim_Text_Freetype_Engine;

typedef struct _Enesim_Text_Freetype_Engine_Class
{
	Enesim_Text_Engine_Class parent;
	/* load_font */
} Enesim_Text_Freetype_Engine_Class;
#endif

#if HAVE_FREETYPE
typedef struct _Enesim_Text_Freetype
{
	FT_Library library;
	Eina_Lock lock;
} Enesim_Text_Freetype;

typedef struct _Enesim_Text_Freetype_Glyph
{
	FT_GlyphSlot glyph;
	Enesim_Buffer_Sw_Data *data;
} Enesim_Text_Freetype_Glyph;

static void _raster_callback(const int y,
               const int count,
               const FT_Span * const spans,
               void * const user)
{
	Enesim_Text_Freetype_Glyph *efg = (Enesim_Text_Freetype_Glyph *)user;
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
 *                        The Enesim's text interface                         *
 *----------------------------------------------------------------------------*/
static const char * _enesim_text_engine_freetype_type_get(void)
{
	return "enesim.text.engine.freetype";
}

static void * _enesim_text_engine_freetype_init(void)
{
	Enesim_Text_Freetype *thiz;

	thiz = calloc(1, sizeof(Enesim_Text_Freetype));
	FT_Init_FreeType(&thiz->library);
	eina_lock_new(&thiz->lock);
	
	return thiz;
}

static void _enesim_text_engine_freetype_shutdown(void *data)
{
	Enesim_Text_Freetype *thiz = data;

	FT_Done_FreeType(thiz->library);
	eina_lock_free(&thiz->lock);
	free(thiz);
}

static void *_enesim_text_engine_freetype_font_load(void *data, const char *name, int index, int size)
{
	Enesim_Text_Freetype *thiz = data;
	FT_Face face;
	FT_Error error;

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

	return face;
}

static void _enesim_text_engine_freetype_font_delete(void *data, void *fdata)
{
	Enesim_Text_Freetype *thiz = data;
	FT_Face face = fdata;

	eina_lock_take(&thiz->lock);
	FT_Done_Face(face);
	eina_lock_release(&thiz->lock);
}

static int _enesim_text_engine_freetype_font_max_ascent_get(void *data EINA_UNUSED, void *fdata)
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

static int _enesim_text_engine_freetype_font_max_descent_get(void *data EINA_UNUSED, void *fdata)
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

static Enesim_Text_Glyph * _enesim_text_engine_freetype_glyph_get(void *edata, void *fdata, Eina_Unicode c)
{
	Enesim_Text_Freetype *thiz = edata;
	Enesim_Text_Glyph *g = NULL;
	FT_UInt gindex;
	FT_Face face = fdata;

	eina_lock_take(&thiz->lock);
	gindex = FT_Get_Char_Index(face, c);
	if (!FT_Load_Glyph(face, gindex, FT_LOAD_NO_BITMAP))
	{
		g = enesim_text_glyph_new(fdata, c);
	}
	eina_lock_release(&thiz->lock);
	return g;
#if 0
	{
		if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
		{
			Enesim_Text_Freetype_Glyph efg;
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
			FT_Outline_Render(thiz->library, &face->glyph->outline, &params);
			g->surface = enesim_surface_new_data_from(
					ENESIM_FORMAT_ARGB8888, width, height,
					EINA_FALSE, gdata, width * 4, NULL,
					NULL);
no_surface:
			g->origin = (face->glyph->metrics.horiBearingY >> 6);
			g->x_advance = (face->glyph->metrics.horiAdvance >> 6);
			g->code = c;
		}
	}
#endif
}

static Eina_Bool _enesim_text_engine_freetype_glyph_load(void *edata, void *fdata, Enesim_Text_Glyph *g, int formats)
{
	Enesim_Text_Freetype *thiz = edata;
	eina_lock_take(&thiz->lock);
	eina_lock_release(&thiz->lock);
	return EINA_FALSE;
}

static Enesim_Text_Engine_Descriptor _enesim_text_engine_freetype_descriptor  = {
	/* .init 			= */ _enesim_text_engine_freetype_init,
	/* .shutdown 			= */ _enesim_text_engine_freetype_shutdown,
	/* .type_get 			= */ _enesim_text_engine_freetype_type_get,
	/* .font_load 			= */ _enesim_text_engine_freetype_font_load,
	/* .font_delete 		= */ _enesim_text_engine_freetype_font_delete,
	/* .font_max_ascent_get 	= */ _enesim_text_engine_freetype_font_max_ascent_get,
	/* .font_max_descent_get 	= */ _enesim_text_engine_freetype_font_max_descent_get,
	/* .font_glyph_get 		= */ _enesim_text_engine_freetype_glyph_get,
	/* .glyph_load 			= */ _enesim_text_engine_freetype_glyph_load,
};
#endif
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_text_engine_freetype_init(void)
{
#if HAVE_FREETYPE
	_engine = enesim_text_engine_new(&_enesim_text_engine_freetype_descriptor);
#endif
}

void enesim_text_engine_freetype_shutdown(void)
{
#if HAVE_FREETYPE
	enesim_text_engine_unref(_engine);
#endif
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

