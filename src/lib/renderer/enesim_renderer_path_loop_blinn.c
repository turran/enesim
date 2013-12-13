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

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_path.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_path.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_path_private.h"
#include "enesim_list_private.h"
#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_path_abstract_private.h"
#include "enesim_path_normalizer_private.h"

#if BUILD_OPENGL
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
#endif

/* The idea is to tesselate the whole path without taking into account the
 * curves, just normalize theme to quadratic/cubic (i.e remove the arc).
 * once that's done we can use a shader and set the curve coefficients on the
 * vertex data, thus provide a simple in/out check to define the real curve
 * http://www.mdk.org.pl/2007/10/27/curvy-blues
 */
#if BUILD_OPENGL
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_PATH_LOOP_BLINN(o) ENESIM_OBJECT_INSTANCE_CHECK(o,	\
		Enesim_Renderer_Path_Loop_Blinn,				\
		enesim_renderer_path_loop_blinn_descriptor_get())

typedef struct _Enesim_Renderer_Path_OpenGL_Loop_Blinn_Polygon
{
	GLenum type;
	Enesim_Polygon *polygon;
} Enesim_Renderer_Path_OpenGL_Loop_Blinn_Polygon;

typedef struct _Enesim_Renderer_Path_OpenGL_Loop_Blinn_Path
{
	Eina_List *flat_polygons;
	Eina_List *curve_polygons;
} Enesim_Renderer_Path_OpenGL_Loop_Blinn_Path;

typedef struct _Enesim_Renderer_Path_OpenGL_Loop_Blinn
{
	Enesim_Renderer_Path_OpenGL_Loop_Blinn_Path stroke;
	Enesim_Renderer_Path_OpenGL_Loop_Blinn_Path fill;
} Enesim_Renderer_Path_OpenGL_Loop_Blinn;

typedef struct _Enesim_Renderer_Path_Loop_Blinn_Class
{
	Enesim_Renderer_Path_Abstract_Class parent;
} Enesim_Renderer_Path_Loop_Blinn_Class;

/*----------------------------------------------------------------------------*
 *                            Tesselator callbacks                            *
 *----------------------------------------------------------------------------*/
#if 0
static void _path_opengl_vertex_cb(GLvoid *vertex, void *data)
{
	Enesim_Renderer_Path_OpenGL_Loop_Blinn_Figure *f = data;
	Enesim_Renderer_Path_OpenGL_Loop_Blinn_Polygon *p;
	Enesim_Point *pt = vertex;
	Eina_List *l;

	/* get the last polygon */
	l = eina_list_last(f->polygons);
	p = eina_list_data_get(l);

	if (!p) return;

	/* add another vertex */
	enesim_polygon_point_append_from_coords(p->polygon, pt->x, pt->y);
	glVertex3f(pt->x, pt->y, 0.0);
}

static void _path_opengl_combine_cb(GLdouble coords[3],
		GLdouble *vertex_data[4] EINA_UNUSED,
		GLfloat weight[4] EINA_UNUSED, GLdouble **dataOut,
		void *data EINA_UNUSED)
{
	Enesim_Point *pt;

	pt = enesim_point_new();
	pt->x = coords[0];
	pt->y = coords[1];
	pt->z = coords[2];
	/* we dont have any information to interpolate with the weight */
	*dataOut = (GLdouble *)pt;
}

static void _path_opengl_begin_cb(GLenum which, void *data)
{
	Enesim_Renderer_Path_OpenGL_Loop_Blinn_Polygon *p;
	Enesim_Renderer_Path_OpenGL_Loop_Blinn_Figure *f = data;

	/* add another polygon */
	p = calloc(1, sizeof(Enesim_Renderer_Path_OpenGL_Loop_Blinn_Polygon));
	p->type = which;
	p->polygon = enesim_polygon_new();
	f->polygons = eina_list_append(f->polygons, p);

	glBegin(which);
}

static void _path_opengl_end_cb(void *data EINA_UNUSED)
{
	glEnd();
}

static void _path_opengl_error_cb(GLenum err_no EINA_UNUSED, void *data EINA_UNUSED)
{
}

static void _path_opengl_tesselate(
		Enesim_Renderer_Path_OpenGL_Loop_Blinn_Figure *glf,
		Enesim_Figure *f)
{
	Enesim_Polygon *p;
	Eina_List *l1;
	GLUtesselator *t;

	_path_opengl_figure_clear(glf);

	/* generate the figures */
	/* TODO we could use the tesselator directly on our own vertex generator? */
	t = gluNewTess();
	gluTessProperty(t, GLU_TESS_WINDING_RULE, GLU_TESS_WINDING_NONZERO);
	gluTessCallback(t, GLU_TESS_VERTEX_DATA, (_GLUfuncptr)&_path_opengl_vertex_cb);
	gluTessCallback(t, GLU_TESS_BEGIN_DATA, (_GLUfuncptr)&_path_opengl_begin_cb);
	gluTessCallback(t, GLU_TESS_END_DATA, (_GLUfuncptr)&_path_opengl_end_cb);
	gluTessCallback(t, GLU_TESS_COMBINE_DATA, (_GLUfuncptr)&_path_opengl_combine_cb);
	gluTessCallback(t, GLU_TESS_ERROR_DATA, (_GLUfuncptr)&_path_opengl_error_cb);

	gluTessBeginPolygon(t, glf);
	EINA_LIST_FOREACH (f->polygons, l1, p)
	{
		Enesim_Point *pt;
		Eina_List *l2;
		//Eina_List *last;

		//last = eina_list_last(p->points);
		gluTessBeginContour(t);
		EINA_LIST_FOREACH(p->points, l2, pt)
		{
			//if (last == l2)
			//	break;
			gluTessVertex(t, (GLdouble *)pt, pt);
		}
		if (p->closed)
		{
			pt = eina_list_data_get(p->points);
			gluTessVertex(t, (GLdouble *)pt, pt);
		}
		gluTessEndContour(t);
	}
	gluTessEndPolygon(t);
	glf->needs_tesselate = EINA_FALSE;
	gluDeleteTess(t);
}

static void _path_opengl_notesselate(
		Enesim_Renderer_Path_OpenGL_Loop_Blinn_Figure *glf)
{
	Eina_List *l1;
	Enesim_Renderer_Path_OpenGL_Loop_Blinn_Polygon *p;

	EINA_LIST_FOREACH(glf->polygons, l1, p)
	{
		Enesim_Point *pt;
		Eina_List *l2;

		glBegin(p->type);
		EINA_LIST_FOREACH(p->polygon->points, l2, pt)
		{
			glVertex3f(pt->x, pt->y, 0.0);
		}
		glEnd();
	}
}
#endif

static void _path_opengl_figure_draw(GLenum fbo,
		GLenum texture,
		Enesim_Renderer_Path_OpenGL_Loop_Blinn_Path *gl_path,
		Enesim_Path *path,
		Enesim_Color color,
		Enesim_Matrix *gm,
		Enesim_Renderer *rel EINA_UNUSED,
		Enesim_Renderer_OpenGL_Data *rdata,
		const Eina_Rectangle *area)
{
	Enesim_Path_Command *cmd;
	Enesim_Path_Command_Line_To line_to;
	Enesim_Path_Command_Move_To move_to;
	Enesim_Path_Command_Cubic_To cubic_to;
	Enesim_Path_Command_Scubic_To scubic_to;
	Enesim_Path_Command_Quadratic_To quadratic_to;
	Enesim_Path_Command_Squadratic_To squadratic_to;
	Enesim_Path_Command_Arc_To arc_to;
	Enesim_Path_Command_Close close;
	Enesim_Path_Cubic cubic;
	Enesim_Curve_Loop_Blinn_Classification classification;
	Eina_List *l;
	double last_x = 0, last_y = 0;

	/* normalize the path using loop&blinn functions */
	EINA_LIST_FOREACH(path->commands, l, cmd)
	{
		double x, y;
		double rx;
		double ry;
		double ctrl_x0;
		double ctrl_y0;
		double ctrl_x1;
		double ctrl_y1;
	
		switch (cmd->type)
		{
			case ENESIM_PATH_COMMAND_MOVE_TO:
			x = cmd->definition.move_to.x;
			y = cmd->definition.move_to.y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			last_x = x;
			last_y = y;
			break;

			case ENESIM_PATH_COMMAND_LINE_TO:
			x = cmd->definition.line_to.x;
			y = cmd->definition.line_to.y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			last_x = x;
			last_y = y;
			break;

#if 0
			case ENESIM_PATH_COMMAND_QUADRATIC_TO:
			x = cmd->definition.quadratic_to.x;
			y = cmd->definition.quadratic_to.y;
			ctrl_x0 = cmd->definition.quadratic_to.ctrl_x;
			ctrl_y0 = cmd->definition.quadratic_to.ctrl_y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			enesim_matrix_point_transform(gm, ctrl_x0, ctrl_y0, &ctrl_x0, &ctrl_y0);
			break;

			case ENESIM_PATH_COMMAND_SQUADRATIC_TO:
			x = scale_x * cmd->definition.squadratic_to.x;
			y = scale_y * cmd->definition.squadratic_to.y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			enesim_path_command_squadratic_to_values_from(&squadratic_to, x, y);
			enesim_path_normalizer_squadratic_to(normalizer, &squadratic_to);
			break;
#endif
			case ENESIM_PATH_COMMAND_CUBIC_TO:
			x = cmd->definition.cubic_to.x;
			y = cmd->definition.cubic_to.y;
			ctrl_x0 = cmd->definition.cubic_to.ctrl_x0;
			ctrl_y0 = cmd->definition.cubic_to.ctrl_y0;
			ctrl_x1 = cmd->definition.cubic_to.ctrl_x1;
			ctrl_y1 = cmd->definition.cubic_to.ctrl_y1;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			enesim_matrix_point_transform(gm, ctrl_x0, ctrl_y0, &ctrl_x0, &ctrl_y0);
			enesim_matrix_point_transform(gm, ctrl_x1, ctrl_y1, &ctrl_x1, &ctrl_y1);
			cubic.start_x = last_x; cubic.start_y = last_y;
			cubic.ctrl_x0 = ctrl_x0; cubic.ctrl_y0 = ctrl_y0;
			cubic.ctrl_x1 = ctrl_x1; cubic.ctrl_y1 = ctrl_y1;
			cubic.end_x = x; cubic.end_y = y;
			enesim_curve_loop_blinn_classify(&cubic, 0.0000001, &classification);
			printf("curve of type %d\n", classification.type);
			break;

#if 0
			case ENESIM_PATH_COMMAND_SCUBIC_TO:
			x = scale_x * cmd->definition.scubic_to.x;
			y = scale_y * cmd->definition.scubic_to.y;
			ctrl_x0 = scale_x * cmd->definition.scubic_to.ctrl_x;
			ctrl_y0 = scale_y * cmd->definition.scubic_to.ctrl_y;

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			enesim_matrix_point_transform(gm, ctrl_x0, ctrl_y0, &ctrl_x0, &ctrl_y0);
			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			enesim_path_command_scubic_to_values_from(&scubic_to, x, y, ctrl_x0, ctrl_y0);
			enesim_path_normalizer_scubic_to(normalizer, &scubic_to);
			break;

			case ENESIM_PATH_COMMAND_ARC_TO:
			x = scale_x * cmd->definition.arc_to.x;
			y = scale_y * cmd->definition.arc_to.y;
			rx = scale_x * cmd->definition.arc_to.rx;
			ry = scale_y * cmd->definition.arc_to.ry;
			ca = cos(cmd->definition.arc_to.angle * M_PI / 180.0);
			sa = sin(cmd->definition.arc_to.angle * M_PI / 180.0);

			enesim_matrix_point_transform(gm, x, y, &x, &y);
			rx = rx * hypot((ca * gm->xx) + (sa * gm->xy), (ca * gm->yx) + (sa * gm->yy));
			ry = ry * hypot((ca * gm->xy) - (sa * gm->xx), (ca * gm->yy) - (sa * gm->yx));
			ca = atan2((ca * gm->yx) + (sa * gm->yy), (ca * gm->xx) + (sa * gm->xy));

			x = ((int) (2*x + 0.5)) / 2.0;
			y = ((int) (2*y + 0.5)) / 2.0;
			enesim_path_command_arc_to_values_from(&arc_to, rx, ry, ca * 180.0 / M_PI,
					x, y, cmd->definition.arc_to.large,
					cmd->definition.arc_to.sweep);
			enesim_path_normalizer_arc_to(normalizer, &arc_to);
			break;

			case ENESIM_PATH_COMMAND_CLOSE:
			close.close = cmd->definition.close.close;
			enesim_path_normalizer_close(normalizer, &close);
			break;
#endif
			default:
			break;
		}
	}
}

static void _path_opengl_fill_and_stroke_draw(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop, const Eina_Rectangle *area,
		int x, int y)
{
}

static void _path_opengl_fill_or_stroke_draw(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop, const Eina_Rectangle *area,
		int x, int y)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Renderer_Path_OpenGL_Loop_Blinn *gl;
	Enesim_Renderer_Path_OpenGL_Loop_Blinn_Path *gl_path;
	Enesim_Path *path;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_Renderer_Shape_Draw_Mode dm;
	Enesim_Renderer *rel;
	Enesim_Buffer_OpenGL_Data *sdata;
	Enesim_Color final_color, color;
	Enesim_Matrix m;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	gl = &thiz->loop_blinn;

	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	sdata = enesim_surface_backend_data_get(s);
	dm = enesim_renderer_shape_draw_mode_get(r);
	enesim_renderer_transformation_get(r, &m);

	if (dm & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
	{
		printf("stroke\n");
		gl_path = &gl->stroke;
		path = thiz->path;
		enesim_renderer_shape_stroke_setup(r, &final_color, &rel);
	}
	else
	{
		printf("fill\n");
		gl_path = &gl->fill;
		path = thiz->path;
		enesim_renderer_shape_fill_setup(r, &final_color, &rel);
	}

	/* render there */
	_path_opengl_figure_draw(sdata->fbo, sdata->textures[0], gl_path, path, final_color, &m, rel, rdata, area);

	if (rel)
		enesim_renderer_unref(rel);
}

/*----------------------------------------------------------------------------*
 *                          Path Abstract interface                           *
 *----------------------------------------------------------------------------*/
static void _enesim_renderer_path_loop_blinn_path_set(Enesim_Renderer *r,
		Enesim_Path *path)
{
	Enesim_Renderer_Path_Loop_Blinn *thiz;

	thiz = ENESIM_RENDERER_PATH_LOOP_BLINN(r);
	if (thiz->path != path)
	{
		if (thiz->path) enesim_path_unref(thiz->path);
		thiz->path = path;
		thiz->new_path = EINA_TRUE;
		thiz->path_changed = EINA_TRUE;
		thiz->generated = EINA_FALSE;
	}
	else
	{
		enesim_path_unref(path);
	}
}

/*----------------------------------------------------------------------------*
 *                             Shape interface                                *
 *----------------------------------------------------------------------------*/
static void _enesim_renderer_path_loop_blinn_shape_features_get(
		Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Shape_Feature *features)
{
	*features = ENESIM_RENDERER_SHAPE_FEATURE_FILL_RENDERER |
			ENESIM_RENDERER_SHAPE_FEATURE_STROKE_RENDERER;
}

static Eina_Bool _enesim_renderer_path_loop_blinn_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Loop_Blinn *thiz;

	thiz = ENESIM_RENDERER_PATH_LOOP_BLINN(r);
	if (thiz->new_path || thiz->path_changed || (thiz->path && thiz->path->changed) || (thiz->dashes_changed))
		return EINA_TRUE;
	else
		return EINA_FALSE;
}

static Eina_Bool _enesim_renderer_path_loop_blinn_opengl_setup(
		Enesim_Renderer *r,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Shape_Draw_Mode dm;
	const Enesim_Renderer_Shape_State *css;

	css = enesim_renderer_shape_state_get(r);

	/* check what to draw, stroke, fill or stroke + fill */
	dm = css->current.draw_mode;
	/* fill + stroke */
	if (dm == ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE_FILL)
	{
		*draw = _path_opengl_fill_and_stroke_draw;
	}
	else
	{
		*draw = _path_opengl_fill_or_stroke_draw;
	}

	return EINA_TRUE;
}

static void _enesim_renderer_path_loop_blinn_opengl_cleanup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED)
{
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _enesim_renderer_path_loop_blinn_name(
		Enesim_Renderer *r EINA_UNUSED)
{
	return "enesim_path_loop_blinn";
}

static void _enesim_renderer_path_loop_blinn_features_get(
		Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_PROJECTIVE |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static Eina_Bool _enesim_renderer_path_loop_blinn_opengl_initialize(
		Enesim_Renderer *r EINA_UNUSED,
		int *num_programs EINA_UNUSED,
		Enesim_Renderer_OpenGL_Program ***programs EINA_UNUSED)
{
	return EINA_TRUE;
}

/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_PATH_ABSTRACT_DESCRIPTOR,
		Enesim_Renderer_Path_Loop_Blinn, Enesim_Renderer_Path_Loop_Blinn_Class,
		enesim_renderer_path_loop_blinn);

static void _enesim_renderer_path_loop_blinn_class_init(void *k)
{
	Enesim_Renderer_Path_Abstract_Class *klass;
	Enesim_Renderer_Shape_Class *s_klass;
	Enesim_Renderer_Class *r_klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _enesim_renderer_path_loop_blinn_name;
	r_klass->bounds_get = _enesim_renderer_path_loop_blinn_bounds_get;
	r_klass->features_get = _enesim_renderer_path_loop_blinn_features_get;
	r_klass->opengl_initialize = _enesim_renderer_path_loop_blinn_opengl_initialize;

	s_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	s_klass->features_get = _enesim_renderer_path_loop_blinn_shape_features_get;
	s_klass->has_changed = _enesim_renderer_path_loop_blinn_has_changed;
	s_klass->opengl_setup = _enesim_renderer_path_loop_blinn_opengl_setup;
	s_klass->opengl_cleanup = _enesim_renderer_path_loop_blinn_opengl_cleanup;

	klass = ENESIM_RENDERER_PATH_ABSTRACT_CLASS(k);
	klass->path_set = _enesim_renderer_path_loop_blinn_path_set;
	klass->is_available = _enesim_renderer_path_is_available;
}

static void _enesim_renderer_path_loop_blinn_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_renderer_path_loop_blinn_instance_deinit(void *o)
{
	Enesim_Renderer_Path_Loop_Blinn *thiz;

	thiz = ENESIM_RENDERER_PATH_LOOP_BLINN(o);
}
#endif
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_path_loop_blinn_new(void)
{
#if BUILD_OPENGL
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_path_loop_blinn);
	return r;
#else
	return NULL;
#endif
}
