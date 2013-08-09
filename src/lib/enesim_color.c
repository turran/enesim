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
#include "enesim_private.h"
#include "libargb.h"

#include "enesim_color.h"
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
 * @brief Transform the given premultiplied color to a ARGB one.
 *
 * @param c The premultiplied color.
 * @return the ARGB color.
 *
 * This function transforms the premultiplied color @p c to an ARGB one.
 */
EAPI Enesim_Argb enesim_color_argb_to(Enesim_Color c)
{
	Enesim_Argb argb;

	argb8888_unpre_argb_from(&argb, c);
	return argb;
}

/**
 * @brief Transform the given ARGB color to a premultiplied one.
 *
 * @param argb The ARGB color.
 * @return the premultiplied color.
 *
 * This function transforms the @p argb color to a premultiplied one.
 */
EAPI Enesim_Color enesim_color_argb_from(Enesim_Argb argb)
{
	Enesim_Color c;

	argb8888_unpre_argb_to(argb, &c);
	return c;
}

/**
 * @brief Set the given premultiplied color with the given components.
 *
 * @param[out] color The Enesim color to set.
 * @param[in] a The alpha component.
 * @param[in] r The red component.
 * @param[in] g The green component.
 * @param[in] b The blue component.
 *
 * This function sets the premultiplied color @p color with the
 * components @p a,  @p r, @p g and @p b.
 *
 * @see enesim_color_components_to()
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
 * @brief Retrieve the component of the given premultiplied color.
 *
 * @param[in] color The Enesim color to set.
 * @param[out] a The alpha component.
 * @param[out] r The red component.
 * @param[out] g The green component.
 * @param[out] b The blue component.
 *
 * This function retrieves the ARGB components of the premultiplied
 * color @p color and stores them into @p a,  @p r, @p g and @p b.
 *
 * @see enesim_color_components_from()
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
 * @brief Set the given ARGB color with the given components.
 *
 * @param[out] argb The ARGB color to set.
 * @param[in] a The alpha component.
 * @param[in] r The red component.
 * @param[in] g The green component.
 * @param[in] b The blue component.
 *
 * This function sets the @p argb color with the components @p a,
 * @p r, @p g and @p b.
 *
 * @see enesim_argb_components_to()
 */
EAPI void enesim_argb_components_from(Enesim_Argb *argb, uint8_t a, uint8_t r, uint8_t g, uint8_t b)
{
	if (argb) *argb = a << 24 | r << 16 | g << 8 | b;
}

/**
 * @brief Retrieve the component of the given ARGB color.
 *
 * @param[in] argb The ARGB color to set.
 * @param[out] a The alpha component.
 * @param[out] r The red component.
 * @param[out] g The green component.
 * @param[out] b The blue component.
 *
 * This function stores the @p argb color components into @p a, @p r,
 * @p g and @p b. @p a, @p r, @p g and @p b can each be @c NULL.
 *
 * @see enesim_argb_components_from()
 */
EAPI void enesim_argb_components_to(Enesim_Argb argb, uint8_t *a, uint8_t *r, uint8_t *g, uint8_t *b)
{
	if (a) *a = argb >> 24;
	if (r) *r = (argb >> 16) & 0xff;
	if (g) *g = (argb >> 8) & 0xff;
	if (b) *b = (argb) & 0xff;
}
