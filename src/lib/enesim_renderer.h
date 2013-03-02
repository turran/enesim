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
#ifndef ENESIM_RENDERER_H_
#define ENESIM_RENDERER_H_

/**
 * @defgroup Enesim_Renderer_Group Renderer
 * @{
 */

typedef struct _Enesim_Renderer Enesim_Renderer; /**< Renderer Handler */

/** Flags that specify what a renderer supports */
typedef enum _Enesim_Renderer_Feature
{
	ENESIM_RENDERER_FEATURE_TRANSLATE	= (1 << 0), /**< The renderer can be translated using the origin property */
	ENESIM_RENDERER_FEATURE_AFFINE 		= (1 << 1), /**< Affine transformation */
	ENESIM_RENDERER_FEATURE_PROJECTIVE 	= (1 << 2), /**< Perspective transformations */
	ENESIM_RENDERER_FEATURE_A8 		= (1 << 3), /**< Supports A8 surfaces */
	ENESIM_RENDERER_FEATURE_ARGB8888 	= (1 << 4), /**< Supports ARGB8888 surfaces */
	ENESIM_RENDERER_FEATURE_QUALITY 	= (1 << 5), /**< Supports the quality property */
} Enesim_Renderer_Feature;

#define ENESIM_RENDERER_FEATURE_TRANSFORMATION (ENESIM_RENDERER_FEATURE_AFFINE & ENESIM_RENDERER_FEATURE_PROJECTIVE)

/* TODO rmove this to the sw backend */
typedef enum _Enesim_Renderer_Sw_Hint
{
	ENESIM_RENDERER_HINT_COLORIZE 		= (1 << 0), /**< Can draw directly using the color property */
	ENESIM_RENDERER_HINT_ROP 		= (1 << 1), /**< Can draw directly using the raster operation */
	ENESIM_RENDERER_HINT_MASK 		= (1 << 2), /**< Can draw directly using the mask renderer */
} Enesim_Renderer_Sw_Hint;

/**
 * Callback function of the Enesim_Renderer_Damages_Get_Cb descriptor function
 * @param r
 * @param area
 * @param past
 * @param data
 */
typedef Eina_Bool (*Enesim_Renderer_Damage_Cb)(Enesim_Renderer *r, const Eina_Rectangle *area, Eina_Bool past, void *data);


EAPI void enesim_renderer_private_set(Enesim_Renderer *r, const char *name, void *data);
EAPI void * enesim_renderer_private_get(Enesim_Renderer *r, const char *name);

EAPI Enesim_Renderer * enesim_renderer_ref(Enesim_Renderer *r);
EAPI void enesim_renderer_unref(Enesim_Renderer *r);
EAPI int enesim_renderer_ref_count(Enesim_Renderer *r);

EAPI void enesim_renderer_lock(Enesim_Renderer *r);
EAPI void enesim_renderer_unlock(Enesim_Renderer *r);

EAPI void enesim_renderer_name_set(Enesim_Renderer *r, const char *name);
EAPI void enesim_renderer_name_get(Enesim_Renderer *r, const char **name);
EAPI void enesim_renderer_transformation_set(Enesim_Renderer *r, const Enesim_Matrix *m);
EAPI void enesim_renderer_transformation_get(Enesim_Renderer *r, Enesim_Matrix *m);
EAPI void enesim_renderer_transformation_type_get(Enesim_Renderer *r,
		Enesim_Matrix_Type *type);
EAPI void enesim_renderer_origin_set(Enesim_Renderer *r, double x, double y);
EAPI void enesim_renderer_origin_get(Enesim_Renderer *r, double *x, double *y);
EAPI void enesim_renderer_x_origin_set(Enesim_Renderer *r, double x);
EAPI void enesim_renderer_x_origin_get(Enesim_Renderer *r, double *x);
EAPI void enesim_renderer_y_origin_set(Enesim_Renderer *r, double y);
EAPI void enesim_renderer_y_origin_get(Enesim_Renderer *r, double *y);
EAPI void enesim_renderer_visibility_set(Enesim_Renderer *r, Eina_Bool visibility);
EAPI void enesim_renderer_visibility_get(Enesim_Renderer *r, Eina_Bool *visibility);
EAPI void enesim_renderer_color_set(Enesim_Renderer *r, Enesim_Color color);
EAPI void enesim_renderer_color_get(Enesim_Renderer *r, Enesim_Color *color);
EAPI void enesim_renderer_rop_set(Enesim_Renderer *r, Enesim_Rop rop);
EAPI void enesim_renderer_rop_get(Enesim_Renderer *r, Enesim_Rop *rop);
EAPI void enesim_renderer_mask_set(Enesim_Renderer *r, Enesim_Renderer *mask);
EAPI void enesim_renderer_mask_get(Enesim_Renderer *r, Enesim_Renderer **mask);
EAPI void enesim_renderer_quality_set(Enesim_Renderer *r, Enesim_Quality quality);
EAPI void enesim_renderer_quality_get(Enesim_Renderer *r, Enesim_Quality *quality);

EAPI void enesim_renderer_bounds(Enesim_Renderer *r, Enesim_Rectangle *rect);
EAPI void enesim_renderer_bounds_extended(Enesim_Renderer *r, Enesim_Rectangle *prev, Enesim_Rectangle *curr);

EAPI void enesim_renderer_destination_bounds(Enesim_Renderer *r, Eina_Rectangle *rect, int x, int y);
EAPI void enesim_renderer_destination_bounds_extended(Enesim_Renderer *r, Eina_Rectangle *prev, Eina_Rectangle *curr, int x, int y);

EAPI void enesim_renderer_features_get(Enesim_Renderer *r, Enesim_Renderer_Feature *features);
EAPI void enesim_renderer_sw_hints_get(Enesim_Renderer *r, Enesim_Renderer_Sw_Hint *hints);
EAPI Eina_Bool enesim_renderer_is_inside(Enesim_Renderer *r, double x, double y);
EAPI Eina_Bool enesim_renderer_has_changed(Enesim_Renderer *r);
EAPI Eina_Bool enesim_renderer_state_has_changed(Enesim_Renderer *r);
EAPI void enesim_renderer_damages_get(Enesim_Renderer *r, Enesim_Renderer_Damage_Cb cb, void *data);

EAPI Eina_Bool enesim_renderer_setup(Enesim_Renderer *r, Enesim_Surface *s, Enesim_Log **error);
EAPI void enesim_renderer_cleanup(Enesim_Renderer *r, Enesim_Surface *s);

EAPI Eina_Bool enesim_renderer_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Eina_Rectangle *clip, int x, int y, Enesim_Log **error);
EAPI Eina_Bool enesim_renderer_draw_list(Enesim_Renderer *r, Enesim_Surface *s,
		Eina_List *clips, int x, int y, Enesim_Log **error);

#ifdef ENESIM_EXTENSION
/* opengl backend descriptor functions */
typedef enum _Enesim_Renderer_OpenGL_Shader_Type
{
	ENESIM_SHADER_VERTEX,
	ENESIM_SHADER_GEOMETRY,
	ENESIM_SHADER_FRAGMENT,
	ENESIM_SHADERS,
} Enesim_Renderer_OpenGL_Shader_Type;

typedef struct _Enesim_Renderer_OpenGL_Shader Enesim_Renderer_OpenGL_Shader;
typedef struct _Enesim_Renderer_OpenGL_Program Enesim_Renderer_OpenGL_Program;

struct _Enesim_Renderer_OpenGL_Shader
{
	Enesim_Renderer_OpenGL_Shader_Type type;
	const char *name;
	const char *source;
};

struct _Enesim_Renderer_OpenGL_Program
{
	const char *name;
	Enesim_Renderer_OpenGL_Shader **shaders;
	int num_shaders;
};

typedef void (*Enesim_Renderer_OpenGL_Draw)(Enesim_Renderer *r, Enesim_Surface *s,
		const Eina_Rectangle *area, int dw, int dh);


#endif

/**
 * @}
 */

#endif /*ENESIM_RENDERER_H_*/
