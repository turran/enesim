/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2010 Jorge Luis Zapata
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
/* We need to start removing all the extra logic in the renderer and move it
 * here for simple usage
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Renderer_State
{
	struct {
		Enesim_Matrix transformation;
		Enesim_Matrix_Type transformation_type;
		Eina_Bool visibility;
		Enesim_Rop rop;
		Enesim_Color color;
		Enesim_Renderer *mask;
		double ox;
		double oy;
	} current, past;
	char *name;
	Eina_Bool changed;
} Enesim_Renderer_Simple_State;

typedef struct _Enesim_Renderer_Simple
{
	Enesim_Renderer_State state;
} Enesim_Renderer_Simple;

static void _enesim_renderer_transformation_set(Enesim_Renderer *r
		const Enesim_Matrix *m)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_transformation_set(&thiz->state, m);
}

static void _enesim_renderer_transformation_get(Enesim_Renderer *r,
		Enesim_Matrix *m)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_transformation_get(&thiz->state, m);
}

static void _enesim_renderer_rop_set(Enesim_Renderer *r,
		Enesim_Rop rop)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_rop_set(&thiz->state, rop);
}

static void _enesim_renderer_rop_get(Enesim_Renderer *r,
		Enesim_Rop *rop)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_rop_get(&thiz->state, rop);
}

static void _enesim_renderer_visibility_set(Enesim_Renderer *r,
		Eina_Bool visibility)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_visibility_set(&thiz->state, visibility);
}

static void _enesim_renderer_visibility_get(Enesim_Renderer *r,
		Eina_Bool *visibility)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_visibility_get(&thiz->state, visibility);
}

static void _enesim_renderer_color_set(Enesim_Renderer *r,
		Enesim_Color color)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_color_set(&thiz->state, color);
}

static void _enesim_renderer_color_get(Enesim_Renderer *r,
		Enesim_Color *color)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_color_get(&thiz->state, color);
}

static void _enesim_renderer_x_set(Enesim_Renderer *r,
		double x)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_x_set(&thiz->state, x);
}

static void _enesim_renderer_x_get(Enesim_Renderer *r,
		double *x)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_x_get(&thiz->state, x);
}

static void _enesim_renderer_y_set(Enesim_Renderer *r,
		double y)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_y_set(&thiz->state, y);
}

static void _enesim_renderer_y_get(Enesim_Renderer *r,
		double *y)
{
	Enesim_Renderer_Simple *thiz;

	thiz = _simple_get(r);
	enesim_renderer_state_y_get(&thiz->state, y);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Eina_Bool enesim_renderer_state_transformation_set(Enesim_Renderer_State *thiz,
		const Enesim_Matrix *m)
{
	if (!m) return EINA_FALSE;
	thiz->current.transformation = *m;
	thiz->changed = EINA_TRUE;
	return EINA_TRUE;
}

Eina_Bool enesim_renderer_state_transformation_get(Enesim_Renderer_State *thiz,
		Enesim_Matrix *m)
{
	if (!m) return EINA_FALSE;
	*m = state->current.transformation;
	return EINA_TRUE;
}

void enesim_renderer_state_rop_set(Enesim_Renderer_State *thiz,
		Enesim_Rop rop)
{
	thiz->current.rop = rop;
	thiz->changed = EINA_TRUE;
}

Eina_Bool enesim_renderer_state_rop_get(Enesim_Renderer_State *thiz,
		Enesim_Rop *rop)
{
	if (!rop) return EINA_FALSE;
	*rop = state->current.rop;
	return EINA_TRUE;
}

void enesim_renderer_state_visibility_set(Enesim_Renderer_State *thiz,
		Eina_Bool visibility)
{
	thiz->current.visibility = visibility;
	thiz->changed = EINA_TRUE;
}

Eina_Bool enesim_renderer_state_visibility_get(Enesim_Renderer_State *thiz,
		Eina_Bool *visibility)
{
	if (!visibility) return EINA_FALSE;
	*visibility = state->current.visibility;
	return EINA_TRUE;
}

void enesim_renderer_state_color_set(Enesim_Renderer_State *thiz,
		Enesim_Color color)
{
	thiz->current.color = color;
	thiz->changed = EINA_TRUE;
}

Eina_Bool enesim_renderer_state_color_get(Enesim_Renderer_State *thiz,
		Enesim_Color *color)
{
	if (!color) return EINA_FALSE;
	*color = state->current.color;
	return EINA_TRUE;
}

void enesim_renderer_state_x_set(Enesim_Renderer_State *thiz, double x)
{
	thiz->current.ox = x;
	thiz->changed = EINA_TRUE;
}

Eina_Bool enesim_renderer_state_x_get(Enesim_Renderer_State *thiz, double *x)
{
	if (!x) return EINA_FALSE;
	*x = thiz->current.ox;
	return EINA_FALSE;
}

void enesim_renderer_state_y_set(Enesim_Renderer_State *thiz, double y)
{
	thiz->current.oy = y;
	thiz->changed = EINA_TRUE;
}

Eina_Bool enesim_renderer_state_y_get(Enesim_Renderer_State *thiz, double *y)
{
	if (!y) return EINA_FALSE;
	*y = thiz->current.oy;
	return EINA_FALSE;
}

void enesim_renderer_state_commit(Enesim_Renderer_State *thiz)
{
	Enesim_Renderer *old_mask;

	/* keep the referenceable objects */
	old_mask = thiz->past.mask;
	/* swp the state */
	thiz->past = thiz->current;
	/* increment the referenceable objects */
	if (thiz->past.mask)
		enesim_renderer_ref(thiz->past.mask);
	/* release the referenceable objects */
	if (old_mask)
		enesim_renderer_unref(old_mask);
}

Eina_Bool enesim_renderer_state_changed(Enesim_Renderer_State *thiz)
{
	if (!thiz->changed)
		return EINA_FALSE;
	/* the visibility */
	if (thiz->current.visibility != thiz->past.visibility)
	{
		return EINA_TRUE;
	}
	/* the rop */
	if (thiz->current.rop != thiz->past.rop)
	{
		return EINA_TRUE;
	}
	/* the color */
	if (thiz->current.color != thiz->past.color)
	{
		return EINA_TRUE;
	}
	/* the mask */
	if (thiz->current.mask && !thiz->past.mask)
		return EINA_TRUE;
	if (!thiz->current.mask && thiz->past.mask)
		return EINA_TRUE;
	if (thiz->current.mask)
	{
		if (enesim_renderer_has_changed(thiz->current.mask))
		{
			DBG("The mask renderer '%s' has changed", thiz->current.mask->name);
			return EINA_TRUE;
		}
	}
	/* the origin */
	if (thiz->current.ox != thiz->past.ox || thiz->current.oy != thiz->past.oy)
	{
		return EINA_TRUE;
	}
	/* the transformation */
	if (thiz->current.transformation_type != thiz->past.transformation_type)
	{
		return EINA_TRUE;
	}

	if (!enesim_matrix_is_equal(&thiz->current.transformation, &thiz->past.transformation))
	{
		return EINA_TRUE;
	}

	return EINA_FALSE;

}

void enesim_renderer_state_clear(Enesim_Renderer_State *thiz)
{
	/* past */
	if (thiz->past.mask)
	{
		enesim_renderer_unref(thiz->past.mask)
		thiz->past.mask = NULL;
	}
	/* current */
	if (thiz->current.mask)
	{
		enesim_renderer_unref(thiz->current.mask);
		thiz->current.mask = NULL;
	}

	if (thiz->name)
	{
		free(thiz->name);
		thiz->name = NULL;
	}
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

