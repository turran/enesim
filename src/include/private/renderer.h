/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2008 Jorge Luis Zapata
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


/* eina fixed point helpers, this should go to eina some day */
#define EINA_F16P16_ONE (1 << 16)
#define EINA_F16P16_HALF (1 << 15)

struct _Enesim_Renderer
{
	EINA_MAGIC;
	Enesim_Renderer_Flag flags;
	Enesim_Rop rop;
	/* FIXME why dont have a descriptor pointer directly? */
	/* common functions */
	Enesim_Renderer_Delete free;
	Enesim_Renderer_Boundings boundings;
	/* software backend */
	Enesim_Renderer_Sw_Fill sw_fill;
	Enesim_Renderer_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	void *data;
	/* the renderer common properties */
	Enesim_Color color;
	double ox, oy; /* the origin */
	struct {
		Enesim_Matrix original;
		Enesim_F16p16_Matrix values;
		Enesim_Matrix_Type type;
	} matrix;
};

typedef struct _Enesim_Renderer_Shape
{
	Enesim_Renderer base;
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
} Enesim_Renderer_Shape;

typedef struct _Enesim_Renderer_Gradient
{
	Enesim_Renderer base;
	Enesim_Renderer_Gradient_Mode mode;
	uint32_t *src;
	int slen;
	Eina_List *stops;
} Enesim_Renderer_Gradient;

/* Helper functions needed by other renderers */
static inline Eina_F16p16 enesim_point_f16p16_transform(Eina_F16p16 x, Eina_F16p16 y,
		Eina_F16p16 cx, Eina_F16p16 cy, Eina_F16p16 cz)
{
	return eina_f16p16_mul(cx, x) + eina_f16p16_mul(cy, y) + cz;
}

static inline void renderer_identity_setup(Enesim_Renderer *r, int x, int y,
		Eina_F16p16 *fpx, Eina_F16p16 *fpy)
{
	Eina_F16p16 xx, yy;
	Eina_F16p16 ox, oy;

	ox = eina_f16p16_float_from(r->ox);
	oy = eina_f16p16_float_from(r->oy);

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

	ox = eina_f16p16_float_from(r->ox);
	oy = eina_f16p16_float_from(r->oy);

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

	ox = eina_f16p16_float_from(r->ox);
	oy = eina_f16p16_float_from(r->oy);

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

Eina_Bool enesim_renderer_sw_setup(Enesim_Renderer *r);
void enesim_renderer_sw_cleanup(Enesim_Renderer *r);
void enesim_renderer_relative_set(Enesim_Renderer *r, Enesim_Renderer *rel, Enesim_Matrix *old_matrix, double *old_ox, double *old_oy);
void enesim_renderer_relative_unset(Enesim_Renderer *r1, Enesim_Renderer *rel, Enesim_Matrix *old_matrix, double old_ox, double old_oy);

/* common shape renderer functions */
void enesim_renderer_shape_init(Enesim_Renderer *r);
Eina_Bool enesim_renderer_shape_setup(Enesim_Renderer *r);
Eina_Bool enesim_renderer_shape_sw_setup(Enesim_Renderer *r);
void enesim_renderer_shape_cleanup(Enesim_Renderer *r);
/* common gradient renderer functions */
void enesim_renderer_gradient_init(Enesim_Renderer *r);
void enesim_renderer_gradient_state_setup(Enesim_Renderer *r, int len);

#endif
