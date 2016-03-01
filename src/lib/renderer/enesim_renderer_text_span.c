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
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_path.h"
#include "enesim_text.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_text_span.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_color_private.h"
#include "enesim_color_fill_private.h"
#include "enesim_text_private.h"
#include "enesim_list_private.h"
#include "enesim_coord_private.h"
#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"

/**
 * TODO
 * - Handle the shape properties, use a path renderer
 * - Use a compound renderer with multiple image renderers
 * - Add a background/foreground color. Use a rectangle renderer for that
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_LOG_DEFAULT enesim_log_renderer

#define ENESIM_RENDERER_TEXT_SPAN(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Text_Span,					\
		enesim_renderer_text_span_descriptor_get())

typedef struct _Enesim_Renderer_Text_Span_State
{
	/* our current smart text buffer */
	Enesim_Text_Buffer *buffer;
	struct {
		Enesim_Text_Font *font;
	} current, past;
	Eina_Bool changed : 1;
	Eina_Bool buffer_changed : 1;
	Eina_Bool glyphs_generated : 1;
} Enesim_Renderer_Text_Span_State;

typedef struct _Enesim_Renderer_Text_Span
{
	Enesim_Renderer_Shape parent;
	Enesim_Renderer_Text_Span_State state;
	Enesim_Text_Direction direction;
	/* sw drawing */
	Enesim_Matrix_F16p16 matrix;
	/* span geometry */
	unsigned int width;
	unsigned int height;
	/* max ascent/descent */
	int top;
	int bottom;
} Enesim_Renderer_Text_Span;

typedef struct _Enesim_Renderer_Text_Span_Class {
	Enesim_Renderer_Shape_Class parent;
} Enesim_Renderer_Text_Span_Class;

static Eina_Bool _enesim_renderer_text_span_font_changed(Enesim_Renderer_Text_Span *thiz)
{
	if (!thiz->state.changed)
		return EINA_FALSE;
	/* check that the current state is different from the past state */
	if (thiz->state.current.font != thiz->state.past.font)
		return EINA_TRUE;
	return EINA_FALSE;
}

static void _enesim_renderer_text_span_font_generate(Enesim_Renderer_Text_Span *thiz)
{
	/* first do the setup of the fonts */
	if (_enesim_renderer_text_span_font_changed(thiz))
	{
		thiz->state.changed = EINA_FALSE;
		thiz->state.glyphs_generated = EINA_FALSE;
	}
}

static Eina_Bool _enesim_renderer_text_span_glyphs_changed(Enesim_Renderer_Text_Span *thiz)
{
	if (enesim_text_buffer_smart_is_dirty(thiz->state.buffer) || thiz->state.buffer_changed)
		return EINA_TRUE;
	return EINA_FALSE;
}

/* TODO check that the glyph load was ok, if not, just break this step but return TRUE
 * so on the rendering we will load it directly
 */
static Eina_Bool _enesim_renderer_text_span_glyphs_generate(Enesim_Renderer_Text_Span *thiz)
{
	if (!thiz->state.current.font)
	{
		return EINA_FALSE;
	}
	/* we dont take into account the buffer_changed, given that we set it
	 * to true everytime we clear it
	 */
	if (enesim_text_buffer_smart_is_dirty(thiz->state.buffer) || !thiz->state.glyphs_generated)
	{
		Enesim_Text_Glyph *g = NULL;
		Eina_Unicode unicode;
		int iidx = 0;
		const char *text;
		int masc;
		int mdesc;
		unsigned int width = 0;
		
		text = enesim_text_buffer_string_get(thiz->state.buffer);
		while ((unicode = eina_unicode_utf8_next_get(text, &iidx)))
		{

			g = enesim_text_font_glyph_get(thiz->state.current.font, unicode);
			if (!g) continue;
			/* calculate the max len */
			width += g->x_advance;
		}
		/* remove the last advance, use only the glyph image size */
		if (g && g->surface)
		{
			int w, h;

			enesim_surface_size_get(g->surface, &w, &h);
			width -= g->x_advance - w;
		}
		/* set our own bounds */
		masc = enesim_text_font_max_ascent_get(thiz->state.current.font);
		mdesc = enesim_text_font_max_descent_get(thiz->state.current.font);
		//printf("masc %d mdesc %d\n", masc, mdesc);
		thiz->width = width;
		thiz->height = masc + mdesc;
		thiz->top = masc;
		thiz->bottom = mdesc;

		thiz->state.glyphs_generated = EINA_TRUE;
		thiz->state.buffer_changed = EINA_TRUE;
		enesim_text_buffer_smart_clear(thiz->state.buffer);
	}
	return EINA_TRUE;
}

/* load the font, load the glyphs and generate the bounds if needed */
static Eina_Bool _enesim_renderer_text_span_generate(Enesim_Renderer_Text_Span *thiz)
{
	_enesim_renderer_text_span_font_generate(thiz);
	return _enesim_renderer_text_span_glyphs_generate(thiz);
}

/* x and y must be inside the bounding box */
static inline Eina_Bool _enesim_renderer_text_span_get_glyph_at_ltr(
		Enesim_Renderer_Text_Span *thiz, Enesim_Text_Font *font,
		int x, int *next, Enesim_Text_Glyph_Position *position)
{
	Eina_Bool ret = EINA_FALSE;
	Eina_Unicode unicode;
	int iidx = 0;
	int idx = 0;
	int rcoord = 0;
	const char *text;

	text = enesim_text_buffer_string_get(thiz->state.buffer);
	while ((unicode = eina_unicode_utf8_next_get(text, &iidx)))
	{
		Enesim_Text_Glyph *g;
		//ERR("idx %d", idx);
		g = enesim_text_font_glyph_get(font, unicode);
		if (!g)
		{
			WRN("No such glyph for %08x", unicode);
			continue;
		}
		//ERR("x: %d y: %d rcoord: %d w: %d", x, y, rcoord, w);
		if (x >= rcoord && x < rcoord + g->x_advance)
		{
			position->glyph = g;
			position->index = idx;
			position->distance = rcoord;
			if (next) *next = iidx;
			ret = EINA_TRUE;
			break;
		}
		rcoord += g->x_advance;
		idx = iidx;
	}
	return ret;

}

#if 0
/* x and y must be inside the bounding box */
static inline Eina_Bool _enesim_renderer_text_span_get_glyph_at_rtl(Enesim_Renderer_Text_Span *thiz, Enesim_Text_Font *font, int x, int y, Enesim_Text_Glyph_Position *position)
{
	Eina_Bool ret = EINA_FALSE;
	int idx = 0;
	int rcoord = 0;
	const char *c;
	size_t len;
	const char *text;

	text = enesim_text_buffer_string_get(thiz->state.buffer);
	len = enesim_text_buffer_string_length(thiz->state.buffer);
	idx = len;
	for (c = text + len; c && c >= text; c--)
	{
		Enesim_Text_Glyph *g;
		int w, h;

		g = enesim_text_font_glyph_get(font, *c);
		if (!g)
		{
			WRN("No such glyph for %c", *c);
			continue;
		}
		if (!g->surface) goto advance;
		enesim_surface_size_get(g->surface, &w, &h);
		if (x >= rcoord && x <= rcoord + w)
		{
			position->glyph = g;
			position->index = idx;
			position->distance = thiz->width - rcoord;
			ret = EINA_TRUE;
			break;
		}
advance:
		rcoord += g->x_advance;
		idx--;
	}
	return ret;
}
#endif
/*----------------------------------------------------------------------------*
 *                            The drawing functions                           *
 *----------------------------------------------------------------------------*/
/* This might the worst case possible, we need to fetch at every pixel what glyph we are at
 * then fetch such pixel value and finally fill the destination surface
 */
static void _enesim_renderer_text_span_draw_affine(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Text_Span *thiz;
	Enesim_Text_Font *font;
	Eina_F16p16 xx, yy;
	double ox, oy;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;

	/* setup the affine coordinates */
	thiz = ENESIM_RENDERER_TEXT_SPAN(r);

	font = thiz->state.current.font;
	if (!font)
	{
		/* weird case ... it might be related to the multiple threads */
		return;
	}
	enesim_renderer_origin_get(r, &ox, &oy);
	enesim_coord_affine_setup(&xx, &yy, x, y, ox, oy, &thiz->matrix);

	while (dst < end)
	{
		Enesim_Text_Glyph_Position position;
		Enesim_Text_Glyph *g;
		Eina_F16p16 gxx, gyy;
		int w, h;
		uint32_t p0 = 0;
		int sx;
		uint32_t *src;
		size_t stride;

		sx = eina_f16p16_int_to(xx);

		/* Our sampling algo goes to the right, so increase by 1 */
		/* FIXME decide what to use, get() / load()? */
		if (!_enesim_renderer_text_span_get_glyph_at_ltr(thiz, font, sx + 1, NULL, &position))
		{
			goto next;
		}
		g = position.glyph;
		if (!g->surface) goto next;

		enesim_surface_size_get(g->surface, &w, &h);
		enesim_surface_sw_data_get(g->surface, (void **)&src, &stride);
		gxx = xx - eina_f16p16_int_from(position.distance);
		gyy = yy - eina_f16p16_int_from((thiz->top - g->origin));

		p0 = enesim_coord_sample_good_restrict(src, stride, w, h, gxx, gyy);
next:
		yy += thiz->matrix.yx;
		xx += thiz->matrix.xx;
		*dst++ = p0;
	}
}

static void _enesim_renderer_text_span_draw_ltr_identity(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Text_Span *thiz;
	Enesim_Text_Font *font;
	Enesim_Text_Glyph_Position position;
	Enesim_Text_Glyph *g;
	Eina_Unicode unicode;
	uint32_t *dst = ddata;
	uint32_t *end = dst + len;
	double ox, oy;
	int rx;
	int rlen;
	int idx;
	const char *text;
	Enesim_Color color;
	Enesim_Color fcolor;
	Enesim_Renderer *fpaint = NULL;
	uint32_t *fbuf = NULL, *buf = NULL;

	thiz = ENESIM_RENDERER_TEXT_SPAN(r);
	enesim_renderer_origin_get(r, &ox, &oy);
	y -= oy;
	x -= ox;

	fpaint = enesim_renderer_shape_fill_renderer_get(r);
	if (fpaint)
	{
		buf = alloca(sizeof(unsigned int) * len);
		fbuf = buf;
		enesim_renderer_sw_draw(fpaint, x, y, len, fbuf);
		enesim_renderer_unref(fpaint);
	}

	fcolor = enesim_renderer_shape_fill_color_get(r);
	color = enesim_renderer_color_get(r);
	if (color != ENESIM_COLOR_FULL)
		fcolor = enesim_color_mul4_sym(color, fcolor);

	font = thiz->state.current.font;
	if (!font)
	{
		/* weird case ... it might be related to the multiple threads */
		ERR("No font available?");
		return;
	}
	/* advance to the first character that is inside the x+len span */
	if (!_enesim_renderer_text_span_get_glyph_at_ltr(thiz, font, x, &idx, &position))
	{
		ERR("Can not get glyph at %d %d %d", x, y, len);
		return;
	}
	g = position.glyph;
	rx = x - position.distance;
	rlen = g->x_advance - rx < len ? g->x_advance - rx : len;

	/* draw the first char, for every char after this, we just advance */
	if (!g->surface)
	{
		memset(dst, 0, rlen * sizeof(uint32_t));
	}
	else
	{
		int w, h;
		int ry;

		enesim_surface_size_get(g->surface, &w, &h);
		ry = y - (thiz->top - g->origin);
		/* clear the in case we are out of the image vertical/horizontal bounds */
		if (ry < 0 || ry >= h || rx >= w)
		{
			memset(dst, 0, rlen * sizeof(uint32_t));
		}
		else
		{
			uint32_t *mask;
			size_t stride;
			int slen;

			slen = w - rx < rlen ? w - rx : rlen;
			enesim_surface_sw_data_get(g->surface, (void **)&mask, &stride);
			mask = enesim_color_at(mask, stride, rx, ry);
			if (fpaint)
			{
				if (fcolor != 0xffffffff)
					enesim_color_fill_sp_argb8888_color_argb8888_alpha(dst, slen, fbuf, fcolor, mask);
				else
					enesim_color_fill_sp_argb8888_none_argb8888_alpha(dst, slen, fbuf, mask);
			}
			else
				enesim_color_fill_sp_none_color_argb8888_alpha(dst, slen, fcolor, mask);
			/* clear the area that we have not drawn on the horizontal bounds */
			if (slen < rlen)
			{
				memset(dst + slen, 0, (rlen - slen) * sizeof(uint32_t));
			}
		}
	}
	dst += rlen;
	len -= rlen;
	fbuf += rlen;

	/* now draw the following chars */
	text = enesim_text_buffer_string_get(thiz->state.buffer);
	while (dst < end && (unicode = eina_unicode_utf8_next_get(text, &idx)))
	{
		g = enesim_text_font_glyph_get(font, unicode);
		if (!g) continue;
		if (!g->surface)
		{
			rlen = g->x_advance < len ? g->x_advance : len;
			memset(dst, 0, rlen * sizeof(uint32_t));
		}
		else
		{
			int w, h;
			int ry;
			uint32_t *mask;
			size_t stride;

			enesim_surface_size_get(g->surface, &w, &h);

			ry = y - (thiz->top - g->origin);
			rlen = len < w ? len : w;

			/* clear the in case we are out of the image vertical bounds */
			if (ry < 0 || ry >= h)
			{
				memset(dst, 0, rlen * sizeof(uint32_t));
				goto after;
			}

			enesim_surface_sw_data_get(g->surface, (void **)&mask, &stride);
			mask = enesim_color_at(mask, stride, 0, ry);
			if (fpaint)
			{
				if (fcolor != 0xffffffff)
					enesim_color_fill_sp_argb8888_color_argb8888_alpha(dst, rlen, fbuf, fcolor, mask);
				else
					enesim_color_fill_sp_argb8888_none_argb8888_alpha(dst, rlen, fbuf, mask);
			}
			else
				enesim_color_fill_sp_none_color_argb8888_alpha(dst, rlen, fcolor, mask);
			/* clear the area that we have not drawn on the horizontal bounds */
after:
			if (w < g->x_advance)
			{
				int rend;

				rend = g->x_advance - w;
				rend = rlen + rend < len ? rend : len - rlen;
				memset(dst + rlen, 0, rend * sizeof(uint32_t));
			}
		}
		/* finally advance */
		dst += g->x_advance;
		len -= g->x_advance;
		fbuf += g->x_advance;
	}
}

/*----------------------------------------------------------------------------*
 *                             Shape interface                                *
 *----------------------------------------------------------------------------*/
static Eina_Bool _enesim_renderer_text_span_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Text_Span *thiz;
	Enesim_Matrix_Type type;
	Enesim_Matrix matrix, inv;
	const char *text;

	thiz = ENESIM_RENDERER_TEXT_SPAN(r);
	if (!thiz) return EINA_FALSE;

	text = enesim_text_buffer_string_get(thiz->state.buffer);
	if (!text)
	{
		DBG("No text set");
		return EINA_FALSE;
	}
	if (!_enesim_renderer_text_span_generate(thiz))
		return EINA_FALSE;

	type = enesim_renderer_transformation_type_get(r);
	switch (type)
	{
		case ENESIM_MATRIX_TYPE_IDENTITY:
		*fill = _enesim_renderer_text_span_draw_ltr_identity;
		break;

		case ENESIM_MATRIX_TYPE_AFFINE:
		enesim_renderer_transformation_get(r, &matrix);
		enesim_matrix_inverse(&matrix, &inv);
		enesim_matrix_matrix_f16p16_to(&inv,
			&thiz->matrix);
		*fill = _enesim_renderer_text_span_draw_affine;
		break;

		default:
		break;
	}

#if 0
	enesim_text_font_dump(thiz->state.current.font, "/tmp/");
#endif

	return EINA_TRUE;
}

static void _enesim_renderer_text_span_sw_cleanup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Renderer_Text_Span *thiz;

	thiz = ENESIM_RENDERER_TEXT_SPAN(r);
	/* swap the states */
	if (thiz->state.past.font)
	{
		enesim_text_font_unref(thiz->state.past.font);
		thiz->state.past.font = NULL;
	}
	if (thiz->state.current.font)
	{
		thiz->state.past.font = enesim_text_font_ref(thiz->state.current.font);
	}
	/* clear the text buffer */
	enesim_text_buffer_smart_clear(thiz->state.buffer);
	/* clear our flags */
	thiz->state.buffer_changed = EINA_FALSE;
}

static Eina_Bool _enesim_renderer_text_span_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Text_Span *thiz;

	thiz = ENESIM_RENDERER_TEXT_SPAN(r);
	if (_enesim_renderer_text_span_font_changed(thiz))
		return EINA_TRUE;
	if (_enesim_renderer_text_span_glyphs_changed(thiz))
		return EINA_TRUE;
	return EINA_FALSE;
}

static Eina_Bool _enesim_renderer_text_span_geometry_get(Enesim_Renderer *r,
		Enesim_Rectangle *geometry)
{
	Enesim_Renderer_Text_Span *thiz;
	double ox, oy;

	thiz = ENESIM_RENDERER_TEXT_SPAN(r);
	/* we should calculate the current width/height */
	_enesim_renderer_text_span_generate(thiz);
	geometry->x = 0;
	geometry->y = 0;
	geometry->w = thiz->width;
	geometry->h = thiz->height;
	/* first translate */
	enesim_renderer_origin_get(r, &ox, &oy);
	geometry->x += ox;
	geometry->y += oy;

	return EINA_TRUE;
}
/*----------------------------------------------------------------------------*
 *                             Renderer interface                             *
 *----------------------------------------------------------------------------*/
static const char * _enesim_renderer_text_span_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "text_span";
}

static Eina_Bool _enesim_renderer_text_span_bounds(Enesim_Renderer *r,
		Enesim_Rectangle *rect, Enesim_Log **log EINA_UNUSED)
{
	Enesim_Matrix_Type type;

	_enesim_renderer_text_span_geometry_get(r, rect);
	type = enesim_renderer_transformation_type_get(r);
	/* transform the geometry */
	if (type != ENESIM_MATRIX_TYPE_IDENTITY)
	{
		Enesim_Quad q;
		Enesim_Matrix m;

		enesim_renderer_transformation_get(r, &m);
		enesim_matrix_rectangle_transform(&m, rect, &q);
		enesim_quad_rectangle_to(&q, rect);
	}
	return EINA_TRUE;
}

static void _enesim_renderer_text_span_features_get(Enesim_Renderer *r EINA_UNUSED,
		int *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_ARGB8888 |
			ENESIM_RENDERER_FEATURE_AFFINE;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_SHAPE_DESCRIPTOR,
		Enesim_Renderer_Text_Span, Enesim_Renderer_Text_Span_Class,
		enesim_renderer_text_span);

static void _enesim_renderer_text_span_class_init(void *k)
{
	Enesim_Renderer_Class *r_klass;
	Enesim_Renderer_Shape_Class *klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _enesim_renderer_text_span_name;
	r_klass->bounds_get = _enesim_renderer_text_span_bounds;
	r_klass->features_get = _enesim_renderer_text_span_features_get;

	klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	klass->has_changed = _enesim_renderer_text_span_has_changed;
	klass->sw_setup = _enesim_renderer_text_span_sw_setup;
	klass->sw_cleanup = _enesim_renderer_text_span_sw_cleanup;
	klass->geometry_get = _enesim_renderer_text_span_geometry_get;
}

static void _enesim_renderer_text_span_instance_init(void *o)
{
	Enesim_Renderer_Text_Span *thiz;
	Enesim_Text_Buffer *real;

	thiz = ENESIM_RENDERER_TEXT_SPAN(o);
	real = enesim_text_buffer_utf8_new(0);
	thiz->state.buffer = enesim_text_buffer_smart_new(real);
}

static void _enesim_renderer_text_span_instance_deinit(void *o)
{
	Enesim_Renderer_Text_Span *thiz;

	thiz = ENESIM_RENDERER_TEXT_SPAN(o);
	if (thiz->state.current.font)
	{
		enesim_text_font_unref(thiz->state.current.font);
		thiz->state.current.font = NULL;
	}
	if (thiz->state.past.font)
	{
		free(thiz->state.past.font);
		thiz->state.past.font = NULL;
	}
	if (thiz->state.buffer)
	{
		enesim_text_buffer_unref(thiz->state.buffer);
		thiz->state.buffer = NULL;
	}
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

/**
 * Creates a text span renderer
 * @return The new renderer
 */
EAPI Enesim_Renderer * enesim_renderer_text_span_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_text_span);
	return r;
}

/**
 * @brief Sets the text of the text span
 * @param[in] r The text span renderer to set the text to
 * @param[in] str The text span text
 */
EAPI void enesim_renderer_text_span_text_set(Enesim_Renderer *r, const char *str)
{
	Enesim_Renderer_Text_Span *thiz;

	thiz = ENESIM_RENDERER_TEXT_SPAN(r);
	enesim_text_buffer_string_set(thiz->state.buffer, str, -1);
}

EAPI const char * enesim_renderer_text_span_text_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Text_Span *thiz;

	thiz = ENESIM_RENDERER_TEXT_SPAN(r);
	return enesim_text_buffer_string_get(thiz->state.buffer);
}

EAPI void enesim_renderer_text_span_direction_set(Enesim_Renderer *r, Enesim_Text_Direction direction)
{
	Enesim_Renderer_Text_Span *thiz;

	thiz = ENESIM_RENDERER_TEXT_SPAN(r);
	if (!thiz) return;
	thiz->direction = direction;
}

EAPI Enesim_Text_Direction enesim_renderer_text_span_direction_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Text_Span *thiz;

	thiz = ENESIM_RENDERER_TEXT_SPAN(r);
	return thiz->direction;
}

EAPI Enesim_Text_Buffer * enesim_renderer_text_span_buffer_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Text_Span *thiz;

	thiz = ENESIM_RENDERER_TEXT_SPAN(r);
	return enesim_text_buffer_ref(thiz->state.buffer);
}

EAPI Enesim_Text_Buffer * enesim_renderer_text_span_real_buffer_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Text_Span *thiz;

	thiz = ENESIM_RENDERER_TEXT_SPAN(r);
	return enesim_text_buffer_smart_real_get(thiz->state.buffer);
}

EAPI void enesim_renderer_text_span_real_buffer_set(Enesim_Renderer *r, Enesim_Text_Buffer *b)
{
	Enesim_Renderer_Text_Span *thiz;

	thiz = ENESIM_RENDERER_TEXT_SPAN(r);
	enesim_text_buffer_smart_real_set(thiz->state.buffer, b);
}

EAPI Enesim_Text_Font * enesim_renderer_text_span_font_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Text_Span *thiz;

	thiz = ENESIM_RENDERER_TEXT_SPAN(r);
	if (thiz->state.current.font)
		return enesim_text_font_ref(thiz->state.current.font);
	else
		return NULL;
}

EAPI void enesim_renderer_text_span_font_set(Enesim_Renderer *r, Enesim_Text_Font *font)
{
	Enesim_Renderer_Text_Span *thiz;

	thiz = ENESIM_RENDERER_TEXT_SPAN(r);
	if (!thiz) return;
	if (thiz->state.current.font)
		enesim_text_font_unref(thiz->state.current.font);
	thiz->state.current.font = font;
	thiz->state.changed = EINA_TRUE;
}

EAPI Eina_Bool enesim_renderer_text_span_glyph_coord_at(Enesim_Renderer *r,
		int x, int y, int *index, int *start, int *end)
{
	Enesim_Renderer_Text_Span *thiz;
	Eina_Rectangle bounds;
	Eina_Rectangle cursor;
	Eina_Bool found = EINA_FALSE;
	Eina_Unicode unicode;
	int iidx = 0;
	int idx = 0;
	int rcoord = 0;
	const char *text;
	double ox, oy;

	if (!enesim_renderer_destination_bounds_get(r, &bounds, 0, 0, NULL))
		return EINA_FALSE;

	/* check that x, y is inside the bounding box */
	eina_rectangle_coords_from(&cursor, x, y, 1, 1);
	if (!eina_rectangles_intersect(&bounds, &cursor))
		return EINA_FALSE;

	thiz = ENESIM_RENDERER_TEXT_SPAN(r);
	/* iterate over every glyph and find the intersection */
	enesim_renderer_origin_get(r, &ox, &oy);
	y -= oy;
	x -= ox;

	text = enesim_text_buffer_string_get(thiz->state.buffer);
	while ((unicode = eina_unicode_utf8_next_get(text, &iidx)))
	{
		Enesim_Text_Glyph *g;
		int w, h;

		g = enesim_text_font_glyph_get(thiz->state.current.font, unicode);
		if (!g)
		{
			WRN("No such glyph for %08x", unicode);
			continue;
		}
		if (!g->surface) goto advance;
		/* check if the coord is inside the surface */
		enesim_surface_size_get(g->surface, &w, &h);
		w = g->x_advance < w ? g->x_advance : w;
		if (x >= (rcoord - 1) && x < rcoord + w)
		{
			if (index) *index = idx;
			if (start) *start = rcoord -1 + ox;
			if (end) *end = rcoord - 1 + g->x_advance + ox;
			found = EINA_TRUE;
			break;
		}
advance:
		rcoord += g->x_advance;
		idx++;
	}
	return found;
}

EAPI Eina_Bool enesim_renderer_text_span_glyph_index_at(Enesim_Renderer *r,
		int index, int *start, int *end)
{
	Enesim_Renderer_Text_Span *thiz;
	Eina_Bool found = EINA_FALSE;
	Eina_Unicode unicode;
	int iidx = 0;
	int idx = 0;
	int rcoord = 0;
	const char *text;

	thiz = ENESIM_RENDERER_TEXT_SPAN(r);

	/* make sure to load the glyphs first */
	_enesim_renderer_text_span_generate(thiz);
	text = enesim_text_buffer_string_get(thiz->state.buffer);
	/* iterate until we find the correct index, go adding the x advance */
	while ((unicode = eina_unicode_utf8_next_get(text, &iidx)))
	{
		Enesim_Text_Glyph *g;
		int w, h;

		g = enesim_text_font_glyph_get(thiz->state.current.font, unicode);
		if (!g)
		{
			WRN("No such glyph for %08x", unicode);
			continue;
		}
		if (!g->surface) goto advance;
		/* check if the coord is inside the surface */
		enesim_surface_size_get(g->surface, &w, &h);
		w = g->x_advance < w ? g->x_advance : w;
		if (index == idx)
		{
			double ox, oy;

			enesim_renderer_origin_get(r, &ox, &oy);
			if (start) *start = rcoord -1 + ox;
			if (end) *end = rcoord - 1 + g->x_advance + ox;
			found = EINA_TRUE;
			break;
		}
advance:
		rcoord += g->x_advance;
		idx++;
	}
	return found;
}
