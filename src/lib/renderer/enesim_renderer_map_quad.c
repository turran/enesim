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

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_quad.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_map_quad.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_color_private.h"
#include "enesim_renderer_private.h"
#include "enesim_draw_cache_private.h"
#include "enesim_coord_private.h"
/*
 * A colors and/or texture mapped quad renderer.
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */

#define ENESIM_RENDERER_MAP_QUAD(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Map_Quad,					\
		enesim_renderer_map_quad_descriptor_get())


typedef struct _Enesim_Renderer_Map_Quad
{
	Enesim_Renderer parent;
	/* properties */
	Enesim_Surface *src;
	Enesim_Argb vcolors[4];
	double vx[4], vy[4];

	/* The state variables */
	/* private */
	double qx[4], qy[4];
	int sgn[4];

	uint32_t *ssrc;
	int sw, sh;
	uint32_t *src_c;
	int sw_c, sh_c;

	Enesim_Matrix it, ct;
	int  lx, rx, ty, by;

	Enesim_Color rcolor;

	Eina_Bool changed;
} Enesim_Renderer_Map_Quad;

typedef struct _Enesim_Renderer_Map_Quad_Class {
	Enesim_Renderer_Class parent;
} Enesim_Renderer_Map_Quad_Class;

static Eina_Bool _map_quad_state_setup(Enesim_Renderer_Map_Quad *thiz,
		Enesim_Renderer *r, Enesim_Surface *s, Enesim_Rop rop,
		Enesim_Log **l)
{
	if (thiz->src)
	{
		/* lock the surface for read only */
		enesim_surface_lock(thiz->src, EINA_FALSE);
	}
	return EINA_TRUE;
}

static void _map_quad_state_cleanup(Enesim_Renderer_Map_Quad *thiz,
		Enesim_Renderer *r EINA_UNUSED, Enesim_Surface *s)
{
	if (thiz->src)
	{
		enesim_surface_unlock(thiz->src);
	}
	thiz->changed = EINA_FALSE;
}

static Enesim_Renderer_Sw_Fill _span_color;
static Enesim_Renderer_Sw_Fill _span_src;
static Enesim_Renderer_Sw_Fill _span_src_color;
/*----------------------------------------------------------------------------*
 *                        The Software fill variants                          *
 *----------------------------------------------------------------------------*/

static void
_span_color(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Map_Quad *thiz = ENESIM_RENDERER_MAP_QUAD(r);
	unsigned int *d = ddata, *e = d + len;
	int lx = thiz->rx, rx = thiz->lx;
	int i = 0, active = 0;
	unsigned int *src = thiz->src_c;
	int sw = thiz->sw_c, sh = thiz->sh_c;
	int sxx, syy, dxx, dyy;
	double sxf = 0, syf = 0;
	Enesim_Color color = thiz->rcolor;

	if ( (y < thiz->ty) || (y >= thiz->by) || 
		(x >= thiz->rx) || ((x + len) <= thiz->lx) || !src || !color)
	{
get_out:
		while (d < e)
			*d++ = 0;
		return;
	}

	if (color == 0xffffffff)
		color = 0;

	/* get left and right points on the quad on this scanline */
	while (i < 4)
	{
		int j = i+1;

		if (i == 3)  j = 0;

		if ( ((thiz->qy[i] < (y + 1)) && (thiz->qy[j] >= y)) ||
			((thiz->qy[j] < (y + 1)) && (thiz->qy[i] >= y)) )
		{
			double lxt, rxt;

			active++;

			if (thiz->sgn[i])
			{
				double d = (thiz->qx[j] - thiz->qx[i]) / (thiz->qy[j] - thiz->qy[i]);

				if (thiz->sgn[i] > 0)
				{
					lxt = (d * (y - thiz->qy[i])) + thiz->qx[i];
					rxt = (d * (y + 1 - thiz->qy[i])) + thiz->qx[i];
				}
				else
				{
					lxt = (d * (y + 1 - thiz->qy[i])) + thiz->qx[i];
					rxt = (d * (y - thiz->qy[i])) + thiz->qx[i];
				}
				if (lxt < MIN(thiz->qx[i], thiz->qx[j]))  
					lxt = MIN(thiz->qx[i], thiz->qx[j]);
				if (rxt > MAX(thiz->qx[i], thiz->qx[j]))  
					rxt = MAX(thiz->qx[i], thiz->qx[j]);
			}
			else
			{
				lxt = MIN(thiz->qx[i], thiz->qx[j]);
				rxt = MAX(thiz->qx[i], thiz->qx[j]);
			}
			if (lx > lxt)  lx = lxt;
			if (rx < rxt)  rx = rxt;

		}
		i++;
	}

	if (!active || ((rx - lx) < 1) || (rx <= x) || (lx >= (x + len)))
		goto get_out;

	/* get corresponding points and increments on src space */
	enesim_matrix_point_transform(&(thiz->ct), lx, y, &sxf, &syf);
	sxx = sxf * 65536;  syy = syf * 65536;
	enesim_matrix_point_transform(&(thiz->ct), rx, y, &sxf, &syf);
	dxx = ((sxf * 65536) - sxx) / (double)(rx - lx);
	dyy = ((syf * 65536) - syy) / (double)(rx - lx);

	rx -= x;
	if (len > rx)
	{
		len -= rx;
		memset(d + rx, 0, sizeof(unsigned int) * len);
		e -= len;
	}

	lx -= x;
	if (lx > 0)
	{
		memset(d, 0, sizeof(unsigned int) * lx);
		d += lx;
	}
	else
	{
		sxx -= lx * dxx;
		syy -= lx * dyy;
	}

	while (d < e)
	{
		unsigned int p0 = 0;

		x = sxx >> 16;
		y = syy >> 16;

		if ( (((unsigned) (x + 1)) < (sw + 1)) & (((unsigned) (y + 1)) < (sh + 1)) )
		{
			unsigned int *p, p1 = 0, p2 = 0, p3 = 0;

			p = src + (y * sw) + x;

			if ((x > -1) & (y > - 1))
				p0 = *p;

			if ((y > -1) & ((x + 1) < sw))
				p1 = *(p + 1);

			if ((y + 1) < sh)
			{
				if (x > -1)
					p2 = *(p + sw);
				if ((x + 1) < sw)
					p3 = *(p + sw + 1);
			}

			if (p0 | p1 | p2 | p3)
			{
				int ax, ay;

				ax = 1 + ((sxx & 0xffff) >> 8);
				ay = 1 + ((syy & 0xffff) >> 8);

				p0 = enesim_color_interp_256(ax, p1, p0);
				p2 = enesim_color_interp_256(ax, p3, p2);
				p0 = enesim_color_interp_256(ay, p2, p0);
			}
			if (p0 && ((p0>>24) < 255))
			{
				int a = 1 + (p0>>24);

				p0 = MUL_A_256(a, p0 | 0xff000000);
			}
			if (color && p0)
				p0 = enesim_color_mul4_sym(p0, color);
		}
		*d++ = p0;
		sxx += dxx; syy += dyy;
	}
}


static void
_span_src(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Map_Quad *thiz = ENESIM_RENDERER_MAP_QUAD(r);
	unsigned int *d = ddata, *e = d + len, *src = thiz->ssrc;
	int lx = thiz->rx, rx = thiz->lx;
	int i = 0, active = 0;
	int sw = thiz->sw, sh = thiz->sh;
	int sxx, syy, dxx, dyy;
	double sxf = 0, syf = 0;
	Enesim_Color color = thiz->rcolor;

	if ( (y < thiz->ty) || (y >= thiz->by) || 
		(x >= thiz->rx) || ((x + len) <= thiz->lx) || !src || !color)
	{
get_out:
		while (d < e)
		   *d++ = 0;
		return;
	}
	if (color == 0xffffffff)
		color = 0;

	/* get left and right points on the quad on this scanline */
	while (i < 4)
	{
		int j = i+1;

		if (i == 3)  j = 0;

		if ( ((thiz->qy[i] < (y + 1)) && (thiz->qy[j] >= y)) ||
			((thiz->qy[j] < (y + 1)) && (thiz->qy[i] >= y)) )
		{
			double lxt, rxt;

			active++;

			if (thiz->sgn[i])
			{
				double d = (thiz->qx[j] - thiz->qx[i]) / (thiz->qy[j] - thiz->qy[i]);

				if (thiz->sgn[i] > 0)
				{
					lxt = (d * (y - thiz->qy[i])) + thiz->qx[i];
					rxt = (d * (y + 1 - thiz->qy[i])) + thiz->qx[i];
				}
				else
				{
					lxt = (d * (y + 1 - thiz->qy[i])) + thiz->qx[i];
					rxt = (d * (y - thiz->qy[i])) + thiz->qx[i];
				}
				if (lxt < MIN(thiz->qx[i], thiz->qx[j]))
					lxt = MIN(thiz->qx[i], thiz->qx[j]);
				if (rxt > MAX(thiz->qx[i], thiz->qx[j]))
					rxt = MAX(thiz->qx[i], thiz->qx[j]);
			}
			else
			{
				lxt = MIN(thiz->qx[i], thiz->qx[j]);
				rxt = MAX(thiz->qx[i], thiz->qx[j]);
			}
			if (lx > lxt)  lx = lxt;
			if (rx < rxt)  rx = rxt;
		}
		i++;
	}

	if (!active || ((rx - lx) < 1) || (rx <= x) || (lx >= (x + len)))
		goto get_out;

	/* get corresponding points and increments on src space */
	enesim_matrix_point_transform(&(thiz->it), lx, y, &sxf, &syf);
	sxx = sxf * 65536;  syy = syf * 65536;
	enesim_matrix_point_transform(&(thiz->it), rx, y, &sxf, &syf);
	dxx = ((sxf * 65536) - sxx) / (double)(rx - lx);
	dyy = ((syf * 65536) - syy) / (double)(rx - lx);

	rx -= x;
	if (len > rx)
	{
		len -= rx;
		memset(d + rx, 0, sizeof(unsigned int) * len);
		e -= len;
	}

	lx -= x;
	if (lx > 0)
	{
		memset(d, 0, sizeof(unsigned int) * lx);
		d += lx;
	}
	else
	{
		sxx -= lx * dxx;
		syy -= lx * dyy;
	}

	while (d < e)
	{
		unsigned int p0 = 0;

		x = sxx >> 16;
		y = syy >> 16;

		if ( (((unsigned) (x + 1)) < (sw + 1)) & (((unsigned) (y + 1)) < (sh + 1)) )
		{
			unsigned int *p, p1 = 0, p2 = 0, p3 = 0;

			p = src + (y * sw) + x;

			if ((x > -1) & (y > - 1))
				p0 = *p;

			if ((y > -1) & ((x + 1) < sw))
				p1 = *(p + 1);

			if ((y + 1) < sh)
			{
				if (x > -1)
					p2 = *(p + sw);
				if ((x + 1) < sw)
					p3 = *(p + sw + 1);
			}

			if (p0 | p1 | p2 | p3)
			{
				int ax, ay;

				ax = 1 + ((sxx & 0xffff) >> 8);
				ay = 1 + ((syy & 0xffff) >> 8);

				p0 = enesim_color_interp_256(ax, p1, p0);
				p2 = enesim_color_interp_256(ax, p3, p2);
				p0 = enesim_color_interp_256(ay, p2, p0);
			}
			if (color && p0)
				p0 = enesim_color_mul4_sym(p0, color);
		}
		*d++ = p0;
		sxx += dxx; syy += dyy;
	}
}


static void
_span_src_color(Enesim_Renderer *r,
		int x, int y, int len, void *ddata)
{
	Enesim_Renderer_Map_Quad *thiz = ENESIM_RENDERER_MAP_QUAD(r);
	unsigned int *d = ddata, *e = d + len, *src = thiz->src;
	unsigned int *src_c = thiz->src_c;
	int lx = thiz->rx, rx = thiz->lx;
	int i = 0, active = 0;
	int sw = thiz->sw, sh = thiz->sh;
	int sw_c = thiz->sw_c, sh_c = thiz->sh_c;
	int sxx, syy, dxx, dyy;
	int sxx_c, syy_c, dxx_c, dyy_c;
	double sxf = 0, syf = 0;
	Enesim_Color = thiz->rcolor;

	if ( (y < thiz->ty) || (y >= thiz->by) || 
		(x >= thiz->rx) || ((x + len) <= thiz->lx) || !src || !src_c || !color)
	{
get_out:
		while (d < e)
		   *d++ = 0;
		return;
	}

	if (color == 0xffffffff)
		color = 0;
	/* get left and right points on the quad on this scanline */
	while (i < 4)
	{
		int j = i+1;

		if (i == 3)  j = 0;

		if ( ((thiz->qy[i] < (y + 1)) && (thiz->qy[j] >= y)) ||
			((thiz->qy[j] < (y + 1)) && (thiz->qy[i] >= y)) )
		{
			double lxt, rxt;

			active++;

			if (thiz->sgn[i])
			{
				double d = (thiz->qx[j] - thiz->qx[i]) / (thiz->qy[j] - thiz->qy[i]);

				if (thiz->sgn[i] > 0)
				{
					lxt = (d * (y - thiz->qy[i])) + thiz->qx[i];
					rxt = (d * (y + 1 - thiz->qy[i])) + thiz->qx[i];
				}
				else
				{
					lxt = (d * (y + 1 - thiz->qy[i])) + thiz->qx[i];
					rxt = (d * (y - thiz->qy[i])) + thiz->qx[i];
				}
				if (lxt < MIN(thiz->qx[i], thiz->qx[j]))  
					lxt = MIN(thiz->qx[i], thiz->qx[j]);
				if (rxt > MAX(thiz->qx[i], thiz->qx[j]))  
					rxt = MAX(thiz->qx[i], thiz->qx[j]);
			}
			else
			{
				lxt = MIN(thiz->qx[i], thiz->qx[j]);
				rxt = MAX(thiz->qx[i], thiz->qx[j]);
			}
			if (lx > lxt)  lx = lxt;
			if (rx < rxt)  rx = rxt;
		}
		i++;
	}

	if (!active || ((rx - lx) < 1) || (rx <= x) || (lx >= (x + len)))
		goto get_out;

	/* get corresponding points and increments on src space */
	enesim_matrix_point_transform(&(thiz->it), lx, y, &sxf, &syf);
	sxx = sxf * 65536;  syy = syf * 65536;
	enesim_matrix_point_transform(&(thiz->it), rx, y, &sxf, &syf);
	dxx = ((sxf * 65536) - sxx) / (double)(rx - lx);
	dyy = ((syf * 65536) - syy) / (double)(rx - lx);

	enesim_matrix_point_transform(&(thiz->ct), lx, y, &sxf, &syf);
	sxx_c = sxf * 65536;  syy_c = syf * 65536;
	enesim_matrix_point_transform(&(thiz->ct), rx, y, &sxf, &syf);
	dxx_c = ((sxf * 65536) - sxx_c) / (double)(rx - lx);
	dyy_c = ((syf * 65536) - syy_c) / (double)(rx - lx);

	rx -= x;
	if (len > rx)
	{
		len -= rx;
		memset(d + rx, 0, sizeof(unsigned int) * len);
		e -= len;
	}

	lx -= x;
	if (lx > 0)
	{
		memset(d, 0, sizeof(unsigned int) * lx);
		d += lx;
	}
	else
	{
		sxx -= lx * dxx;
		syy -= lx * dyy;
		sxx_c -= lx * dxx_c;
		syy_c -= lx * dyy_c;
	}

	while (d < e)
	{
		unsigned int p0 = 0, q0 = 0;

		x = sxx >> 16;
		y = syy >> 16;

		if ( (((unsigned) (x + 1)) < (sw + 1)) & (((unsigned) (y + 1)) < (sh + 1)) )
		{
			unsigned int *p, p1 = 0, p2 = 0, p3 = 0;

			p = src + (y * sw) + x;

			if ((x > -1) & (y > - 1))
				p0 = *p;

			if ((y > -1) & ((x + 1) < sw))
				p1 = *(p + 1);

			if ((y + 1) < sh)
			{
				if (x > -1)
					p2 = *(p + sw);
				if ((x + 1) < sw)
					p3 = *(p + sw + 1);
			}

			if (p0 | p1 | p2 | p3)
			{
				int ax, ay;

				ax = 1 + ((sxx & 0xffff) >> 8);
				ay = 1 + ((syy & 0xffff) >> 8);

				p0 = enesim_color_interp_256(ax, p1, p0);
				p2 = enesim_color_interp_256(ax, p3, p2);
				p0 = enesim_color_interp_256(ay, p2, p0);
			}
		}

		x = sxx_c >> 16; 
		y = syy_c >> 16;

		if ( (((unsigned) (x + 1)) < (sw_c + 1)) & (((unsigned) (y + 1)) < (sh_c + 1)) )
		{
			unsigned int *p, p1 = 0, p2 = 0, p3 = 0;

			p = src_c + (y * sw_c) + x;

			if ((x > -1) & (y > - 1))
				q0 = *p;

			if ((y > -1) & ((x + 1) < sw_c))
				p1 = *(p + 1);

			if ((y + 1) < sh_c)
			{
				if (x > -1)
					p2 = *(p + sw_c);
				if ((x + 1) < sw_c)
					p3 = *(p + sw_c + 1);
			}

			if (q0 | p1 | p2 | p3)
			{
				int ax, ay;

				ax = 1 + ((sxx_c & 0xffff) >> 8);
				ay = 1 + ((syy_c & 0xffff) >> 8);

				q0 = enesim_color_interp_256(ax, p1, q0);
				p2 = enesim_color_interp_256(ax, p3, p2);
				q0 = enesim_color_interp_256(ay, p2, q0);
			}
			if (q0 && ((q0>>24) < 255))
			{
				int a = 1 + (q0>>24);

				q0 = MUL_A_256(a, q0 | 0xff000000);
			}
		}
		if (p0 | q0)
			p0 = enesim_color_mul4_sym(p0, q0);
		if (color && p0)
			p0 = enesim_color_mul4_sym(p0, color);
		*d++ = p0;
		sxx += dxx; syy += dyy;
		sxx_c += dxx_c; syy_c += dyy_c;
	}
}



/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _map_quad_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "map_quad";
}

static Eina_Bool _map_quad_sw_vertices_setup(Enesim_Renderer *r)
{
	Enesim_Renderer_Map_Quad *thiz;
	int i;
	double lx, rx, ty, by;
	double fx, fy;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	/* assign initial working vertices */
	enesim_renderer_origin_get(r, &fx, &fy);
	i = 0;
	while(i < 4)
	{
		thiz->qx[i] = thiz->vx[i] - fx;
		thiz->qy[i] = thiz->vy[i] - fy;
		i++;
	}
	/* if there's a transform */
	if (enesim_renderer_transformation_type_get(r) != ENESIM_MATRIX_TYPE_IDENTITY)
	{
		Enesim_Matrix t;

		enesim_renderer_transformation_get(r, &t);
		i = 0;
		while(i < 4)
		{
			enesim_matrix_point_transform(&t, thiz->qx[i], thiz->qy[i], &fx, &fy);
			thiz->qx[i] = fx;
			thiz->qy[i] = fy;
			i++;
		}
	}

	/* get bounds */
	ty = by = thiz->qy[0];
	i = 1;
	while (i < 4)
	{
		if (thiz->qy[i] < ty)
			ty = thiz->qy[i];
		if (thiz->qy[i] > by)
			by = thiz->qy[i];
		i++;
	}
	lx = rx = thiz->qx[0];
	i = 1;
	while (i < 4)
	{
		if (thiz->qx[i] < lx)
			lx = thiz->qx[i];
		if (thiz->qx[i] > rx)
			rx = thiz->qx[i];
		i++;
	}
	if ((rx - lx) < 2) return EINA_FALSE;
	if ((by - ty) < 2) return EINA_FALSE;

	/* normalize and align working vertices */
	i = 0;
	fx = (rx - lx - 1) / (rx - lx);
	fy = (by - ty - 1) / (by - ty);
	while(i < 4)
	{
		thiz->qx[i] = ((int)(256 * (lx + ((thiz->qx[i] - lx) * fx)))) / 256.0;
		thiz->qy[i] = ((int)(256 * (ty + ((thiz->qy[i] - ty) * fy)))) / 256.0;
		i++;
	}

	thiz->lx = lx;  thiz->rx = rx - (1 / 256.0);
	thiz->ty = ty;  thiz->by = by - (1 / 256.0);

	/* get signs of vertex edges */
	i = 0;
	while (i < 4)
	{
		int j = i + 1;

		if (i == 3) j = 0;

		fx = thiz->qx[j] - thiz->qx[i];
		fy = thiz->qy[j] - thiz->qy[i];

		thiz->sgn[i] = 0;
		if ((fy != 0) && (fx != 0))
		{
			thiz->sgn[i] = 1;
			if (fy > 0)
			{
				if (fx < 0)
					thiz->sgn[i] = -1;
			}
			else
			{
				if (fx > 0)
					thiz->sgn[i] = -1;
			}
		}
		i++;
	}
	return EINA_TRUE;
}

static Eina_Bool _map_quad_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop, 
		Enesim_Renderer_Sw_Fill *fill, Enesim_Log **l)
{
	Enesim_Renderer_Map_Quad *thiz;
	Enesim_Quad sq, dq;
	Eina_Bool do_cols = 0;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	if (!_map_quad_state_setup(thiz, r, s, rop, l))
		return EINA_FALSE;
	if (thiz->src)
	{
		size_t stride;

		if (!enesim_surface_map(thiz->src, (void **)&thiz->ssrc, &stride))
		{
			_map_quad_state_cleanup(thiz, r, s);
			return EINA_FALSE;
		}
		enesim_surface_size_get(thiz->src, &(thiz->sw), &(thiz->sh));
		if ((thiz->sw < 2) || (thiz->sh < 2))
			return EINA_FALSE;
	}

	/* setup internal vertices and get bounds */
	if (!_map_quad_sw_vertices_setup(r))
		return EINA_FALSE;

	/* setup quad transforms */
	dq.x0 = thiz->qx[0];  dq.y0 = thiz->qy[0];
	dq.x1 = thiz->qx[1];  dq.y1 = thiz->qy[1];
	dq.x2 = thiz->qx[2];  dq.y2 = thiz->qy[2];
	dq.x3 = thiz->qx[3];  dq.y3 = thiz->qy[3];

	if (thiz->src)
	{
		sq.x0 = sq.y0 = 0;
		sq.x1 = thiz->sw - 1; sq.y1 = 0;
		sq.x2 = thiz->sw - 1; sq.y2 = thiz->sh - 1;
		sq.x3 = 0; sq.y3 = thiz->sh - 1;

		if (!enesim_matrix_quad_quad_map(&(thiz->it), &dq, &sq))
			return EINA_FALSE;

		if ( (thiz->vcolors[0] != 0xffffffff) | (thiz->vcolors[1] != 0xffffffff) | 
			(thiz->vcolors[2] != 0xffffffff) | (thiz->vcolors[3] != 0xffffffff) )
		{
			thiz->sw_c = thiz->sh_c = 3;
			if (!thiz->src_c)
				thiz->src_c = malloc(thiz->sw_c * thiz->sh_c * sizeof(unsigned int));
			if (thiz->src_c)
			{
				sq.x0 = sq.y0 = 0;
				sq.x1 = 2; sq.y1 = 0;
				sq.x2 = 2; sq.y2 = 2;
				sq.x3 = 0; sq.y3 = 2;
				if (!enesim_matrix_quad_quad_map(&(thiz->ct), &dq, &sq))
					return EINA_FALSE;
				thiz->src_c[0] = thiz->vcolors[0];  thiz->src_c[2] = thiz->vcolors[1];
				thiz->src_c[6] = thiz->vcolors[3];  thiz->src_c[8] = thiz->vcolors[2];
				thiz->src_c[1] = enesim_color_interp_256(128, thiz->src_c[0], thiz->src_c[2]);
				thiz->src_c[7] = enesim_color_interp_256(128, thiz->src_c[6], thiz->src_c[8]);
				thiz->src_c[3] = enesim_color_interp_256(128, thiz->src_c[0], thiz->src_c[6]);
				thiz->src_c[5] = enesim_color_interp_256(128, thiz->src_c[2], thiz->src_c[8]);
				thiz->src_c[4] = enesim_color_interp_256(128, thiz->src_c[3], thiz->src_c[5]);
				do_cols = 1;
			}
		}
		else
		{
			free(thiz->src_c);
			thiz->src_c = NULL;
		}
	}
	else
	{
		thiz->sw_c = thiz->sh_c = 3;
		if (!thiz->src_c)
			thiz->src_c = malloc(thiz->sw_c * thiz->sh_c * sizeof(unsigned int));
		if (thiz->src_c)
		{
			sq.x0 = sq.y0 = 0;
			sq.x1 = 2; sq.y1 = 0;
			sq.x2 = 2; sq.y2 = 2;
			sq.x3 = 0; sq.y3 = 2;
			if (!enesim_matrix_quad_quad_map(&(thiz->ct), &dq, &sq))
				return EINA_FALSE;
			thiz->src_c[0] = thiz->vcolors[0];  thiz->src_c[2] = thiz->vcolors[1];
			thiz->src_c[6] = thiz->vcolors[3];  thiz->src_c[8] = thiz->vcolors[2];
			thiz->src_c[1] = enesim_color_interp_256(128, thiz->src_c[0], thiz->src_c[2]);
			thiz->src_c[7] = enesim_color_interp_256(128, thiz->src_c[6], thiz->src_c[8]);
			thiz->src_c[3] = enesim_color_interp_256(128, thiz->src_c[0], thiz->src_c[6]);
			thiz->src_c[5] = enesim_color_interp_256(128, thiz->src_c[2], thiz->src_c[8]);
			thiz->src_c[4] = enesim_color_interp_256(128, thiz->src_c[3], thiz->src_c[5]);
		}
	}

	thiz->rcolor = enesim_renderer_color_get(r);
	/* assign span funcs */
	if (!thiz->src)
		*fill = _span_color;
	else
	{
		*fill = _span_src;
		if (do_cols)
			*fill = _span_src_color;
	}
	if (!*fill) return EINA_FALSE;

	return EINA_TRUE;
}


static void _map_quad_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Map_Quad *thiz;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	_map_quad_state_cleanup(thiz, r, s);
}

static void _map_quad_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_ARGB8888 | ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_PROJECTIVE;
}

static void _map_quad_sw_hints_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Rop rop EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_SW_HINT_COLORIZE;
}

static Eina_Bool _map_quad_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Map_Quad *thiz;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	return thiz->changed;
}

static void _map_quad_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *rect)
{
	Enesim_Renderer_Map_Quad *thiz;

	if (!_map_quad_sw_vertices_setup(r))
	{
		rect->x = rect->y = rect->w = rect->h = 0;
		return;
	}
	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	rect->x = thiz->lx;
	rect->y = thiz->ty;
	rect->w = thiz->rx - rect->x;
	rect->h = thiz->by - rect->y;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_DESCRIPTOR,
		Enesim_Renderer_Map_Quad, Enesim_Renderer_Map_Quad_Class,
		enesim_renderer_map_quad);

static void _enesim_renderer_map_quad_class_init(void *k)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _map_quad_name;
	klass->bounds_get = _map_quad_bounds_get;
	klass->features_get = _map_quad_features_get;
	klass->has_changed =  _map_quad_has_changed;
	klass->sw_hints_get = _map_quad_sw_hints_get;
	klass->sw_setup = _map_quad_sw_setup;
	klass->sw_cleanup = _map_quad_sw_cleanup;
}

static void _enesim_renderer_map_quad_instance_init(void *o)
{
	Enesim_Renderer_Map_Quad *thiz = ENESIM_RENDERER_MAP_QUAD(o);

	/* initial properties */
	thiz->vx[0] = thiz->vy[0] = 0;
	thiz->vx[1] = 2; thiz->vy[1] = 0;
	thiz->vx[2] = 2; thiz->vy[2] = 2;
	thiz->vx[3] = 0; thiz->vy[3] = 2;

	thiz->vcolors[0] = thiz->vcolors[1] = thiz->vcolors[2] = thiz->vcolors[3] = 0xffffffff;
}

static void _enesim_renderer_map_quad_instance_deinit(void *o EINA_UNUSED)
{
	Enesim_Renderer_Map_Quad *thiz = ENESIM_RENDERER_MAP_QUAD(o);

	if (thiz->src)
	{
		enesim_surface_unref(thiz->src);
		thiz->src = NULL;
	}
	if (thiz->src_c)
	{
		free(thiz->src);
		thiz->src_c = NULL;
	}
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Creates a new map_quad renderer
 * @return The renderer
 */
EAPI Enesim_Renderer * enesim_renderer_map_quad_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_map_quad);
	return r;
}

/**
 * @brief Sets a vertex's coordinates
 * @ender_prop{vertex}
 * @param[in] r The map quad renderer
 * @param[in] x The x coord of the vertex
 * @param[in] y The y coord of the vertex
 * @param[in] index The index the vertex
 */
EAPI void enesim_renderer_map_quad_vertex_set(Enesim_Renderer *r,
	double x, double y, int index)
{
	Enesim_Renderer_Map_Quad *thiz;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	if (!thiz) return;

	if (index < 0) index = 0;
	if (index > 3) index = 3;
	thiz->vx[index] = x;
	thiz->vy[index] = y;
	thiz->changed = EINA_TRUE;
}


/**
 * @brief Gets a vertex's coordinates
 * @ender_prop{vertex}
 * @param[in] r The map quad renderer
 * @param[in] x The x coord of the vertex to get
 * @param[in] y The y coord of the vertex to get
 * @param[in] index The index the vertex
 */
EAPI void enesim_renderer_map_quad_vertex_get(Enesim_Renderer *r,
	double *x, double *y, int index)
{
	Enesim_Renderer_Map_Quad *thiz;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	if (!thiz) return;

	if (index < 0) index = 0;
	if (index > 3) index = 3;
	if (x) *x = thiz->vx[index];
	if (y) *y = thiz->vy[index];
}

/**
 * @brief Sets the source surface to use as the source data
 * @ender_prop{source_surface}
 * @param[in] r The map quad renderer
 * @param[in] src The surface to map @ender_transfer{full}
 */
EAPI void enesim_renderer_map_quad_source_surface_set(Enesim_Renderer *r, Enesim_Surface *src)
{
	Enesim_Renderer_Map_Quad *thiz;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	if (!thiz) return;
	if (thiz->src)
	{
		enesim_surface_unref(thiz->src);
		thiz->src = NULL;
	}
	thiz->src = src;
	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the source surface used as the source data
 * @ender_prop{source_surface}
 * @param[in] r The map quad renderer
 * @return The surface to map @ender_transfer{none}
 */
EAPI Enesim_Surface * enesim_renderer_map_quad_source_surface_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Map_Quad *thiz;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	if (!thiz) return NULL;
	return enesim_surface_ref(thiz->src);
}

/**
 * @brief Sets the non-premul color for a vertex
 * @ender_prop{vertex_color}
 * @param[in] r The map quad renderer
 * @param[in] color The argb color
 * @param[in] index The vertex index
 */
EAPI void enesim_renderer_map_quad_vertex_color_set(Enesim_Renderer *r, Enesim_Argb color, int index)
{
	Enesim_Renderer_Map_Quad *thiz;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	if (!thiz) return;

	if (index < 0) index = 0;
	if (index > 3) index = 3;
	thiz->vcolors[index] = color;

	thiz->changed = EINA_TRUE;
}

/**
 * @brief Gets the color of a vertex
 * @ender_prop{vertex_color}
 * @param[in] r The map quad renderer
 * @param[in] index The vertex index
 * @return the color of the vertex
 */
EAPI Enesim_Argb enesim_renderer_map_quad_vertex_color_get(Enesim_Renderer *r, int index)
{
	Enesim_Renderer_Map_Quad *thiz;

	thiz = ENESIM_RENDERER_MAP_QUAD(r);
	if (!thiz) return 0;

	if (index < 0) index = 0;
	if (index > 3) index = 3;
	return thiz->vcolors[index];
}

