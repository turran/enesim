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
#ifndef _ENESIM_RENDERER_PATH_KIIA_PRIVATE_H_
#define _ENESIM_RENDERER_PATH_KIIA_PRIVATE_H_

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_figure.h"
#include "enesim_path.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_color_private.h"
#include "enesim_color_fill_private.h"
#include "enesim_color_mul4_sym_private.h"
#include "enesim_list_private.h"
#include "enesim_vector_private.h"
#include "enesim_figure_private.h"
#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_path_abstract_private.h"

#define ENESIM_RENDERER_PATH_KIIA(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Path_Kiia,					\
		enesim_renderer_path_kiia_descriptor_get())


/* A worker is in charge of rasterize one span */
typedef struct _Enesim_Renderer_Path_Kiia_Worker
{
	/* a span of length equal to the width of the bounds to store the sample mask */
	void *mask;
	void *omask;
	/* keep track of the current Y this worker is doing, to ease
	 * the x increment on the edges
	 */
	int y;
} Enesim_Renderer_Path_Kiia_Worker;

typedef struct _Enesim_Renderer_Path_Kiia_Edge
{
	/* The top/bottom coordinates rounded to increment of 1/num samples */
	double y0, y1;
	double x0, x1;
	/* The x coordinate at yy0 */
	double mx;
	/* The increment on x when y increments 1/num samples */
	double slope;
	int sgn;
} Enesim_Renderer_Path_Kiia_Edge;

/* Its own definition of an edge */
typedef struct _Enesim_Renderer_Path_Kiia_Edge_Sw
{
	/* The top/bottom coordinates rounded to increment of 1/num samples */
	Eina_F16p16 yy0, yy1;
	/* The left/right coordinates in fixed point */
	Eina_F16p16 xx0, xx1;
	/* The x coordinate at yy0 */
	Eina_F16p16 mx;
	/* The increment on x when y increments 1/num samples */
	Eina_F16p16 slope;
	int sgn;
} Enesim_Renderer_Path_Kiia_Edge_Sw;

typedef struct _Enesim_Renderer_Path_Kiia_Figure
{
	Enesim_Figure *figure;
	void *edges;
	int nedges;
	Enesim_Renderer *ren;
	Enesim_Color color;
#if BUILD_OPENCL
	Enesim_Surface *s;
#endif
} Enesim_Renderer_Path_Kiia_Figure;

typedef struct _Enesim_Renderer_Path_Kiia
{
	Enesim_Renderer_Path_Abstract parent;
	/* private */
	/* True if the edges are already generated */
	Eina_Bool edges_generated;
	Enesim_Backend edges_generated_backend;
	/* The figures themselves */
	Enesim_Renderer_Path_Kiia_Figure fill;
	Enesim_Renderer_Path_Kiia_Figure stroke;
	Enesim_Renderer_Path_Kiia_Figure *current;

	/* The coordinates of the figure */
	int lx;
	int rx;
	Eina_F16p16 llx;
	/* The number of samples (8, 16, 32) */
	int nsamples;
	/* the increment on the y direction (i.e 1/num samples) */
	Eina_F16p16 inc;
	/* the pattern to use */
	Eina_F16p16 *pattern;
	/* One worker per cpu */
	Enesim_Renderer_Path_Kiia_Worker *workers;
	int nworkers;
} Enesim_Renderer_Path_Kiia;

typedef struct _Enesim_Renderer_Path_Kiia_Class {
	Enesim_Renderer_Path_Abstract_Class parent;
} Enesim_Renderer_Path_Kiia_Class;

/* different fill implementations */
void enesim_renderer_path_kiia_32_even_odd_color_simple(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_32_even_odd_renderer_simple(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_32_non_zero_color_simple(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_32_non_zero_renderer_simple(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);

void enesim_renderer_path_kiia_32_even_odd_color_color_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_32_non_zero_color_color_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_32_even_odd_renderer_color_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_32_even_odd_color_renderer_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_32_even_odd_renderer_renderer_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_32_non_zero_renderer_color_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_32_non_zero_color_renderer_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_32_non_zero_renderer_renderer_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);

void enesim_renderer_path_kiia_16_even_odd_color_simple(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_16_even_odd_renderer_simple(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_16_non_zero_color_simple(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_16_non_zero_renderer_simple(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);

void enesim_renderer_path_kiia_16_even_odd_color_color_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_16_non_zero_color_color_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_16_even_odd_renderer_color_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_16_even_odd_color_renderer_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_16_even_odd_renderer_renderer_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_16_non_zero_renderer_color_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_16_non_zero_color_renderer_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_16_non_zero_renderer_renderer_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);

void enesim_renderer_path_kiia_8_even_odd_color_simple(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_8_even_odd_renderer_simple(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_8_non_zero_color_simple(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_8_non_zero_renderer_simple(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);

void enesim_renderer_path_kiia_8_even_odd_color_color_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_8_non_zero_color_color_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_8_even_odd_renderer_color_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_8_even_odd_color_renderer_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_8_even_odd_renderer_renderer_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_8_non_zero_renderer_color_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_8_non_zero_color_renderer_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);
void enesim_renderer_path_kiia_8_non_zero_renderer_renderer_full(Enesim_Renderer *r,
		int x, int y, int len, void *ddata);

typedef void (*Enesim_Renderer_Path_Kiia_Worker_Setup)(Enesim_Renderer *r, int y, int len);

void enesim_renderer_path_kiia_32_even_odd_worker_setup(Enesim_Renderer *r, int y, int len);
void enesim_renderer_path_kiia_16_even_odd_worker_setup(Enesim_Renderer *r, int y, int len);
void enesim_renderer_path_kiia_8_even_odd_worker_setup(Enesim_Renderer *r, int y, int len);

void enesim_renderer_path_kiia_32_non_zero_worker_setup(Enesim_Renderer *r, int y, int len);
void enesim_renderer_path_kiia_16_non_zero_worker_setup(Enesim_Renderer *r, int y, int len);
void enesim_renderer_path_kiia_8_non_zero_worker_setup(Enesim_Renderer *r, int y, int len);

#endif
