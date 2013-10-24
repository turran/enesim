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
#ifndef ENESIM_CONVERTER_H_
#define ENESIM_CONVERTER_H_

/**
 * @defgroup Enesim_Converter_Group Converter
 * @brief Target to target and native to target pixel data converter
 * @{
 */

EAPI Eina_Bool enesim_converter_buffer(Enesim_Buffer *b, Enesim_Buffer *dst);
EAPI Eina_Bool enesim_converter_surface(Enesim_Surface *s, Enesim_Buffer *dst);
/** @} */
#endif /*ENESIM_CONVERTER_H_*/
