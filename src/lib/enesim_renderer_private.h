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
#ifndef ENESIM_RENDERER_PRIVATE_H_
#define ENESIM_RENDERER_PRIVATE_H_

#if BUILD_OPENGL
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
#endif

#include "enesim_compositor_private.h"
#include "enesim_renderer_sw_private.h"
#include "enesim_renderer_opencl_private.h"
#include "enesim_renderer_opengl_private.h"

Enesim_Object_Descriptor * enesim_renderer_descriptor_get(void);
#define ENESIM_RENDERER_DESCRIPTOR enesim_renderer_descriptor_get()
#define ENESIM_RENDERER_CLASS(k) ENESIM_OBJECT_CLASS_CHECK(k, Enesim_Renderer_Class, ENESIM_RENDERER_DESCRIPTOR)
#define ENESIM_RENDERER_CLASS_GET(o) ENESIM_RENDERER_CLASS(ENESIM_OBJECT_INSTANCE_CLASS(o))
#define ENESIM_RENDERER(o) ENESIM_OBJECT_INSTANCE_CHECK(o, Enesim_Renderer, ENESIM_RENDERER_DESCRIPTOR)

/** Helper macro to add an error on a renderer based function */
#define ENESIM_RENDERER_LOG(r, error, fmt, ...) \
	enesim_renderer_log_add(r, error, __FILE__, __FUNCTION__, __LINE__, fmt, ## __VA_ARGS__);

/** Renderer API/ABI version */
#define ENESIM_RENDERER_API 0

/*----------------------------------------------------------------------------*
 *                          State related functions                           *
 *----------------------------------------------------------------------------*/
typedef struct _Enesim_Renderer_State
{
	struct {
		Enesim_Color color;
		Enesim_Renderer *mask;
		Enesim_Channel mchannel;
		Eina_Bool visibility;
		Enesim_Matrix transformation;
		Enesim_Matrix_Type transformation_type;
		Enesim_Quality quality;
		double ox;
		double oy;
	} current, past;
	Eina_Bool changed;
} Enesim_Renderer_State;

/*----------------------------------------------------------------------------*
 *                        Descriptor related functions                        *
 *----------------------------------------------------------------------------*/
/* common descriptor functions */
typedef const char * (*Enesim_Renderer_Base_Name_Get_Cb)(Enesim_Renderer *r);
typedef Eina_Bool (*Enesim_Renderer_Is_Supported_Cb)(Enesim_Renderer *r, Enesim_Surface *s);
typedef Eina_Bool (*Enesim_Renderer_Is_Inside_Cb)(Enesim_Renderer *r, double x, double y);
typedef Enesim_Alpha_Hint (*Enesim_Renderer_Alpha_Hints_Get_Cb)(Enesim_Renderer *r);
typedef Eina_Bool (*Enesim_Renderer_Bounds_Get_Cb)(Enesim_Renderer *r,
		Enesim_Rectangle *rect, Enesim_Log **error);
typedef void (*Enesim_Renderer_Features_Get)(Enesim_Renderer *r,
		int *features);
typedef Eina_Bool (*Enesim_Renderer_Has_Changed_Cb)(Enesim_Renderer *r);
typedef Eina_Bool (*Enesim_Renderer_Damages_Get_Cb)(Enesim_Renderer *r,
		const Eina_Rectangle *old_bounds,
		Enesim_Renderer_Damage cb, void *data);

/* software backend descriptor functions */
typedef void (*Enesim_Renderer_Sw_Hints_Get_Cb)(Enesim_Renderer *r,
		Enesim_Rop rop, Enesim_Renderer_Sw_Hint *hints);
typedef Eina_Bool (*Enesim_Renderer_Sw_Setup)(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Rop rop,
		Enesim_Renderer_Sw_Fill *fill,
		Enesim_Log **error);
typedef void (*Enesim_Renderer_Sw_Cleanup)(Enesim_Renderer *r, Enesim_Surface *s);
/* opencl backend descriptor functions */
typedef Eina_Bool (*Enesim_Renderer_OpenCL_Setup)(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Rop rop,
		const char **program_name, const char **program_source,
		size_t *program_length,
		Enesim_Log **error);
typedef void (*Enesim_Renderer_OpenCL_Cleanup)(Enesim_Renderer *r, Enesim_Surface *s);
typedef Eina_Bool (*Enesim_Renderer_OpenCL_Kernel_Setup)(Enesim_Renderer *r, Enesim_Surface *s);

/* OpenGL descriptor functions */
typedef Eina_Bool (*Enesim_Renderer_OpenGL_Initialize)(Enesim_Renderer *r,
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***programs);
typedef Eina_Bool (*Enesim_Renderer_OpenGL_Setup)(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Rop rop,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **error);
typedef void (*Enesim_Renderer_OpenGL_Cleanup)(Enesim_Renderer *r, Enesim_Surface *s);

typedef struct _Enesim_Renderer_Class
{
	Enesim_Object_Class base;
	Enesim_Renderer_Base_Name_Get_Cb base_name_get;
	Enesim_Renderer_Is_Supported_Cb is_supported;
	Enesim_Renderer_Bounds_Get_Cb bounds_get;
	Enesim_Renderer_Features_Get features_get;
	Enesim_Renderer_Is_Inside_Cb is_inside;
	Enesim_Renderer_Damages_Get_Cb damages_get;
	Enesim_Renderer_Has_Changed_Cb has_changed;
	Enesim_Renderer_Alpha_Hints_Get_Cb alpha_hints_get;
	/* software based functions */
	Enesim_Renderer_Sw_Hints_Get_Cb sw_hints_get;
	Enesim_Renderer_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	/* opencl based functions */
	Enesim_Renderer_OpenCL_Setup opencl_setup;
	Enesim_Renderer_OpenCL_Kernel_Setup opencl_kernel_setup;
	Enesim_Renderer_OpenCL_Cleanup opencl_cleanup;
	/* opengl based functions */
	Enesim_Renderer_OpenGL_Initialize opengl_initialize;
	Enesim_Renderer_OpenGL_Setup opengl_setup;
	Enesim_Renderer_OpenGL_Cleanup opengl_cleanup;
	/* we should expand from here */
} Enesim_Renderer_Class;


/*----------------------------------------------------------------------------*
 *                         Renderer related functions                         *
 *----------------------------------------------------------------------------*/
struct _Enesim_Renderer
{
	Enesim_Object_Instance base;
	EINA_MAGIC
	int ref;
	/* the private data */
	Eina_Lock lock;
	Eina_Hash *prv_data;
	int current_features;
	Enesim_Rectangle current_bounds;
	Enesim_Rectangle past_bounds;
	Eina_Rectangle current_destination_bounds;
	Eina_Rectangle past_destination_bounds;
	Enesim_Rop current_rop;
	char *name;
	Enesim_Renderer_State state;
	/* backend data */
	/* given that we can use the same renderer to draw into a software
	 * surface or opencl surface, we need an array to keep *ALL* the
	 * possible data */
	void *backend_data[ENESIM_BACKEND_LAST];
	Eina_Bool in_setup : 1;
};

void enesim_renderer_init(void);
void enesim_renderer_shutdown(void);

const Enesim_Renderer_State * enesim_renderer_state_get(Enesim_Renderer *r);
Eina_Bool enesim_renderer_state_has_changed(Enesim_Renderer *r);
void enesim_renderer_state_commit(Enesim_Renderer *r);

void enesim_renderer_propagate(Enesim_Renderer *r, Enesim_Renderer *to);
void * enesim_renderer_backend_data_get(Enesim_Renderer *r, Enesim_Backend b);
void enesim_renderer_backend_data_set(Enesim_Renderer *r, Enesim_Backend b, void *data);
void enesim_renderer_log_add(Enesim_Renderer *r, Enesim_Log **error, const char *file,
		const char *function, int line, char *fmt, ...);
Eina_Bool enesim_renderer_setup(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, Enesim_Log **error);
void enesim_renderer_cleanup(Enesim_Renderer *r, Enesim_Surface *s);


#if BUILD_OPENCL
Eina_Bool enesim_renderer_opencl_setup(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, Enesim_Log **error);
void enesim_renderer_opencl_cleanup(Enesim_Renderer *r, Enesim_Surface *s);
void enesim_renderer_opencl_draw(Enesim_Renderer *r, Enesim_Surface *s, Enesim_Rop rop,
		Eina_Rectangle *area, int x, int y);
void enesim_renderer_opencl_init(void);
void enesim_renderer_opencl_shutdown(void);
void enesim_renderer_opencl_free(Enesim_Renderer *r);
#endif

#endif
