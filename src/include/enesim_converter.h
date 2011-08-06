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
#ifndef ENESIM_CONVERTER_H_
#define ENESIM_CONVERTER_H_

/**
 * @defgroup Enesim_Converter_Group Converter
 * @{
 */

EAPI Enesim_Buffer_Format enesim_converter_format_get(uint8_t aoffset, uint8_t alen,
		uint8_t roffset, uint8_t rlen, uint8_t goffset, uint8_t glen,
		uint8_t boffset, uint8_t blen);

EAPI uint8_t enesim_converter_format_depth_get(Enesim_Buffer_Format fmt);

EAPI Eina_Bool enesim_converter_buffer(Enesim_Buffer *b, Enesim_Buffer *dst,
		Enesim_Angle angle,
		Eina_Rectangle *clip,
		int x, int y);
EAPI Eina_Bool enesim_converter_surface(Enesim_Surface *s, Enesim_Buffer *dst,
		Enesim_Angle angle,
		Eina_Rectangle *clip,
		int x, int y);
/** @} */
#endif /*ENESIM_CONVERTER_H_*/
