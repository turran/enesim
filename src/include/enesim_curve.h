#ifndef _ENESIM_CURVE_H
#define _ENESIM_CURVE_H

typedef void (*Enesim_Curve_Vertex_Add_Callback)(double x, double y, void *data);

void enesim_curve3_decasteljau_generate(double x1, double y1, double x2, double y2,
		double x3, double y3, Enesim_Curve_Vertex_Add_Callback cb, void *data);

void enesim_curve4_decasteljau_generate(double x1, double y1, double x2, double y2,
		double x3, double y3, double x4, double y4, Enesim_Curve_Vertex_Add_Callback
		cb, void *data);
#endif
