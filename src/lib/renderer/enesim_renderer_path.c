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
#define ENESIM_RENDERER_PATH_MAGIC_CHECK(d) \
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_RENDERER_PATH_MAGIC))\
			EINA_MAGIC_FAIL(d, ENESIM_RENDERER_PATH_MAGIC);\
	} while(0)

typedef struct _Enesim_Renderer_Path
{
	EINA_MAGIC
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

static inline Enesim_Renderer_Path * _path_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;

	thiz = enesim_renderer_shape_data_get(r);
	ENESIM_RENDERER_PATH_MAGIC_CHECK(thiz);

	return thiz;
}

static Enesim_Renderer * _path_implementation_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Path *thiz;
	const Enesim_Renderer_State *rstate;
	const Enesim_Renderer_Shape_State *sstate;
	Enesim_Renderer *ret;

	thiz = _path_get(r);
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

	thiz = _path_get(r);
	enesim_renderer_sw_draw(thiz->current, x, y, len, ddata);
}

#if BUILD_OPENGL
static void _path_opengl_draw(Enesim_Renderer *r,
		Enesim_Surface *s, const Eina_Rectangle *area, int w, int h)
{
	Enesim_Renderer_Path *thiz;

	thiz = _path_get(r);
	enesim_renderer_opengl_draw(thiz->current, s, area, w, h);
}
#endif

static Eina_Bool _path_setup(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Log **l)
{
	Enesim_Renderer_Path *thiz;

	thiz = _path_get(r);
	thiz->current = _path_implementation_get(r);
	if (!enesim_renderer_setup(thiz->current, s, l))
	{
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

static void _path_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer_Path *thiz;

	thiz = _path_get(r);
	enesim_renderer_cleanup(thiz->current, s);
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

	thiz = _path_get(r);
	enesim_renderer_unref(thiz->enesim);
#if BUILD_CAIRO
	enesim_renderer_unref(thiz->cairo);
#endif
	free(thiz);
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

	thiz = _path_get(r);
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

static Enesim_Renderer_Shape_Descriptor _path_descriptor = {
	/* .version =			*/ ENESIM_RENDERER_API,
	/* .name =			*/ _path_name,
	/* .free =			*/ _path_free,
	/* .bounds_get =		*/ _path_bounds_get,
	/* .features_get =		*/ _path_features_get,
	/* .is_inside =			*/ NULL,
	/* .damage =			*/ NULL,
	/* .has_changed =		*/ _path_has_changed,
	/* .sw_hints_get =		*/ _path_sw_hints,
	/* .sw_setup =			*/ _path_sw_setup,
	/* .sw_cleanup =		*/ _path_sw_cleanup,
	/* .opencl_setup =		*/ NULL,
	/* .opencl_kernel_setup =	*/ NULL,
	/* .opencl_cleanup =		*/ NULL,
#if BUILD_OPENGL
	/* .opengl_initialize =		*/ NULL,
	/* .opengl_setup =		*/ _path_opengl_setup,
	/* .opengl_cleanup =		*/ _path_opengl_cleanup,
#else
	/* .opengl_initialize =		*/ NULL,
	/* .opengl_setup =		*/ NULL,
	/* .opengl_cleanup =		*/ NULL,
#endif
	/* .shape_features_get =	*/ _path_shape_features_get,
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
	Enesim_Renderer_Path *thiz;
	Enesim_Renderer *r;

	thiz = calloc(1, sizeof(Enesim_Renderer_Path));
	EINA_MAGIC_SET(thiz, ENESIM_RENDERER_PATH_MAGIC);

	r = enesim_renderer_path_enesim_new();
	thiz->enesim = r;
#if BUILD_CAIRO
	r = enesim_renderer_path_cairo_new();
	thiz->cairo = r;
#endif
	r = enesim_renderer_shape_new(&_path_descriptor, thiz);
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
EAPI void enesim_renderer_path_command_add(Enesim_Renderer *r, Enesim_Path_Command *cmd)
{
	Enesim_Renderer_Path *thiz;
	Enesim_Path_Command *new_command;

	thiz = _path_get(r);

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
