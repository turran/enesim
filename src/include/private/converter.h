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
#ifndef CONVERTER_H_
#define CONVERTER_H_

typedef void (*Enesim_Converter_2D)(Enesim_Buffer_Data *data, uint32_t dw, uint32_t dh,
		uint32_t dpitch, uint32_t *src, uint32_t sw, uint32_t sh,
		uint32_t spitch);

typedef void (*Enesim_Converter_1D)(Enesim_Buffer_Data *data, uint32_t len, uint32_t *src);

#define ENESIM_CONVERTER_1D(f) ((Enesim_Converter_1D)(f))
#define ENESIM_CONVERTER_2D(f) ((Enesim_Converter_2D)(f))

EAPI void enesim_converter_span_register(Enesim_Converter_1D cnv,
		Enesim_Buffer_Format dfmt, Enesim_Angle angle, Enesim_Format sfmt);
EAPI void enesim_converter_surface_register(Enesim_Converter_2D cnv,
		Enesim_Buffer_Format dfmt, Enesim_Angle angle, Enesim_Format sfmt);

EAPI Enesim_Converter_1D enesim_converter_span_get(Enesim_Buffer_Format dfmt,
		Enesim_Angle angle, Enesim_Format f);
EAPI Enesim_Converter_2D enesim_converter_surface_get(Enesim_Buffer_Format dfmt,
		Enesim_Angle angle, Enesim_Format f);

#endif
