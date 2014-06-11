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
#include "enesim_private.h"
#include "libargb.h"

#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_coord_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_coord_identity_setup(Eina_F16p16 *fpx, Eina_F16p16 *fpy,
		int x, int y, double pre_x, double pre_y)
{
	Eina_F16p16 ox, oy;

	ox = eina_f16p16_double_from(pre_x);
	oy = eina_f16p16_double_from(pre_y);

	*fpx = eina_f16p16_int_from(x);
	*fpy = eina_f16p16_int_from(y);

	*fpx = eina_f16p16_sub(*fpx, ox);
	*fpy = eina_f16p16_sub(*fpy, oy);
}

/*
 * x' = (xx * x) + (xy * y) + xz;
 * y' = (yx * x) + (yy * y) + yz;
 */
void enesim_coord_affine_setup(Eina_F16p16 *fpx, Eina_F16p16 *fpy,
		int x, int y, double pre_x, double pre_y,
		const Enesim_Matrix_F16p16 *matrix)
{
	Eina_F16p16 xx, yy;
	Eina_F16p16 ox, oy;

	ox = eina_f16p16_double_from(pre_x);
	oy = eina_f16p16_double_from(pre_y);

	xx = eina_f16p16_int_from(x);
	yy = eina_f16p16_int_from(y);

	*fpx = enesim_point_f16p16_transform(xx, yy, matrix->xx,
			matrix->xy, matrix->xz);
	*fpy = enesim_point_f16p16_transform(xx, yy, matrix->yx,
			matrix->yy, matrix->yz);

	*fpx = eina_f16p16_sub(*fpx, ox);
	*fpy = eina_f16p16_sub(*fpy, oy);
}

/*
 * x' = (xx * x) + (xy * y) + xz;
 * y' = (yx * x) + (yy * y) + yz;
 * z' = (zx * x) + (zy * y) + zz;
 */
void enesim_coord_projective_setup(Eina_F16p16 *fpx, Eina_F16p16 *fpy,
		Eina_F16p16 *fpz, int x, int y, double pre_x, double pre_y,
		const Enesim_Matrix_F16p16 *matrix)
{
	Eina_F16p16 xx, yy;
	Eina_F16p16 ox, oy;

	ox = eina_f16p16_double_from(pre_x);
	oy = eina_f16p16_double_from(pre_y);

	xx = eina_f16p16_int_from(x);
	yy = eina_f16p16_int_from(y);
	*fpx = enesim_point_f16p16_transform(xx, yy, matrix->xx,
			matrix->xy, matrix->xz);
	*fpy = enesim_point_f16p16_transform(xx, yy, matrix->yx,
			matrix->yy, matrix->yz);
	*fpz = enesim_point_f16p16_transform(xx, yy, matrix->zx,
			matrix->zy, matrix->zz);

	*fpx = eina_f16p16_sub(*fpx, ox);
	*fpy = eina_f16p16_sub(*fpy, oy);
}

/**
 * Sampling algorithms,
 * A pixel goes from 0 to 1, with its center placed on the top-left corner,
 * so a value of 0.9 is still inside the pixel
 */
/* A bilinear sampling */
uint32_t enesim_coord_sample_good_restrict(uint32_t *data, size_t stride, int sw,
		int sh, Eina_F16p16 xx, Eina_F16p16 yy)
{
	Eina_F16p16 sww;
	Eina_F16p16 shh;
	Eina_F16p16 minone;

	sww = eina_f16p16_int_from(sw) + EINA_F16P16_ONE;
	shh = eina_f16p16_int_from(sh) + EINA_F16P16_ONE;
	minone = eina_f16p16_int_from(-1);

	if (xx < sww && yy < shh && xx >= minone && yy >= minone)
	{
		int x, y;
		uint32_t p0 = 0, p1 = 0, p2 = 0, p3 = 0;

		/* we use the int_to given that it floors the value
		 * floor(-0.8) = -1, so we always pick the left-top pixel first
		 */
		x = eina_f16p16_int_to(xx);
		y = eina_f16p16_int_to(yy);

		/* pick the coord */
		data = argb8888_at(data, stride, x, y);

		if ((x > -1) && (y > - 1) && (x < sw) && (y < sh))
			p0 = *data;

		if ((y > -1) && (y < sh) && ((x + 1) < sw))
			p1 = *(data + 1);

		if ((y + 1) < sh)
		{
			if ((x > -1) && (x < sw))
				p2 = *((uint32_t *)((uint8_t *)data + stride));
			if ((x + 1) < sw)
				p3 = *((uint32_t *)((uint8_t *)data + stride) + 1);
		}

		if (p0 | p1 | p2 | p3)
		{
			uint16_t ax, ay;

			/* to make it be in the 1 - 256 range , note that for
			 * negative values, the fracc already does the two's complement
			 * and thus the value is correct
			 */
			ax = 1 + (eina_f16p16_fracc_get(xx) >> 8);
			ay = 1 + (eina_f16p16_fracc_get(yy) >> 8);

			p0 = argb8888_interp_256(ax, p1, p0);
			p2 = argb8888_interp_256(ax, p3, p2);
			p0 = argb8888_interp_256(ay, p2, p0);
		}
		return p0;
	}
	else
		return 0;
}

uint32_t enesim_coord_sample_good_repeat(uint32_t *data, size_t stride, int sw,
		int sh, Eina_F16p16 xx, Eina_F16p16 yy)
{
	Eina_F16p16 sww;
	Eina_F16p16 shh;
	Eina_F16p16 txx, tyy;
	Eina_F16p16 x1, y1;
	int x, y;
	uint32_t p0 = 0, p1 = 0, p2 = 0, p3 = 0;
	uint16_t ax, ay;

	sww = eina_f16p16_int_from(sw);
	shh = eina_f16p16_int_from(sh);

	/* trunc it */
	txx = eina_f16p16_int_from(eina_f16p16_int_to(xx));
	tyy = eina_f16p16_int_from(eina_f16p16_int_to(yy));

	/* get the real coords */
	x = eina_f16p16_int_to(enesim_coord_repeat(txx, sww));
	y = eina_f16p16_int_to(enesim_coord_repeat(tyy, shh));
	x1 = eina_f16p16_int_to(enesim_coord_repeat(txx + EINA_F16P16_ONE, sww));
	y1 = eina_f16p16_int_to(enesim_coord_repeat(tyy + EINA_F16P16_ONE, shh));
	p0 = *(argb8888_at(data, stride, x, y));
	p1 = *(argb8888_at(data, stride, x1, y));
	p2 = *(argb8888_at(data, stride, x, y1));
	p3 = *(argb8888_at(data, stride, x1, y1));

	/* sample */
	ax = 1 + (eina_f16p16_fracc_get(xx) >> 8);
	ay = 1 + (eina_f16p16_fracc_get(yy) >> 8);

	p0 = argb8888_interp_256(ax, p1, p0);
	p2 = argb8888_interp_256(ax, p3, p2);
	p0 = argb8888_interp_256(ay, p2, p0);
	return p0;
}

uint32_t enesim_coord_sample_good_reflect(uint32_t *data, size_t stride, int sw,
		int sh, Eina_F16p16 xx, Eina_F16p16 yy)
{
	Eina_F16p16 sww;
	Eina_F16p16 shh;
	Eina_F16p16 txx, tyy;
	Eina_F16p16 x1, y1;
	int x, y;
	uint32_t p0 = 0, p1 = 0, p2 = 0, p3 = 0;
	uint16_t ax, ay;

	sww = eina_f16p16_int_from(sw);
	shh = eina_f16p16_int_from(sh);

	/* trunc it */
	txx = eina_f16p16_int_from(eina_f16p16_int_to(xx));
	tyy = eina_f16p16_int_from(eina_f16p16_int_to(yy));

	/* get the real coords */
	x = eina_f16p16_int_to(enesim_coord_reflect(txx, sww));
	y = eina_f16p16_int_to(enesim_coord_reflect(tyy, shh));
	x1 = eina_f16p16_int_to(enesim_coord_reflect(txx + EINA_F16P16_ONE, sww));
	y1 = eina_f16p16_int_to(enesim_coord_reflect(tyy + EINA_F16P16_ONE, shh));
	p0 = *(argb8888_at(data, stride, x, y));
	p1 = *(argb8888_at(data, stride, x1, y));
	p2 = *(argb8888_at(data, stride, x, y1));
	p3 = *(argb8888_at(data, stride, x1, y1));

	/* sample */
	ax = 1 + (eina_f16p16_fracc_get(xx) >> 8);
	ay = 1 + (eina_f16p16_fracc_get(yy) >> 8);

	p0 = argb8888_interp_256(ax, p1, p0);
	p2 = argb8888_interp_256(ax, p3, p2);
	p0 = argb8888_interp_256(ay, p2, p0);
	return p0;
}

/*
 * Just pick the nearest pixel based on x,y
 */
uint32_t enesim_coord_sample_fast(uint32_t *data, size_t stride, int sw,
		int sh, Eina_F16p16 xx, Eina_F16p16 yy)
{
	int x, y;

	x = eina_f16p16_int_to(xx);
	y = eina_f16p16_int_to(yy);
	if (x < sw && y < sh && x >= 0 && y >= 0)
	{
		uint32_t *ret;

		ret = argb8888_at(data, stride, x, y);
		return *ret;
	}
	else
		return 0;
}
/** @endcond */