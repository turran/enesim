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

static void _enesim_text_glyph_freetype_load_surface(Enesim_Text_Glyph *g,
		FT_GlyphSlot glyph)
{
	Enesim_Text_Glyph_Freetype_Load_Data efg;
	Enesim_Buffer_Sw_Data sdata;
	FT_Library lib;
	FT_Outline *outline = &glyph->outline;
	FT_Raster_Params params;
	unsigned int width, height;
	void *gdata;

	lib = enesim_text_engine_freetype_lib_get(g->font->engine);
	width = glyph->metrics.width >> 6;
	height = glyph->metrics.height >> 6;
	if (!width || !height)
		return;

	gdata = calloc(width * height, sizeof(uint32_t));

	sdata.argb8888_pre.plane0 = gdata;
	sdata.argb8888_pre.plane0_stride = width * 4;

	efg.data = &sdata;
	efg.glyph = glyph;

	memset(&params, 0, sizeof(params));
	params.flags = FT_RASTER_FLAG_AA | FT_RASTER_FLAG_DIRECT;
	params.gray_spans = _raster_callback;
	params.user = &efg;

	FT_Outline_Render(lib, outline, &params);
	g->surface = enesim_surface_new_data_from(
			ENESIM_FORMAT_ARGB8888, width, height,
			EINA_FALSE, gdata, width * 4, NULL,
			NULL);
}

static Eina_Bool _enesim_text_glyph_freetype_load_path(Enesim_Text_Glyph *g,
		FT_GlyphSlot glyph)
{
	FT_Outline *outline = &glyph->outline;
	FT_Vector v_last;
	FT_Vector v_control;
	FT_Vector v_start;
	FT_Vector *point;
	FT_Vector *limit;
	/* index of contour in outline */
	FT_Int n;
	/* index of first point in contour */
	FT_UInt first;
	/* current point's state */
	FT_Int tag;
	unsigned int height;
	char *tags;

	g->path = enesim_path_new();
	height = glyph->metrics.height >> 6;
	first = 0;
	for (n = 0; n < outline->n_contours; n++)
	{
		/* index of last point in contour */
		FT_Int last;

		last = outline->contours[n];
		if (last < 0)
			goto bad_outline;
		limit = outline->points + last;
		v_start = outline->points[first];
		v_last = outline->points[last];

		v_control = v_start;

		point = outline->points + first;
		tags = outline->tags + first;
		tag = FT_CURVE_TAG(tags[0]);

		/* A contour cannot start with a cubic control point! */
		if (tag == FT_CURVE_TAG_CUBIC)
			goto bad_outline;

		/* check first point to determine origin */
		if (tag == FT_CURVE_TAG_CONIC)
		{
			/* first point is conic control. Yes, this happens. */
			if (FT_CURVE_TAG(outline->tags[last]) == FT_CURVE_TAG_ON)
			{
				/* start at last point if it is on the curve */
				v_start = v_last;
				limit--;
			}
			else
			{
				/* if both first and last points are conic,
				 * start at their middle and record its position
				 * for closure 
				 */
				v_start.x = (v_start.x + v_last.x) / 2;
				v_start.y = (v_start.y + v_last.y) / 2;

				/* v_last = v_start; */
			}
			point--;
			tags--;
		}
		enesim_path_move_to(g->path, v_start.x / 64.0,
				v_start.y / 64.0 * -1 + height);
		while (point < limit)
		{
			point++;
			tags++;
			tag = FT_CURVE_TAG(tags[0]);
			switch (tag)
			{
				/* emit a single line_to */
				case FT_CURVE_TAG_ON:
				{
					enesim_path_line_to(g->path,
							point->x / 64.0,
							point->y / 64.0 * -1 + height);
					continue;
				}

				/* consume conic arcs */
				case FT_CURVE_TAG_CONIC:
				v_control.x = point->x;
				v_control.y = point->y;
do_conic:
				if (point < limit)
				{
					FT_Vector v_middle;

					point++;
					tags++;
					tag = FT_CURVE_TAG(tags[0]);
					if (tag == FT_CURVE_TAG_ON)
					{
						enesim_path_quadratic_to(g->path,
								v_control.x / 64.0,
								v_control.y / 64.0 * -1 + height,
								point->x / 64.0,
								point->y / 64.0 * -1 + height);
						continue;
					}
					if (tag != FT_CURVE_TAG_CONIC)
						goto bad_outline;

					v_middle.x = (v_control.x + point->x) / 2;
					v_middle.y = (v_control.y + point->y) / 2;
					enesim_path_quadratic_to(g->path,
							v_control.x / 64.0,
							v_control.y / 64.0 * -1 + height,
							v_middle.x / 64.0,
							v_middle.y / 64.0 * -1 + height);

					v_control = *point;
					goto do_conic;
				}

				enesim_path_quadratic_to(g->path,
						v_control.x / 64.0,
						v_control.y / 64.0 * -1 + height,
						v_start.x / 64.0,
						v_start.y / 64.0 * -1 + height);
				goto close;

				/* FT_CURVE_TAG_CUBIC */
				default:
				{
					FT_Vector vec1, vec2;

					if (point + 1 > limit || FT_CURVE_TAG(tags[1]) != FT_CURVE_TAG_CUBIC)
						goto bad_outline;

					point += 2;
					tags += 2;

					vec1.x = point[-2].x;
					vec1.y = point[-2].y;
					vec2.x = point[-1].x;
					vec2.y = point[-1].y;
					if (point <= limit)
					{
						FT_Vector vec;

						vec.x = point->x;
						vec.y = point->y;
						enesim_path_cubic_to(g->path,
								vec1.x / 64.0,
								vec1.y / 64.0 * -1 + height,
								vec2.x / 64.0,
								vec2.y / 64.0 * -1 + height,
								vec.x / 64.0,
								vec.y / 64.0 * -1 + height);
						continue;
					}
					enesim_path_cubic_to(g->path,
							vec1.x / 64.0,
							vec1.y / 64.0 * -1 + height,
							vec2.x / 64.0,
							vec2.y / 64.0,
							v_start.x / 64.0 * -1 + height,
							v_start.y / 64.0 * -1 + height);
					goto close;
				}
			}
		}

		/* close the contour */
		enesim_path_close(g->path);
close:
		first = last + 1;
	}

	return EINA_TRUE;

bad_outline:
	enesim_path_unref(g->path);
	g->path = NULL;
	return EINA_FALSE;
}

/*----------------------------------------------------------------------------*
 *                          The text glyph interface                          *
 *----------------------------------------------------------------------------*/
static Eina_Bool _enesim_text_glyph_freetype_load(Enesim_Text_Glyph *g,
		int formats)
{
	Enesim_Text_Glyph_Freetype *thiz;
	FT_Face face;

	enesim_text_engine_freetype_lock(g->font->engine);
	face = enesim_text_font_freetype_face_get(g->font);

	thiz = ENESIM_TEXT_GLYPH_FREETYPE(g);
	if (!FT_Load_Glyph(face, thiz->index, FT_LOAD_NO_BITMAP))
	{
		if (face->glyph->format == FT_GLYPH_FORMAT_OUTLINE)
		{
			g->origin = (face->glyph->metrics.horiBearingY >> 6);
			g->x_advance = (face->glyph->metrics.horiAdvance >> 6);
			if (formats & ENESIM_TEXT_GLYPH_FORMAT_SURFACE)
			{
				_enesim_text_glyph_freetype_load_surface(g, face->glyph);
			}

			if (formats & ENESIM_TEXT_GLYPH_FORMAT_PATH)
			{
				_enesim_text_glyph_freetype_load_path(g, face->glyph);
			}
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
