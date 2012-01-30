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
typedef enum Enesim_Renderer_Flag
{
	ENESIM_RENDERER_FLAG_TRANSLATE		= (1 << 0), /**< The renderer can be translated using the origin property */
	ENESIM_RENDERER_FLAG_SCALE		= (1 << 1), /**< The renderer can be scaled using the scale property */
	ENESIM_RENDERER_FLAG_AFFINE 		= (1 << 2), /**< Affine transformation */
	ENESIM_RENDERER_FLAG_PROJECTIVE 	= (1 << 3), /**< Perspective transformations */
	ENESIM_RENDERER_FLAG_COLORIZE 		= (1 << 4), /**< Use the renderer color directly */
	ENESIM_RENDERER_FLAG_A8 		= (1 << 5), /**< Supports A8 surfaces */
	ENESIM_RENDERER_FLAG_ARGB8888 		= (1 << 6), /**< Supports ARGB8888 surfaces */
	ENESIM_RENDERER_FLAG_ROP 		= (1 << 7), /**< Can draw directly using the raster operation */
	ENESIM_RENDERER_FLAG_QUALITY 		= (1 << 8), /**< Supports the quality property */
	ENESIM_RENDERER_FLAG_MASK 		= (1 << 9), /**< The renderer can use the mask directly */
	ENESIM_RENDERER_FLAG_GEOMETRY 		= (1 << 10), /**< Geometry transformation */
} Enesim_Renderer_Flag;

#define ENESIM_RENDERER_FLAGS 11

/**
 * Callback function of the Enesim_Renderer_Damage descriptor function
 * @param r
 * @param area
 * @param past
 * @param data
 */
typedef Eina_Bool (*Enesim_Renderer_Damage_Cb)(Enesim_Renderer *r, Eina_Rectangle *area, Eina_Bool past, void *data);


EAPI void enesim_renderer_private_set(Enesim_Renderer *r, const char *name, void *data);
EAPI void * enesim_renderer_private_get(Enesim_Renderer *r, const char *name);

EAPI Enesim_Renderer * enesim_renderer_ref(Enesim_Renderer *r);
EAPI void enesim_renderer_unref(Enesim_Renderer *r);

EAPI void enesim_renderer_name_set(Enesim_Renderer *r, const char *name);
EAPI void enesim_renderer_name_get(Enesim_Renderer *r, const char **name);
EAPI void enesim_renderer_transformation_set(Enesim_Renderer *r, const Enesim_Matrix *m);
EAPI void enesim_renderer_transformation_get(Enesim_Renderer *r, Enesim_Matrix *m);
EAPI void enesim_renderer_geometry_transformation_set(Enesim_Renderer *r, const Enesim_Matrix *m);
EAPI void enesim_renderer_geometry_transformation_get(Enesim_Renderer *r, Enesim_Matrix *m);
EAPI void enesim_renderer_origin_set(Enesim_Renderer *r, double x, double y);
EAPI void enesim_renderer_origin_get(Enesim_Renderer *r, double *x, double *y);
EAPI void enesim_renderer_x_origin_set(Enesim_Renderer *r, double x);
EAPI void enesim_renderer_x_origin_get(Enesim_Renderer *r, double *x);
EAPI void enesim_renderer_y_origin_set(Enesim_Renderer *r, double y);
EAPI void enesim_renderer_y_origin_get(Enesim_Renderer *r, double *y);
EAPI void enesim_renderer_scale_set(Enesim_Renderer *r, double x, double y);
EAPI void enesim_renderer_scale_get(Enesim_Renderer *r, double *x, double *y);
EAPI void enesim_renderer_x_scale_set(Enesim_Renderer *r, double x);
EAPI void enesim_renderer_x_scale_get(Enesim_Renderer *r, double *x);
EAPI void enesim_renderer_y_scale_set(Enesim_Renderer *r, double y);
EAPI void enesim_renderer_y_scale_get(Enesim_Renderer *r, double *y);
EAPI void enesim_renderer_color_set(Enesim_Renderer *r, Enesim_Color color);
EAPI void enesim_renderer_color_get(Enesim_Renderer *r, Enesim_Color *color);
EAPI void enesim_renderer_rop_set(Enesim_Renderer *r, Enesim_Rop rop);
EAPI void enesim_renderer_rop_get(Enesim_Renderer *r, Enesim_Rop *rop);
EAPI void enesim_renderer_mask_set(Enesim_Renderer *r, Enesim_Renderer *mask);
EAPI void enesim_renderer_mask_get(Enesim_Renderer *r, Enesim_Renderer **mask);

EAPI void enesim_renderer_boundings(Enesim_Renderer *r, Enesim_Rectangle *rect);
EAPI void enesim_renderer_boundings_extended(Enesim_Renderer *r, Enesim_Rectangle *prev, Enesim_Rectangle *curr);

EAPI void enesim_renderer_destination_boundings(Enesim_Renderer *r, Eina_Rectangle *rect, int x, int y);
EAPI void enesim_renderer_destination_boundings_extended(Enesim_Renderer *r, Eina_Rectangle *prev, Eina_Rectangle *curr, int x, int y);

EAPI void enesim_renderer_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags);
EAPI Eina_Bool enesim_renderer_is_inside(Enesim_Renderer *r, double x, double y);
EAPI Eina_Bool enesim_renderer_has_changed(Enesim_Renderer *r);
EAPI void enesim_renderer_damages_get(Enesim_Renderer *r, Enesim_Renderer_Damage_Cb cb, void *data);

EAPI Eina_Bool enesim_renderer_setup(Enesim_Renderer *r, Enesim_Surface *s, Enesim_Error **error);
EAPI void enesim_renderer_cleanup(Enesim_Renderer *r, Enesim_Surface *s);

EAPI Eina_Bool enesim_renderer_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Eina_Rectangle *clip, int x, int y, Enesim_Error **error);
EAPI Eina_Bool enesim_renderer_draw_list(Enesim_Renderer *r, Enesim_Surface *s,
		Eina_List *clips, int x, int y, Enesim_Error **error);

EAPI void enesim_renderer_error_add(Enesim_Renderer *r, Enesim_Error **error, const char *file,
		const char *function, int line, char *fmt, ...);

#ifdef ENESIM_EXTENSION
/** Helper macro to add an error on a renderer based function */
#define ENESIM_RENDERER_ERROR(r, error, fmt, ...) \
	enesim_renderer_error_add(r, error, __FILE__, __FUNCTION__, __LINE__, fmt, ## __VA_ARGS__);

/** Renderer API/ABI version */
#define ENESIM_RENDERER_API 0

typedef struct _Enesim_Renderer_Descriptor Enesim_Renderer_Descriptor; /**< Renderer Descriptor Handler */

typedef enum _Enesim_Renderer_States
{
	ENESIM_STATE_CURRENT,
	ENESIM_STATE_PAST,
	ENESIM_RENDERER_STATES,
} Enesim_Renderer_States;

typedef struct _Enesim_Renderer_State
{
	Enesim_Rop rop;
	Enesim_Color color;
	Enesim_Quality quality;
	double ox, oy; /* the origin */
	double sx, sy; /* the scale */
	Enesim_Renderer *mask;
	Enesim_Matrix transformation;
	Enesim_Matrix_Type transformation_type;
	Enesim_Matrix geometry_transformation;
	Enesim_Matrix_Type geometry_transformation_type;
} Enesim_Renderer_State;

/* common descriptor functions */
typedef const char * (*Enesim_Renderer_Name)(Enesim_Renderer *r);
typedef void (*Enesim_Renderer_Delete)(Enesim_Renderer *r);
typedef Eina_Bool (*Enesim_Renderer_Inside)(Enesim_Renderer *r, double x, double y);
typedef void (*Enesim_Renderer_Boundings)(Enesim_Renderer *r,
		const Enesim_Renderer_State *state[ENESIM_RENDERER_STATES],
		Enesim_Rectangle *rect);
typedef void (*Enesim_Renderer_Destination_Boundings)(Enesim_Renderer *r,
		const Enesim_Renderer_State *state[ENESIM_RENDERER_STATES],
		Eina_Rectangle *dboundings);
typedef void (*Enesim_Renderer_Flags)(Enesim_Renderer *r, Enesim_Renderer_Flag *flags);
typedef Eina_Bool (*Enesim_Renderer_Has_Changed)(Enesim_Renderer *r);
typedef void (*Enesim_Renderer_Damage)(Enesim_Renderer *r, Enesim_Renderer_Damage_Cb cb, void *data);

/* software backend descriptor functions */
typedef void (*Enesim_Renderer_Sw_Fill)(Enesim_Renderer *r, int x, int y,
		unsigned int len, void *dst);

typedef Eina_Bool (*Enesim_Renderer_Sw_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *state[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Renderer_Sw_Fill *fill,
		Enesim_Error **error);
typedef void (*Enesim_Renderer_Sw_Cleanup)(Enesim_Renderer *r, Enesim_Surface *s);
/* opencl backend descriptor functions */
typedef Eina_Bool (*Enesim_Renderer_OpenCL_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *state[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		const char **program_name, const char **program_source,
		size_t *program_length,
		Enesim_Error **error);
typedef void (*Enesim_Renderer_OpenCL_Cleanup)(Enesim_Renderer *r, Enesim_Surface *s);
typedef Eina_Bool (*Enesim_Renderer_OpenCL_Kernel_Setup)(Enesim_Renderer *r, Enesim_Surface *s);

/* opengl backend descriptor functions */
typedef enum _Enesim_Renderer_OpenGL_Shader_Type
{
	ENESIM_SHADER_VERTEX,
	ENESIM_SHADER_GEOMETRY,
	ENESIM_SHADER_FRAGMENT,
	ENESIM_SHADERS,
} Enesim_Renderer_OpenGL_Shader_Type;

typedef struct _Enesim_Renerer_OpenGL_Shader
{
	Enesim_Renderer_OpenGL_Shader_Type type;
	const char *name;
	const char *source;
	size_t size;
} Enesim_Renderer_OpenGL_Shader;

typedef Eina_Bool (*Enesim_Renderer_OpenGL_Setup)(Enesim_Renderer *r,
		const Enesim_Renderer_State *state[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		int *num_shaders,
		Enesim_Renderer_OpenGL_Shader **shaders,
		Enesim_Error **error);
typedef void (*Enesim_Renderer_OpenGL_Cleanup)(Enesim_Renderer *r, Enesim_Surface *s);
typedef Eina_Bool (*Enesim_Renderer_OpenGL_Shader_Setup)(Enesim_Renderer *r, Enesim_Surface *s);

struct _Enesim_Renderer_Descriptor {
	/* common */
	unsigned int version;
	Enesim_Renderer_Name name;
	Enesim_Renderer_Delete free;
	Enesim_Renderer_Boundings boundings;
	Enesim_Renderer_Destination_Boundings destination_boundings;
	Enesim_Renderer_Flags flags;
	Enesim_Renderer_Inside is_inside;
	Enesim_Renderer_Damage damage;
	Enesim_Renderer_Has_Changed has_changed;
	/* software based functions */
	Enesim_Renderer_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	/* opencl based functions */
	Enesim_Renderer_OpenCL_Setup opencl_setup;
	Enesim_Renderer_OpenCL_Kernel_Setup opencl_kernel_setup;
	Enesim_Renderer_OpenCL_Cleanup opencl_cleanup;
	/* opengl based functions */
	Enesim_Renderer_OpenGL_Setup opengl_setup;
	Enesim_Renderer_OpenGL_Shader_Setup opengl_shader_setup;
	Enesim_Renderer_OpenGL_Cleanup opengl_cleanup;
};

EAPI Enesim_Renderer * enesim_renderer_new(Enesim_Renderer_Descriptor
		*descriptor, void *data);
EAPI void * enesim_renderer_data_get(Enesim_Renderer *r);

EAPI void enesim_renderer_relative_set(Enesim_Renderer *r, const Enesim_Renderer_State *rel, Enesim_Renderer_State *old_state);
EAPI void enesim_renderer_relative_unset(Enesim_Renderer *r, Enesim_Renderer_State *state);

/* TODO remove this one */
EAPI Enesim_Renderer_Sw_Fill enesim_renderer_sw_fill_get(Enesim_Renderer *r);

EAPI void enesim_renderer_sw_draw(Enesim_Renderer *r, int x, int y, unsigned int len, uint32_t *data);

#endif

#include "enesim_renderer_shape.h"
#include "enesim_renderer_rectangle.h"
#include "enesim_renderer_circle.h"
#include "enesim_renderer_ellipse.h"
#include "enesim_renderer_figure.h"
#include "enesim_renderer_path.h"
#include "enesim_renderer_checker.h"
#include "enesim_renderer_dispmap.h"
#include "enesim_renderer_raddist.h"
#include "enesim_renderer_grid.h"
#include "enesim_renderer_stripes.h"
#include "enesim_renderer_image.h"
#include "enesim_renderer_gradient.h"
#include "enesim_renderer_gradient_linear.h"
#include "enesim_renderer_gradient_radial.h"
#include "enesim_renderer_compound.h"
#include "enesim_renderer_importer.h"
#include "enesim_renderer_line.h"
#include "enesim_renderer_background.h"
#include "enesim_renderer_pattern.h"
#include "enesim_renderer_perlin.h"
#include "enesim_renderer_clipper.h"
#include "enesim_renderer_transition.h"

/**
 * @}
 */

#endif /*ENESIM_RENDERER_H_*/
