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
#ifndef ENESIM_RENDERER_PATH_H_
#define ENESIM_RENDERER_PATH_H_

/**
 * @}
 * @defgroup Enesim_Renderer_Path_Group Path
 * @brief Path renderer
 * @ingroup Enesim_Renderer_Shape_Group
 * @{
 */

EAPI Enesim_Renderer * enesim_renderer_path_new(void);

EAPI void enesim_renderer_path_path_set(Enesim_Renderer *r, Enesim_Path *path);
EAPI Enesim_Path * enesim_renderer_path_path_get(Enesim_Renderer *r);

/**
 * @}
 */

#endif
