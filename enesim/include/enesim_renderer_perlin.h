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
#ifndef ENESIM_RENDERER_PERLIN_H_
#define ENESIM_RENDERER_PERLIN_H_

/**
 * @defgroup Enesim_Renderer_Perlin_Group Perlin
 * @ingroup Enesim_Renderer_Group
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_perlin_new(void);
EAPI void enesim_renderer_perlin_octaves_set(Enesim_Renderer *r, unsigned int octaves);
EAPI void enesim_renderer_perlin_persistence_set(Enesim_Renderer *r, double persistence);
EAPI void enesim_renderer_perlin_amplitude_set(Enesim_Renderer *r, double ampl);
EAPI void enesim_renderer_perlin_xfrequency_set(Enesim_Renderer *r, double freq);
EAPI void enesim_renderer_perlin_yfrequency_set(Enesim_Renderer *r, double freq);

/**
 * @}
 */

#endif
