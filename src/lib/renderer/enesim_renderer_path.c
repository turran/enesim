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
#endif

#include "enesim_renderer_private.h"
#include "enesim_renderer_shape_private.h"
#include "enesim_renderer_path_abstract_private.h"

/* TODO later we need to use the priority system to choose the backend */
#define CHOOSE_CAIRO 0
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_renderer

#define ENESIM_RENDERER_PATH(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Renderer_Path,					\
		enesim_renderer_path_descriptor_get())

typedef struct _Enesim_Renderer_Path
{
	Enesim_Renderer_Shape parent;
	/* properties */
	Eina_List *commands;
	Eina_Bool changed;
	/* private */
	Enesim_Renderer *enesim;
#if BUILD_CAIRO
	Enesim_Renderer *cairo;
#endif
	Enesim_Renderer *current;
} Enesim_Renderer_Path;

typedef struct _Enesim_Renderer_Path_Class {
	Enesim_Renderer_Shape_Class parent;
} Enesim_Renderer_Path_Class;

static Enesim_Renderer * _path_implementation_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;
	const Enesim_Renderer_State *rstate;
	const Enesim_Renderer_Shape_State *sstate;
	Enesim_Renderer *ret;

	thiz = ENESIM_RENDERER_PATH(r);
	/* get the state properties */
	rstate = enesim_renderer_state_get(r);
	sstate = enesim_renderer_shape_state_get(r);

	/* TODO get the best implementation for such properties and features */
#if CHOOSE_CAIRO
	ret = thiz->cairo;
#else
	ret = thiz->enesim;
#endif

	/* propagate all the shape properties */
	enesim_renderer_shape_propagate(r, ret);
	/* propagate all the renderer properties */
	enesim_renderer_propagate(r, ret);
	/* set the commands */
	if (thiz->changed || ret != thiz->current)
		enesim_renderer_path_abstract_commands_set(ret, thiz->commands);
	return ret;
}

static void _path_span(Enesim_Renderer *r,
		int x, int y, unsigned int len, void *ddata)
{
	Enesim_Renderer_Path *thiz;

	thiz = ENESIM_RENDERER_PATH(r);
	enesim_renderer_sw_draw(thiz->current, x, y, len, ddata);
}

#if BUILD_OPENGL
static void _path_opengl_draw(Enesim_Renderer *r,
		Enesim_Surface *s, const Eina_Rectangle *area, int w, int h)
{
	Enesim_Renderer_Path *thiz;

	thiz = ENESIM_RENDERER_PATH(r);
	enesim_renderer_opengl_draw(thiz->current, s, area, w, h);
}
#endif

static Eina_Bool _path_setup(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Log **l)
{
	Enesim_Renderer_Path *thiz;

	thiz = ENESIM_RENDERER_PATH(r);
	thiz->current = _path_implementation_get(r);
	if (!thiz->current)
	{
		ENESIM_RENDERER_LOG(r, l, "No path implementation found");
		return EINA_FALSE;
	}
	if (!enesim_renderer_setup(thiz->current, s, l))
	{
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

static void _path_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Path *thiz;

	thiz = ENESIM_RENDERER_PATH(r);
	if (!thiz->current)
	{
		WRN("Doing a cleanup without a setup");
		return;
	}
	enesim_renderer_shape_state_commit(r);
	enesim_renderer_cleanup(thiz->current, s);
	thiz->current = NULL;
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
	if (!_path_setup(r, s, l))
		return EINA_FALSE;

	*draw = _path_span;
	return EINA_TRUE;
}

static void _path_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	_path_cleanup(r, s);
}

static void _path_features_get(Enesim_Renderer *r,
		Enesim_Renderer_Feature *features)
{
	Enesim_Renderer *current;

	current = _path_implementation_get(r);
	enesim_renderer_features_get(current, features);
}

static void _path_sw_hints(Enesim_Renderer *r,
		Enesim_Renderer_Sw_Hint *hints)
{
	Enesim_Renderer *current;

	current = _path_implementation_get(r);
	enesim_renderer_sw_hints_get(current, hints);
}

static Eina_Bool _path_has_changed(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;

	thiz = ENESIM_RENDERER_PATH(r);
	return thiz->changed;
}

static void _path_shape_features_get(Enesim_Renderer *r, Enesim_Shape_Feature *features)
{
	Enesim_Renderer *current;

	current = _path_implementation_get(r);
	enesim_renderer_shape_features_get(current, features);
}

static void _path_bounds_get(Enesim_Renderer *r,
		Enesim_Rectangle *bounds)
{
	Enesim_Renderer *current;

	current = _path_implementation_get(r);
	enesim_renderer_bounds(current, bounds);
}

#if BUILD_OPENGL
static Eina_Bool _path_opengl_setup(Enesim_Renderer *r,
		Enesim_Surface *s,
		Enesim_Renderer_OpenGL_Draw *draw,
		Enesim_Log **l)
{
	if (!_path_setup(r, s, l))
		return EINA_FALSE;

	*draw = _path_opengl_draw;
	return EINA_TRUE;
}

static void _path_opengl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	_path_cleanup(r, s);
}
#endif
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RENDERER_SHAPE_DESCRIPTOR,
		Enesim_Renderer_Path, Enesim_Renderer_Path_Class,
		enesim_renderer_path);

static void _enesim_renderer_path_class_init(void *k)
{
	Enesim_Renderer_Class *klass;
	Enesim_Renderer_Shape_Class *shape_klass;

	shape_klass = ENESIM_RENDERER_SHAPE_CLASS(k);
	shape_klass->has_changed = _path_has_changed;
	shape_klass->features_get = _path_shape_features_get;

	klass = ENESIM_RENDERER_CLASS(k);
	klass->base_name_get = _path_name;
	klass->sw_hints_get = _path_sw_hints;
	klass->bounds_get = _path_bounds_get;
	klass->features_get = _path_features_get;
	/* override the renderer setup or we will do the setup of the
	 * fill/stroke renderers twice
	 */
	klass->sw_setup = _path_sw_setup;
	klass->sw_cleanup = _path_sw_cleanup;
#if BUILD_OPENGL
	klass->opengl_setup = _path_opengl_setup;
	klass->opengl_cleanup = _path_opengl_cleanup;
#endif
}

static void _enesim_renderer_path_instance_init(void *o)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Renderer *r;

	thiz = ENESIM_RENDERER_PATH(o);
	r = enesim_renderer_path_enesim_new();
	thiz->enesim = r;
#if BUILD_CAIRO
	r = enesim_renderer_path_cairo_new();
	thiz->cairo = r;
#endif
}

static void _enesim_renderer_path_instance_deinit(void *o)
{
	Enesim_Renderer_Path *thiz;

	thiz = ENESIM_RENDERER_PATH(o);
	enesim_renderer_unref(thiz->enesim);
#if BUILD_CAIRO
	enesim_renderer_unref(thiz->cairo);
#endif
}
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

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_renderer_path);
	return r;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_command_clear(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Path_Command *c;

	thiz = ENESIM_RENDERER_PATH(r);
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
EAPI void enesim_renderer_path_command_add(Enesim_Renderer *r, Enesim_Path_Command *cmd)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Path_Command *new_command;

	thiz = ENESIM_RENDERER_PATH(r);

	/* do not allow a move to command after another move to, just simplfiy them */
	if (cmd->type == ENESIM_PATH_COMMAND_MOVE_TO)
	{
		Enesim_Path_Command *last_command;
		Eina_List *last;

		last = eina_list_last(thiz->commands);
		last_command = eina_list_data_get(last);
		if (last_command && last_command->type == ENESIM_PATH_COMMAND_MOVE_TO)
		{
			last_command->definition.move_to.x = cmd->definition.move_to.x;
			last_command->definition.move_to.y = cmd->definition.move_to.y;
			return;
		}
	}

	new_command = malloc(sizeof(Enesim_Path_Command));
	*new_command = *cmd;
	thiz->commands = eina_list_append(thiz->commands, new_command);
	thiz->changed = EINA_TRUE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_command_set(Enesim_Renderer *r,
		Eina_List *list)
{
	Enesim_Path_Command *cmd;
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
EAPI void enesim_renderer_path_command_get(Enesim_Renderer *r,
		Eina_List **list)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Path_Command *cmd;
	Eina_List *l;

	thiz = ENESIM_RENDERER_PATH(r);
	EINA_LIST_FOREACH(thiz->commands, l, cmd)
	{
		Enesim_Path_Command *new_cmd;
		new_cmd = calloc(1, sizeof(Enesim_Path_Command));
		*new_cmd = *cmd;
		*list = eina_list_append(*list, new_cmd);
	}
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_path_move_to(Enesim_Renderer *r, double x, double y)
{
	Enesim_Path_Command cmd;

	cmd.type = ENESIM_PATH_COMMAND_MOVE_TO;
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
	Enesim_Path_Command cmd;

	cmd.type = ENESIM_PATH_COMMAND_LINE_TO;
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
	Enesim_Path_Command cmd;

	cmd.type = ENESIM_PATH_COMMAND_SQUADRATIC_TO;
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
	Enesim_Path_Command cmd;

	cmd.type = ENESIM_PATH_COMMAND_QUADRATIC_TO;
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
	Enesim_Path_Command cmd;

	cmd.type = ENESIM_PATH_COMMAND_CUBIC_TO;
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
	Enesim_Path_Command cmd;

	cmd.type = ENESIM_PATH_COMMAND_SCUBIC_TO;
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
	Enesim_Path_Command cmd;

	cmd.type = ENESIM_PATH_COMMAND_ARC_TO;
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
	Enesim_Path_Command cmd;

	cmd.type = ENESIM_PATH_COMMAND_CLOSE;
	cmd.definition.close.close = close;
	enesim_renderer_path_command_add(r, &cmd);
}
