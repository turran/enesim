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
#ifndef VECTOR_H_
#define VECTOR_H_

/* The z is required for the gl backend
 * to avoid allocating a new GLdouble[3]
 */
typedef struct _Enesim_Point
{
	double x;
	double y;
	double z;
} Enesim_Point;

typedef struct _Enesim_Line
{
	double a;
	double b;
	double c;
} Enesim_Line;

typedef struct _Enesim_Polygon
{
	Eina_List *points;
	Eina_Bool closed : 1;
	double threshold;
	double xmax;
	double xmin;
	double ymax;
	double ymin;
} Enesim_Polygon;

typedef struct _Enesim_Figure
{
	Eina_List *polygons;
} Enesim_Figure;

typedef struct _Enesim_F16p16_Point
{
	Eina_F16p16 x;
	Eina_F16p16 y;
} Enesim_F16p16_Point;

typedef struct _Enesim_F16p16_Line
{
	Eina_F16p16 a;
	Eina_F16p16 b;
	Eina_F16p16 c;
} Enesim_F16p16_Line;

typedef struct _Enesim_F16p16_Vector
{
	int xx0, yy0, xx1, yy1;
	int a, b, c;
	int sgn;
} Enesim_F16p16_Vector;

typedef struct _Enesim_F16p16_Edge
{
	int xx0, yy0, xx1, yy1;
	int e, de;
	int lx;
	int counted : 1;
} Enesim_F16p16_Edge;

/* p0 the initial point
 * vx the x direction
 * vy the y direction
 */
static inline void enesim_f16p16_line_f16p16_direction_from(Enesim_F16p16_Line *l,
		Enesim_F16p16_Point *p0, Eina_F16p16 vx, Eina_F16p16 vy)
{
	l->a = vy;
	l->b = -vx;
	l->c = eina_f16p16_mul(vx, p0->y) - eina_f16p16_mul(vy, p0->x);
}

static inline void enesim_line_direction_from(Enesim_Line *l,
		Enesim_Point *p0, double vx, double vy)
{
	l->a = vy;
	l->b = -vx;
	l->c = (vx * p0->y) - (vy * p0->x);
}

static inline Eina_F16p16 enesim_f16p16_line_affine_setup(Enesim_F16p16_Line *l,
		Eina_F16p16 xx, Eina_F16p16 yy)
{
	Eina_F16p16 ret;

	ret = eina_f16p16_mul(l->a, xx) + eina_f16p16_mul(l->b, yy) + l->c;
	return ret;
}

Enesim_Point * enesim_point_new(void);
Enesim_Point * enesim_point_new_from_coords(double x, double y);

Enesim_Polygon * enesim_polygon_new(void);
void enesim_polygon_delete(Enesim_Polygon *thiz);
int enesim_polygon_point_count(Enesim_Polygon *thiz);
void enesim_polygon_point_append_from_coords(Enesim_Polygon *thiz, double x, double y);
void enesim_polygon_point_prepend_from_coords(Enesim_Polygon *thiz, double x, double y);
void enesim_polygon_clear(Enesim_Polygon *thiz);
void enesim_polygon_close(Enesim_Polygon *thiz, Eina_Bool close);
void enesim_polygon_merge(Enesim_Polygon *thiz, Enesim_Polygon *to_merge);
Eina_Bool enesim_polygon_bounds(const Enesim_Polygon *thiz, double *xmin, double *ymin, double *xmax, double *ymax);
void enesim_polygon_threshold_set(Enesim_Polygon *p, double threshold);

Enesim_Figure * enesim_figure_new(void);
void enesim_figure_delete(Enesim_Figure *thiz);
int enesim_figure_polygon_count(Enesim_Figure *thiz);
void enesim_figure_polygon_append(Enesim_Figure *thiz, Enesim_Polygon *p);
void enesim_figure_polygon_remove(Enesim_Figure *thiz, Enesim_Polygon *p);
Eina_Bool enesim_figure_bounds(const Enesim_Figure *thiz, double *xmin, double *ymin, double *xmax, double *ymax);
void enesim_figure_clear(Enesim_Figure *thiz);
void enesim_figure_dump(Enesim_Figure *f);

#endif
