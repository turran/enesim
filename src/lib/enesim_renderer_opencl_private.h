/* ENESIM - Drawing Library
 * Copyright (C) 2007-2017 Jorge Luis Zapata
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
#ifndef ENESIM_RENDERER_CL_PRIVATE_H_
#define ENESIM_RENDERER_CL_PRIVATE_H_

void enesim_renderer_opencl_init(void);
void enesim_renderer_opencl_shutdown(void);

void enesim_renderer_opencl_free(Enesim_Renderer *r);

#endif
