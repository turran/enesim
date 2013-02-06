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
typedef struct _Enesim_Renderer_Shape_State
{
	struct {
		struct {
			Enesim_Color color;
			Enesim_Renderer *r;
			double weight;
			Enesim_Shape_Stroke_Location location;
			Enesim_Shape_Stroke_Cap cap;
			Enesim_Shape_Stroke_Join join;
		} stroke;

		struct {
			Enesim_Color color;
			Enesim_Renderer *r;
			Enesim_Shape_Fill_Rule rule;
		} fill;
		Enesim_Shape_Draw_Mode draw_mode;
	} current, past;
	Eina_List *stroke_dashes;

	Eina_Bool stroke_dashes_changed;
	Eina_Bool changed;
} Enesim_Renderer_Shape_State;
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_renderer_shape_state_commit(Enesim_Renderer_Shape_State *thiz)
{
	Enesim_Renderer *old_stroke;
	Enesim_Renderer *old_fill;

	/* keep the referenceable objects */
	/* swap the state */
	thiz->past = thiz->current;
	/* increment the referenceable objects */
	/* release the referenceable objects */
}

Eina_Bool enesim_renderer_shape_state_changed(
		Enesim_Renderer_Shape_State *thiz)
{
	if (!thiz->changed)
		return EINA_FALSE;
	/* we wont compare the stroke dashes, it has changed, then
	 * modify it directly
	 */
	if (thiz->dash_changed)
		return EINA_TRUE;
	/* the stroke */
	/* color */
	if (thiz->current.stroke.color != thiz->past.stroke.color)
		return EINA_TRUE;
	/* weight */
	if (thiz->current.stroke.weight != thiz->past.stroke.weight)
		return EINA_TRUE;
	/* location */
	if (thiz->current.stroke.location != thiz->past.stroke.location)
		return EINA_TRUE;
	/* join */
	if (thiz->current.stroke.join != thiz->past.stroke.join)
		return EINA_TRUE;
	/* cap */
	if (thiz->current.stroke.cap != thiz->past.stroke.cap)
		return EINA_TRUE;
	/* color */
	if (thiz->current.fill.color != thiz->past.fill.color)
		return EINA_TRUE;
	/* renderer */
	if (thiz->current.fill.r != thiz->past.fill.r)
		return EINA_TRUE;
	/* fill rule */
	if (thiz->current.fill.rule != thiz->past.fill.rule)
		return EINA_TRUE;
	/* draw mode */
	if (thiz->current.draw_mode != thiz->past.draw_mode)
		return EINA_TRUE;
	return EINA_FALSE;
}

void enesim_renderer_state_clear(Enesim_Renderer_Shape_State *thiz)
{
	/* unref */
	/* free */
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

