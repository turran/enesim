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

#define GLERR {\
        GLenum err; \
        err = glGetError(); \
        printf("Error %x\n", err); \
        }

#define ENESIM_LOG_DEFAULT enesim_log_renderer

#if BUILD_OPENGL
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_RENDERER_PATH_NV(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Path_Nv,					\
		enesim_renderer_path_nv_descriptor_get())

typedef struct _Enesim_Renderer_Path_Nv
{
	Enesim_Renderer_Path_Abstract parent;
	GLuint path_id;
	Enesim_Path *path;
	Eina_Bool dashes_changed;
	Eina_Bool path_changed;
	Eina_Bool new_path;
	Eina_Bool generated;
	
} Enesim_Renderer_Path_Nv;

typedef struct _Enesim_Renderer_Path_Nv_Class
{
	Enesim_Renderer_Path_Abstract_Class parent;
} Enesim_Renderer_Path_Nv_Class;

static void _enesim_renderer_path_nv_draw(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop, const Eina_Rectangle *area,
		int x, int y)
{
	Enesim_Renderer_Path_Nv *thiz;
	Enesim_Matrix m;
	GLfloat fm[16];

	thiz = ENESIM_RENDERER_PATH_NV(r);

	enesim_opengl_target_surface_set(s, x, y);
	enesim_opengl_rop_set(rop);

	/* add our own transformation matrix */
	enesim_renderer_transformation_get(r, &m);
	fm[0] = m.xx; fm[4] = m.xy; fm[8] = m.xz; fm[12] = 0;
	fm[1] = m.yx; fm[5] = m.yy; fm[9] = m.yz; fm[13] = 0;
	fm[2] = m.zx; fm[6] = m.zy; fm[10] = m.zz; fm[14] = 0;
	fm[3] = 0; fm[7] = 0; fm[11] = 0; fm[15] = 1;
	glMatrixMode(GL_PROJECTION);
	glMultMatrixf(fm);

	glEnable(GL_STENCIL_TEST);
	glStencilFunc(GL_NOTEQUAL, 0, 0x1F);
	glStencilOp(GL_KEEP, GL_KEEP, GL_ZERO);

	glClearStencil(0);
	glStencilMask(~0);
	glClear(GL_STENCIL_BUFFER_BIT);

	/* fill */
	if (enesim_renderer_shape_draw_mode_get(r) & ENESIM_RENDERER_SHAPE_DRAW_MODE_FILL)
	{
		Enesim_Color final_color;
		Enesim_Renderer *ren;

		enesim_renderer_shape_fill_setup(r, &final_color, &ren);
		/* TODO use the renderer */
		if (ren) enesim_renderer_unref(ren);

		glStencilFillPathNV(thiz->path_id, GL_COUNT_UP_NV, 0x1F); 
		glColor4f(argb8888_red_get(final_color) / 255.0,
			argb8888_green_get(final_color) / 255.0,
			argb8888_blue_get(final_color) / 255.0,
			argb8888_alpha_get(final_color) / 255.0);
		glCoverFillPathNV(thiz->path_id, GL_BOUNDING_BOX_NV);
	}

	/* stroke */
	if (enesim_renderer_shape_draw_mode_get(r) & ENESIM_RENDERER_SHAPE_DRAW_MODE_STROKE)
	{
		Enesim_Color final_color;
		Enesim_Renderer *ren;

		enesim_renderer_shape_stroke_setup(r, &final_color, &ren);
		/* TODO use the renderer */
		if (ren) enesim_renderer_unref(ren);

		glStencilStrokePathNV(thiz->path_id, 0x1, ~0);
		glColor4f(argb8888_red_get(final_color) / 255.0,
			argb8888_green_get(final_color) / 255.0,
			argb8888_blue_get(final_color) / 255.0,
			argb8888_alpha_get(final_color) / 255.0);
		glCoverStrokePathNV(thiz->path_id, GL_CONVEX_HULL_NV);
	}
	glDisable(GL_STENCIL_TEST);
}

static void _enesim_renderer_path_nv_setup_stroke(
		Enesim_Renderer_Path_Nv *thiz)
{
	Enesim_Renderer *r = ENESIM_RENDERER(thiz);
	GLenum join_style = GL_BEVEL_NV;
	GLenum cap_style = GL_ROUND_NV;

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
	Eina_Bool changed = EINA_TRUE;
	GLuint path_id;
	GLenum err;
	GLubyte *cmd, *cmds;
	GLfloat *coord, *coords;
	int num_cmds;
	int num_coords = 0;

	/* TODO check if the path has changed or not */
	if (thiz->generated)
		return EINA_TRUE;

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
			*cmd = 'M';
			*coord++ = pcmd->definition.move_to.x;
			*coord++ = pcmd->definition.move_to.y;
			num_coords += 2;
			break;
			
			case ENESIM_PATH_COMMAND_LINE_TO:
			*cmd = 'L';
			*coord++ = pcmd->definition.line_to.x;
			*coord++ = pcmd->definition.line_to.y;
			num_coords += 2;
			break;

			case ENESIM_PATH_COMMAND_ARC_TO:
			*cmd = 'A';
			num_coords += 7;
			*coord++ = pcmd->definition.arc_to.x;
			*coord++ = pcmd->definition.arc_to.y;
			*coord++ = pcmd->definition.arc_to.rx;
			*coord++ = pcmd->definition.arc_to.ry;
			*coord++ = pcmd->definition.arc_to.angle;
			*coord++ = pcmd->definition.arc_to.large;
			*coord++ = pcmd->definition.arc_to.sweep;
			break;

			case ENESIM_PATH_COMMAND_CLOSE:
			*cmd = 'Z';
			break;

			case ENESIM_PATH_COMMAND_CUBIC_TO:
			num_coords += 6;
			*cmd = 'C';
			*coord++ = pcmd->definition.cubic_to.x;
			*coord++ = pcmd->definition.cubic_to.y;
			*coord++ = pcmd->definition.cubic_to.ctrl_x0;
			*coord++ = pcmd->definition.cubic_to.ctrl_y0;
			*coord++ = pcmd->definition.cubic_to.ctrl_x1;
			*coord++ = pcmd->definition.cubic_to.ctrl_y1;
			break;

			case ENESIM_PATH_COMMAND_SCUBIC_TO:
			num_coords += 4;
			*cmd = 'S';
			*coord++ = pcmd->definition.scubic_to.x;
			*coord++ = pcmd->definition.scubic_to.y;
			*coord++ = pcmd->definition.scubic_to.ctrl_x;
			*coord++ = pcmd->definition.scubic_to.ctrl_y;
			break;

			case ENESIM_PATH_COMMAND_QUADRATIC_TO:
			num_coords += 4;
			*cmd = 'Q';
			*coord++ = pcmd->definition.quadratic_to.x;
			*coord++ = pcmd->definition.quadratic_to.y;
			*coord++ = pcmd->definition.quadratic_to.ctrl_x;
			*coord++ = pcmd->definition.quadratic_to.ctrl_y;
			break;

			case ENESIM_PATH_COMMAND_SQUADRATIC_TO:
			num_coords += 2;
			*cmd = 'T';
			*coord++ = pcmd->definition.squadratic_to.x;
			*coord++ = pcmd->definition.squadratic_to.y;
			break;

			default:
			break;
		}
		cmd++;
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
	_enesim_renderer_path_nv_setup_stroke(thiz);
	_enesim_renderer_path_nv_setup_fill(thiz);

	thiz->generated = EINA_TRUE;

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
		thiz->new_path = EINA_TRUE;
		thiz->path_changed = EINA_TRUE;
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
		INF("No ARB_framebuffer_object extension found");
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

static Eina_Bool _enesim_renderer_path_nv_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Path_Nv *thiz;

	thiz = ENESIM_RENDERER_PATH_NV(r);
	if (thiz->new_path || thiz->path_changed || (thiz->path && thiz->path->changed) || (thiz->dashes_changed))
		return EINA_TRUE;
	else
		return EINA_FALSE;
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
			ENESIM_RENDERER_FEATURE_ARGB8888;
}

static void _enesim_renderer_path_nv_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer_Path_Nv *thiz;
	GLenum err;
	float fbounds[4];
	
	thiz = ENESIM_RENDERER_PATH_NV(r);
	if (!_enesim_renderer_path_nv_upload_path(thiz))
		goto failed;

	/* TODO check if the path is filled/stroked */
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
	return;

failed:
	bounds->x = 0;
	bounds->y = 0;
	bounds->w = 0;
	bounds->h = 0;
}

static Eina_Bool _enesim_renderer_path_nv_opengl_initialize(
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
	s_klass->has_changed = _enesim_renderer_path_nv_has_changed;
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
}
#endif
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_renderer_path_nv_new(void)
{
#if BUILD_OPENGL
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
