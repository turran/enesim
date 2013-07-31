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
#ifndef ENESIM_RENDERER_GRADIENT_H_
#define ENESIM_RENDERER_GRADIENT_H_

/**
 * @defgroup Enesim_Renderer_Gradient_Group Gradient
 * @brief Gradient renderer
 * @ingroup Enesim_Renderer_Group
 * @{
 */

typedef struct _Enesim_Renderer_Gradient_Stop
{
	Enesim_Argb argb;
	double pos;
} Enesim_Renderer_Gradient_Stop;

EAPI void enesim_renderer_gradient_stop_add(Enesim_Renderer *r, Enesim_Renderer_Gradient_Stop *stop);
EAPI void enesim_renderer_gradient_stop_clear(Enesim_Renderer *r);
EAPI void enesim_renderer_gradient_stop_set(Enesim_Renderer *r,
		Eina_List *list);
EAPI void enesim_renderer_gradient_mode_set(Enesim_Renderer *r, Enesim_Repeat_Mode mode);
EAPI void enesim_renderer_gradient_mode_get(Enesim_Renderer *r, Enesim_Repeat_Mode *mode);

/**
 * @}
 */

#endif
