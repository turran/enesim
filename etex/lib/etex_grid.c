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
/* for now we are using only chars, until we need utf8 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Etex_Grid_Cell
{
	Enesim_Color background;
	Enesim_Color foreground;
	char c;
} Etex_Grid_Cell;

typedef struct _Etex_Grid
{
	Etex *etex;
	Etex_Font *font;
	unsigned int cell_width;
	unsigned int cell_height;
	unsigned int rows;
	unsigned int columns;
	Etex_Grid_Cell *cells;
} Etex_Grid;

static inline Etex_Grid * _etex_grid_get(Enesim_Renderer *r)
{
	Etex_Grid *thiz;

	thiz = etex_base_data_get(r);
	return thiz;
}

static inline Eina_Bool _etex_grid_has_changed(Enesim_Renderer *r)
{
	Etex_Grid *thiz;
	Eina_Bool invalidate = EINA_FALSE;

	thiz = _etex_grid_get(r);
	/* check that the current state is different from the old state */
	invalidate = etex_base_has_changed(r);
	return invalidate;
}

static inline void _etex_grid_update(Enesim_Renderer *r)
{
}

static inline Eina_Bool _etex_grid_calculate(Enesim_Renderer *r)
{
	Etex_Grid *thiz;
	Etex_Font *font;
	Eina_Bool invalidate;
	int masc;
	int mdesc;

	invalidate = _etex_grid_has_changed(r);
	if (invalidate) return EINA_TRUE;

	thiz = _etex_grid_get(r);
	/* update our font */
	etex_base_setup(r);
	font = etex_base_font_get(r);
	if (!font) return EINA_FALSE;
	/* we should preload every glyph now? or better preload only an area?
	 * like whenever we want to draw some row we can preload the group
	 * of rows based on that and keep track of what has been updated
	 * and what not?
	 */

	etex_base_max_ascent_get(r, &masc);
	etex_base_max_descent_get(r, &mdesc);
	thiz->cell_height = masc + mdesc;
	if (thiz->font)
	{
		etex_font_unref(thiz->font);
	}
	thiz->font = font;
	_etex_grid_update(r);

	return EINA_TRUE;
}

static void _etex_grid_draw_identity(Enesim_Renderer *r, int x, int y, unsigned int len, void *dst)
{
	/* on drawing a possibility is to always have a cell_width*cell_height allocated space
	 * then fill the background color and then blend on top of it the char
	 */
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _etex_grid_name(Enesim_Renderer *r)
{
	return "etex_grid";
}

static Eina_Bool _etex_grid_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill, Enesim_Error **error)
{
	Etex_Grid *e;

	e = _etex_grid_get(r);
	if (!e) return EINA_FALSE;
	if (!_etex_grid_calculate(r))
		return EINA_FALSE;
	*fill = _etex_grid_draw_identity;

	return EINA_TRUE;
}

static void _etex_grid_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Etex_Grid *thiz;

	thiz = _etex_grid_get(r);
}

static void _etex_grid_free(Enesim_Renderer *r)
{
	Etex_Grid *thiz;

	thiz = _etex_grid_get(r);
	free(thiz);
}

static void _etex_grid_boundings(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *rect)
{
	Etex_Grid *thiz;
	unsigned int size;

	thiz = _etex_grid_get(r);

	_etex_grid_calculate(r);
	rect->x = 0;
	rect->y = 0;
	rect->h = thiz->cell_height * thiz->rows;
	/* FIXME for now */
	etex_base_size_get(r, &size);
	rect->w = thiz->cell_width * size;
}

static void _etex_grid_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	Etex_Grid *thiz;

	thiz = _etex_grid_get(r);
	if (!thiz)
	{
		*flags = 0;
		return;
	}
	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_ARGB8888;
}

static Enesim_Renderer_Descriptor _etex_grid_descriptor = {
	/* .version = 			*/ ENESIM_RENDERER_API,
	/* .name = 			*/ _etex_grid_name,
	/* .free = 			*/ _etex_grid_free,
	/* .boundings = 		*/ _etex_grid_boundings,
	/* .destination_transform = 	*/ NULL,
	/* .flags = 			*/ _etex_grid_flags,
	/* .is_inside = 		*/ NULL,
	/* .damage = 			*/ NULL,
	/* .has_changed = 		*/ NULL,
	/* .sw_setup = 			*/ _etex_grid_setup,
	/* .sw_cleanup = 		*/ _etex_grid_cleanup,
};

static Enesim_Renderer * _etex_grid_new(Etex *etex)
{
	Etex_Grid *thiz;
	Enesim_Renderer *r;

	thiz = calloc(1, sizeof(Etex_Grid));
	if (!thiz)
		return NULL;

	thiz->etex = etex;

	r = etex_base_new(etex, &_etex_grid_descriptor, thiz);
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
EAPI Enesim_Renderer * etex_grid_new(void)
{
	return _etex_grid_new(etex_default_get());
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Renderer * etex_grid_new_from_etex(Etex *e)
{
	return _etex_grid_new(e);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void etex_grid_columns_set(Enesim_Renderer *r, unsigned int columns)
{
	Etex_Grid *thiz;

	thiz = _etex_grid_get(r);
	thiz->columns = columns;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void etex_grid_rows_set(Enesim_Renderer *r, unsigned int rows)
{
	Etex_Grid *thiz;

	thiz = _etex_grid_get(r);
	thiz->rows = rows;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void etex_grid_text_set(Enesim_Renderer *r, Etex_Grid_String *string)
{
	Etex_Grid *thiz;

	thiz = _etex_grid_get(r);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void etex_grid_char_set(Enesim_Renderer *r, Etex_Grid_Char *ch)
{
	Etex_Grid *thiz;

	thiz = _etex_grid_get(r);
}
