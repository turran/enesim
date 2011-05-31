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
#ifndef ENESIM_EINA_H_
#define ENESIM_EINA_H_

/* Here we put all the stuff that should be moved to eina */

#define EINA_F16P16_ONE (1 << 16)
#define EINA_F16P16_HALF (1 << 15)

static inline Eina_F16p16 eina_f16p16_double_from(double v)
{
   Eina_F16p16 r;

   r = (Eina_F16p16)(v * 65536.0f + (v < 0 ? -0.5f : 0.5f));
   return r;

}

static inline double eina_f16p16_double_to(Eina_F16p16 v)
{
   double r;

   r = v / 65536.0f;
   return r;
}

#endif
