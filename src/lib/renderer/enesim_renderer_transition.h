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
#ifndef ENESIM_RENDERER_TRANSITION_H_
#define ENESIM_RENDERER_TRANSITION_H_

/**
 * @file
 * @ender_group{Enesim_Renderer_Transition}
 */

/**
 * @defgroup Enesim_Renderer_Transition Transition
 * @brief Rederer that blends two renderers @ender_inherits{Enesim_Renderer}
 * @ingroup Enesim_Renderer
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_transition_new(void);
EAPI void enesim_renderer_transition_level_set(Enesim_Renderer *r, double level);
EAPI double enesim_renderer_transition_level_get(Enesim_Renderer *r);

EAPI void enesim_renderer_transition_source_set(Enesim_Renderer *r, Enesim_Renderer *r0);
EAPI Enesim_Renderer * enesim_renderer_transition_source_get(Enesim_Renderer *r);

EAPI void enesim_renderer_transition_target_set(Enesim_Renderer *r, Enesim_Renderer *r1);
EAPI Enesim_Renderer * enesim_renderer_transition_target_get(Enesim_Renderer *r);

/**
 * @}
 */

#endif
