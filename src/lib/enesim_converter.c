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

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef Enesim_Converter_1D Enesim_Converter_1D_Lut[ENESIM_BUFFER_FORMATS][ENESIM_ANGLES][ENESIM_FORMATS];
typedef Enesim_Converter_2D Enesim_Converter_2D_Lut[ENESIM_BUFFER_FORMATS][ENESIM_ANGLES][ENESIM_FORMATS];

Enesim_Converter_1D_Lut _converters_1d;
Enesim_Converter_2D_Lut _converters_2d;
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_converter_init(void)
{
	enesim_converter_argb8888_init();
	enesim_converter_rgb888_init();
	enesim_converter_bgr888_init();
	enesim_converter_rgb565_init();
}
void enesim_converter_shutdown(void)
{
}

void enesim_converter_span_register(Enesim_Converter_1D cnv,
		Enesim_Buffer_Format dfmt, Enesim_Angle angle, Enesim_Format sfmt)
{
	_converters_1d[dfmt][angle][sfmt] = cnv;
}

void enesim_converter_surface_register(Enesim_Converter_2D cnv,
		Enesim_Buffer_Format dfmt, Enesim_Angle angle, Enesim_Format sfmt)
{
	//printf("registering converter dfmt = %d sfmt = %d\n", dfmt, sfmt);
	_converters_2d[dfmt][angle][sfmt] = cnv;
}

Enesim_Converter_1D enesim_converter_span_get(Enesim_Buffer_Format dfmt,
		Enesim_Angle angle, Enesim_Format f)
{
	return _converters_1d[dfmt][angle][f];
}

Enesim_Converter_2D enesim_converter_surface_get(Enesim_Buffer_Format dfmt,
		Enesim_Angle angle, Enesim_Format f)
{
	return _converters_2d[dfmt][angle][f];
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 *
 */
EAPI Eina_Bool enesim_converter_surface(Enesim_Surface *s, Enesim_Buffer *dst,
		Enesim_Angle angle,
		Eina_Rectangle *clip,
		int x, int y)
{
	Enesim_Converter_2D converter;
	Enesim_Buffer_Format dfmt;
	Enesim_Format sfmt;
	Enesim_Buffer_Sw_Data data;
	uint32_t *sdata;
	size_t stride;

	dfmt = enesim_buffer_format_get(dst);
	sfmt = enesim_surface_format_get(s);

	converter = enesim_converter_surface_get(dfmt, angle, sfmt);
	if (!converter) return EINA_FALSE;

	enesim_buffer_data_get(dst, &data);
	/* FIXME check the stride too */
	enesim_surface_data_get(s, &sdata, &stride);
	/* TODO check the clip and x, y */
	converter(&data, clip->w, clip->h, sdata, clip->w, clip->h, stride);

	return EINA_TRUE;
}
