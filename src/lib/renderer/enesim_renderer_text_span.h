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
#ifndef ENESIM_RENDERER_TEXT_SPAN_H_
#define ENESIM_RENDERER_TEXT_SPAN_H_

/**
 * @}
 * @defgroup Enesim_Renderer_Text_Span_Renderer_Group Span Renderer
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_text_span_new(void);
EAPI Enesim_Renderer * enesim_renderer_text_span_new_from_engine(Enesim_Text_Engine *e);
EAPI void enesim_renderer_text_span_text_set(Enesim_Renderer *r, const char *str);
EAPI void enesim_renderer_text_span_text_get(Enesim_Renderer *r, const char **str);
EAPI void enesim_renderer_text_span_direction_get(Enesim_Renderer *r, Enesim_Text_Direction *direction);
EAPI void enesim_renderer_text_span_direction_set(Enesim_Renderer *r, Enesim_Text_Direction direction);
EAPI void enesim_renderer_text_span_buffer_get(Enesim_Renderer *r, Enesim_Text_Buffer **b);
EAPI int enesim_renderer_text_span_index_at(Enesim_Renderer *r, int x, int y);

/**
 * @}
 */

#endif
