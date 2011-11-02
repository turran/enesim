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
 * @{
 */

typedef uint32_t Enesim_Argb; /**< ARGB color without premultiplication */
typedef uint32_t Enesim_Color; /**< Internal representation of an ARGB color */
#define ENESIM_COLOR_FULL 0xffffffff /**< Simple definition of a full (opaque white) color */

typedef uint8_t Enesim_Alpha;

EAPI Enesim_Argb enesim_color_argb_to(Enesim_Color c);
EAPI Enesim_Color enesim_color_argb_from(Enesim_Argb argb);

/* FIXME do we actually need the format ? */
EAPI void enesim_color_components_from(Enesim_Color *color,
		Enesim_Format f, uint8_t a, uint8_t r, uint8_t g, uint8_t b);
EAPI void enesim_color_components_to(Enesim_Color color,
		Enesim_Format f, uint8_t *a, uint8_t *r, uint8_t *g, uint8_t *b);

/** @} */ //End of Enesim_Color_Group

#endif /*ENESIM_COLOR_H_*/

