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
#ifndef ENESIM_RENDERER_PATH_H_
#define ENESIM_RENDERER_PATH_H_

/**
 * @}
 * @defgroup Enesim_Renderer_Path_Group Path
 * @ingroup Enesim_Renderer_Shape_Group
 * @{
 */

EAPI Enesim_Renderer * enesim_renderer_path_new(void);

EAPI void enesim_renderer_path_command_clear(Enesim_Renderer *r);
EAPI void enesim_renderer_path_command_add(Enesim_Renderer *r, Enesim_Path_Command *cmd);
EAPI void enesim_renderer_path_command_set(Enesim_Renderer *r, Eina_List *l);
EAPI void enesim_renderer_path_command_get(Enesim_Renderer *r, Eina_List **list);

EAPI void enesim_renderer_path_move_to(Enesim_Renderer *r, double x, double y);
EAPI void enesim_renderer_path_line_to(Enesim_Renderer *r, double x, double y);
EAPI void enesim_renderer_path_squadratic_to(Enesim_Renderer *r, double x, double y);
EAPI void enesim_renderer_path_quadratic_to(Enesim_Renderer *r, double ctrl_x,
		double ctrl_y, double x, double y);
EAPI void enesim_renderer_path_cubic_to(Enesim_Renderer *r, double ctrl_x0,
		double ctrl_y0, double ctrl_x, double ctrl_y, double x,
		double y);
EAPI void enesim_renderer_path_scubic_to(Enesim_Renderer *r, double ctrl_x, double ctrl_y,
		double x, double y);
EAPI void enesim_renderer_path_arc_to(Enesim_Renderer *r, double rx, double ry, double angle,
                   unsigned char large, unsigned char sweep, double x, double y);
EAPI void enesim_renderer_path_close(Enesim_Renderer *r, Eina_Bool close);

/**
 * @}
 */

#endif
