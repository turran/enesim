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
#ifndef COMPOSITOR_H_
#define COMPOSITOR_H_

#define ENESIM_COMPOSITOR_SPAN(f) ((Enesim_Compositor_Span)(f))
#define ENESIM_COMPOSITOR_POINT(f) ((Enesim_Compositor_Point)(f))

#define PT_C(f, op) enesim_compositor_##f##_pt_color_##op
#define PT_P(f, sf, op) enesim_compositor_##f##_pt_pixel_##sf##_##op
#define PT_MC(f, mf, op) enesim_compositor_##f##_pt_mask_color_##mf##_##op
#define PT_PC(f, sf, op) enesim_compositor_##f##_pt_pixel_color_##sf##_##op
#define PT_PM(f, sf, mf, op) enesim_compositor_##f##_pt_pixel_mask_##sf##_##mf##_##op

#define SP_C(f, op) enesim_compositor_##f##_sp_color_##op
#define SP_P(f, sf, op) enesim_compositor_##f##_sp_pixel_##sf##_##op
#define SP_MC(f, mf, op) enesim_compositor_##f##_sp_mask_color_##mf##_##op
#define SP_PC(f, sf, op) enesim_compositor_##f##_sp_pixel_color_##sf##_##op
#define SP_PM(f, sf, mf, op) enesim_compositor_##f##_sp_pixel_mask_##sf##_##mf##_##op

void enesim_compositor_init(void);
void enesim_compositor_shutdown(void);

#endif /* COMPOSITOR_H_*/
