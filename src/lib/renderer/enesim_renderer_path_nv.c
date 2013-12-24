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

#if BUILD_OPENGL_NV_PATH
#include "Enesim_OpenGL.h"
#include "enesim_opengl_private.h"
#endif

#include "enesim_surface_private.h"

#define GLERR {\
        GLenum err; \
        err = glGetError(); \
        printf("Error %x\n", err); \
        }

#define ENESIM_LOG_DEFAULT enesim_log_renderer

#if BUILD_OPENGL_NV_PATH
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_PATH_NV(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Path_Nv,					\
		enesim_renderer_path_nv_descriptor_get())

typedef struct _Enesim_Renderer_Path_Nv
{
	Enesim_Renderer_Path_Abstract parent;
	Enesim_Path *path;
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
static Enesim_Renderer_OpenGL_Shader _enesim_renderer_path_nv_shader = {
	/* .type 	= */ ENESIM_SHADER_FRAGMENT,
	/* .name 	= */ "enesim_renderer_path_nv",
	/* .source	= */
#include "enesim_renderer_opengl_common_texture.glsl"
};

static Enesim_Renderer_OpenGL_Shader *_enesim_renderer_path_nv_shaders[] = {
	&_enesim_renderer_path_nv_shader,
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

static Eina_Bool _enesim_renderer_path_nv_common_texture_setup(GLenum pid,
		Enesim_Surface *s, Enesim_Color color)
{
	Enesim_Buffer_OpenGL_Data *backend_data;
	int texture_u;
	int color_u;

	glUseProgramObjectARB(pid);
	color_u = glGetUniformLocationARB(pid, "color");
	texture_u = glGetUniformLocationARB(pid, "texture");

	glUniform4fARB(color_u,
			argb8888_red_get(color) / 255.0,
			argb8888_green_get(color) / 255.0,
			argb8888_blue_get(color) / 255.0,
			argb8888_alpha_get(color) / 255.0);


	backend_data = enesim_surface_backend_data_get(s);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, backend_data->textures[0]);
	glUniform1i(texture_u, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	return EINA_TRUE;
}

/* future code to draw using more samples */
#if 0
void _fill_sample(Enesim_Renderer_Path_Nv *thiz, Enesim_Color final_color)
{
	const int coveragePassesToAccumulate = 5;
	int i;

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 0x80, 0x7F);
	glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);  // tricky: zero 0x7F mask stencil on covers, but set 0x80

	//glClearColor(0, 0, 0, 0);
	//glClear(GL_COLOR_BUFFER_BIT);

	glColorMask(0,0,0,1);  // just update alpha
	// M STENCIL+COVER PASSES to accumulate jittered path coverage into framebuffer's alpha channel 
	glStencilFillPathNV(thiz->path_id, GL_COUNT_UP_NV, 0x7F);
	glCoverFillPathNV(thiz->path_id, GL_PATH_FILL_COVER_MODE_NV);
	glEnable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE); // sum up alpha
	glColor4f(0, 0, 0, 1.0/coveragePassesToAccumulate);

	static const GLfloat jitters[5][2] = { {0,0}, {5,0},  {-5, 0}, {0, 5}, {0, -5} /* various small subpixel jitter X & Y values */ };
	for (i=0; i<coveragePassesToAccumulate ; i++) {
		glMatrixPushEXT(GL_PROJECTION);
		{
			glMatrixTranslatefEXT(GL_PROJECTION, jitters[i][0], jitters[i][1], 0);
			glStencilFillPathNV(thiz->path_id, GL_COUNT_UP_NV, 0x7F);
			glCoverFillPathNV(thiz->path_id, GL_PATH_FILL_COVER_MODE_NV);
		}
		glMatrixPopEXT(GL_PROJECTION);
	}
	// FINAL COVER PASS uses accumulated coverage stashed in destination alpha
	glColorMask(1,1,1,1);
	// modulate RGB with destination alpha and then zero destination alpha
	glBlendFuncSeparate(GL_DST_ALPHA, GL_ZERO, GL_DST_ALPHA, GL_ZERO);
	glColor4f(argb8888_red_get(final_color) / 255.0,
		argb8888_green_get(final_color) / 255.0,
		argb8888_blue_get(final_color) / 255.0,
		argb8888_alpha_get(final_color) / 255.0);
	glStencilFunc(GL_EQUAL, 0x80, 0xFF);  // update any sample touched in earlier passes
	glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);  // now set stencil back to zero (clearing 0x80)
	glCoverFillPathInstancedNV(coveragePassesToAccumulate, 
			GL_UNSIGNED_BYTE, "\0\0\0\0\0",  // tricky: draw path objects path+0,path+0,path+0,path+0
			thiz->path_id,  // this is that path object that is added to zero four times
			GL_BOUNDING_BOX_OF_BOUNDING_BOXES_NV, GL_TRANSLATE_2D_NV, jitters);
	enesim_opengl_rop_set(ENESIM_ROP_FILL);
}
#endif

static inline void _enesim_renderer_path_nv_sub_draw(Enesim_Renderer *r,
		Enesim_Color color, Enesim_Surface **s, Enesim_Pool *p,
		const Eina_Rectangle *area)
{
	int w, h;

	if (!r) return;

	if (*s)
	{
		enesim_surface_size_get(*s, &w, &h);
		if (w != area->w || h != area->h)
		{
			enesim_surface_unref(*s);
			*s = NULL;
		}
	}

	if (!*s)
	{
		*s = enesim_surface_new_pool_from(ENESIM_FORMAT_ARGB8888,
				area->w, area->h, p);
	}
	enesim_renderer_opengl_draw(r, *s, ENESIM_ROP_FILL, area, 0, 0);
}

static void _enesim_renderer_path_nv_draw(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop, const Eina_Rectangle *area,
		int x, int y)
{
	Enesim_Renderer_Path_Nv *thiz;
	Enesim_Renderer_Shape_Draw_Mode draw_mode;
	Enesim_Renderer *fren = NULL;
	Enesim_Renderer *sren = NULL;
	Enesim_Renderer_OpenGL_Data *rdata;
	Enesim_Color scolor;
	Enesim_Color fcolor;
	Enesim_Matrix m;
	Enesim_Pool *pool;
	GLfloat fm[16];
	GLenum status;

	thiz = ENESIM_RENDERER_PATH_NV(r);

	rdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_OPENGL);
	draw_mode = enesim_renderer_shape_draw_mode_get(r);
	pool = enesim_surface_pool_get(s);
	/* draw the temporary fill/stroke renderers */
	if (draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL)
	{
		enesim_renderer_shape_fill_setup(r, &fcolor, &fren);
		 _enesim_renderer_path_nv_sub_draw(fren, fcolor, &thiz->fsrc,
				pool, area);
	}
	if (draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
	{
		enesim_renderer_shape_stroke_setup(r, &scolor, &sren);
		 _enesim_renderer_path_nv_sub_draw(sren, scolor, &thiz->ssrc,
				pool, area);
	}

	/* finally draw */
	enesim_opengl_target_surface_set(s, x, y);
	/* attach the stencil buffer on the framebuffer */
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, thiz->sb);
	status = glCheckFramebufferStatusEXT(GL_FRAMEBUFFER_EXT);
	if (status != GL_FRAMEBUFFER_COMPLETE_EXT)
	{
		ERR("The framebuffer failed %d", status);
		return;
	}
	enesim_opengl_rop_set(rop);

	glClearStencil(0);
	glStencilMask(~0);
	glClear(GL_STENCIL_BUFFER_BIT);

	/* add our own transformation matrix */
	enesim_renderer_transformation_get(r, &m);
	enesim_opengl_matrix_convert(&m, fm);

	glMatrixMode(GL_PROJECTION);
	glMultMatrixf(fm);

	/* fill */
	if (draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL)
	{
		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

		if (fren)
		{
			Enesim_OpenGL_Compiled_Program *cp;
			cp = &rdata->program->compiled[0];
			_enesim_renderer_path_nv_common_texture_setup(cp->id, thiz->fsrc, fcolor);
		}
		glStencilFillPathNV(thiz->path_id, GL_COUNT_UP_NV, 0xFF); 
		glColor4f(argb8888_red_get(fcolor) / 255.0,
			argb8888_green_get(fcolor) / 255.0,
			argb8888_blue_get(fcolor) / 255.0,
			argb8888_alpha_get(fcolor) / 255.0);
		glCoverFillPathNV(thiz->path_id, GL_BOUNDING_BOX_NV);
		glUseProgramObjectARB(0);
	}

	/* stroke */
	if (draw_mode & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
	{
		glEnable(GL_STENCIL_TEST);
		glStencilFunc(GL_NOTEQUAL, 0, 0xFF);
		glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

		if (sren)
		{
			Enesim_OpenGL_Compiled_Program *cp;
			cp = &rdata->program->compiled[0];
			_enesim_renderer_path_nv_common_texture_setup(cp->id, thiz->ssrc, scolor);
		}
		glStencilStrokePathNV(thiz->path_id, 0x1, ~0);
		glColor4f(argb8888_red_get(scolor) / 255.0,
			argb8888_green_get(scolor) / 255.0,
			argb8888_blue_get(scolor) / 255.0,
			argb8888_alpha_get(scolor) / 255.0);
		glCoverStrokePathNV(thiz->path_id, GL_CONVEX_HULL_NV);
	}

	/* unref the renderers */
	enesim_renderer_unref(fren);
	enesim_renderer_unref(sren);
	enesim_pool_unref(pool);

	/* restore the state */
	glClearStencil(0);
	glClear(GL_STENCIL_BUFFER_BIT);
	glColor4f(1, 1, 1, 1);
	glDisable(GL_STENCIL_TEST);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, 0);
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
	Eina_List *l;
	GLuint path_id;
	GLenum err;
	GLubyte *cmd, *cmds;
	GLfloat *coord, *coords;
	GLfloat coeffs[6] = { 1, 0, 0, 0, 1, 0};
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
			case ENESIM_PATH_COMMAND_MOVE_TO:
			*cmd++ = 'M';
			*coord++ = pcmd->definition.move_to.x;
			*coord++ = pcmd->definition.move_to.y;
			num_coords += 2;
			break;
			
			case ENESIM_PATH_COMMAND_LINE_TO:
			*cmd++ = 'L';
			*coord++ = pcmd->definition.line_to.x;
			*coord++ = pcmd->definition.line_to.y;
			num_coords += 2;
			break;

			case ENESIM_PATH_COMMAND_ARC_TO:
			*cmd++ = 'A';
			num_coords += 7;
			*coord++ = pcmd->definition.arc_to.rx;
			*coord++ = pcmd->definition.arc_to.ry;
			*coord++ = pcmd->definition.arc_to.angle;
			*coord++ = pcmd->definition.arc_to.large;
			*coord++ = pcmd->definition.arc_to.sweep;
			*coord++ = pcmd->definition.arc_to.x;
			*coord++ = pcmd->definition.arc_to.y;
			break;

			case ENESIM_PATH_COMMAND_CLOSE:
			*cmd++ = 'Z';
			break;

			case ENESIM_PATH_COMMAND_CUBIC_TO:
			num_coords += 6;
			*cmd++ = 'C';
			*coord++ = pcmd->definition.cubic_to.ctrl_x0;
			*coord++ = pcmd->definition.cubic_to.ctrl_y0;
			*coord++ = pcmd->definition.cubic_to.ctrl_x1;
			*coord++ = pcmd->definition.cubic_to.ctrl_y1;
			*coord++ = pcmd->definition.cubic_to.x;
			*coord++ = pcmd->definition.cubic_to.y;
			break;

			case ENESIM_PATH_COMMAND_SCUBIC_TO:
			num_coords += 4;
			*cmd++ = 'S';
			*coord++ = pcmd->definition.scubic_to.ctrl_x;
			*coord++ = pcmd->definition.scubic_to.ctrl_y;
			*coord++ = pcmd->definition.scubic_to.x;
			*coord++ = pcmd->definition.scubic_to.y;
			break;

			case ENESIM_PATH_COMMAND_QUADRATIC_TO:
			num_coords += 4;
			*cmd++ = 'Q';
			*coord++ = pcmd->definition.quadratic_to.ctrl_x;
			*coord++ = pcmd->definition.quadratic_to.ctrl_y;
			*coord++ = pcmd->definition.quadratic_to.x;
			*coord++ = pcmd->definition.quadratic_to.y;
			break;

			case ENESIM_PATH_COMMAND_SQUADRATIC_TO:
			num_coords += 2;
			*cmd++ = 'T';
			*coord++ = pcmd->definition.squadratic_to.x;
			*coord++ = pcmd->definition.squadratic_to.y;
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
	/* we pass x on s and y on t */
	glPathTexGenNV(GL_TEXTURE0, GL_OBJECT_LINEAR, 2, coeffs);
	err = glGetError();
	if (err)
	{
		ERR("glPathTexGenNV failed 0x%08x", err);
	}
propagate:
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
	int w, h;

	thiz = ENESIM_RENDERER_PATH_NV(r);
	/* TODO later we might need that every surface also owns a stencil buffer? */
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
	}

	/* upload the commands */
	if (!_enesim_renderer_path_nv_upload_path(thiz))
		return EINA_FALSE;

	*draw = _enesim_renderer_path_nv_draw;
	return EINA_TRUE;
}

static void _enesim_renderer_path_nv_opengl_cleanup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED)
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

static void _enesim_renderer_path_nv_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
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
	if (type != ENESIM_MATRIX_IDENTITY)
	{
		Enesim_Quad q;

		enesim_matrix_rectangle_transform(&m, bounds, &q);
		enesim_quad_rectangle_to(&q, bounds);
	}

	return;

failed:
	bounds->x = 0;
	bounds->y = 0;
	bounds->w = 0;
	bounds->h = 0;
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
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
