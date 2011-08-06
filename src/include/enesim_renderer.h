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

/** Renderer API/ABI version */
#define ENESIM_RENDERER_API 0

/** Flags that specify what a renderer supports */
typedef enum Enesim_Renderer_Flag
{
	ENESIM_RENDERER_FLAG_TRANSLATE		= (1 << 0), /**< The renderer can be translated using the origin property */
	ENESIM_RENDERER_FLAG_AFFINE 		= (1 << 1), /**< Affine transformation */
	ENESIM_RENDERER_FLAG_PROJECTIVE 	= (1 << 2), /**< Perspective transformations */
	ENESIM_RENDERER_FLAG_COLORIZE 		= (1 << 3), /**< Use the renderer color directly */
	ENESIM_RENDERER_FLAG_A8 		= (1 << 4), /**< Supports A8 surfaces */
	ENESIM_RENDERER_FLAG_ARGB8888 		= (1 << 5), /**< Supports ARGB8888 surfaces */
	ENESIM_RENDERER_FLAG_ROP 		= (1 << 6), /**< Can draw directly using the raster operation */
	ENESIM_RENDERER_FLAG_QUALITY 		= (1 << 7), /**< Supports the quality property */
} Enesim_Renderer_Flag;

typedef struct _Enesim_Renderer Enesim_Renderer; /**< Renderer Handler */
typedef struct _Enesim_Renderer_Descriptor Enesim_Renderer_Descriptor; /**< Renderer Descriptor Handler */

typedef void (*Enesim_Renderer_Sw_Fill)(Enesim_Renderer *r, int x, int y,
		unsigned int len, uint32_t *dst);

typedef void (*Enesim_Renderer_Delete)(Enesim_Renderer *r);
typedef Eina_Bool (*Enesim_Renderer_Inside)(Enesim_Renderer *r, double x, double y);
typedef void (*Enesim_Renderer_Boundings)(Enesim_Renderer *r, Enesim_Rectangle *rect);
typedef void (*Enesim_Renderer_Flags)(Enesim_Renderer *r, Enesim_Renderer_Flag *flags);
typedef Eina_Bool (*Enesim_Renderer_Sw_Setup)(Enesim_Renderer *r,
		Enesim_Renderer_Sw_Fill *fill);
typedef void (*Enesim_Renderer_Sw_Cleanup)(Enesim_Renderer *r);

typedef Eina_Bool (*Enesim_Renderer_OpenCL_Setup)(Enesim_Renderer *r, Enesim_Surface *s,
		const char **program_name, const char **program_source, size_t *program_length);
typedef void (*Enesim_Renderer_OpenCL_Cleanup)(Enesim_Renderer *r, Enesim_Surface *s);
typedef Eina_Bool (*Enesim_Renderer_OpenCL_Kernel_Setup)(Enesim_Renderer *r, Enesim_Surface *s);

struct _Enesim_Renderer_Descriptor {
	/* common */
	unsigned int version;
	Enesim_Renderer_Delete free;
	Enesim_Renderer_Boundings boundings;
	Enesim_Renderer_Flags flags;
	Enesim_Renderer_Inside is_inside;
	/* software based functions */
	Enesim_Renderer_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	/* opencl based functions */
	Enesim_Renderer_OpenCL_Setup opencl_setup;
	Enesim_Renderer_OpenCL_Kernel_Setup opencl_kernel_setup;
	Enesim_Renderer_OpenCL_Cleanup opencl_cleanup;
};

EAPI Enesim_Renderer * enesim_renderer_new(Enesim_Renderer_Descriptor
		*descriptor, void *data);
EAPI void * enesim_renderer_data_get(Enesim_Renderer *r);
EAPI Enesim_Renderer_Sw_Fill enesim_renderer_sw_fill_get(Enesim_Renderer *r);
EAPI Eina_Bool enesim_renderer_sw_setup(Enesim_Renderer *r);
EAPI void enesim_renderer_sw_cleanup(Enesim_Renderer *r);

EAPI void enesim_renderer_private_set(Enesim_Renderer *r, const char *name, void *data);
EAPI void * enesim_renderer_private_get(Enesim_Renderer *r, const char *name);

EAPI void enesim_renderer_transformation_set(Enesim_Renderer *r, Enesim_Matrix *m);
EAPI void enesim_renderer_transformation_get(Enesim_Renderer *r, Enesim_Matrix *m);
EAPI void enesim_renderer_delete(Enesim_Renderer *r);
EAPI void enesim_renderer_origin_set(Enesim_Renderer *r, double x, double y);
EAPI void enesim_renderer_origin_get(Enesim_Renderer *r, double *x, double *y);
EAPI void enesim_renderer_x_origin_set(Enesim_Renderer *r, double x);
EAPI void enesim_renderer_x_origin_get(Enesim_Renderer *r, double *x);
EAPI void enesim_renderer_y_origin_set(Enesim_Renderer *r, double y);
EAPI void enesim_renderer_y_origin_get(Enesim_Renderer *r, double *y);
EAPI void enesim_renderer_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Eina_Rectangle *clip, int x, int y);
EAPI void enesim_renderer_draw_list(Enesim_Renderer *r, Enesim_Surface *s,
		Eina_List *clips, int x, int y);
EAPI void enesim_renderer_color_set(Enesim_Renderer *r, Enesim_Color color);
EAPI void enesim_renderer_color_get(Enesim_Renderer *r, Enesim_Color *color);
EAPI void enesim_renderer_boundings(Enesim_Renderer *r, Enesim_Rectangle *rect);
EAPI void enesim_renderer_translated_boundings(Enesim_Renderer *r, Enesim_Rectangle *rect);
EAPI void enesim_renderer_destination_boundings(Enesim_Renderer *r, Eina_Rectangle *rect, int x, int y);
EAPI void enesim_renderer_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags);
EAPI void enesim_renderer_rop_set(Enesim_Renderer *r, Enesim_Rop rop);
EAPI void enesim_renderer_rop_get(Enesim_Renderer *r, Enesim_Rop *rop);
EAPI Eina_Bool enesim_renderer_is_inside(Enesim_Renderer *r, double x, double y);

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

/**
 * @defgroup Enesim_Renderer_Hswitch_Group Horizontal Switch
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_hswitch_new(void);
EAPI void enesim_renderer_hswitch_w_set(Enesim_Renderer *r, int w);
EAPI void enesim_renderer_hswitch_h_set(Enesim_Renderer *r, int h);
EAPI void enesim_renderer_hswitch_left_set(Enesim_Renderer *r,
		Enesim_Renderer *left);
EAPI void enesim_renderer_hswitch_right_set(Enesim_Renderer *r,
		Enesim_Renderer *right);
EAPI void enesim_renderer_hswitch_step_set(Enesim_Renderer *r, double step);
/**
 * @}
 */

/**
 * @}
 * @defgroup Enesim_Renderer_Transition_Group Transition
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_transition_new(void);
EAPI void enesim_renderer_transition_level_set(Enesim_Renderer *r, double level);
EAPI void enesim_renderer_transition_source_set(Enesim_Renderer *r, Enesim_Renderer *r0);
EAPI void enesim_renderer_transition_target_set(Enesim_Renderer *r, Enesim_Renderer *r1);
EAPI void enesim_renderer_transition_offset_set(Enesim_Renderer *r, int x, int y);
/**
 * @}
 * @defgroup Enesim_Renderer_Importer_Group Importer
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_importer_new(void);
EAPI void enesim_renderer_importer_angle_set(Enesim_Renderer *r, Enesim_Angle angle);
EAPI void enesim_renderer_importer_buffer_set(Enesim_Renderer *r, Enesim_Buffer *buffer);
/**
 * @}
 * @defgroup Enesim_Renderer_Perlin_Group Perlin
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_perlin_new(void);
EAPI void enesim_renderer_perlin_octaves_set(Enesim_Renderer *r, unsigned int octaves);
EAPI void enesim_renderer_perlin_persistence_set(Enesim_Renderer *r, double persistence);
EAPI void enesim_renderer_perlin_xfrequency_set(Enesim_Renderer *r, double freq);
EAPI void enesim_renderer_perlin_yfrequency_set(Enesim_Renderer *r, double freq);


/**
 * @}
 * @defgroup Enesim_Renderer_Gradient_Group Gradient
 * @{
 */

typedef enum _Enesim_Renderer_Gradient_Mode
{
	ENESIM_GRADIENT_RESTRICT,
	ENESIM_GRADIENT_PAD,
	ENESIM_GRADIENT_REFLECT,
	ENESIM_GRADIENT_REPEAT,
	ENESIM_GRADIENT_MODES,
} Enesim_Renderer_Gradient_Mode;

typedef struct _Enesim_Renderer_Gradient_Stop
{
	Enesim_Argb argb;
	double pos;
} Enesim_Renderer_Gradient_Stop;

EAPI void enesim_renderer_gradient_stop_add(Enesim_Renderer *r, Enesim_Renderer_Gradient_Stop *stop);
EAPI void enesim_renderer_gradient_clear(Enesim_Renderer *r);
EAPI void enesim_renderer_gradient_stop_set(Enesim_Renderer *r,
		Eina_List *list);
EAPI void enesim_renderer_gradient_mode_set(Enesim_Renderer *r, Enesim_Renderer_Gradient_Mode mode);
EAPI void enesim_renderer_gradient_mode_get(Enesim_Renderer *r, Enesim_Renderer_Gradient_Mode *mode);

/**
 * @defgroup Enesim_Renderer_Gradient_Linear_Group Linear
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_gradient_linear_new(void);
EAPI void enesim_renderer_gradient_linear_x0_set(Enesim_Renderer *r, double x0);
EAPI void enesim_renderer_gradient_linear_y0_set(Enesim_Renderer *r, double y0);
EAPI void enesim_renderer_gradient_linear_x1_set(Enesim_Renderer *r, double x1);
EAPI void enesim_renderer_gradient_linear_y1_set(Enesim_Renderer *r, double y1);
EAPI void enesim_renderer_gradient_linear_pos_set(Enesim_Renderer *r, double x0,
		double y0, double x1, double y1);

/**
 * @}
 * @defgroup Enesim_Renderer_Gradient_Radial_Group Radial
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_gradient_radial_new(void);
EAPI void enesim_renderer_gradient_radial_center_x_set(Enesim_Renderer *r, double v);
EAPI void enesim_renderer_gradient_radial_center_y_set(Enesim_Renderer *r, double v);
EAPI void enesim_renderer_gradient_radial_radius_y_set(Enesim_Renderer *r, double v);
EAPI void enesim_renderer_gradient_radial_radius_x_set(Enesim_Renderer *r, double v);

/**
 * @}
 * @}
 * @defgroup Enesim_Renderer_Compound_Group Compound
 * @{
 */

EAPI Enesim_Renderer * enesim_renderer_compound_new(void);
EAPI void enesim_renderer_compound_layer_add(Enesim_Renderer *r,
		Enesim_Renderer *rend);
EAPI void enesim_renderer_compound_layer_remove(Enesim_Renderer *r,
		Enesim_Renderer *rend);
EAPI void enesim_renderer_compound_layer_clear(Enesim_Renderer *r);
EAPI void enesim_renderer_compound_layer_set(Enesim_Renderer *r,
		Eina_List *list);
/**
 * @}
 * @defgroup Enesim_Renderer_Background_Group Background
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_background_new(void);
EAPI void enesim_renderer_background_color_set(Enesim_Renderer *r,
		Enesim_Color color);

/**
 * @}
 * @defgroup Enesim_Renderer_Clipper_Group Clipper
 * @{
 */

EAPI Enesim_Renderer * enesim_renderer_clipper_new(void);
EAPI void enesim_renderer_clipper_content_set(Enesim_Renderer *r,
		Enesim_Renderer *content);
EAPI void enesim_renderer_clipper_content_get(Enesim_Renderer *r,
		Enesim_Renderer **content);
EAPI void enesim_renderer_clipper_width_set(Enesim_Renderer *r,
		double width);
EAPI void enesim_renderer_clipper_width_get(Enesim_Renderer *r,
		double *width);
EAPI void enesim_renderer_clipper_height_set(Enesim_Renderer *r,
		double height);
EAPI void enesim_renderer_clipper_height_get(Enesim_Renderer *r,
		double *height);

/**
 * @}
 * @}
 */

#endif /*ENESIM_RENDERER_H_*/
