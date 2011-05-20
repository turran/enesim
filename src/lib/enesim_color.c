/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2008 Jorge Luis Zapata
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
 * Create a pixel from the given unpremultiplied components
 */
EAPI void enesim_color_components_from(Enesim_Color *color,
		Enesim_Format f, uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
	switch (f)
	{
		case ENESIM_FORMAT_ARGB8888:
		{
			uint16_t alpha = a + 1;
			*color = (a << 24) | (((r * alpha) >> 8) << 16)
					| (((g * alpha) >> 8) << 8)
					| ((b * alpha) >> 8);
		}
		break;

		case ENESIM_FORMAT_A8:
		*color = a;
		break;

		default:
		break;
	}
}

/**
 * Extract the components from a color
 */
EAPI void enesim_color_components_to(Enesim_Color color,
		Enesim_Format f, uint8_t *a, uint8_t *r, uint8_t *g, uint8_t *b)
{
	switch (f)
	{
		case ENESIM_FORMAT_ARGB8888:
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
		break;

		case ENESIM_FORMAT_A8:
		if (a) *a = (uint8_t)color;
		break;

		default:
		break;
	}
}
