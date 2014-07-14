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
#ifndef ENESIM_RENDERER_COMPOUND_H_
#define ENESIM_RENDERER_COMPOUND_H_

/**
 * @file
 * @ender_group{Enesim_Renderer_Compound_Layer}
 * @ender_group{Enesim_Renderer_Compound}
 */

/**
 * @defgroup Enesim_Renderer_Compound Compound
 * @brief Renderer that contains other renderers
 * @ingroup Enesim_Renderer
 * @{
 */

/**
 * @defgroup Enesim_Renderer_Compound_Layer Compound Layer
 * @brief Compound layer @ender_inherits{Enesim_Renderer}
 * @ingroup Enesim_Renderer_Compound
 * @{
 */

typedef struct _Enesim_Renderer_Compound_Layer Enesim_Renderer_Compound_Layer;

EAPI Enesim_Renderer_Compound_Layer * enesim_renderer_compound_layer_new(void);
EAPI void enesim_renderer_compound_layer_renderer_set(
		Enesim_Renderer_Compound_Layer *l, Enesim_Renderer *r);
EAPI void enesim_renderer_compound_layer_rop_set(
		Enesim_Renderer_Compound_Layer *l, Enesim_Rop rop);
EAPI Enesim_Renderer_Compound_Layer * enesim_renderer_compound_layer_ref(
		Enesim_Renderer_Compound_Layer *l);
EAPI void enesim_renderer_compound_layer_unref(Enesim_Renderer_Compound_Layer *l);

typedef Eina_Bool (*Enesim_Renderer_Compound_Cb)(Enesim_Renderer *r,
		Enesim_Renderer_Compound_Layer *layer, void *data);

/**
 * @}
 */

EAPI Enesim_Renderer * enesim_renderer_compound_new(void);
EAPI void enesim_renderer_compound_layer_add(Enesim_Renderer *r,
		Enesim_Renderer_Compound_Layer *layer);
EAPI void enesim_renderer_compound_layer_remove(Enesim_Renderer *r,
		Enesim_Renderer_Compound_Layer *layer);
EAPI void enesim_renderer_compound_layer_clear(Enesim_Renderer *r);

EAPI void enesim_renderer_compound_layer_foreach(Enesim_Renderer *r,
		Enesim_Renderer_Compound_Cb cb, void *data);
EAPI void enesim_renderer_compound_layer_reverse_foreach(Enesim_Renderer *r,
		Enesim_Renderer_Compound_Cb cb, void *data);

EAPI void enesim_renderer_compound_background_enable_set(Enesim_Renderer *r, Eina_Bool enable);
EAPI Eina_Bool enesim_renderer_compound_background_enable_get(Enesim_Renderer *r);

EAPI void enesim_renderer_compound_background_color_set(Enesim_Renderer *r, Enesim_Color color);
EAPI Enesim_Color enesim_renderer_compound_background_color_get(Enesim_Renderer *r);
/**
 * @}
 */

#endif

