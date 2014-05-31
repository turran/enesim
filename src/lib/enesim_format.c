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

#include "enesim_main.h"
#include "enesim_format.h"
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
 * @brief Get the string based name of a format.
 *
 * @param[in] f The format to get the name from.
 * @return The name of the format.
 *
 * This function returns a string associated to the format @p f, for
 * convenience display of the format. If @p f is not a valid format or
 * an unsupported one, @c NULL is returned.
 */
EAPI const char * enesim_format_name_get(Enesim_Format f)
{
	switch (f)
	{
		case ENESIM_FORMAT_ARGB8888:
		return "argb8888";

		case ENESIM_FORMAT_A8:
		return "a8";

		default:
		return NULL;

	}
}

/**
 * @brief Get the total size of bytes for a given a format and a size.
 *
 * @param[in] f The format.
 * @param[in] w The width.
 * @param[in] h The height.
 * @return The size in bytes of a bitmap.
 *
 * This function returns the size in bytes of a bitmap of format @p f,
 * width @p w and @p height h. If the format is not valid or is not
 * supported, 0 is returned.
 */
EAPI size_t enesim_format_size_get(Enesim_Format f, uint32_t w, uint32_t h)
{
	switch (f)
	{
		case ENESIM_FORMAT_ARGB8888:
		return w * h * sizeof(uint32_t);

		case ENESIM_FORMAT_A8:
		return w * h * sizeof(uint8_t);

		default:
		return 0;

	}
}

/**
 * @brief Return the stride for a given format and width.
 *
 * @param[in] fmt The format.
 * @param[in] w The width.
 * @return The stride in bytes.
 *
 * This function returns the stride in bytes of a bitmap of format @p fmt
 * and width @p w. If the format is not valid or is not supported, 0 is
 * returned.
 *
 * @note This function calls enesim_format_size_get() with 1 as height.
 */
EAPI size_t enesim_format_stride_get(Enesim_Format fmt, uint32_t w)
{
	return enesim_format_size_get(fmt, w, 1);
}
