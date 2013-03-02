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

#include "Enesim_Text.h"
#include "enesim_text_private.h"
/* for now we are using only chars, until we need utf8 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Text_Grid_Cell
{
	Enesim_Color background;
	Enesim_Color foreground;
	char c;
} Enesim_Text_Grid_Cell;

typedef struct _Enesim_Text_Grid
{
	Enesim_Text_Engine *engine;
	Enesim_Text_Font *font;
	unsigned int cell_width;
	unsigned int cell_height;
	unsigned int rows;
	unsigned int columns;
	Enesim_Text_Grid_Cell *cells;
} Enesim_Text_Grid;

static inline Enesim_Text_Grid * _enesim_text_grid_get(Enesim_Renderer *r)
{
	Enesim_Text_Grid *thiz;

	thiz = enesim_text_base_data_get(r);
	return thiz;
}

static inline Eina_Bool _enesim_text_grid_has_changed(Enesim_Renderer *r)
{
	Enesim_Text_Grid *thiz;
	Eina_Bool invalidate = EINA_FALSE;

	thiz = _enesim_text_grid_get(r);
	/* check that the current state is different from the old state */
	invalidate = enesim_text_base_has_changed(r);
	return invalidate;
}

static inline void _enesim_text_grid_update(Enesim_Renderer *r)
{
}

static inline Eina_Bool _enesim_text_grid_calculate(Enesim_Renderer *r)
{
	Enesim_Text_Grid *thiz;
	Enesim_Text_Font *font;
	Eina_Bool invalidate;
	int masc;
	int mdesc;

	invalidate = _enesim_text_grid_has_changed(r);
	if (invalidate) return EINA_TRUE;

	thiz = _enesim_text_grid_get(r);
	/* update our font */
	enesim_text_base_setup(r);
	font = enesim_text_base_font_get(r);
	if (!font) return EINA_FALSE;
	/* we should preload every glyph now? or better preload only an area?
	 * like whenever we want to draw some row we can preload the group
	 * of rows based on that and keep track of what has been updated
	 * and what not?
	 */

	enesim_text_base_max_ascent_get(r, &masc);
	enesim_text_base_max_descent_get(r, &mdesc);
	thiz->cell_height = masc + mdesc;
	if (thiz->font)
	{
		enesim_text_font_unref(thiz->font);
	}
	thiz->font = font;
	_enesim_text_grid_update(r);

	return EINA_TRUE;
}

static void _enesim_text_grid_draw_identity(Enesim_Renderer *r, int x, int y, unsigned int len, void *dst)
{
	/* on drawing a possibility is to always have a cell_width*cell_height allocated space
	 * then fill the background color and then blend on top of it the char
	 */
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _enesim_text_grid_name(Enesim_Renderer *r)
{
	return "enesim_text_grid";
}

static Eina_Bool _enesim_text_grid_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Enesim_Text_Grid *e;

	e = _enesim_text_grid_get(r);
	if (!e) return EINA_FALSE;
	if (!_enesim_text_grid_calculate(r))
		return EINA_FALSE;
	*fill = _enesim_text_grid_draw_identity;

	return EINA_TRUE;
}

static void _enesim_text_grid_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Text_Grid *thiz;

	thiz = _enesim_text_grid_get(r);
}

static void _enesim_text_grid_free(Enesim_Renderer *r)
{
	Enesim_Text_Grid *thiz;

	thiz = _enesim_text_grid_get(r);
	free(thiz);
}

static void _enesim_text_grid_bounds(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *rect)
{
	Enesim_Text_Grid *thiz;
	unsigned int size;

	thiz = _enesim_text_grid_get(r);

	_enesim_text_grid_calculate(r);
	rect->x = 0;
	rect->y = 0;
	rect->h = thiz->cell_height * thiz->rows;
	/* FIXME for now */
	enesim_text_base_size_get(r, &size);
	rect->w = thiz->cell_width * size;
}

static void _enesim_text_grid_features_get(Enesim_Renderer *r, Enesim_Renderer_Feature *features)
{
	Enesim_Text_Grid *thiz;

	thiz = _enesim_text_grid_get(r);
	if (!thiz)
	{
		*features = 0;
		return;
	}
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static Enesim_Renderer_Descriptor _enesim_text_grid_descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _enesim_text_grid_name,
	/* .free = 			*/ _enesim_text_grid_free,
	/* .bounds_get = 		*/ _enesim_text_grid_bounds,
	/* .destination_transform = 	*/ NULL,
	/* .features_get = 			*/ _enesim_text_grid_features_get,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ NULL,
	/* .sw_setup = 			*/ _enesim_text_grid_setup,
	/* .sw_cleanup = 		*/ _enesim_text_grid_cleanup,
};

static Enesim_Renderer * _enesim_text_grid_new(Enesim_Text_Engine *engine)
{
	Enesim_Text_Grid *thiz;
	Enesim_Renderer *r;

	thiz = calloc(1, sizeof(Enesim_Text_Grid));
	if (!thiz)
		return NULL;

	thiz->engine = engine;

	r = enesim_text_base_new(engine, &_enesim_text_grid_descriptor, thiz);
	if (!thiz) goto renderer_err;

	return r;

renderer_err:
	free(thiz);
	return NULL;
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Renderer * enesim_text_grid_new(void)
{
	return _enesim_text_grid_new(enesim_text_engine_default_get());
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Renderer * enesim_text_grid_new_from_engine(Enesim_Text_Engine *e)
{
	return _enesim_text_grid_new(e);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_text_grid_columns_set(Enesim_Renderer *r, unsigned int columns)
{
	Enesim_Text_Grid *thiz;

	thiz = _enesim_text_grid_get(r);
	thiz->columns = columns;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_text_grid_rows_set(Enesim_Renderer *r, unsigned int rows)
{
	Enesim_Text_Grid *thiz;

	thiz = _enesim_text_grid_get(r);
	thiz->rows = rows;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_text_grid_text_set(Enesim_Renderer *r, Enesim_Text_Grid_String *string)
{
	Enesim_Text_Grid *thiz;

	thiz = _enesim_text_grid_get(r);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_text_grid_char_set(Enesim_Renderer *r, Enesim_Text_Grid_Char *ch)
{
	Enesim_Text_Grid *thiz;

	thiz = _enesim_text_grid_get(r);
}
