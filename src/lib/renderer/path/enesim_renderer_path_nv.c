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
#include "enesim_matrix.h"
#include "enesim_path.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_path.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_color_private.h"
#include "enesim_path_private.h"
#include "enesim_list_private.h"
#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_path_abstract_private.h"
#include "enesim_path_normalizer_private.h"

#if BUILD_OPENGL_NV_PATH
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
#endif

#include "enesim_surface_private.h"

#define ENESIM_LOG_DEFAULT enesim_log_renderer

#if BUILD_OPENGL_NV_PATH
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_RENDERER_PATH_NV(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Path_Nv,					\
		enesim_renderer_path_nv_descriptor_get())

typedef struct _Enesim_Renderer_Path_Nv
{
	Enesim_Renderer_Path_Abstract parent;
	Enesim_Path *path;
	Enesim_Surface *s;
	Enesim_Surface *fsrc;
	Enesim_Surface *ssrc;
	GLuint path_id;
	GLuint sb;
	int last_w, last_h;
	int last_path_change;
	Eina_Bool dashes_changed;
	Eina_Bool generated;
} Enesim_Renderer_Path_Nv;

typedef struct _Enesim_Renderer_Path_Nv_Class
{
	Enesim_Renderer_Path_Abstract_Class parent;
} Enesim_Renderer_Path_Nv_Class;

/* the only shader */
static Enesim_Renderer_OpenGL_Shader *_enesim_renderer_path_nv_shaders[] = {
	&enesim_renderer_opengl_shader_texture,
	NULL,
};

/* the only program */
static Enesim_Renderer_OpenGL_Program _enesim_renderer_path_nv_program = {
	/* .name 		= */ "enesim_renderer_path_nv",
	/* .shaders 		= */ _enesim_renderer_path_nv_shaders,
	/* .num_shaders		= */ 1,
};

static Enesim_Renderer_OpenGL_Program *_enesim_renderer_path_nv_programs[] = {
	&_enesim_renderer_path_nv_program,
	NULL,
};

static void _enesim_renderer_path_nv_stencil_create(Enesim_Renderer_Path_Nv *thiz,
		Enesim_Surface *s)
{
	int w, h;

	/* create our stencil buffer */
	enesim_surface_size_get(s, &w, &h);
	if (thiz->sb)
	{
		if (w != thiz->last_w || h != thiz->last_h)
		{
			glDeleteRenderbuffers(1, &thiz->sb);
			thiz->sb = 0;
		}
	}

	if (!thiz->sb)
	{
		GLuint sb;

		glGenRenderbuffers(1, &sb);
		glBindRenderbuffer(GL_RENDERBUFFER, sb);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
		thiz->last_w = w;
		thiz->last_h = h;
		thiz->sb = sb;
		glBindRenderbuffer(GL_RENDERBUFFER, 0);
	}
}

static void _enesim_renderer_path_nv_fill_shader_setup(
		Enesim_Renderer_OpenGL_Data *rdata, Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Color color, int x, int y)
{
	if (r)
	{
		Enesim_OpenGL_Compiled_Program *cp;

		cp = &rdata->program->compiled[0];
		enesim_renderer_opengl_shader_texture_setup(cp->id,
				0, s, color, x, y);
	}
}

static void _enesim_renderer_path_nv_fill_good_cover(GLenum path_id,
	GLfloat jitters[][2], int num_jitters)
{
	int i;

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 0x80, 0x7f);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glUseProgramObjectARB(0);

	enesim_opengl_rop_set(ENESIM_ROP_FILL);
	/* just update alpha */
	glColorMask(0, 0, 0, 1);
	glColor4f(0, 0, 0, 1.0);
	/* first draw the whole figure */
	glStencilFillPathNV(path_id, GL_COUNT_UP_NV, 0x7F);
	glCoverFillPathNV(path_id, GL_BOUNDING_BOX_NV);

	/* accumulate jittered path coverage into fbo alpha channel */
	/* sum up alpha */
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glColor4f(0, 0, 0, 1.0 / num_jitters);
	for (i = 1; i < num_jitters; i++)
	{
		glMatrixPushEXT(GL_PROJECTION);
		{
			glMatrixTranslatefEXT(GL_PROJECTION, jitters[i][0], jitters[i][1], 0);
			glStencilFillPathNV(path_id, GL_COUNT_UP_NV, 0x7F);
			glCoverFillPathNV(path_id, GL_BOUNDING_BOX_NV);
		}
		glMatrixPopEXT(GL_PROJECTION);
	}
}

static void _enesim_renderer_path_nv_fill_good_draw(GLenum path_id,
		Enesim_Color fcolor, GLfloat jitters[][2], int num_jitters)
{
	int i;

	/* now draw */
	glColorMask(1, 1, 1, 1);
	glColor4f(1, 1, 1, 1);

	// modulate RGB with destination alpha and then zero destination alpha
	glBlendFuncSeparate(GL_DST_ALPHA, GL_ZERO, GL_DST_ALPHA, GL_ZERO);
	glColor4f(enesim_color_red_get(fcolor) / 255.0,
		enesim_color_green_get(fcolor) / 255.0,
		enesim_color_blue_get(fcolor) / 255.0,
		enesim_color_alpha_get(fcolor) / 255.0);
	glStencilFunc(GL_EQUAL, 0x80, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
	for (i = 0; i < num_jitters; i++)
	{
		glMatrixPushEXT(GL_PROJECTION);
		{
			glMatrixTranslatefEXT(GL_PROJECTION, jitters[i][0], jitters[i][1], 0);
			glCoverFillPathNV(path_id, GL_BOUNDING_BOX_NV);
		}
		glMatrixPopEXT(GL_PROJECTION);
	}
	glUseProgramObjectARB(0);
	glBindTexture(GL_TEXTURE_2D, 0);

}

static void _enesim_renderer_path_nv_stroke_good_cover(GLenum path_id,
		GLfloat jitters[][2], int num_jitters)
{
	int i;

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 0x80, 0x7f);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);
	glUseProgramObjectARB(0);

	enesim_opengl_rop_set(ENESIM_ROP_FILL);
	/* just update alpha */
	glColorMask(0, 0, 0, 1);
	glColor4f(0, 0, 0, 1.0);
	/* first draw the whole figure */
	glStencilStrokePathNV(path_id, GL_COUNT_UP_NV, 0x7F);
	glCoverStrokePathNV(path_id, GL_BOUNDING_BOX_NV);

	/* accumulate jittered path coverage into fbo alpha channel */
	/* sum up alpha */
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	glColor4f(0, 0, 0, 1.0 / num_jitters);
	for (i = 1; i < num_jitters; i++)
	{
		glMatrixPushEXT(GL_PROJECTION);
		{
			glMatrixTranslatefEXT(GL_PROJECTION, jitters[i][0], jitters[i][1], 0);
			glStencilStrokePathNV(path_id, GL_COUNT_UP_NV, 0x7F);
			glCoverStrokePathNV(path_id, GL_BOUNDING_BOX_NV);
		}
		glMatrixPopEXT(GL_PROJECTION);
	}
}

static void _enesim_renderer_path_nv_stroke_good_draw(GLenum path_id, Enesim_Color fcolor,
		GLfloat jitters[][2], int num_jitters)
{
	int i;

	/* now draw */
	glColorMask(1, 1, 1, 1);
	glColor4f(1, 1, 1, 1);

	glBlendFuncSeparate(GL_DST_ALPHA, GL_ZERO, GL_DST_ALPHA, GL_ZERO);
	glColor4f(enesim_color_red_get(fcolor) / 255.0,
		enesim_color_green_get(fcolor) / 255.0,
		enesim_color_blue_get(fcolor) / 255.0,
		enesim_color_alpha_get(fcolor) / 255.0);
	glStencilFunc(GL_EQUAL, 0x80, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);
	for (i = 0; i < num_jitters; i++)
	{
		glMatrixPushEXT(GL_PROJECTION);
		{
			glMatrixTranslatefEXT(GL_PROJECTION, jitters[i][0], jitters[i][1], 0);
			glCoverStrokePathNV(path_id, GL_BOUNDING_BOX_NV);
		}
		glMatrixPopEXT(GL_PROJECTION);
	}
	glUseProgramObjectARB(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

static void _enesim_renderer_path_nv_fill_fast(GLenum path_id, Enesim_Color fcolor)
{
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

	glStencilFillPathNV(path_id, GL_COUNT_UP_NV, 0xFF); 
	glColor4f(enesim_color_red_get(fcolor) / 255.0,
		enesim_color_green_get(fcolor) / 255.0,
		enesim_color_blue_get(fcolor) / 255.0,
		enesim_color_alpha_get(fcolor) / 255.0);
	glCoverFillPathNV(path_id, GL_BOUNDING_BOX_NV);
	glUseProgramObjectARB(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

static void _enesim_renderer_path_nv_stroke_fast(GLenum path_id, Enesim_Color scolor)
{
	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
	glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

	glStencilStrokePathNV(path_id, 0x1, ~0);
	glColor4f(enesim_color_red_get(scolor) / 255.0,
		enesim_color_green_get(scolor) / 255.0,
		enesim_color_blue_get(scolor) / 255.0,
		enesim_color_alpha_get(scolor) / 255.0);
	glCoverStrokePathNV(path_id, GL_CONVEX_HULL_NV);
	glUseProgramObjectARB(0);
	glBindTexture(GL_TEXTURE_2D, 0);
}

static inline void _enesim_renderer_path_nv_create_surface(Enesim_Surface **s,
		Enesim_Pool *p, int w, int h)
{
	int sw, sh;
	if (*s)
	{
		enesim_surface_size_get(*s, &sw, &sh);
		if (sw != w || sh != h)
		{
			enesim_surface_unref(*s);
			*s = NULL;
		}
	}

	if (!*s)
	{
		*s = enesim_surface_new_pool_from(ENESIM_FORMAT_ARGB8888,
				w, h, p);
	}
}

static inline void _enesim_renderer_path_nv_sub_draw(Enesim_Renderer *r,
		Enesim_Surface **s, Enesim_Pool *p,
		const Eina_Rectangle *area, int x, int y)
{
	Eina_Rectangle drawing;

	if (!r)
	{
		enesim_pool_unref(p);
		return;
	}

	_enesim_renderer_path_nv_create_surface(s, p, area->w, area->h);
	eina_rectangle_coords_from(&drawing, 0, 0, area->w, area->h);
	enesim_renderer_opengl_draw(r, *s, ENESIM_ROP_FILL, &drawing, -(area->x - x), -(area->y - y));
}

static void _enesim_renderer_path_nv_draw(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop, const Eina_Rectangle *area,
		int x, int y)
{
	Enesim_Renderer_Path_Nv *thiz;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_Renderer_Shape_Draw_Mode draw_mode;
	Enesim_Renderer *fren = NULL;
	Enesim_Renderer *sren = NULL;
	Enesim_Surface *rs;
	Enesim_Color scolor;
	Enesim_Color fcolor;
	Enesim_Pool *pool;
	Enesim_Quality quality;
	Enesim_Matrix m, tx;
	Eina_Rectangle rarea;
	GLenum status;
	GLfloat fm[16];
	GLfloat jitters[5][2] = { {0, 0} , {1, 0}, {-1, 0}, {0, 1}, {0, -1} };
	int passes = 5;
	int w, h;

	thiz = ENESIM_RENDERER_PATH_NV(r);

	draw_mode = enesim_renderer_shape_draw_mode_get(r);
	quality = enesim_renderer_quality_get(r);
	pool = enesim_surface_pool_get(s);

	/* for good rendering we need to use a temporary surface for
	 * correct blending
	 */
	if (quality == ENESIM_QUALITY_FAST)
	{
		rs = s;
		rarea = *area;
		enesim_matrix_translate(&tx, x, y);
	}
	else
	{
		Enesim_Buffer_OpenGL_Data *sdata;

		_enesim_renderer_path_nv_create_surface(&thiz->s,
				enesim_pool_ref(pool), area->w, area->h);
		sdata = enesim_surface_backend_data_get(thiz->s);
		glGenFramebuffersEXT(1, &sdata->fbo);
		rs = thiz->s;
		enesim_matrix_translate(&tx, -(area->x - x), -(area->y - y));
		eina_rectangle_coords_from(&rarea, 0, 0, area->w, area->h);
	}

	/* draw the temporary fill/stroke renderers */
	if (draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL)
	{
		enesim_renderer_shape_fill_setup(r, &fcolor, &fren);
		 _enesim_renderer_path_nv_sub_draw(fren, &thiz->fsrc,
				enesim_pool_ref(pool), area, x, y);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
	if (draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
	{
		enesim_renderer_shape_stroke_setup(r, &scolor, &sren);
		 _enesim_renderer_path_nv_sub_draw(sren, &thiz->ssrc,
				enesim_pool_ref(pool), area, x, y);
		glBindTexture(GL_TEXTURE_2D, 0);
	}

	/* finally draw */
	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	/* create our stencil buffer here given that the surface used for the setup
	 * might be different that the one for the drawing (a temporary one for example)
	 */
	_enesim_renderer_path_nv_stencil_create(thiz, rs);
	enesim_opengl_target_surface_set(rs);
	/* attach the stencil buffer on the framebuffer */
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, thiz->sb);
	status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		ERR("The framebuffer setup failed 0x%08x", status);
		return;
	}
	enesim_opengl_rop_set(rop);
	/* set the clipping area */
	enesim_surface_size_get(rs, &w, &h);
	enesim_opengl_clip_set(&rarea, w, h);

	glClearStencil(0);
	glStencilMask(~0);
	glClear(GL_STENCIL_BUFFER_BIT);

	/* add our own transformation matrix */
	enesim_renderer_transformation_get(r, &m);
	enesim_matrix_compose(&tx, &m, &m);
	enesim_opengl_matrix_convert(&m, fm);

	glMatrixMode(GL_PROJECTION);
	glMultMatrixf(fm);

	glMatrixMode(GL_TEXTURE);
	glLoadIdentity();

	/* fill */
	if (draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL)
	{
		if (quality == ENESIM_QUALITY_FAST)
		{
			_enesim_renderer_path_nv_fill_shader_setup(rdata,
					fren, thiz->fsrc, fcolor, area->x - x,
					area->y - y);
			_enesim_renderer_path_nv_fill_fast(thiz->path_id, fcolor);
		}
		else
		{
			_enesim_renderer_path_nv_fill_good_cover(thiz->path_id,
					jitters, passes);
			_enesim_renderer_path_nv_fill_shader_setup(rdata,
					fren, thiz->fsrc, fcolor, area->x - x,
					area->y - y);
			_enesim_renderer_path_nv_fill_good_draw(thiz->path_id,
					fcolor, jitters, passes);
		}
	}

	/* stroke */
	if (draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
	{
		if (quality == ENESIM_QUALITY_FAST)
		{
			_enesim_renderer_path_nv_fill_shader_setup(rdata,
					sren, thiz->ssrc, scolor, area->x - x,
					area->y - y);
			_enesim_renderer_path_nv_stroke_fast(thiz->path_id, scolor);
		}
		else
		{
			_enesim_renderer_path_nv_stroke_good_cover(thiz->path_id,
					jitters, passes);
			_enesim_renderer_path_nv_fill_shader_setup(rdata,
					sren, thiz->ssrc, scolor, area->x - x,
					area->y - y);
			_enesim_renderer_path_nv_stroke_good_draw(thiz->path_id, scolor,
					jitters, passes);
		}
	}

	/* we no longer need to clip */
	enesim_opengl_clip_unset();

	/* compose in case we used another surface */
	if (quality != ENESIM_QUALITY_FAST)
	{
		Enesim_OpenGL_Compiled_Program *cp;

		cp = &rdata->program->compiled[0];
		enesim_renderer_opengl_shader_texture_setup(cp->id,
				0, thiz->s, ENESIM_COLOR_FULL, area->x, area->y);
		enesim_opengl_target_surface_set(s);
		enesim_opengl_rop_set(rop);
		enesim_opengl_draw_area(area);
	}

	/* unref the renderers */
	enesim_renderer_unref(fren);
	enesim_renderer_unref(sren);
	/* unref the pool */
	enesim_pool_unref(pool);

	/* restore the state */
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT);
	glColor4f(1, 1, 1, 1);
	glDisable(GL_STENCIL_TEST);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
	glUseProgramObjectARB(0);
	enesim_opengl_target_surface_set(NULL);
}

static void _enesim_renderer_path_nv_setup_stroke(
		Enesim_Renderer_Path_Nv *thiz)
{
	Enesim_Renderer *r = ENESIM_RENDERER(thiz);
	GLenum join_style = GL_BEVEL_NV;
	GLenum cap_style = GL_ROUND_NV;
	GLenum err;

	switch (enesim_renderer_shape_stroke_cap_get(r))
	{
		case ENESIM_RENDERER_SHAPE_STROKE_CAP_BUTT:
		cap_style = GL_FLAT;
		break;

		case ENESIM_RENDERER_SHAPE_STROKE_CAP_ROUND:
		cap_style = GL_ROUND_NV;
		break;

		case ENESIM_RENDERER_SHAPE_STROKE_CAP_SQUARE:
		cap_style = GL_SQUARE_NV;
		break;

		default:
		break;
	}
	switch (enesim_renderer_shape_stroke_join_get(r))
	{
		case ENESIM_RENDERER_SHAPE_STROKE_JOIN_BEVEL:
		join_style = GL_BEVEL_NV;
		break;

		case ENESIM_RENDERER_SHAPE_STROKE_JOIN_ROUND:
		join_style = GL_ROUND_NV;
		break;

		case ENESIM_RENDERER_SHAPE_STROKE_JOIN_MITER:
		join_style = GL_MITER_TRUNCATE_NV;
		break;

		default:
		break;
	}
	glPathParameteriNV(thiz->path_id, GL_PATH_INITIAL_END_CAP_NV, cap_style);
	glPathParameteriNV(thiz->path_id, GL_PATH_JOIN_STYLE_NV, join_style);
	glPathParameterfNV(thiz->path_id, GL_PATH_STROKE_WIDTH_NV,
			enesim_renderer_shape_stroke_weight_get(r));
	/* TODO add the dashes */
	err = glGetError();
	if (err)
	{
		ERR("Setting up the stroke failed 0x%08x", err);
	}
}

static void _enesim_renderer_path_nv_setup_fill(
		Enesim_Renderer_Path_Nv *thiz EINA_UNUSED)
{
	/* TODO fill rule */
}

static Eina_Bool _enesim_renderer_path_nv_upload_path(
		Enesim_Renderer_Path_Nv *thiz)
{
	Enesim_Path_Command *pcmd;
	Enesim_Matrix m;
	Eina_List *l;
	GLuint path_id;
	GLenum err;
	GLubyte *cmd, *cmds;
	GLfloat *coord, *coords;
	GLfloat coeffs[6] = { 1, 0, 0, 0, 1, 0 };
	int num_cmds;
	int num_coords = 0;

	/* TODO check if the path has changed or not */
	if (thiz->last_path_change != thiz->path->changed)
		thiz->generated = EINA_FALSE;
	/* TODO we are having some cases where the path is invalid? */
	if (thiz->path_id && !glIsPathNV(thiz->path_id))
		thiz->generated = EINA_FALSE;

	/* FIXME looks like we can not cache the path
	 * the bounds are invalid
	 */
	//if (thiz->generated)
	//	goto propagate;

	if (thiz->path_id)
	{
		glDeletePathsNV(thiz->path_id, 1);
		thiz->path_id = 0;
	}

	path_id = glGenPathsNV(1);
	err = glGetError();
	if (err)
	{
		ERR("glGetPathsNV failed 0x%08x", err);
		return EINA_FALSE;
	}

	/* generate our path coords */ 
	num_cmds = eina_list_count(thiz->path->commands);
	cmd = cmds = malloc(sizeof(GLubyte) * num_cmds);
	/* pick the worst case (arc) to avoid having to realloc every time */
	coord = coords = malloc(sizeof(GLfloat) * 7 * num_cmds);
	EINA_LIST_FOREACH (thiz->path->commands, l, pcmd)
	{
		switch (pcmd->type)
		{
			case ENESIM_PATH_COMMAND_TYPE_MOVE_TO:
			*cmd++ = 'M';
			*coord++ = pcmd->data.move_to.x;
			*coord++ = pcmd->data.move_to.y;
			num_coords += 2;
			break;
			
			case ENESIM_PATH_COMMAND_TYPE_LINE_TO:
			*cmd++ = 'L';
			*coord++ = pcmd->data.line_to.x;
			*coord++ = pcmd->data.line_to.y;
			num_coords += 2;
			break;

			case ENESIM_PATH_COMMAND_TYPE_ARC_TO:
			*cmd++ = 'A';
			num_coords += 7;
			*coord++ = pcmd->data.arc_to.rx;
			*coord++ = pcmd->data.arc_to.ry;
			*coord++ = pcmd->data.arc_to.angle;
			*coord++ = pcmd->data.arc_to.large;
			*coord++ = pcmd->data.arc_to.sweep;
			*coord++ = pcmd->data.arc_to.x;
			*coord++ = pcmd->data.arc_to.y;
			break;

			case ENESIM_PATH_COMMAND_TYPE_CLOSE:
			*cmd++ = 'Z';
			break;

			case ENESIM_PATH_COMMAND_TYPE_CUBIC_TO:
			num_coords += 6;
			*cmd++ = 'C';
			*coord++ = pcmd->data.cubic_to.ctrl_x0;
			*coord++ = pcmd->data.cubic_to.ctrl_y0;
			*coord++ = pcmd->data.cubic_to.ctrl_x1;
			*coord++ = pcmd->data.cubic_to.ctrl_y1;
			*coord++ = pcmd->data.cubic_to.x;
			*coord++ = pcmd->data.cubic_to.y;
			break;

			case ENESIM_PATH_COMMAND_TYPE_SCUBIC_TO:
			num_coords += 4;
			*cmd++ = 'S';
			*coord++ = pcmd->data.scubic_to.ctrl_x;
			*coord++ = pcmd->data.scubic_to.ctrl_y;
			*coord++ = pcmd->data.scubic_to.x;
			*coord++ = pcmd->data.scubic_to.y;
			break;

			case ENESIM_PATH_COMMAND_TYPE_QUADRATIC_TO:
			num_coords += 4;
			*cmd++ = 'Q';
			*coord++ = pcmd->data.quadratic_to.ctrl_x;
			*coord++ = pcmd->data.quadratic_to.ctrl_y;
			*coord++ = pcmd->data.quadratic_to.x;
			*coord++ = pcmd->data.quadratic_to.y;
			break;

			case ENESIM_PATH_COMMAND_TYPE_SQUADRATIC_TO:
			num_coords += 2;
			*cmd++ = 'T';
			*coord++ = pcmd->data.squadratic_to.x;
			*coord++ = pcmd->data.squadratic_to.y;
			break;

			default:
			break;
		}
	}

	glPathCommandsNV(path_id, num_cmds, cmds, num_coords, GL_FLOAT, coords);
	free(coords);
	free(cmds);

	err = glGetError();
	if (err)
	{
		ERR("glPathCommandsNV failed 0x%08x", err);
		return EINA_FALSE;
	}

	/* set the properties */
	thiz->path_id = path_id;
	thiz->generated = EINA_TRUE;
	thiz->last_path_change = thiz->path->changed;

	/* set the coordinate textures */
	/* we need to transform the object linear with the transformation
	 * matrix of the renderer
	 */
	enesim_renderer_transformation_get(ENESIM_RENDERER(thiz), &m);
	coeffs[0] = m.xx; coeffs[1] = m.xy; coeffs[2] = 0;
	coeffs[3] = m.yx; coeffs[4] = m.yy; coeffs[5] = 0;
	glPathTexGenNV(GL_TEXTURE0, GL_OBJECT_LINEAR, 2, coeffs);
	err = glGetError();
	if (err)
	{
		ERR("glPathTexGenNV failed 0x%08x", err);
	}
//propagate:
	_enesim_renderer_path_nv_setup_stroke(thiz);
	_enesim_renderer_path_nv_setup_fill(thiz);

	return EINA_TRUE;
}
/*----------------------------------------------------------------------------*
 *                          Path Abstract interface                           *
 *----------------------------------------------------------------------------*/
static void _enesim_renderer_path_nv_path_set(Enesim_Renderer *r,
		Enesim_Path *path)
{
	Enesim_Renderer_Path_Nv *thiz;

	thiz = ENESIM_RENDERER_PATH_NV(r);
	if (thiz->path != path)
	{
		if (thiz->path) enesim_path_unref(thiz->path);
		thiz->path = path;
		thiz->generated = EINA_FALSE;
	}
	else
	{
		enesim_path_unref(path);
	}
}

static Eina_Bool _enesim_renderer_path_is_available(
		Enesim_Renderer *r EINA_UNUSED)
{
	/* check that we have the valid extension on the system */
	enesim_opengl_init();
	if (!GLEW_NV_path_rendering)
	{
		INF("No NV_path_rendering extension found");
		return EINA_FALSE;
	}
	return EINA_TRUE;
}
/*----------------------------------------------------------------------------*
 *                             Shape interface                                *
 *----------------------------------------------------------------------------*/
static void _enesim_renderer_path_nv_shape_features_get(
		Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Shape_Feature *features)
{
	*features = ENESIM_RENDERER_SHAPE_FEATURE_FILL_RENDERER |
			ENESIM_RENDERER_SHAPE_FEATURE_STROKE_RENDERER;
}

static Eina_Bool _enesim_renderer_path_nv_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l EINA_UNUSED)
{
	Enesim_Renderer_Path_Nv *thiz;

	thiz = ENESIM_RENDERER_PATH_NV(r);

	/* upload the commands */
	if (!_enesim_renderer_path_nv_upload_path(thiz))
		return EINA_FALSE;

	*draw = _enesim_renderer_path_nv_draw;
	return EINA_TRUE;
}

static void _enesim_renderer_path_nv_opengl_cleanup(
		Enesim_Renderer *r EINA_UNUSED, Enesim_Surface *s EINA_UNUSED)
{
}
/*----------------------------------------------------------------------------*
 *                      The Enesim's renderer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _enesim_renderer_path_nv_name(
		Enesim_Renderer *r EINA_UNUSED)
{
	return "enesim_path_nv";
}

static void _enesim_renderer_path_nv_features_get(
		Enesim_Renderer *r EINA_UNUSED,
		Enesim_Renderer_Feature *features)
{
	*features = ENESIM_RENDERER_FEATURE_TRANSLATE |
			ENESIM_RENDERER_FEATURE_AFFINE |
			ENESIM_RENDERER_FEATURE_PROJECTIVE |
			ENESIM_RENDERER_FEATURE_BACKEND_OPENGL |
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static Eina_Bool _enesim_renderer_path_nv_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds, Enesim_Log **log EINA_UNUSED)
{
	Enesim_Renderer_Path_Nv *thiz;
	Enesim_Matrix m;
	Enesim_Matrix_Type type;
	GLenum err;
	float fbounds[4];
	
	thiz = ENESIM_RENDERER_PATH_NV(r);
	if (!_enesim_renderer_path_nv_upload_path(thiz))
		goto failed;

	if (enesim_renderer_shape_draw_mode_get(r) &
			ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
	{
		glGetPathParameterfvNV(thiz->path_id, GL_PATH_STROKE_BOUNDING_BOX_NV, fbounds);
	}
	else
	{
		glGetPathParameterfvNV(thiz->path_id, GL_PATH_OBJECT_BOUNDING_BOX_NV, fbounds);
	}
	err = glGetError();
	if (err)
	{
		ERR("GetPathParameterfvNV failed 0x%08x", err);
		goto failed;
	}
	bounds->x = fbounds[0];
	bounds->y = fbounds[1];
	bounds->w = fbounds[2] - bounds->x;
	bounds->h = fbounds[3] - bounds->y;

	/* now transform */
	enesim_renderer_transformation_get(r, &m);
	type = enesim_renderer_transformation_type_get(r);

	/* transform the bounds */
	if (type != ENESIM_MATRIX_TYPE_IDENTITY)
	{
		Enesim_Quad q;

		enesim_matrix_rectangle_transform(&m, bounds, &q);
		enesim_quad_rectangle_to(&q, bounds);
	}

	/* add one pixel for the antialias */
	bounds->x -= 1;
	bounds->y -= 1;
	bounds->w += 2;
	bounds->h += 2;

	return EINA_TRUE;

failed:
	bounds->x = 0;
	bounds->y = 0;
	bounds->w = 0;
	bounds->h = 0;
	return EINA_FALSE;
}

static Eina_Bool _enesim_renderer_path_nv_opengl_initialize(
		Enesim_Renderer *r EINA_UNUSED,
		int *num_programs,
		Enesim_Renderer_OpenGL_Program ***programs)
{
	*num_programs = 1;
	*programs = _enesim_renderer_path_nv_programs;
	return EINA_TRUE;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_PATH_ABSTRACT_DESCRIPTOR,
		Enesim_Renderer_Path_Nv, Enesim_Renderer_Path_Nv_Class,
		enesim_renderer_path_nv);

static void _enesim_renderer_path_nv_class_init(void *k)
{
	Enesim_Renderer_Path_Abstract_Class *klass;
	Enesim_Renderer_Shape_Class *s_klass;
	Enesim_Renderer_Class *r_klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _enesim_renderer_path_nv_name;
	r_klass->bounds_get = _enesim_renderer_path_nv_bounds_get;
	r_klass->features_get = _enesim_renderer_path_nv_features_get;
	r_klass->opengl_initialize = _enesim_renderer_path_nv_opengl_initialize;

	s_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	s_klass->features_get = _enesim_renderer_path_nv_shape_features_get;
	s_klass->opengl_setup = _enesim_renderer_path_nv_opengl_setup;
	s_klass->opengl_cleanup = _enesim_renderer_path_nv_opengl_cleanup;

	klass = ENESIM_RENDERER_PATH_ABSTRACT_CLASS(k);
	klass->path_set = _enesim_renderer_path_nv_path_set;
	klass->is_available = _enesim_renderer_path_is_available;
}

static void _enesim_renderer_path_nv_instance_init(void *o EINA_UNUSED)
{
	Enesim_Renderer *r = o;
	enesim_renderer_quality_set(r, ENESIM_QUALITY_FAST);
}

static void _enesim_renderer_path_nv_instance_deinit(void *o)
{
	Enesim_Renderer_Path_Nv *thiz;

	thiz = ENESIM_RENDERER_PATH_NV(o);
	if (thiz->path_id)
	{
		glDeletePathsNV(thiz->path_id, 1);
		thiz->path_id = 0;
	}
	if (thiz->path)
		enesim_path_unref(thiz->path);
	if (thiz->sb)
		glDeleteRenderbuffers(1, &thiz->sb);

	if (thiz->fsrc)
		enesim_surface_unref(thiz->fsrc);
	if (thiz->ssrc)
		enesim_surface_unref(thiz->ssrc);
}
#endif
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_path_nv_new(void)
{
#if BUILD_OPENGL_NV_PATH
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_path_nv);
	return r;
#else
	return NULL;
#endif
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
