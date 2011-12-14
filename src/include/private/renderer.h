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
#ifndef RENDERER_H_
#define RENDERER_H_

/* TODO
 * + add a surface drawer too, not only span based :)
 * + add common parameters to the renderer here like transformation matrix and quality
 */
struct _Enesim_Renderer
{
	EINA_MAGIC
	int ref;
	Enesim_Renderer_State current;
	Enesim_Renderer_State past;
	/* the private data */
	Eina_Hash *prv_data;
	Enesim_Rectangle current_boundings;
	Enesim_Rectangle past_boundings;
	/* the descriptor */
	Enesim_Renderer_Descriptor descriptor;
	void *data;
	/* backend data */
	/* given that we can use the same renderer to draw into a software
	 * surface or opencl surface, we need an array to keep *ALL* the
	 * possible data */
	void *backend_data[ENESIM_BACKENDS];
};

void enesim_renderer_init(void);
void enesim_renderer_shutdown(void);

#if BUILD_OPENCL
typedef struct _Enesim_Renderer_OpenCL_Data
{
	cl_kernel kernel;
} Enesim_Renderer_OpenCL_Data;

Eina_Bool enesim_renderer_opencl_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		Enesim_Surface *s,
		Enesim_Error **error);
void enesim_renderer_opencl_cleanup(Enesim_Renderer *r, Enesim_Surface *s);
void enesim_renderer_opencl_draw(Enesim_Renderer *r, Enesim_Surface *s, Eina_Rectangle *area,
		int x, int y, Enesim_Renderer_Flag flags);
void enesim_renderer_opencl_init(void);
void enesim_renderer_opencl_shutdown(void);

#endif

#if BUILD_OPENGL
typedef struct _Enesim_Renderer_OpenGL_Data
{

} Enesim_Renderer_OpenGL_Data;
#endif


typedef struct _Enesim_Renderer_Sw_Data
{
	Enesim_Renderer_Sw_Fill fill;
} Enesim_Renderer_Sw_Data;


typedef struct _Enesim_Renderer_Shape_State
{
	struct {
		Enesim_Color color;
		Enesim_Renderer *r; /* TODO */
		double weight;
	} stroke;

	struct {
		Enesim_Color color;
		Enesim_Renderer *r;
	} fill;
	Enesim_Shape_Draw_Mode draw_mode;
} Enesim_Renderer_Shape_State;


typedef Eina_Bool (*Enesim_Renderer_Shape_Sw_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		Enesim_Surface *s,
		Enesim_Error **error);
typedef Eina_Bool (*Enesim_Renderer_Shape_OpenCL_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *state,
		const Enesim_Renderer_Shape_State *sstate,
		Enesim_Surface *s,
		const char **program_name, const char **program_source,
		size_t *program_length,
		Enesim_Error **error);

/* FIXME use this descriptor */
typedef struct _Enesim_Renderer_Shape_Descriptor {
	/* common */
	Enesim_Renderer_Name name;
	Enesim_Renderer_Delete free;
	Enesim_Renderer_Boundings boundings;
	Enesim_Renderer_Flags flags;
	Enesim_Renderer_Inside is_inside;
	Enesim_Renderer_Damage damage;
	Enesim_Renderer_Has_Changed has_changed;
	/* software based functions */
	Enesim_Renderer_Shape_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	/* opencl based functions */
	Enesim_Renderer_Shape_OpenCL_Setup opencl_setup;
	Enesim_Renderer_OpenCL_Kernel_Setup opencl_kernel_setup;
	Enesim_Renderer_OpenCL_Cleanup opencl_cleanup;
} Enesim_Renderer_Shape_Descriptor;

/* Helper functions needed by other renderers */
static inline Eina_F16p16 enesim_point_f16p16_transform(Eina_F16p16 x, Eina_F16p16 y,
		Eina_F16p16 cx, Eina_F16p16 cy, Eina_F16p16 cz)
{
	return eina_f16p16_mul(cx, x) + eina_f16p16_mul(cy, y) + cz;
}

void enesim_renderer_identity_setup(Enesim_Renderer *r, int x, int y,
		Eina_F16p16 *fpx, Eina_F16p16 *fpy);
void enesim_renderer_affine_setup(Enesim_Renderer *r, int x, int y,
		Enesim_F16p16_Matrix *matrix,
		Eina_F16p16 *fpx, Eina_F16p16 *fpy);
void enesim_renderer_projective_setup(Enesim_Renderer *r, int x, int y,
		Enesim_F16p16_Matrix *matrix,
		Eina_F16p16 *fpx, Eina_F16p16 *fpy, Eina_F16p16 *fpz);
void enesim_renderer_relative_set(Enesim_Renderer *r, Enesim_Renderer *rel, Enesim_Matrix *old_matrix, double *old_ox, double *old_oy, double *old_sx, double *old_sy);
void enesim_renderer_relative_unset(Enesim_Renderer *r1, Enesim_Renderer *rel, Enesim_Matrix *old_matrix, double old_ox, double old_oy, double old_sx, double old_sy);

/* common shape renderer functions */
Enesim_Renderer * enesim_renderer_shape_new(Enesim_Renderer_Descriptor *descriptor, void *data);
Eina_Bool enesim_renderer_shape_setup(Enesim_Renderer *r, const Enesim_Renderer_State *state, Enesim_Surface *s, Enesim_Error **error);
void enesim_renderer_shape_cleanup(Enesim_Renderer *r, Enesim_Surface *s);
void * enesim_renderer_shape_data_get(Enesim_Renderer *r);
/* common gradient renderer functions */
typedef Eina_F16p16 (*Enesim_Renderer_Gradient_Distance)(Enesim_Renderer *r, Eina_F16p16 x, Eina_F16p16 y);
typedef int (*Enesim_Renderer_Gradient_Length)(Enesim_Renderer *r);

typedef struct _Enesim_Renderer_Gradient_Descriptor
{
	Enesim_Renderer_Gradient_Distance distance;
	Enesim_Renderer_Gradient_Length length;
	Enesim_Renderer_Name name;
	Enesim_Renderer_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	Enesim_Renderer_Delete free;
	Enesim_Renderer_Boundings boundings;
	Enesim_Renderer_Destination_Transform destination_transform;
	Enesim_Renderer_Inside is_inside;
	Enesim_Renderer_Has_Changed has_changed;
} Enesim_Renderer_Gradient_Descriptor;

Enesim_Renderer * enesim_renderer_gradient_new(Enesim_Renderer_Gradient_Descriptor *gdescriptor, void *data);
void * enesim_renderer_gradient_data_get(Enesim_Renderer *r);
Enesim_Color enesim_renderer_gradient_color_get(Enesim_Renderer *r, Eina_F16p16 pos);

void * enesim_renderer_backend_data_get(Enesim_Renderer *r, Enesim_Backend b);
void enesim_renderer_backend_data_set(Enesim_Renderer *r, Enesim_Backend b, void *data);

/* sw backend */
void enesim_renderer_sw_init(void);
void enesim_renderer_sw_shutdown(void);
void enesim_renderer_sw_draw(Enesim_Renderer *r,
		Enesim_Surface *s, Eina_Rectangle *area,
		int x, int y, Enesim_Renderer_Flag flags);
void enesim_renderer_sw_draw_list(Enesim_Renderer *r,
		Enesim_Surface *s, Eina_Rectangle *area,
		Eina_List *clips, int x, int y, Enesim_Renderer_Flag flags);

Eina_Bool enesim_renderer_sw_setup(Enesim_Renderer *r, const Enesim_Renderer_State *state, Enesim_Surface *s, Enesim_Error **error);
void enesim_renderer_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s);

#endif
