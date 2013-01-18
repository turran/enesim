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
#ifndef ENESIM_RENDERER_TEXT_BASE_H_
#define ENESIM_RENDERER_TEXT_BASE_H_

/**
 * @defgroup Enesim_Text_Base_Renderer_Group Base Renderer
 * @{
 */

EAPI void enesim_text_base_font_name_set(Enesim_Renderer *r, const char *font);
EAPI void enesim_text_base_font_name_get(Enesim_Renderer *r, const char **font);
EAPI void enesim_text_base_size_set(Enesim_Renderer *r, unsigned int size);
EAPI void enesim_text_base_size_get(Enesim_Renderer *r, unsigned int *size);
EAPI void enesim_text_base_max_ascent_get(Enesim_Renderer *r, int *masc);
EAPI void enesim_text_base_max_descent_get(Enesim_Renderer *r, int *mdesc);
EAPI Enesim_Text_Font * enesim_text_base_font_get(Enesim_Renderer *r);

/**
 * @}
 */

#endif

