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
#ifndef ENESIM_RENDERER_TEXT_GRID_H_
#define ENESIM_RENDERER_TEXT_GRID_H_

/**
 * @defgroup Enesim_Text_Grid_Renderer_Group Text Grid
 * @ingroup Enesim_Renderer_Group
 * @{
 */

typedef struct _Enesim_Text_Grid_Char
{
	char c;
	unsigned int row;
	unsigned int column;
	Enesim_Color foreground;
	Enesim_Color background;
} Enesim_Text_Grid_Char;

typedef struct _Enesim_Text_Grid_String
{
	const char *string;
	unsigned int row;
	unsigned int column;
	Enesim_Color foreground;
	Enesim_Color background;
} Enesim_Text_Grid_String;

EAPI Enesim_Renderer * enesim_text_grid_new(void);
EAPI Enesim_Renderer * enesim_text_grid_new_from_engine(Enesim_Text_Engine *e);
EAPI void enesim_text_grid_columns_set(Enesim_Renderer *r, unsigned int columns);
EAPI void enesim_text_grid_rows_set(Enesim_Renderer *r, unsigned int rows);

/**
 * @}
 */

#endif

