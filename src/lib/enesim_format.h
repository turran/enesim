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

#ifndef ENESIM_FORMAT_H_
#define ENESIM_FORMAT_H_

/**
 * @file
 * @ender_group{Enesim_Format}
 */

/**
 * @defgroup Enesim_Format Format
 * @{
 */

/**
 * Surface formats
 */
typedef enum _Enesim_Format
{
	ENESIM_FORMAT_NONE,
	ENESIM_FORMAT_ARGB8888,  /**< argb32 */
	ENESIM_FORMAT_A8, /**< a8 */
} Enesim_Format;

/**< The number of formats */
#define ENESIM_FORMAT_LAST (ENESIM_FORMAT_A8 + 1)

EAPI const char * enesim_format_name_get(Enesim_Format f);
EAPI size_t enesim_format_size_get(Enesim_Format f, uint32_t w, uint32_t h);
EAPI size_t enesim_format_stride_get(Enesim_Format fmt, uint32_t w);


/** @} */ //End of Enesim_Buffer

#endif
