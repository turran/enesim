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
#include "enesim_error.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_path.h"

#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_path_abstract_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Renderer_Path_Abstract
{
	Enesim_Renderer_Path_Abstract_Commands_Set_Cb commands_set;
	Enesim_Renderer_Delete_Cb free;
	void *data;
} Enesim_Renderer_Path_Abstract;

static Enesim_Renderer_Path_Abstract * _path_abstract_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Abstract *thiz;
	thiz = enesim_renderer_shape_data_get(r);
	return thiz;
}

static void _path_abstract_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Abstract *thiz;
	thiz = enesim_renderer_shape_data_get(r);
	thiz->free(r);
	free(thiz);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_path_abstract_new(Enesim_Renderer_Path_Abstract_Descriptor *descriptor, void *data)
{
	Enesim_Renderer_Path_Abstract *thiz;
	Enesim_Renderer_Shape_Descriptor pdescriptor;
	Enesim_Renderer *r;

	thiz = calloc(1, sizeof(Enesim_Renderer_Path_Abstract));
	thiz->data = data;
	thiz->free = descriptor->free;
	thiz->commands_set = descriptor->commands_set;

	pdescriptor.version = descriptor->version;
	pdescriptor.base_name_get = descriptor->base_name_get;
	pdescriptor.free = _path_abstract_free;
	pdescriptor.bounds_get = descriptor->bounds_get;
	pdescriptor.destination_bounds_get = descriptor->destination_bounds_get;
	pdescriptor.flags_get = descriptor->flags_get;
	pdescriptor.is_inside = descriptor->is_inside;
	pdescriptor.damages_get = descriptor->damages_get;
	pdescriptor.has_changed = descriptor->has_changed;
	pdescriptor.sw_hints_get = descriptor->sw_hints_get;
	pdescriptor.sw_setup = descriptor->sw_setup;
	pdescriptor.sw_cleanup = descriptor->sw_cleanup;
	pdescriptor.opengl_initialize = descriptor->opengl_initialize;
	pdescriptor.opengl_setup = descriptor->opengl_setup;
	pdescriptor.opengl_cleanup = descriptor->opengl_cleanup;
	pdescriptor.opencl_kernel_setup = descriptor->opencl_kernel_setup;
	pdescriptor.opencl_setup = descriptor->opencl_setup;
	pdescriptor.opencl_cleanup = descriptor->opencl_cleanup;
	pdescriptor.feature_get = descriptor->feature_get;

	r = enesim_renderer_shape_new(&pdescriptor, thiz);
	return r;
}

void * enesim_renderer_path_abstract_data_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Abstract *thiz;

	thiz = _path_abstract_get(r);
	return thiz->data;
}

void enesim_renderer_path_abstract_commands_set(Enesim_Renderer *r, const Eina_List *commands)
{
	Enesim_Renderer_Path_Abstract *thiz;

	thiz = _path_abstract_get(r);
	thiz->commands_set(r, commands);
}

/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

