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
#include "enesim_buffer_private.h"
#include "enesim_surface_private.h"
#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_path_abstract_private.h"
#include "enesim_path_normalizer_private.h"
#include "enesim_vector_private.h"

#if BUILD_OPENGL
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
#endif

#define ENESIM_LOG_DEFAULT enesim_log_renderer

#if BUILD_OPENGL
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define GLERR {\
        GLenum err; \
        err = glGetError(); \
        printf("Error %x\n", err); \
        }

#define ENESIM_RENDERER_PATH_TESSELATOR(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Path_Tesselator,					\
		enesim_renderer_path_tesselator_descriptor_get())

typedef struct _Enesim_Renderer_Path_Tesselator_Class
{
	Enesim_Renderer_Path_Abstract_Class parent;
} Enesim_Renderer_Path_Tesselator_Class;

typedef struct _Enesim_Renderer_Path_Tesselator_Polygon
{
	GLenum type;
	Enesim_Polygon *polygon;
} Enesim_Renderer_Path_Tesselator_Polygon;

typedef struct _Enesim_Renderer_Path_Tesselator_Figure
{
	Eina_List *polygons;
	Enesim_Surface *tmp;
	Enesim_Surface *renderer_s;
	Eina_Bool needs_tesselate : 1;
} Enesim_Renderer_Path_Tesselator_Figure;

typedef struct _Enesim_Renderer_Path_Tesselator
{
	Enesim_Renderer_Path_Abstract parent;
	Enesim_Renderer_Path_Tesselator_Figure stroke;
	Enesim_Renderer_Path_Tesselator_Figure fill;
} Enesim_Renderer_Path_Tesselator;

static void _path_opengl_figure_clear(
		Enesim_Renderer_Path_Tesselator_Figure *f)
{
	if (f->polygons)
	{
		Enesim_Renderer_Path_Tesselator_Polygon *p;

		EINA_LIST_FREE(f->polygons, p)
		{
			enesim_polygon_delete(p->polygon);
			free(p);
		}
		f->polygons = NULL;
	}
}

static void _enesim_renderer_path_tesselator_generate_figures(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Tesselator *thiz;

	thiz = ENESIM_RENDERER_PATH_TESSELATOR(r);

	enesim_renderer_path_abstract_generate(r);
	thiz->fill.needs_tesselate = EINA_TRUE;
	thiz->stroke.needs_tesselate = EINA_TRUE;
}
/*----------------------------------------------------------------------------*
 *                                Shaders                                     *
 *----------------------------------------------------------------------------*/
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

/*----------------------------------------------------------------------------*
 *                            Tesselator callbacks                            *
 *----------------------------------------------------------------------------*/
static void _path_opengl_vertex_cb(GLvoid *vertex, void *data)
{
	Enesim_Renderer_Path_Tesselator_Figure *f = data;
	Enesim_Renderer_Path_Tesselator_Polygon *p;
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
	Enesim_Renderer_Path_Tesselator_Polygon *p;
	Enesim_Renderer_Path_Tesselator_Figure *f = data;

	/* add another polygon */
	p = calloc(1, sizeof(Enesim_Renderer_Path_Tesselator_Polygon));
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
		Enesim_Renderer_Path_Tesselator_Figure *glf,
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
	gluTessCallback(t, GLU_TESS_VERTEX_DATA, (GLvoid (*) ())&_path_opengl_vertex_cb);
	gluTessCallback(t, GLU_TESS_BEGIN_DATA, (GLvoid (*) ())&_path_opengl_begin_cb);
	gluTessCallback(t, GLU_TESS_END_DATA, (GLvoid (*) ())&_path_opengl_end_cb);
	gluTessCallback(t, GLU_TESS_COMBINE_DATA, (GLvoid (*) ())&_path_opengl_combine_cb);
	gluTessCallback(t, GLU_TESS_ERROR_DATA, (GLvoid (*) ())&_path_opengl_error_cb);

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
		Enesim_Renderer_Path_Tesselator_Figure *glf)
{
	Eina_List *l1;
	Enesim_Renderer_Path_Tesselator_Polygon *p;

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

static void _path_opengl_silhoutte_draw(Enesim_Figure *f,
		const Eina_Rectangle *area)
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

#if 0
static void _path_opengl_blit(GLenum fbo, GLenum dst,
		GLenum src,
		const Eina_Rectangle *area)
{
	/* merge the two */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, fbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			GL_TEXTURE_2D, dst, 0);

	glBindTexture(GL_TEXTURE_2D, src);
	enesim_opengl_rop_set(ENESIM_ROP_BLEND);
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
#endif

static void _path_opengl_figure_draw(GLenum fbo,
		GLenum texture,
		Enesim_Renderer_Path_Tesselator_Figure *gf,
		Enesim_Figure *f,
		Enesim_Color color,
		Enesim_Renderer *rel EINA_UNUSED,
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

	enesim_opengl_rop_set(ENESIM_ROP_FILL);
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

static void _path_opengl_fill_or_stroke_draw(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop, const Eina_Rectangle *area,
		int x, int y)
{
	Enesim_Renderer_Path_Tesselator *thiz;
	Enesim_Renderer_Path_Abstract *parent;
	Enesim_Renderer_Path_Tesselator_Figure *gf;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_Renderer *rel;
	Enesim_Renderer_Shape_Draw_Mode dm;
	Enesim_Buffer_OpenGL_Data *sdata;
	Enesim_Figure *f;
	Enesim_Color final_color;
	GLint viewport[4];
	GLenum texture;
	int w, h;

	thiz = ENESIM_RENDERER_PATH_TESSELATOR(r);
	parent = ENESIM_RENDERER_PATH_ABSTRACT(r);

	enesim_surface_size_get(s, &w, &h);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	sdata = enesim_surface_backend_data_get(s);
	dm = enesim_renderer_shape_draw_mode_get(r);
	if (dm & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
	{
		gf = &thiz->stroke;
		f = parent->stroke_figure;
		enesim_renderer_shape_stroke_setup(r, &final_color, &rel);
	}
	else
	{
		gf = &thiz->fill;
		f = parent->fill_figure;
		enesim_renderer_shape_fill_setup(r, &final_color, &rel);
	}

	/* create the texture */
	texture = enesim_opengl_texture_new(area->w, area->h);
	glGetIntegerv(GL_VIEWPORT, viewport);
	/* render there */
	_path_opengl_figure_draw(sdata->fbo, texture, gf, f, final_color, rel, rdata, EINA_TRUE, area);
	/* finally compose such texture with the destination texture */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, sdata->fbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			GL_TEXTURE_2D, sdata->textures[0], 0);
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, -1, 1);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	glBindTexture(GL_TEXTURE_2D, texture);
	enesim_opengl_rop_set(ENESIM_ROP_BLEND);
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
	enesim_opengl_rop_set(ENESIM_ROP_FILL);

	if (rel)
		enesim_renderer_unref(rel);
}

/* for fill and stroke we need to draw the stroke first on a
 * temporary fbo, then the fill into a temporary fbo too
 * finally draw again into the destination using the temporary
 * fbos as a source. If the pixel is different than transparent
 * then multiply that color with the current color
 */
static void _path_opengl_fill_and_stroke_draw(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop, const Eina_Rectangle *area,
		int x, int y)
{
	Enesim_Renderer_Path_Tesselator *thiz;
	Enesim_Renderer_Path_Abstract *parent;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_Buffer_OpenGL_Data *sdata;
	Enesim_OpenGL_Compiled_Program *cp;
	Enesim_Renderer *rel;
	Enesim_Color final_color;
	GLenum textures[2];
	GLint viewport[4];
	int w, h;

	thiz = ENESIM_RENDERER_PATH_TESSELATOR(r);
	parent = ENESIM_RENDERER_PATH_ABSTRACT(r);

	enesim_surface_size_get(s, &w, &h);
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	sdata = enesim_surface_backend_data_get(s);

	/* create the fill texture */
	textures[0] = enesim_opengl_texture_new(area->w, area->h);
	/* create the stroke texture */
	textures[1] = enesim_opengl_texture_new(area->w, area->h);

	glGetIntegerv(GL_VIEWPORT, viewport);
	glViewport(0, 0, area->w, area->h);

	/* draw the fill into the newly created buffer */
	enesim_renderer_shape_fill_setup(r, &final_color, &rel);
	_path_opengl_figure_draw(sdata->fbo, textures[0], &thiz->fill,
			parent->fill_figure, final_color, rel, rdata, EINA_FALSE, area);
	if (rel) enesim_renderer_unref(rel);

	/* draw the stroke into the newly created buffer */
	enesim_renderer_shape_stroke_setup(r, &final_color, &rel);
	/* FIXME this one is slow but only after the other */
	_path_opengl_figure_draw(sdata->fbo, textures[1], &thiz->stroke,
			parent->stroke_figure, final_color, rel, rdata, EINA_TRUE, area);
	if (rel) enesim_renderer_unref(rel);

	/* now use the real destination surface to draw the merge fragment */
	glBindFramebufferEXT(GL_FRAMEBUFFER_EXT, sdata->fbo);
	glFramebufferTexture2DEXT(GL_FRAMEBUFFER_EXT, GL_COLOR_ATTACHMENT0_EXT,
			GL_TEXTURE_2D, sdata->textures[0], 0);
	cp = &rdata->program->compiled[1];
	_path_opengl_merge_shader_setup(cp->id, textures[1], textures[0]);
	glViewport(0, 0, w, h);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glOrtho(0, w, h, 0, -1, 1);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	enesim_opengl_rop_set(ENESIM_ROP_BLEND);
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
	enesim_opengl_rop_set(ENESIM_ROP_FILL);
}

/*----------------------------------------------------------------------------*
 *                             Shape interface                                *
 *----------------------------------------------------------------------------*/
static void _enesim_renderer_path_tesselator_shape_features_get(
		Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Shape_Feature *features)
{
	*features = ENESIM_RENDERER_SHAPE_FEATURE_FILL_RENDERER |
			ENESIM_RENDERER_SHAPE_FEATURE_STROKE_RENDERER;
}

static Eina_Bool _enesim_renderer_path_tesselator_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Tesselator *thiz;

#if 0
	thiz = ENESIM_RENDERER_PATH_TESSELATOR(r);
	if (thiz->new_path || thiz->path_changed || (thiz->path && thiz->path->changed) || (thiz->dashes_changed))
		return EINA_TRUE;
	else
#endif
		return EINA_FALSE;
}

static Eina_Bool _enesim_renderer_path_tesselator_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Path_Tesselator *thiz;
	Enesim_Renderer_Shape_Draw_Mode dm;
	const Enesim_Renderer_Shape_State *css;

	thiz = ENESIM_RENDERER_PATH_TESSELATOR(r);

	css = enesim_renderer_shape_state_get(r);

	/* generate the figures */
	if (enesim_renderer_path_abstract_needs_generate(r))
	{
		_enesim_renderer_path_tesselator_generate_figures(r);
	}

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

static void _enesim_renderer_path_tesselator_opengl_cleanup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED)
{
	enesim_renderer_path_abstract_cleanup(r);
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _enesim_renderer_path_tesselator_name(
		Enesim_Renderer *r EINA_UNUSED)
{
	return "enesim_path_tesselator";
}

static void _enesim_renderer_path_tesselator_features_get(
		Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_PROJECTIVE |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static void _enesim_renderer_path_tesselator_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Path_Abstract *parent;
	const Enesim_Renderer_State *cs;
	const Enesim_Renderer_Shape_State *css;
	double xmin;
	double ymin;
	double xmax;
	double ymax;
	double swx, swy;

	parent = ENESIM_RENDERER_PATH_ABSTRACT(r);
	cs = enesim_renderer_state_get(r);
	css = enesim_renderer_shape_state_get(r);

	if (enesim_renderer_path_abstract_needs_generate(r))
	{
		_enesim_renderer_path_tesselator_generate_figures(r);
	}

	if (!parent->fill_figure)
	{
		bounds->x = 0;
		bounds->y = 0;
		bounds->w = 0;
		bounds->h = 0;
		return;
	}

	enesim_renderer_shape_stroke_weight_setup(r, &swx, &swy);
	if (css->current.draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
	{
		if (swx > 1.0 || swy > 1.0)
		{
			if (!enesim_figure_bounds(parent->stroke_figure, &xmin, &ymin, &xmax, &ymax))
				goto failed;
		}
		else
		{
			if (!enesim_figure_bounds(parent->fill_figure, &xmin, &ymin, &xmax, &ymax))
				goto failed;
			/* add the stroke offset, even if the basic figure has its own bounds
			 * we need to define the correct one here
			 */
			xmin -= 0.5;
			ymin -= 0.5;
			xmax += 0.5;
			ymax += 0.5;
		}
	}
	else
	{
		if (!enesim_figure_bounds(parent->fill_figure, &xmin, &ymin, &xmax, &ymax))
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

static Eina_Bool _enesim_renderer_path_tesselator_opengl_initialize(
		Enesim_Renderer *r EINA_UNUSED,
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***programs)
{
	*programs = _path_programs;
	*num_programs = 3;
	return EINA_TRUE;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_PATH_ABSTRACT_DESCRIPTOR,
		Enesim_Renderer_Path_Tesselator, Enesim_Renderer_Path_Tesselator_Class,
		enesim_renderer_path_tesselator);

static void _enesim_renderer_path_tesselator_class_init(void *k)
{
	Enesim_Renderer_Path_Abstract_Class *klass;
	Enesim_Renderer_Shape_Class *s_klass;
	Enesim_Renderer_Class *r_klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _enesim_renderer_path_tesselator_name;
	r_klass->bounds_get = _enesim_renderer_path_tesselator_bounds_get;
	r_klass->features_get = _enesim_renderer_path_tesselator_features_get;
	r_klass->opengl_initialize = _enesim_renderer_path_tesselator_opengl_initialize;

	s_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	s_klass->features_get = _enesim_renderer_path_tesselator_shape_features_get;
	s_klass->has_changed = _enesim_renderer_path_tesselator_has_changed;
	s_klass->opengl_setup = _enesim_renderer_path_tesselator_opengl_setup;
	s_klass->opengl_cleanup = _enesim_renderer_path_tesselator_opengl_cleanup;

	klass = ENESIM_RENDERER_PATH_ABSTRACT_CLASS(k);
}

static void _enesim_renderer_path_tesselator_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_renderer_path_tesselator_instance_deinit(void *o)
{
	Enesim_Renderer_Path_Tesselator *thiz;

	thiz = ENESIM_RENDERER_PATH_TESSELATOR(o);
}
#endif
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_path_tesselator_new(void)
{
#if BUILD_OPENGL
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_path_tesselator);
	return r;
#else
	return NULL;
#endif
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
