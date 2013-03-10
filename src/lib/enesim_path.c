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
#include "enesim_private.h"

#include "enesim_main.h"
#include "enesim_path.h"

#include "enesim_path_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_path_command_quadratic_cubic_to(Enesim_Path_Quadratic *q,
		Enesim_Path_Cubic *c)
{
	/* same start and end points */
	c->end_x = q->end_x;
	c->start_x = q->start_x;
	c->end_y = q->end_y;
	c->start_y = q->start_y;
	/* create the new control points */
	c->ctrl_x0 = q->start_x + (2.0/3.0 * (q->ctrl_x - q->start_x));
	c->ctrl_x1 = q->end_x + (2.0/3.0 * (q->ctrl_x - q->end_x));

	c->ctrl_y0 = q->start_y + (2.0/3.0 * (q->ctrl_y - q->start_y));
	c->ctrl_y1 = q->end_y + (2.0/3.0 * (q->ctrl_y - q->end_y));
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

