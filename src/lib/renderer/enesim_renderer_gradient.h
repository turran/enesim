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
#ifndef ENESIM_RENDERER_GRADIENT_H_
#define ENESIM_RENDERER_GRADIENT_H_

/**
 * @file
 * @ender_group{Enesim_Renderer_Gradient_Stop}
 * @ender_group{Enesim_Renderer_Gradient}
 */

/**
 * @defgroup Enesim_Renderer_Gradient_Stop Gradient Stop
 * @ingroup Enesim_Renderer_Gradient
 * @{
 */

/** A gradient stop definition */
typedef struct _Enesim_Renderer_Gradient_Stop
{
	Enesim_Argb argb; /**< The argb color of the stop */
	double pos; /**< The position of the stop in the 0..1 range */
} Enesim_Renderer_Gradient_Stop;

/**
 * @}
 * @defgroup Enesim_Renderer_Gradient Gradient
 * @brief Gradient renderer @ender_inherits{Enesim_Renderer}
 * @ingroup Enesim_Renderer
 * @{
 */

EAPI void enesim_renderer_gradient_stop_add(Enesim_Renderer *r, Enesim_Renderer_Gradient_Stop *stop);
EAPI void enesim_renderer_gradient_stop_clear(Enesim_Renderer *r);
EAPI void enesim_renderer_gradient_repeat_mode_set(Enesim_Renderer *r, Enesim_Repeat_Mode mode);
EAPI Enesim_Repeat_Mode enesim_renderer_gradient_repeat_mode_get(Enesim_Renderer *r);

/**
 * @}
 */

#endif
