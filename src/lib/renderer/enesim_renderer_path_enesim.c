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
#include "enesim_compositor.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_path.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#ifdef BUILD_OPENGL
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
#endif

#include "enesim_buffer_private.h"
#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_path_abstract_private.h"
#include "enesim_vector_private.h"
#include "enesim_rasterizer_private.h"
#include "enesim_curve_private.h"
#include "enesim_path_generator_private.h"

/* Some useful macros for debugging */
/* In case this is set, if the path has a stroke, only the stroke will be
 * rendered
 */
#define WIREFRAME 0
#define DUMP 0
/**
 * TODO
 * - Use the threshold on the curve state
 */
/*
 * Some formulas found on the research process:
 * l1 = Ax + By + C
 * l2 || l1 => l2 = Ax + By + C'
 * C' = C + d * hypot(A, B);
 */

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_PATH_ENESIM(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Path_Enesim,					\
		enesim_renderer_path_enesim_descriptor_get())

#if BUILD_OPENGL
typedef struct _Enesim_Renderer_Path_Enesim_OpenGL_Polygon
{
	GLenum type;
	Enesim_Polygon *polygon;
} Enesim_Renderer_Path_Enesim_OpenGL_Polygon;

typedef struct _Enesim_Renderer_Path_Enesim_OpenGL_Figure
{
	Eina_List *polygons;
	Enesim_Surface *tmp;
	Enesim_Surface *renderer_s;
	Eina_Bool needs_tesselate : 1;
} Enesim_Renderer_Path_Enesim_OpenGL_Figure;

typedef struct _Enesim_Renderer_Path_Enesim_OpenGL
{
	Enesim_Renderer_Path_Enesim_OpenGL_Figure stroke;
	Enesim_Renderer_Path_Enesim_OpenGL_Figure fill;
} Enesim_Renderer_Path_Enesim_OpenGL;
#endif

typedef struct _Enesim_Renderer_Path_Enesim
{
	Enesim_Renderer_Path_Abstract parent;
	/* properties */
	Eina_List *commands;
	/* private */
	Enesim_Path_Generator *stroke_path;
	Enesim_Path_Generator *strokeless_path;
	Enesim_Path_Generator *dashed_path;
#if BUILD_OPENGL
	Enesim_Renderer_Path_Enesim_OpenGL gl;
#endif
	/* TODO put the below data into a path_sw struct */
	Enesim_Figure *fill_figure;
	Enesim_Figure *stroke_figure;
	/* external properties that require a path generation */
	Enesim_Matrix last_matrix;
	Enesim_Shape_Stroke_Join last_join;
	Enesim_Shape_Stroke_Cap last_cap;
	double last_stroke_weight;
	/* internal stuff */
	Enesim_Renderer *bifigure;
	Eina_Bool changed : 1;
	Eina_Bool generated : 1;
} Enesim_Renderer_Path_Enesim;

typedef struct _Enesim_Renderer_Path_Enesim_Class
{
	Enesim_Renderer_Path_Abstract_Class parent;
} Enesim_Renderer_Path_Enesim_Class;

static void _path_span(Enesim_Renderer *r,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Path_Enesim *thiz;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	enesim_renderer_sw_draw(thiz->bifigure, x, y, len, ddata);
}

#if BUILD_OPENGL
static Eina_Bool _path_opengl_ambient_shader_setup(GLenum pid,
		Enesim_Color color)
{
	int final_color_u;

	glUseProgramObjectARB(pid);
	final_color_u = glGetUniformLocationARB(pid, "ambient_final_color");
	glUniform4fARB(final_color_u,
			argb8888_red_get(color) / 255.0,
			argb8888_green_get(color) / 255.0,
			argb8888_blue_get(color) / 255.0,
			argb8888_alpha_get(color) / 255.0);

	return EINA_TRUE;
}

#define GLERR {\
        GLenum err; \
        err = glGetError(); \
        printf("Error %x\n", err); \
        }

static Eina_Bool _path_opengl_merge_shader_setup(GLenum pid,
		GLenum texture0, GLenum texture1)
{
	int t;

	glUseProgramObjectARB(pid);
	t = glGetUniformLocationARB(pid, "merge_texture_0");

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture0);
	glUniform1i(t, 0);

	t = glGetUniformLocationARB(pid, "merge_texture_1");
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, texture1);
	glUniform1i(t, 1);

	glActiveTexture(GL_TEXTURE0);

	return EINA_TRUE;
}

static Enesim_Renderer_OpenGL_Shader _path_shader_ambient = {
	/* .type	= */ ENESIM_SHADER_FRAGMENT,
	/* .name	= */ "ambient",
	/* .source	= */
#include "enesim_renderer_opengl_common_ambient.glsl"
};

static Enesim_Renderer_OpenGL_Shader _path_shader_coordinates = {
	/* .type	= */ ENESIM_SHADER_VERTEX,
	/* .name	= */ "coordinates",
	/* .source	= */
#include "enesim_renderer_opengl_common_vertex.glsl"
};

static Enesim_Renderer_OpenGL_Shader _path_shader_merge = {
	/* .type	= */ ENESIM_SHADER_FRAGMENT,
	/* .name	= */ "merge",
	/* .source	= */
#include "enesim_renderer_opengl_common_merge.glsl"
};

static Enesim_Renderer_OpenGL_Shader _path_shader_silhoutte_vertex = {
	/* .type	= */ ENESIM_SHADER_VERTEX,
	/* .name	= */ "silhoutte_vertex",
	/* .source	= */
#include "enesim_renderer_path_silhoutte_vertex.glsl"
};

static Enesim_Renderer_OpenGL_Shader _path_shader_silhoutte_ambient = {
	/* .type	= */ ENESIM_SHADER_FRAGMENT,
	/* .name	= */ "silhoutte_ambient",
	/* .source	= */
#include "enesim_renderer_path_silhoutte_ambient.glsl"
};

static Enesim_Renderer_OpenGL_Shader *_path_simple_shaders[] = {
	&_path_shader_ambient,
	NULL,
};

static Enesim_Renderer_OpenGL_Shader *_path_complex_shaders[] = {
	&_path_shader_merge,
	&_path_shader_coordinates,
	NULL,
};

static Enesim_Renderer_OpenGL_Shader *_path_silhoutte_shaders[] = {
	&_path_shader_silhoutte_ambient,
	&_path_shader_silhoutte_vertex,
	NULL,
};

static Enesim_Renderer_OpenGL_Program _path_simple_program = {
	/* .name		= */ "path_simple",
	/* .shaders		= */ _path_simple_shaders,
	/* .num_shaders		= */ 1,
};

static Enesim_Renderer_OpenGL_Program _path_complex_program = {
	/* .name		= */ "path_complex",
	/* .shaders		= */ _path_complex_shaders,
	/* .num_shaders		= */ 2,
};

static Enesim_Renderer_OpenGL_Program _path_silhoutte_program = {
	/* .name		= */ "path_silhoutte",
	/* .shaders		= */ _path_silhoutte_shaders,
	/* .num_shaders		= */ 2,
};


static Enesim_Renderer_OpenGL_Program *_path_programs[] = {
	&_path_simple_program,
	&_path_complex_program,
	&_path_silhoutte_program,
	NULL,
};

static void _path_opengl_figure_clear(Enesim_Renderer_Path_Enesim_OpenGL_Figure *f)
{
	if (f->polygons)
	{
		Enesim_Renderer_Path_Enesim_OpenGL_Polygon *p;

		EINA_LIST_FREE(f->polygons, p)
		{
			enesim_polygon_delete(p->polygon);
			free(p);
		}
		f->polygons = NULL;
	}
}
/*----------------------------------------------------------------------------*
 *                            Tesselator callbacks                            *
 *----------------------------------------------------------------------------*/
static void _path_opengl_vertex_cb(GLvoid *vertex, void *data)
{
	Enesim_Renderer_Path_Enesim_OpenGL_Figure *f = data;
	Enesim_Renderer_Path_Enesim_OpenGL_Polygon *p;
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
	Enesim_Renderer_Path_Enesim_OpenGL_Polygon *p;
	Enesim_Renderer_Path_Enesim_OpenGL_Figure *f = data;

	/* add another polygon */
	p = calloc(1, sizeof(Enesim_Renderer_Path_Enesim_OpenGL_Polygon));
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

static void _path_opengl_tesselate(Enesim_Renderer_Path_Enesim_OpenGL_Figure *glf,
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
		Eina_List *last;

		last = eina_list_last(p->points);
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

static void _path_opengl_notesselate(Enesim_Renderer_Path_Enesim_OpenGL_Figure *glf)
{
	Eina_List *l1;
	Enesim_Renderer_Path_Enesim_OpenGL_Polygon *p;

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

static void _path_opengl_silhoutte_draw(Enesim_Figure *f, const Eina_Rectangle *area)
{
	Eina_List *l;
	Enesim_Polygon *p;

	glLineWidth(2);
	glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
	glShadeModel(GL_FLAT);
	EINA_LIST_FOREACH(f->polygons, l, p)
	{
		Enesim_Point *pt;
		Enesim_Point *last;
		Eina_List *l2;

		last = eina_list_data_get(p->points);
		l2 = p->points;

		glBegin(GL_LINE_STRIP);
		glVertex3f(last->x, last->y, 0.0);
		EINA_LIST_FOREACH(l2, l2, pt)
		{
			glTexCoord4f(last->x - area->x, area->h - (last->y - area->y),
					pt->x - area->x, area->h - (pt->y - area->y));
			glVertex3f(pt->x, pt->y, 0.0);
			last = pt;
		}
		if (p->closed)
		{
			pt = eina_list_data_get(p->points);
			glTexCoord4f(last->x - area->x, area->h - last->y - area->y, pt->x - area->x, area->h - pt->y - area->y);
			glVertex3f(pt->x, pt->y, 0.0);
		}
		glEnd();
	}
	glShadeModel(GL_SMOOTH);
	glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_TRUE);
}

static void _path_opengl_blit(GLenum fbo, GLenum dst,
		GLenum src,
		const Eina_Rectangle *area)
{
	/* merge the two */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			GL_TEXTURE_2D, dst, 0);

	glBindTexture(GL_TEXTURE_2D, src);
	enesim_opengl_rop_set(ENESIM_BLEND);
	glBegin(GL_QUADS);
		glTexCoord2d(0, 1);
		glVertex2d(area->x, area->y);

		glTexCoord2d(1, 1);
		glVertex2d(area->x + area->w, area->y);

		glTexCoord2d(1, 0);
		glVertex2d(area->x + area->w, area->y + area->h);

		glTexCoord2d(0, 0);
		glVertex2d(area->x, area->y + area->h);
	glEnd();
}

static void _path_opengl_figure_draw(GLenum fbo,
		GLenum texture,
		Enesim_Renderer_Path_Enesim_OpenGL_Figure *gf,
		Enesim_Figure *f,
		Enesim_Color color,
		Enesim_Renderer *rel,
		Enesim_Renderer_OpenGL_Data *rdata,
		Eina_Bool silhoutte,
		const Eina_Rectangle *area)
{
	Enesim_OpenGL_Compiled_Program *cp;
	/* TODO we still miss to render the relative renderer in case we have one */
	/* if so, render the inner renderer first into a temporary texture */
	/* else use the ambient shader */

	glViewport(0, 0, area->w, area->h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(area->x, area->x + area->w, area->y + area->h, area->y, -1, 1);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();
	//glOrtho(area->x - area->w, area->x + area->w, area->y - area->h, area->y + area->h, -1, 1);

	enesim_opengl_rop_set(ENESIM_FILL);
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			GL_TEXTURE_2D, texture, 0);

	if (silhoutte)
	{
		/* first fill the silhoutte (the anti alias border) */
		cp = &rdata->program->compiled[2];
		_path_opengl_ambient_shader_setup(cp->id, color);
		glUseProgramObjectARB(cp->id);
		_path_opengl_silhoutte_draw(f, area);
	}

	/* now fill the aliased figure on top */
	cp = &rdata->program->compiled[0];
	_path_opengl_ambient_shader_setup(cp->id, color);
	glUseProgramObjectARB(cp->id);

#if DEBUG
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif
	/* check if we need to tesselate again */
	if (gf->needs_tesselate)
	{
		_path_opengl_tesselate(gf, f);
	}
	/* if not, just use the cached vertices */
	else
	{
		_path_opengl_notesselate(gf);
	}
	glUseProgramObjectARB(0);
#if DEBUG
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
#endif
}

static void _path_opengl_stroke_renderer_setup(Enesim_Renderer *r,
	Enesim_Color color,
	Enesim_Color *final_color,
	Enesim_Renderer **orend)
{
	Enesim_Color scolor;

	enesim_renderer_shape_stroke_color_get(r, &scolor);
	enesim_renderer_shape_stroke_renderer_get(r, orend);
	/* multiply the color */
	if (scolor != ENESIM_COLOR_FULL)
		*final_color = argb8888_mul4_sym(color, scolor);
	else
		*final_color = color;
}

static void _path_opengl_fill_renderer_setup(Enesim_Renderer *r,
	Enesim_Color color,
	Enesim_Color *final_color,
	Enesim_Renderer **orend)
{
	Enesim_Color fcolor;

	enesim_renderer_shape_fill_color_get(r, &fcolor);
	enesim_renderer_shape_fill_renderer_get(r, orend);
	/* multiply the color */
	if (fcolor != ENESIM_COLOR_FULL)
		*final_color = argb8888_mul4_sym(color, fcolor);
	else
		*final_color = color;
}

static void _path_opengl_fill_or_stroke_draw(Enesim_Renderer *r,
		Enesim_Surface *s,
		const Eina_Rectangle *area, int w, int h)
{
	Enesim_Renderer_Path_Enesim *thiz;
	Enesim_Renderer_Path_Enesim_OpenGL *gl;
	Enesim_Renderer_Path_Enesim_OpenGL_Figure *gf;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_Renderer *rel;
	Enesim_Buffer_OpenGL_Data *sdata;
	Enesim_Shape_Draw_Mode dm;
	Enesim_Figure *f;
	Enesim_Color color;
	Enesim_Color final_color;
	GLint viewport[4];
	GLenum texture;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	gl = &thiz->gl;

	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	sdata = enesim_surface_backend_data_get(s);
	enesim_renderer_shape_draw_mode_get(r, &dm);
	enesim_renderer_color_get(r, &color);
	if (dm & ENESIM_SHAPE_DRAW_MODE_STROKE)
	{
		gf = &gl->stroke;
		f = thiz->stroke_figure;
		_path_opengl_stroke_renderer_setup(r, color, &final_color, &rel);
	}
	else
	{
		gf = &gl->fill;
		f = thiz->fill_figure;
		_path_opengl_fill_renderer_setup(r, color, &final_color, &rel);
	}

	/* create the texture */
	texture = enesim_opengl_texture_new(area->w, area->h);
	glGetIntegerv(GL_VIEWPORT, viewport);
	/* render there */
	_path_opengl_figure_draw(rdata->fbo, texture, gf, f, final_color, rel, rdata, EINA_TRUE, area);
	/* finally compose such texture with the destination texture */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, rdata->fbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			GL_TEXTURE_2D, sdata->texture, 0);
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, -1, 1);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glBindTexture(GL_TEXTURE_2D, texture);
	enesim_opengl_rop_set(ENESIM_BLEND);
	glBegin(GL_QUADS);
		glTexCoord2d(0, 1);
		glVertex2d(area->x, area->y);

		glTexCoord2d(1, 1);
		glVertex2d(area->x + area->w, area->y);

		glTexCoord2d(1, 0);
		glVertex2d(area->x + area->w, area->y + area->h);

		glTexCoord2d(0, 0);
		glVertex2d(area->x, area->y + area->h);
	glEnd();
	glBindTexture(GL_TEXTURE_2D, 0);
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	enesim_opengl_texture_free(texture);
	enesim_opengl_rop_set(ENESIM_FILL);
}

/* for fill and stroke we need to draw the stroke first on a
 * temporary fbo, then the fill into a temporary fbo too
 * finally draw again into the destination using the temporary
 * fbos as a source. If the pixel is different than transparent
 * then multiply that color with the current color
 */
static void _path_opengl_fill_and_stroke_draw(Enesim_Renderer *r,
		Enesim_Surface *s, const Eina_Rectangle *area, int w, int h)
{
	Enesim_Renderer_Path_Enesim *thiz;
	Enesim_Renderer_Path_Enesim_OpenGL *gl;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_Buffer_OpenGL_Data *sdata;
	Enesim_OpenGL_Compiled_Program *cp;
	Enesim_Renderer *rel;
	Enesim_Color color;
	Enesim_Color final_color;
	GLenum textures[2];
	GLint viewport[4];

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	gl = &thiz->gl;

	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	sdata = enesim_surface_backend_data_get(s);

	/* create the fill texture */
	textures[0] = enesim_opengl_texture_new(area->w, area->h);
	/* create the stroke texture */
	textures[1] = enesim_opengl_texture_new(area->w, area->h);

	enesim_renderer_color_get(r, &color);
	glGetIntegerv(GL_VIEWPORT, viewport);
	glViewport(0, 0, area->w, area->h);

	/* draw the fill into the newly created buffer */
	_path_opengl_fill_renderer_setup(r, color, &final_color, &rel);
	_path_opengl_figure_draw(rdata->fbo, textures[0], &gl->fill,
			thiz->fill_figure, final_color, rel, rdata, EINA_FALSE, area);
	/* draw the stroke into the newly created buffer */
	_path_opengl_stroke_renderer_setup(r, color, &final_color, &rel);
	/* FIXME this one is slow but only after the other */
	_path_opengl_figure_draw(rdata->fbo, textures[1], &gl->stroke,
			thiz->stroke_figure, final_color, rel, rdata, EINA_TRUE, area);
	/* now use the real destination surface to draw the merge fragment */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, rdata->fbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			GL_TEXTURE_2D, sdata->texture, 0);
	cp = &rdata->program->compiled[1];
	_path_opengl_merge_shader_setup(cp->id, textures[1], textures[0]);
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, -1, 1);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	enesim_opengl_rop_set(ENESIM_BLEND);
	//glBindTexture(GL_TEXTURE_2D, textures[0]);
	glBegin(GL_QUADS);
		glTexCoord2d(0, 1);
		glVertex2d(area->x, area->y);

		glTexCoord2d(1, 1);
		glVertex2d(area->x + area->w, area->y);

		glTexCoord2d(1, 0);
		glVertex2d(area->x + area->w, area->y + area->h);

		glTexCoord2d(0, 0);
		glVertex2d(area->x, area->y + area->h);
	glEnd();
	/* destroy the textures */
	enesim_opengl_texture_free(textures[0]);
	enesim_opengl_texture_free(textures[1]);
	/* don't use any program */
	glUseProgramObjectARB(0);
	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	enesim_opengl_rop_set(ENESIM_FILL);
}
#endif

static Eina_Bool _path_needs_generate(Enesim_Renderer_Path_Enesim *thiz,
		const Enesim_Matrix *cgm,
		double stroke_width,
		Enesim_Shape_Stroke_Join join,
		Enesim_Shape_Stroke_Cap cap)
{
	Eina_Bool ret = EINA_FALSE;
	/* some command has been added */
	if (thiz->changed && !thiz->generated)
		ret = EINA_TRUE;
	/* the stroke join is different */
	else if (thiz->last_join != join)
		ret = EINA_TRUE;
	/* the stroke cap is different */
	else if (thiz->last_cap != cap)
		ret = EINA_TRUE;
	else if (thiz->last_stroke_weight != stroke_width)
		ret = EINA_TRUE;
	/* the geometry transformation is different */
	else if (!enesim_matrix_is_equal(cgm, &thiz->last_matrix))
		ret = EINA_TRUE;
	return ret;
}

static void _path_generate_figures(Enesim_Renderer_Path_Enesim *thiz,
		Enesim_Shape_Draw_Mode dm,
		double sw,
		const Enesim_Matrix *transformation,
		double sx,
		double sy,
		Enesim_Shape_Stroke_Join join,
		Enesim_Shape_Stroke_Cap cap,
		Eina_List *dashes)
{
	Enesim_Path_Generator *path;
	Enesim_Figure *stroke_figure = NULL;

	if (thiz->fill_figure)
		enesim_figure_clear(thiz->fill_figure);
	else
		thiz->fill_figure = enesim_figure_new();

	if (thiz->stroke_figure)
		enesim_figure_clear(thiz->stroke_figure);
	else
		thiz->stroke_figure = enesim_figure_new();
	if ((dm & ENESIM_SHAPE_DRAW_MODE_STROKE) && (sw > 1.0))
	{
		if (!dashes)
			path = thiz->stroke_path;
		else
			path = thiz->dashed_path;
		stroke_figure = thiz->stroke_figure;
	}
	else
	{
		path = thiz->strokeless_path;

	}
	enesim_path_generator_figure_set(path, thiz->fill_figure);
	enesim_path_generator_stroke_figure_set(path, thiz->stroke_figure);
	enesim_path_generator_stroke_cap_set(path, cap);
	enesim_path_generator_stroke_join_set(path, join);
	enesim_path_generator_stroke_weight_set(path, sw);
	enesim_path_generator_stroke_dash_set(path, dashes);
	enesim_path_generator_scale_set(path, sx, sy);
	enesim_path_generator_transformation_set(path, transformation);
	enesim_path_generator_generate(path, thiz->commands);

	/* set the fill figure on the bifigure as its under polys */
	enesim_rasterizer_figure_set(thiz->bifigure, thiz->fill_figure);
#if WIREFRAME
	enesim_rasterizer_figure_set(thiz->bifigure, stroke_figure);
#else
	/* set the stroke figure on the bifigure as its under polys */
	enesim_rasterizer_bifigure_over_figure_set(thiz->bifigure, stroke_figure);
#endif

#if DUMP
	if (stroke_figure)
	{
		printf("stroke figure\n");
		enesim_figure_dump(stroke_figure);
	}
	printf("fill figure\n");
	enesim_figure_dump(thiz->fill_figure);
#endif

	thiz->generated = EINA_TRUE;
	/* update the last values */
	thiz->last_join = join;
	thiz->last_cap = cap;
	thiz->last_matrix = *transformation;
	thiz->last_stroke_weight = sw;
#if BUILD_OPENGL
	thiz->gl.fill.needs_tesselate = EINA_TRUE;
	thiz->gl.stroke.needs_tesselate = EINA_TRUE;
#endif
}


static void _path_state_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Path_Enesim *thiz;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	enesim_renderer_cleanup(thiz->bifigure, s);
	thiz->changed = EINA_FALSE;
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _path_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "path";
}

static Eina_Bool _path_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *draw, Enesim_Log **l)
{
	Enesim_Renderer_Path_Enesim *thiz;
	const Enesim_Renderer_State *cs;
	const Enesim_Renderer_Shape_State *css;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	cs = enesim_renderer_state_get(r);
	css = enesim_renderer_shape_state_get(r);

	/* generate the list of points/polygons */
	if (_path_needs_generate(thiz, &cs->current.transformation,
			css->current.stroke.weight,
			css->current.stroke.join, css->current.stroke.cap))
	{
		_path_generate_figures(thiz, css->current.draw_mode, css->current.stroke.weight,
				&cs->current.transformation, 1, 1,
				css->current.stroke.join, css->current.stroke.cap, css->stroke_dashes);
	}

#if WIREFRAME
	enesim_renderer_shape_draw_mode_set(thiz->bifigure, ENESIM_SHAPE_DRAW_MODE_STROKE);
#else
	enesim_renderer_shape_draw_mode_set(thiz->bifigure, css->current.draw_mode);
#endif
	enesim_renderer_shape_stroke_weight_set(thiz->bifigure, css->current.stroke.weight);
	enesim_renderer_shape_stroke_color_set(thiz->bifigure, css->current.stroke.color);
	enesim_renderer_shape_stroke_renderer_set(thiz->bifigure, css->current.stroke.r);
	enesim_renderer_shape_fill_color_set(thiz->bifigure, css->current.fill.color);
	enesim_renderer_shape_fill_renderer_set(thiz->bifigure, css->current.fill.r);
	enesim_renderer_shape_fill_rule_set(thiz->bifigure, css->current.fill.rule);

	enesim_renderer_color_set(thiz->bifigure, cs->current.color);
	enesim_renderer_origin_set(thiz->bifigure, cs->current.ox, cs->current.oy);

	if (!enesim_renderer_setup(thiz->bifigure, s, l))
	{
		return EINA_FALSE;
	}

	*draw = _path_span;

	return EINA_TRUE;
}

static void _path_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	_path_state_cleanup(r, s);
}

static void _path_features_get(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static void _path_sw_hints(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_COLORIZE;
}

static Eina_Bool _path_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Enesim *thiz;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	return thiz->changed;
}

static void _path_shape_features_get(Enesim_Renderer *r EINA_UNUSED, Enesim_Shape_Feature *features)
{
	*features = ENESIM_SHAPE_FLAG_FILL_RENDERER | ENESIM_SHAPE_FLAG_STROKE_RENDERER;
}

static void _path_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Path_Enesim *thiz;
	const Enesim_Renderer_State *cs;
	const Enesim_Renderer_Shape_State *css;
	double xmin;
	double ymin;
	double xmax;
	double ymax;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	cs = enesim_renderer_state_get(r);
	css = enesim_renderer_shape_state_get(r);

	if (_path_needs_generate(thiz, &cs->current.transformation,
			css->current.stroke.weight,
			css->current.stroke.join, css->current.stroke.cap))
	{
		_path_generate_figures(thiz, css->current.draw_mode, css->current.stroke.weight,
				&cs->current.transformation, 1, 1,
				css->current.stroke.join, css->current.stroke.cap, css->stroke_dashes);
	}

	if (!thiz->fill_figure)
	{
		bounds->x = 0;
		bounds->y = 0;
		bounds->w = 0;
		bounds->h = 0;
		return;
	}

	if ((css->current.draw_mode & ENESIM_SHAPE_DRAW_MODE_STROKE) && (css->current.stroke.weight > 1.0))
	{
		if (!enesim_figure_bounds(thiz->stroke_figure, &xmin, &ymin, &xmax, &ymax))
			goto failed;
	}
	else
	{
		if (!enesim_figure_bounds(thiz->fill_figure, &xmin, &ymin, &xmax, &ymax))
			goto failed;
	}

	bounds->x = xmin;
	bounds->w = xmax - xmin;
	bounds->y = ymin;
	bounds->h = ymax - ymin;

	/* translate by the origin */
	bounds->x += cs->current.ox;
	bounds->y += cs->current.oy;
	return;

failed:
	bounds->x = 0;
	bounds->y = 0;
	bounds->w = 0;
	bounds->h = 0;
}

#if BUILD_OPENGL
static Eina_Bool _path_opengl_initialize(Enesim_Renderer *r EINA_UNUSED,
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***programs)
{
	*programs = _path_programs;
	*num_programs = 3;
	return EINA_TRUE;
}

static Eina_Bool _path_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Path_Enesim *thiz;
	Enesim_Renderer_Path_Enesim_OpenGL *gl;
	Enesim_Shape_Draw_Mode dm;
	double sw;
	const Enesim_Renderer_State *cs;
	const Enesim_Renderer_Shape_State *css;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	cs = enesim_renderer_state_get(r);
	css = enesim_renderer_shape_state_get(r);
	gl = &thiz->gl;

	/* generate the figures */
	if (_path_needs_generate(thiz, &cs->current.transformation,
			css->current.stroke.weight,
			css->current.stroke.join, css->current.stroke.cap))
	{
		_path_generate_figures(thiz, css->current.draw_mode, css->current.stroke.weight,
				&cs->current.transformation, 1, 1,
				css->current.stroke.join, css->current.stroke.cap, css->stroke_dashes);
	}

	/* check what to draw, stroke, fill or stroke + fill */
	dm = css->current.draw_mode;
	sw = css->current.stroke.weight;
	/* fill + stroke */
	if (dm == ENESIM_SHAPE_DRAW_MODE_STROKE_FILL && (sw > 1.0))
	{
		*draw = _path_opengl_fill_and_stroke_draw;
	}
	else
	{
		*draw = _path_opengl_fill_or_stroke_draw;
	}

	return EINA_TRUE;
}

static void _path_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	_path_state_cleanup(r, s);
}
#endif

static void _path_commands_set(Enesim_Renderer *r, const Eina_List *commands)
{
	Enesim_Renderer_Path_Enesim *thiz;

	thiz = ENESIM_RENDERER_PATH_ENESIM(r);
	thiz->commands = commands;
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_PATH_ABSTRACT_DESCRIPTOR,
		Enesim_Renderer_Path_Enesim, Enesim_Renderer_Path_Enesim_Class,
		enesim_renderer_path_enesim);

static void _enesim_renderer_path_enesim_class_init(void *k)
{
	Enesim_Renderer_Path_Abstract_Class *klass;
	Enesim_Renderer_Shape_Class *s_klass;
	Enesim_Renderer_Class *r_klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _path_name;
	r_klass->bounds_get = _path_bounds_get;
	r_klass->features_get = _path_features_get;
	r_klass->has_changed = _path_has_changed;
	r_klass->sw_hints_get = _path_sw_hints;
#if BUILD_OPENGL
	r_klass->opengl_initialize = _path_opengl_initialize;
#endif

	s_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	s_klass->features_get = _path_shape_features_get;
	s_klass->sw_setup = _path_sw_setup;
	s_klass->sw_cleanup = _path_sw_cleanup;
#if BUILD_OPENGL
	s_klass->opengl_setup = _path_opengl_setup;
	s_klass->opengl_cleanup = _path_opengl_cleanup;
#endif

	klass = ENESIM_RENDERER_PATH_ABSTRACT_CLASS(k);
	klass->commands_set = _path_commands_set;
}

static void _enesim_renderer_path_enesim_instance_init(void *o)
{
	Enesim_Renderer_Path_Enesim *thiz;
	Enesim_Renderer *r;

	thiz = ENESIM_RENDERER_PATH_ENESIM(o);

	r = enesim_rasterizer_bifigure_new();
	thiz->bifigure = r;

	/* create the different path implementations */
	thiz->stroke_path = enesim_path_generator_stroke_dashless_new();
	thiz->strokeless_path = enesim_path_generator_strokeless_new();
	thiz->dashed_path = enesim_path_generator_dashed_new();
	/* FIXME for now */
	enesim_renderer_shape_stroke_join_set(ENESIM_RENDERER(o), ENESIM_JOIN_ROUND);
}

static void _enesim_renderer_path_enesim_instance_deinit(void *o)
{
	Enesim_Renderer_Path_Enesim *thiz;

	/* TODO: not finished */
	thiz = ENESIM_RENDERER_PATH_ENESIM(o);
	if (thiz->stroke_figure)
		enesim_figure_delete(thiz->stroke_figure);
	if (thiz->fill_figure)
		enesim_figure_delete(thiz->fill_figure);
	if (thiz->dashed_path)
		enesim_path_generator_free(thiz->dashed_path);
	if (thiz->strokeless_path)
		enesim_path_generator_free(thiz->strokeless_path);
	if (thiz->stroke_path)
		enesim_path_generator_free(thiz->stroke_path);
	if (thiz->bifigure)
		enesim_renderer_unref(thiz->bifigure);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_path_enesim_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_path_enesim);
	return r;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

