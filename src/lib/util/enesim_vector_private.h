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

/*----------------------------------------------------------------------------*
 *                                 Points                                     *
 *----------------------------------------------------------------------------*/
Enesim_Point * enesim_point_new(void);
Enesim_Point * enesim_point_new_from_coords(double x, double y, double z);

static inline void enesim_point_3d_cross(Enesim_Point *p0, Enesim_Point *p1,
		Enesim_Point *r)
{
	r->x = (p0->y * p1->z) - (p1->y * p0->z);
	r->y = (p0->z * p1->x) - (p1->z * p0->x);
	r->z = (p0->x * p1->y) - (p1->x * p0->y);
}

static inline double enesim_point_3d_dot(Enesim_Point *p0, Enesim_Point *p1)
{
	return ((p0->x * p1->x) + (p0->y * p1->y) + (p0->z * p1->z));
}

static inline double enesim_point_2d_distance(Enesim_Point *p0, Enesim_Point *p1)
{
	return hypot(p1->x - p0->x, p1->y - p0->y);
}

static inline double enesim_point_3d_distance(Enesim_Point *p0, Enesim_Point *p1)
{
	double x = p1->x - p0->x;
	double y = p1->y - p0->y;
	double z = p1->z - p0->z;
	return sqrt(x*x + y*y + z*z);
}

static inline double enesim_point_2d_length(Enesim_Point *p0)
{
	return hypot(p0->x, p0->y);
}

static inline double enesim_point_3d_length(Enesim_Point *p0)
{
	return sqrt((p0->x* p0->x) + (p0->y * p0->y) + (p0->z * p0->z));
}

static inline double enesim_point_2d_length_squared(Enesim_Point *p0)
{
	return ((p0->x * p0->x) + (p0->y * p0->y));
}

static inline double enesim_point_3d_length_squared(Enesim_Point *p0)
{
	return ((p0->x* p0->x) + (p0->y * p0->y) + (p0->z * p0->z));
}

static inline Eina_Bool enesim_point_3d_normalize(Enesim_Point *p,
		Enesim_Point *r)
{
	double len;

	len = enesim_point_3d_length(p);
	if (len)
	{
		r->x = p->x / len;
		r->y = p->y / len;
		r->z = p->z / len;
		return EINA_TRUE;
	}
	return EINA_FALSE;
}

static inline Eina_Bool enesim_point_2d_normalize(Enesim_Point *p,
		Enesim_Point *r)
{
	double len;

	len = enesim_point_2d_length(p);
	if (len)
	{
		r->x = p->x / len;
		r->y = p->y / len;
		return EINA_TRUE;
	}
	return EINA_FALSE;
}

static inline void enesim_point_2d_sub(Enesim_Point *p0, Enesim_Point *p1,
		Enesim_Point *sub)
{
	sub->x = p0->x - p1->x;
	sub->y = p0->y - p1->y;
}

static inline void enesim_point_3d_sub(Enesim_Point *p0, Enesim_Point *p1,
		Enesim_Point *sub)
{
	sub->x = p0->x - p1->x;
	sub->y = p0->y - p1->y;
	sub->z = p0->z - p1->z;
}

static inline Eina_Bool enesim_point_2d_is_equal(Enesim_Point *p0,
		Enesim_Point *p1, double err)
{
	Enesim_Point sub;
	double d;
	double err2 = err * err;

	enesim_point_2d_sub(p0, p1, &sub);
 	d = enesim_point_2d_length_squared(&sub);
	if (d < err2) return EINA_TRUE;
	else return EINA_FALSE;
}

static inline Eina_Bool enesim_point_3d_is_equal(Enesim_Point *p0,
		Enesim_Point *p1, double err)
{
	Enesim_Point sub;
	double d;
	double err2 = err * err;

	enesim_point_3d_sub(p0, p1, &sub);
 	d = enesim_point_3d_length_squared(&sub);
	if (d < err2) return EINA_TRUE;
	else return EINA_FALSE;
}

static inline void enesim_point_coords_set(Enesim_Point *p, double x, double y,
		double z)
{
	p->x = x;
	p->y = y;
	p->z = z;
}

static inline void enesim_point_coords_get(Enesim_Point *p, double *x, double *y,
		double *z)
{
	*x = p->x;
	*y = p->y;
	*z = p->z;
}

/*----------------------------------------------------------------------------*
 *                                 Polygon                                    *
 *----------------------------------------------------------------------------*/

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

/*----------------------------------------------------------------------------*
 *                                  Figure                                    *
 *----------------------------------------------------------------------------*/
Enesim_Figure * enesim_figure_new(void);
void enesim_figure_delete(Enesim_Figure *thiz);
int enesim_figure_polygon_count(Enesim_Figure *thiz);
void enesim_figure_polygon_append(Enesim_Figure *thiz, Enesim_Polygon *p);
void enesim_figure_polygon_remove(Enesim_Figure *thiz, Enesim_Polygon *p);
Eina_Bool enesim_figure_bounds(const Enesim_Figure *thiz, double *xmin, double *ymin, double *xmax, double *ymax);
void enesim_figure_clear(Enesim_Figure *thiz);
void enesim_figure_dump(Enesim_Figure *f);

#endif
