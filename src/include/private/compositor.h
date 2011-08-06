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
#ifndef COMPOSITOR_H_
#define COMPOSITOR_H_

#define ENESIM_COMPOSITOR_SPAN(f) ((Enesim_Compositor_Span)(f))
#define ENESIM_COMPOSITOR_POINT(f) ((Enesim_Compositor_Point)(f))

void enesim_compositor_init(void);
void enesim_compositor_shutdown(void);

void enesim_compositor_pt_color_register(Enesim_Compositor_Point sp,
		Enesim_Rop rop, Enesim_Format dfmt);
void enesim_compositor_pt_pixel_register(Enesim_Compositor_Point sp,
		Enesim_Rop rop, Enesim_Format dfmt, Enesim_Format sfmt);
void enesim_compositor_pt_mask_color_register(Enesim_Compositor_Point sp,
		Enesim_Rop rop, Enesim_Format dfmt, Enesim_Format mfmt);
void enesim_compositor_pt_pixel_mask_register(Enesim_Compositor_Point sp,
		Enesim_Rop rop, Enesim_Format dfmt, Enesim_Format sfmt,
		Enesim_Format mfmt);
void enesim_compositor_pt_pixel_color_register(Enesim_Compositor_Point sp,
		Enesim_Rop rop, Enesim_Format dfmt, Enesim_Format sfmt);

void enesim_compositor_span_color_register(Enesim_Compositor_Span sp,
		Enesim_Rop rop, Enesim_Format dfmt);
void enesim_compositor_span_pixel_register(Enesim_Compositor_Span sp,
		Enesim_Rop rop, Enesim_Format dfmt, Enesim_Format sfmt);
void enesim_compositor_span_mask_color_register(Enesim_Compositor_Span sp,
		Enesim_Rop rop, Enesim_Format dfmt, Enesim_Format mfmt);
void enesim_compositor_span_pixel_mask_register(Enesim_Compositor_Span sp,
		Enesim_Rop rop, Enesim_Format dfmt, Enesim_Format sfmt,
		Enesim_Format mfmt);
void enesim_compositor_span_pixel_color_register(Enesim_Compositor_Span sp,
		Enesim_Rop rop, Enesim_Format dfmt, Enesim_Format sfmt);

#endif /* COMPOSITOR_H_*/
