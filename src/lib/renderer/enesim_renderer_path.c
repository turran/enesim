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
#include "enesim_error.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_path.h"

#ifdef BUILD_OPENGL
#include "Enesim_OpenGL.h"
#endif

#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_vector_private.h"
#include "enesim_rasterizer_private.h"
#include "enesim_curve_private.h"
#include "enesim_path_private.h"

/* Some useful macros for debugging */
/* In case this is set, if the path has a stroke, only the stroke will be
 * rendered
 */
#define WIREFRAME 0

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
#define ENESIM_RENDERER_PATH_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_PATH_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_PATH_MAGIC);\
	} while(0)

#if BUILD_OPENGL
typedef struct _Enesim_Renderer_Path_OpenGL_Polygon
{
	GLenum type;
	Enesim_Polygon *polygon;
} Enesim_Renderer_Path_OpenGL_Polygon;

typedef struct _Enesim_Renderer_Path_OpenGL_Figure
{
	Eina_List *polygons;
	Enesim_Surface *tmp;
	Enesim_Surface *renderer_s;
	Eina_Bool needs_tesselate : 1;
} Enesim_Renderer_Path_OpenGL_Figure;

typedef struct _Enesim_Renderer_Path_OpenGL
{
	Enesim_Renderer_Path_OpenGL_Figure stroke;
	Enesim_Renderer_Path_OpenGL_Figure fill;
} Enesim_Renderer_Path_OpenGL;
#endif

typedef struct _Enesim_Renderer_Path_State
{
	Enesim_Renderer_State2 rstate;
	Enesim_Renderer_Shape_State2 sstate;
} Enesim_Renderer_Path_State;

typedef struct _Enesim_Renderer_Path
{
	EINA_MAGIC
	/* properties */
	Enesim_Renderer_Path_State state;
	Eina_List *commands;
	/* private */
	Enesim_Path *stroke_path;
	Enesim_Path *strokeless_path;
	Enesim_Path *dashed_path;
#if BUILD_OPENGL
	Enesim_Renderer_Path_OpenGL gl;
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
} Enesim_Renderer_Path;

static inline Enesim_Renderer_Path * _path_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	ENESIM_RENDERER_PATH_MAGIC_CHECK(thiz);

	return thiz;
}
static void _path_span(Enesim_Renderer *r,
		const Enesim_Renderer_State2 *state EINA_UNUSED,
		const Enesim_Renderer_Shape_State2 *sstate EINA_UNUSED,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Path *thiz;

	thiz = _path_get(r);
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

static void _path_opengl_figure_clear(Enesim_Renderer_Path_OpenGL_Figure *f)
{
	if (f->polygons)
	{
		Enesim_Renderer_Path_OpenGL_Polygon *p;

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
	Enesim_Renderer_Path_OpenGL_Figure *f = data;
	Enesim_Renderer_Path_OpenGL_Polygon *p;
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
	Enesim_Renderer_Path_OpenGL_Polygon *p;
	Enesim_Renderer_Path_OpenGL_Figure *f = data;

	/* add another polygon */
	p = calloc(1, sizeof(Enesim_Renderer_Path_OpenGL_Polygon));
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

static void _path_opengl_tesselate(Enesim_Renderer_Path_OpenGL_Figure *glf,
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
	gluTessCallback(t, GLU_TESS_VERTEX_DATA, &_path_opengl_vertex_cb);
	gluTessCallback(t, GLU_TESS_BEGIN_DATA, &_path_opengl_begin_cb);
	gluTessCallback(t, GLU_TESS_END_DATA, &_path_opengl_end_cb);
	gluTessCallback(t, GLU_TESS_COMBINE_DATA, &_path_opengl_combine_cb);
	gluTessCallback(t, GLU_TESS_ERROR_DATA, &_path_opengl_error_cb);

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

static void _path_opengl_notesselate(Enesim_Renderer_Path_OpenGL_Figure *glf)
{
	Eina_List *l1;
	Enesim_Renderer_Path_OpenGL_Polygon *p;

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
		Enesim_Renderer_Path_OpenGL_Figure *gf,
		Enesim_Figure *f,
		Enesim_Color color,
		Enesim_Renderer *rel,
		Enesim_Renderer_OpenGL_Data *rdata,
		Eina_Bool silhoutte,
		const Eina_Rectangle *area)
{
	Enesim_Renderer_OpenGL_Compiled_Program *cp;
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
	Enesim_Renderer_Path *thiz;
	Enesim_Renderer_Path_OpenGL *gl;
	Enesim_Renderer_Path_OpenGL_Figure *gf;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_Renderer *rel;
	Enesim_Buffer_OpenGL_Data *sdata;
	Enesim_Shape_Draw_Mode dm;
	Enesim_Figure *f;
	Enesim_Color color;
	Enesim_Color final_color;
	GLint viewport[4];
	GLenum texture;

	thiz = _path_get(r);
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
	Enesim_Renderer_Path *thiz;
	Enesim_Renderer_Path_OpenGL *gl;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_Buffer_OpenGL_Data *sdata;
	Enesim_Renderer_OpenGL_Compiled_Program *cp;
	Enesim_Renderer *rel;
	Enesim_Color color;
	Enesim_Color final_color;
	GLenum textures[2];
	GLint viewport[4];

	thiz = _path_get(r);
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

static Eina_Bool _path_needs_generate(Enesim_Renderer_Path *thiz,
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

static void _path_generate_figures(Enesim_Renderer_Path *thiz,
		Enesim_Shape_Draw_Mode dm,
		double sw,
		const Enesim_Matrix *transformation,
		double sx,
		double sy,
		Enesim_Shape_Stroke_Join join,
		Enesim_Shape_Stroke_Cap cap,
		Eina_List *dashes)
{
	Enesim_Path *path;
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
	enesim_path_figure_set(path, thiz->fill_figure);
	enesim_path_stroke_figure_set(path, thiz->stroke_figure);
	enesim_path_stroke_cap_set(path, cap);
	enesim_path_stroke_join_set(path, join);
	enesim_path_stroke_weight_set(path, sw);
	enesim_path_stroke_dash_set(path, dashes);
	enesim_path_scale_set(path, sx, sy);
	enesim_path_transformation_set(path, transformation);
	enesim_path_generate(path, thiz->commands);

	/* set the fill figure on the bifigure as its under polys */
	enesim_rasterizer_figure_set(thiz->bifigure, thiz->fill_figure);
#if WIREFRAME
	enesim_rasterizer_figure_set(thiz->bifigure, stroke_figure);
#else
	/* set the stroke figure on the bifigure as its under polys */
	enesim_rasterizer_bifigure_over_figure_set(thiz->bifigure, stroke_figure);
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
	Enesim_Renderer_Path *thiz;

	thiz = _path_get(r);
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

static void _path_free(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;

	/* TODO: not finished */
	thiz = _path_get(r);
	if (thiz->stroke_figure)
		enesim_figure_delete(thiz->stroke_figure);
	if (thiz->fill_figure)
		enesim_figure_delete(thiz->fill_figure);
	if (thiz->dashed_path)
		enesim_path_free(thiz->dashed_path);
	if (thiz->strokeless_path)
		enesim_path_free(thiz->strokeless_path);
	if (thiz->stroke_path)
		enesim_path_free(thiz->stroke_path);
	if (thiz->bifigure)
		enesim_renderer_unref(thiz->bifigure);
	enesim_renderer_path_command_clear(r);
	free(thiz);
}

static Eina_Bool _path_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *draw, Enesim_Error **error)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Color color;
	const Enesim_Renderer_State2 *cs = &thiz->state.rstate;
	const Enesim_Renderer_Shape_State2 *css = &thiz->state.sstate;

	thiz = _path_get(r);

	enesim_renderer_color_get(r, &color);
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

	enesim_renderer_color_set(thiz->bifigure, color);
	enesim_renderer_origin_set(thiz->bifigure, cs->current.ox, cs->current.oy);

	if (!enesim_renderer_setup(thiz->bifigure, s, error))
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

static void _path_flags(Enesim_Renderer *r EINA_UNUSED, const Enesim_Renderer_State2 *state EINA_UNUSED,
		Enesim_Renderer_Flag *flags)
{
	*flags = ENESIM_RENDERER_FLAG_TRANSLATE |
			ENESIM_RENDERER_FLAG_AFFINE |
			ENESIM_RENDERER_FLAG_ARGB8888;
}

static void _path_hints(Enesim_Renderer *r EINA_UNUSED, const Enesim_Renderer_State2 *state EINA_UNUSED,
		Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_HINT_COLORIZE;
}

static Eina_Bool _path_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;

	thiz = _path_get(r);
	return thiz->changed;
}

static void _path_feature_get(Enesim_Renderer *r EINA_UNUSED, Enesim_Shape_Feature *features)
{
	*features = ENESIM_SHAPE_FLAG_FILL_RENDERER | ENESIM_SHAPE_FLAG_STROKE_RENDERER;
}

static void _path_bounds(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Renderer_State2 *cs;
	Enesim_Renderer_Shape_State2 *css;
	double xmin;
	double ymin;
	double xmax;
	double ymax;

	thiz = _path_get(r);
	cs = &thiz->state.rstate;
	css = &thiz->state.sstate;

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

static void _path_destination_bounds(Enesim_Renderer *r,
		Eina_Rectangle *bounds)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Rectangle obounds;

	thiz = _path_get(r);

	_path_bounds(r, &obounds);
	if (obounds.w == 0 && obounds.h == 0)
	{
		bounds->x = 0;
		bounds->y = 0;
		bounds->w = 0;
		bounds->h = 0;

		return;
	}

	bounds->x = floor(obounds.x);
	bounds->y = floor(obounds.y);
	bounds->w = ceil(obounds.x - bounds->x + obounds.w) + 1;
	bounds->h = ceil(obounds.y - bounds->y + obounds.h) + 1;
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
		Enesim_Error **error EINA_UNUSED)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Renderer_Path_OpenGL *gl;
	Enesim_Shape_Draw_Mode dm;
	double sw;
	const Enesim_Renderer_State2 *cs;
	const Enesim_Renderer_Shape_State2 *css;

	thiz = _path_get(r);
	cs = &thiz->state.rstate;
	css = &thiz->state.sstate;
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

static Enesim_Renderer_Shape_Descriptor _path_descriptor = {
	/* .version =			*/ ENESIM_RENDERER_API,
	/* .name =			*/ _path_name,
	/* .free =			*/ _path_free,
	/* .bounds =			*/ _path_bounds,
	/* .destination_bounds =	*/ _path_destination_bounds,
	/* .flags =			*/ _path_flags,
	/* .hints_get =			*/ _path_hints,
	/* .is_inside =			*/ NULL,
	/* .damage =			*/ NULL,
	/* .has_changed =		*/ _path_has_changed,
	/* .feature_get =		*/ _path_feature_get,
	/* .sw_setup =			*/ _path_sw_setup,
	/* .sw_cleanup =		*/ _path_sw_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
#if BUILD_OPENGL
	/* .opengl_initialize =		*/ _path_opengl_initialize,
	/* .opengl_setup =		*/ _path_opengl_setup,
	/* .opengl_cleanup =		*/ _path_opengl_cleanup,
#else
	/* .opengl_initialize =		*/ NULL,
	/* .opengl_setup =		*/ NULL,
	/* .opengl_cleanup =		*/ NULL
#endif
};
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Renderer * enesim_renderer_path_new(void)
{
	Enesim_Renderer *r;
	Enesim_Renderer_Path *thiz;

	thiz = calloc(1, sizeof(Enesim_Renderer_Path));
	if (!thiz) return NULL;
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_PATH_MAGIC);

	r = enesim_rasterizer_bifigure_new();
	if (!r) goto err_figure;
	thiz->bifigure = r;

	/* create the different path implementations */
	thiz->stroke_path = enesim_path_stroke_dashless_new();
	thiz->strokeless_path = enesim_path_strokeless_new();
	thiz->dashed_path = enesim_path_dashed_new();

	r = enesim_renderer_shape_new(&_path_descriptor, thiz);
	/* FIXME for now */
	enesim_renderer_shape_stroke_join_set(r, ENESIM_JOIN_ROUND);
	return r;

err_figure:
	free(thiz);
	return NULL;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_command_clear(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Renderer_Path_Command *c;

	thiz = _path_get(r);
	EINA_LIST_FREE(thiz->commands, c)
	{
		free(c);
	}
	thiz->commands = eina_list_free(thiz->commands);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_command_add(Enesim_Renderer *r, Enesim_Renderer_Path_Command *cmd)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Renderer_Path_Command *new_command;

	thiz = _path_get(r);

	/* do not allow a move to command after another move to, just simplfiy them */
	if (cmd->type == ENESIM_COMMAND_MOVE_TO)
	{
		Enesim_Renderer_Path_Command *last_command;
		Eina_List *last;

		last = eina_list_last(thiz->commands);
		last_command = eina_list_data_get(last);
		if (last_command && last_command->type == ENESIM_COMMAND_MOVE_TO)
		{
			last_command->definition.move_to.x = cmd->definition.move_to.x;
			last_command->definition.move_to.y = cmd->definition.move_to.y;
			return;
		}
	}

	new_command = malloc(sizeof(Enesim_Renderer_Path_Command));
	*new_command = *cmd;
	thiz->commands = eina_list_append(thiz->commands, new_command);
	thiz->changed = EINA_TRUE;
	thiz->generated = EINA_FALSE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_command_set(Enesim_Renderer *r,
		Eina_List *list)
{
	Enesim_Renderer_Path_Command *cmd;
	Eina_List *l;

	enesim_renderer_path_command_clear(r);
	EINA_LIST_FOREACH(list, l, cmd)
	{
		enesim_renderer_path_command_add(r, cmd);
	}
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_move_to(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Path_Command cmd;

	cmd.type = ENESIM_COMMAND_MOVE_TO;
	cmd.definition.move_to.x = x;
	cmd.definition.move_to.y = y;
	enesim_renderer_path_command_add(r, &cmd);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_line_to(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Path_Command cmd;

	cmd.type = ENESIM_COMMAND_LINE_TO;
	cmd.definition.line_to.x = x;
	cmd.definition.line_to.y = y;
	enesim_renderer_path_command_add(r, &cmd);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_squadratic_to(Enesim_Renderer *r, double x,
		double y)
{
	Enesim_Renderer_Path_Command cmd;

	cmd.type = ENESIM_COMMAND_SQUADRATIC_TO;
	cmd.definition.squadratic_to.x = x;
	cmd.definition.squadratic_to.y = y;
	enesim_renderer_path_command_add(r, &cmd);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_quadratic_to(Enesim_Renderer *r, double ctrl_x,
		double ctrl_y, double x, double y)
{
	Enesim_Renderer_Path_Command cmd;

	cmd.type = ENESIM_COMMAND_QUADRATIC_TO;
	cmd.definition.quadratic_to.x = x;
	cmd.definition.quadratic_to.y = y;
	cmd.definition.quadratic_to.ctrl_x = ctrl_x;
	cmd.definition.quadratic_to.ctrl_y = ctrl_y;
	enesim_renderer_path_command_add(r, &cmd);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_cubic_to(Enesim_Renderer *r, double ctrl_x0,
		double ctrl_y0, double ctrl_x, double ctrl_y, double x, double y)
{
	Enesim_Renderer_Path_Command cmd;

	cmd.type = ENESIM_COMMAND_CUBIC_TO;
	cmd.definition.cubic_to.x = x;
	cmd.definition.cubic_to.y = y;
	cmd.definition.cubic_to.ctrl_x0 = ctrl_x0;
	cmd.definition.cubic_to.ctrl_y0 = ctrl_y0;
	cmd.definition.cubic_to.ctrl_x1 = ctrl_x;
	cmd.definition.cubic_to.ctrl_y1 = ctrl_y;
	enesim_renderer_path_command_add(r, &cmd);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_scubic_to(Enesim_Renderer *r, double ctrl_x,
		double ctrl_y, double x, double y)
{
	Enesim_Renderer_Path_Command cmd;

	cmd.type = ENESIM_COMMAND_SCUBIC_TO;
	cmd.definition.scubic_to.x = x;
	cmd.definition.scubic_to.y = y;
	cmd.definition.scubic_to.ctrl_x = ctrl_x;
	cmd.definition.scubic_to.ctrl_y = ctrl_y;
	enesim_renderer_path_command_add(r, &cmd);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_arc_to(Enesim_Renderer *r, double rx, double ry, double angle,
                   unsigned char large, unsigned char sweep, double x, double y)
{
	Enesim_Renderer_Path_Command cmd;

	cmd.type = ENESIM_COMMAND_ARC_TO;
	cmd.definition.arc_to.x = x;
	cmd.definition.arc_to.y = y;
	cmd.definition.arc_to.rx = rx;
	cmd.definition.arc_to.ry = ry;
	cmd.definition.arc_to.angle = angle;
	cmd.definition.arc_to.large = large;
	cmd.definition.arc_to.sweep = sweep;
	enesim_renderer_path_command_add(r, &cmd);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_close(Enesim_Renderer *r, Eina_Bool close)
{
	Enesim_Renderer_Path_Command cmd;

	cmd.type = ENESIM_COMMAND_CLOSE;
	cmd.definition.close.close = close;
	enesim_renderer_path_command_add(r, &cmd);
}
