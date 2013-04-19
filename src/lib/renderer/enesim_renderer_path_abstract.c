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
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_path.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_path.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_path_abstract_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_ABSTRACT_BOILERPLATE(ENESIM_RENDERER_SHAPE_DESCRIPTOR,
		Enesim_Renderer_Path_Abstract,
		Enesim_Renderer_Path_Abstract_Class,
		enesim_renderer_path_abstract);

static void _enesim_renderer_path_abstract_class_init(void *k EINA_UNUSED)
{
}

static void _enesim_renderer_path_abstract_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_renderer_path_abstract_instance_deinit(void *o EINA_UNUSED)
{
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_renderer_path_abstract_commands_set(Enesim_Renderer *r, const Eina_List *commands)
{
	Enesim_Renderer_Path_Abstract_Class *klass;

	klass = ENESIM_RENDERER_PATH_ABSTRACT_CLASS_GET(r);
	klass->commands_set(r, commands);
}

#if 0
void enesim_renderer_path_abstract_state_set(Enesim_Renderer *r, const Enesim_Renderer_State *state)
{
	Enesim_Renderer_Path_Abstract *thiz;

	thiz = _path_abstract_get(r);
	thiz->state_set(r, state);
}

void enesim_renderer_path_abstract_shape_state_set(Enesim_Renderer *r,
		const Enesim_Renderer_Shape_State *state)
{
	Enesim_Renderer_Path_Abstract *thiz;

	thiz = _path_abstract_get(r);
	thiz->shape_state_set(r, state);
}
#endif
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

