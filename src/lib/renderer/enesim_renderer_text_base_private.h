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
#ifndef ENESIM_RENDERER_TEXT_BASE_PRIVATE_H_
#define ENESIM_RENDERER_TEXT_BASE_PRIVATE_H_

typedef struct _Enesim_Text_Base_State
{
	unsigned int size;
	char *font_name;
	char *str;
} Enesim_Text_Base_State;

typedef void (*Enesim_Text_Base_Boundings)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Text_Base_State *sstate[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *boundings);
typedef void (*Enesim_Text_Base_Destination_Boundings)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Text_Base_State *sstate[ENESIM_RENDERER_STATES],
		Eina_Rectangle *boundings);

typedef Eina_Bool (*Enesim_Text_Base_Sw_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Text_Base_State *sstate[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill,
		Enesim_Error **error);
typedef Eina_Bool (*Enesim_Text_Base_OpenCL_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Text_Base_State *sstate[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		const char **program_name, const char **program_source,
		size_t *program_length,
		Enesim_Error **error);
typedef Eina_Bool (*Enesim_Text_Base_OpenGL_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		const Enesim_Text_Base_State *sstate[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Error **error);

typedef struct _Enesim_Text_Base_Descriptor
{
	/* common */
	Enesim_Renderer_Name name;
	Enesim_Renderer_Delete free;
	Enesim_Text_Base_Boundings boundings;
	Enesim_Text_Base_Destination_Boundings destination_boundings;
	Enesim_Renderer_Flags flags;
	Enesim_Renderer_Hints_Get hints_get;
	Enesim_Renderer_Inside is_inside;
	Enesim_Renderer_Damage damage;
	Enesim_Renderer_Has_Changed has_changed;
	/* software based functions */
	Enesim_Text_Base_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	/* opencl based functions */
	Enesim_Text_Base_OpenCL_Setup opencl_setup;
	Enesim_Renderer_OpenCL_Kernel_Setup opencl_kernel_setup;
	Enesim_Renderer_OpenCL_Cleanup opencl_cleanup;
	/* opengl based functions */
	Enesim_Renderer_OpenGL_Initialize opengl_initialize;
	Enesim_Text_Base_OpenGL_Setup opengl_setup;
	Enesim_Renderer_OpenGL_Cleanup opengl_cleanup;
} Enesim_Text_Base_Descriptor;

Enesim_Renderer * enesim_text_base_new(Etex *etex, Enesim_Text_Base_Descriptor *descriptor, void *data);
void * enesim_text_base_data_get(Enesim_Renderer *r);
void enesim_text_base_setup(Enesim_Renderer *r);

#endif


