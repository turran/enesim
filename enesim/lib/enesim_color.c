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
#include "Enesim.h"
#include "enesim_private.h"
#include "libargb.h"
#include "private/format_argb8888_unpre.h" // FIXME for now until we fix this
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * FIXME
 */
EAPI Enesim_Argb enesim_color_argb_to(Enesim_Color c)
{
	Enesim_Argb argb;

	argb8888_unpre_argb_from(&argb, c);
	return argb;
}

/**
 * FIXME
 */
EAPI Enesim_Color enesim_color_argb_from(Enesim_Argb argb)
{
	Enesim_Color c;

	argb8888_unpre_argb_to(argb, &c);
	return c;
}


/**
 * Create a pixel from the given unpremultiplied components
 */
EAPI void enesim_color_components_from(Enesim_Color *color,
		uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
	uint16_t alpha = a + 1;
	*color = (a << 24) | (((r * alpha) >> 8) << 16)
					| (((g * alpha) >> 8) << 8)
					| ((b * alpha) >> 8);
}

/**
  * Extract the components from a color
 */
EAPI void enesim_color_components_to(Enesim_Color color,
		uint8_t *a, uint8_t *r, uint8_t *g, uint8_t *b)
{
	uint8_t pa;

	pa = (color >> 24);
	if ((pa > 0) && (pa < 255))
	{
		if (a) *a = pa;
		if (r) *r = (argb8888_red_get(color) * 255) / pa;
		if (g) *g = (argb8888_green_get(color) * 255) / pa;
		if (b) *b = (argb8888_blue_get(color) * 255) / pa;
	}
	else
	{
		if (a) *a = pa;
		if (r) *r = argb8888_red_get(color);
		if (g) *g = argb8888_green_get(color);
		if (b) *b = argb8888_blue_get(color);
	}
}

/**
 *
 */
EAPI void enesim_argb_components_from(Enesim_Argb *argb, uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
	if (argb) *argb = a << 24 | r << 16 | g << 8 | b;
}

/**
 *
 */
EAPI void enesim_argb_components_to(Enesim_Argb argb, uint8_t *a, uint8_t *r, uint8_t *g, uint8_t *b)
{
	if (a) *a = argb >> 24;
	if (r) *r = (argb >> 16) & 0xff;
	if (g) *g = (argb >> 8) & 0xff;
	if (b) *b = (argb) & 0xff;
}
