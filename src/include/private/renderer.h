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

#if BUILD_OPENCL
typedef struct _Enesim_Renderer_OpenCL_Data
{
	cl_kernel kernel;
} Enesim_Renderer_OpenCL_Data;

Eina_Bool enesim_renderer_opencl_setup(Enesim_Renderer *r, Enesim_Surface *s,
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

struct _Enesim_Renderer
{
	EINA_MAGIC
	int ref;
	/* the renderer common properties */
	char *name;
	Enesim_Rop rop;
	Enesim_Color color;
	Enesim_Quality quality;
	double ox, oy; /* the origin */
	double sx, sy; /* the scale */
	struct {
		Enesim_Matrix original;
		Enesim_F16p16_Matrix values;
		Enesim_Matrix_Type type;
	} matrix;
	/* the private data */
	Eina_Hash *prv_data;
	/* the descriptor */
	Enesim_Renderer_Descriptor *descriptor;
	void *data;
	/* backend data */
	/* given that we can use the same renderer to draw into a software
	 * surface or opencl surface, we need an array to keep *ALL* the
	 * possible data */
	void *backend_data[ENESIM_BACKENDS];
};

typedef struct _Enesim_Renderer_State
{
	Enesim_Rop rop;
	Enesim_Color color;
	Enesim_Quality quality;
	double ox, oy; /* the origin */
	double sx, sy; /* the scale */
	struct {
		Enesim_Matrix original;
		Enesim_F16p16_Matrix values;
		Enesim_Matrix_Type type;
	} matrix;
} Enesim_Renderer_State;

typedef struct _Enesim_Renderer_Shape
{
	struct {
		Enesim_Color color;
		Enesim_Renderer *rend; /* TODO */
		double weight;
		Enesim_Matrix original;
	} stroke;

	struct {
		Enesim_Color color;
		Enesim_Renderer *rend;
		Enesim_Matrix original;
		double ox, oy;
	} fill;
	Enesim_Shape_Draw_Mode draw_mode;
	void *data;
} Enesim_Renderer_Shape;

typedef struct _Enesim_Renderer_Shape_State
{
	struct {
		Enesim_Color color;
		Enesim_Renderer *rend; /* TODO */
		double weight;
		Enesim_Matrix original;
	} stroke;

	struct {
		Enesim_Color color;
		Enesim_Renderer *rend;
		Enesim_Matrix original;
		double ox, oy;
	} fill;
	Enesim_Shape_Draw_Mode draw_mode;
} Enesim_Renderer_Shape_State;

/* Helper functions needed by other renderers */
static inline Eina_F16p16 enesim_point_f16p16_transform(Eina_F16p16 x, Eina_F16p16 y,
		Eina_F16p16 cx, Eina_F16p16 cy, Eina_F16p16 cz)
{
	return eina_f16p16_mul(cx, x) + eina_f16p16_mul(cy, y) + cz;
}

static inline void renderer_identity_setup(Enesim_Renderer *r, int x, int y,
		Eina_F16p16 *fpx, Eina_F16p16 *fpy)
{
	Eina_F16p16 ox, oy;

	ox = eina_f16p16_double_from(r->ox);
	oy = eina_f16p16_double_from(r->oy);

	*fpx = eina_f16p16_int_from(x);
	*fpy = eina_f16p16_int_from(y);

	*fpx = eina_f16p16_sub(*fpx, ox);
	*fpy = eina_f16p16_sub(*fpy, oy);
}

/*
 * x' = (xx * x) + (xy * y) + xz;
 * y' = (yx * x) + (yy * y) + yz;
 */
static inline void renderer_affine_setup(Enesim_Renderer *r, int x, int y,
		Eina_F16p16 *fpx, Eina_F16p16 *fpy)
{
	Eina_F16p16 xx, yy;
	Eina_F16p16 ox, oy;

	ox = eina_f16p16_double_from(r->ox);
	oy = eina_f16p16_double_from(r->oy);

	xx = eina_f16p16_int_from(x);
	yy = eina_f16p16_int_from(y);

	*fpx = enesim_point_f16p16_transform(xx, yy, r->matrix.values.xx,
			r->matrix.values.xy, r->matrix.values.xz);
	*fpy = enesim_point_f16p16_transform(xx, yy, r->matrix.values.yx,
			r->matrix.values.yy, r->matrix.values.yz);

	*fpx = eina_f16p16_sub(*fpx, ox);
	*fpy = eina_f16p16_sub(*fpy, oy);
}

/*
 * x' = (xx * x) + (xy * y) + xz;
 * y' = (yx * x) + (yy * y) + yz;
 * z' = (zx * x) + (zy * y) + zz;
 */
static inline void renderer_projective_setup(Enesim_Renderer *r, int x, int y,
		Eina_F16p16 *fpx, Eina_F16p16 *fpy, Eina_F16p16 *fpz)
{
	Eina_F16p16 xx, yy;
	Eina_F16p16 ox, oy;

	ox = eina_f16p16_double_from(r->ox);
	oy = eina_f16p16_double_from(r->oy);

	xx = eina_f16p16_int_from(x);
	yy = eina_f16p16_int_from(y);
	*fpx = enesim_point_f16p16_transform(xx, yy, r->matrix.values.xx,
			r->matrix.values.xy, r->matrix.values.xz);
	*fpy = enesim_point_f16p16_transform(xx, yy, r->matrix.values.yx,
			r->matrix.values.yy, r->matrix.values.yz);
	*fpz = enesim_point_f16p16_transform(xx, yy, r->matrix.values.zx,
			r->matrix.values.zy, r->matrix.values.zz);

	*fpx = eina_f16p16_sub(*fpx, ox);
	*fpy = eina_f16p16_sub(*fpy, oy);
}

void enesim_renderer_relative_set(Enesim_Renderer *r, Enesim_Renderer *rel, Enesim_Matrix *old_matrix, double *old_ox, double *old_oy);
void enesim_renderer_relative_unset(Enesim_Renderer *r1, Enesim_Renderer *rel, Enesim_Matrix *old_matrix, double old_ox, double old_oy);

/* common shape renderer functions */
Enesim_Renderer * enesim_renderer_shape_new(Enesim_Renderer_Descriptor *descriptor, void *data);
Eina_Bool enesim_renderer_shape_setup(Enesim_Renderer *r);
Eina_Bool enesim_renderer_shape_sw_setup(Enesim_Renderer *r, Enesim_Surface *s, Enesim_Error **error);
void enesim_renderer_shape_sw_cleanup(Enesim_Renderer *r);
void enesim_renderer_shape_cleanup(Enesim_Renderer *r);
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
	Enesim_Renderer_Inside is_inside;
} Enesim_Renderer_Gradient_Descriptor;

Enesim_Renderer * enesim_renderer_gradient_new(Enesim_Renderer_Gradient_Descriptor *gdescriptor, void *data);
void * enesim_renderer_gradient_data_get(Enesim_Renderer *r);
Enesim_Color enesim_renderer_gradient_color_get(Enesim_Renderer *r, Eina_F16p16 pos);

void * enesim_renderer_backend_data_get(Enesim_Renderer *r, Enesim_Backend b);
void enesim_renderer_backend_data_set(Enesim_Renderer *r, Enesim_Backend b, void *data);

/* sw backend */
void enesim_renderer_sw_init(void);
void enesim_renderer_sw_shutdown(void);
void enesim_renderer_sw_draw(Enesim_Renderer *r, Enesim_Surface *s, Eina_Rectangle *area,
		int x, int y, Enesim_Renderer_Flag flags);
void enesim_renderer_sw_draw_list(Enesim_Renderer *r, Enesim_Surface *s, Eina_Rectangle *area,
		Eina_List *clips, int x, int y, Enesim_Renderer_Flag flags);

/* opencl backend */
#endif
