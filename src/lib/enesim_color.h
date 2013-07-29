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
#ifndef ENESIM_COLOR_H_
#define ENESIM_COLOR_H_

/**
 * @defgroup Enesim_Color_Group Color
 * @brief Color definition
 * @{
 */

/**
 * @typedef Enesim_Argb
 *
 * ARGB color without premultiplication.
 */
typedef uint32_t Enesim_Argb;

/**
 * @typedef Enesim_Color
 *
 * Internal representation of an ARGB color
 */
typedef uint32_t Enesim_Color;

/**
 * @ref ENESIM_COLOR_FULL
 *
 * Simple definition of a full (opaque white) color
 */
#define ENESIM_COLOR_FULL 0xffffffff

/**
 * @typedef Enesim_Alpha
 *
 * Internal representation of an alpha component of a color.
 */
typedef uint8_t Enesim_Alpha;

EAPI Enesim_Argb enesim_color_argb_to(Enesim_Color c);
EAPI Enesim_Color enesim_color_argb_from(Enesim_Argb argb);

EAPI void enesim_color_components_from(Enesim_Color *color, uint8_t a, uint8_t r, uint8_t g, uint8_t b);
EAPI void enesim_color_components_to(Enesim_Color color, uint8_t *a, uint8_t *r, uint8_t *g, uint8_t *b);

EAPI void enesim_argb_components_from(Enesim_Argb *argb, uint8_t a, uint8_t r, uint8_t g, uint8_t b);
EAPI void enesim_argb_components_to(Enesim_Argb argb, uint8_t *a, uint8_t *r, uint8_t *g, uint8_t *b);

/**
 * @}
 */

#endif /*ENESIM_COLOR_H_*/

