/* ENESIM - Direct Rendering Library
 * Copyright (C) 2007-2008 Jorge Luis Zapata
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

/** Flags that specify what a renderer supports */
typedef enum Enesim_Renderer_Flag
{
	ENESIM_RENDERER_FLAG_AFFINE 		= (1 << 0), /**< Affine transformation */
	ENESIM_RENDERER_FLAG_PERSPECTIVE 	= (1 << 1), /**< Perspective transformations */
	ENESIM_RENDERER_FLAG_COLORIZE 		= (1 << 2), /**< Use the renderer color directly */
	ENESIM_RENDERER_FLAG_A8 		= (1 << 3), /**< Supports A8 surfaces */
	ENESIM_RENDERER_FLAG_ARGB8888 		= (1 << 4), /**< Supports ARGB8888 surfaces */
	ENESIM_RENDERER_FLAG_ROP 		= (1 << 5), /**< Can draw directly using the raster operation */
} Enesim_Renderer_Flag;

typedef struct _Enesim_Renderer Enesim_Renderer; /**< Renderer Handler */
typedef struct _Enesim_Renderer_Descriptor Enesim_Renderer_Descriptor; /**< Renderer Descriptor Handler */

typedef void (*Enesim_Renderer_Sw_Fill)(Enesim_Renderer *r, int x, int y,
		unsigned int len, uint32_t *dst);

typedef void (*Enesim_Renderer_Delete)(Enesim_Renderer *r);
typedef void (*Enesim_Renderer_Boundings)(Enesim_Renderer *r, Eina_Rectangle *rect);
typedef void (*Enesim_Renderer_Flags)(Enesim_Renderer *r, Enesim_Renderer_Flag *flags);
typedef Eina_Bool (*Enesim_Renderer_Sw_Setup)(Enesim_Renderer *r,
		Enesim_Renderer_Sw_Fill *fill);
typedef void (*Enesim_Renderer_Sw_Cleanup)(Enesim_Renderer *r);

struct _Enesim_Renderer_Descriptor {
	Enesim_Renderer_Sw_Setup sw_setup;
	Enesim_Renderer_Sw_Cleanup sw_cleanup;
	Enesim_Renderer_Delete free;
	Enesim_Renderer_Boundings boundings;
	Enesim_Renderer_Flags flags;
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
EAPI void enesim_renderer_surface_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Eina_Rectangle *clip, int x, int y);
EAPI void enesim_renderer_surface_draw_list(Enesim_Renderer *r, Enesim_Surface *s,
		Eina_List *clips, int x, int y);
EAPI void enesim_renderer_color_set(Enesim_Renderer *r, Enesim_Color color);
EAPI void enesim_renderer_color_get(Enesim_Renderer *r, Enesim_Color *color);
EAPI void enesim_renderer_boundings(Enesim_Renderer *r, Eina_Rectangle *rect);
EAPI void enesim_renderer_translated_boundings(Enesim_Renderer *r, Eina_Rectangle *rect);
EAPI void enesim_renderer_destination_boundings(Enesim_Renderer *r, Eina_Rectangle *rect, int x, int y);
EAPI void enesim_renderer_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags);
EAPI void enesim_renderer_rop_set(Enesim_Renderer *r, Enesim_Rop rop);
EAPI void enesim_renderer_rop_get(Enesim_Renderer *r, Enesim_Rop *rop);
/**
 * @defgroup Enesim_Renderer_Shapes_Group Shapes
 * @{
 */
typedef enum _Enesim_Shape_Draw_Mode
{
	ENESIM_SHAPE_DRAW_MODE_FILL,
	ENESIM_SHAPE_DRAW_MODE_STROKE,
	ENESIM_SHAPE_DRAW_MODE_STROKE_FILL,
} Enesim_Shape_Draw_Mode;

EAPI void enesim_renderer_shape_outline_weight_set(Enesim_Renderer *r, double weight);
EAPI void enesim_renderer_shape_outline_weight_get(Enesim_Renderer *r, double *weight);
EAPI void enesim_renderer_shape_outline_color_set(Enesim_Renderer *r, Enesim_Color stroke_color);
EAPI void enesim_renderer_shape_outline_renderer_set(Enesim_Renderer *r, Enesim_Renderer *o);
EAPI void enesim_renderer_shape_fill_color_set(Enesim_Renderer *r, Enesim_Color fill_color);
EAPI void enesim_renderer_shape_fill_renderer_set(Enesim_Renderer *r, Enesim_Renderer *f);
EAPI void enesim_renderer_shape_draw_mode_set(Enesim_Renderer *r, Enesim_Shape_Draw_Mode draw_mode);
/**
 * @defgroup Enesim_Renderer_Rectangle_Group Rectangle
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_rectangle_new(void);
EAPI void enesim_renderer_rectangle_width_set(Enesim_Renderer *p, unsigned int w);
EAPI void enesim_renderer_rectangle_width_get(Enesim_Renderer *p, unsigned int *w);
EAPI void enesim_renderer_rectangle_height_set(Enesim_Renderer *p, unsigned int h);
EAPI void enesim_renderer_rectangle_height_get(Enesim_Renderer *p, unsigned int *h);
EAPI void enesim_renderer_rectangle_size_set(Enesim_Renderer *p, unsigned int w, unsigned int h);
EAPI void enesim_renderer_rectangle_size_get(Enesim_Renderer *p, unsigned int *w, unsigned int *h);
EAPI void enesim_renderer_rectangle_corner_radius_set(Enesim_Renderer *p, double radius);
EAPI void enesim_renderer_rectangle_corners_set(Enesim_Renderer *p, Eina_Bool tl, Eina_Bool tr, Eina_Bool bl, Eina_Bool br);
EAPI void enesim_renderer_rectangle_top_left_corner_set(Enesim_Renderer *r, Eina_Bool rounded);
EAPI void enesim_renderer_rectangle_top_right_corner_set(Enesim_Renderer *r, Eina_Bool rounded);
EAPI void enesim_renderer_rectangle_bottom_left_corner_set(Enesim_Renderer *r, Eina_Bool rounded);
EAPI void enesim_renderer_rectangle_bottom_right_corner_set(Enesim_Renderer *r, Eina_Bool rounded);
/**
 * @}
 * @defgroup Enesim_Renderer_Circle_Group Circle
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_circle_new(void);
EAPI void enesim_renderer_circle_x_set(Enesim_Renderer *r, double x);
EAPI void enesim_renderer_circle_y_set(Enesim_Renderer *r, double y);
EAPI void enesim_renderer_circle_center_set(Enesim_Renderer *r, double x, double y);
EAPI void enesim_renderer_circle_center_get(Enesim_Renderer *r, double *x, double *y);
EAPI void enesim_renderer_circle_radius_set(Enesim_Renderer *r, double radius);
EAPI void enesim_renderer_circle_radius_get(Enesim_Renderer *r, double *radius);
/**
 * @}
 * @defgroup Enesim_Renderer_Ellipse_Group Ellipse
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_ellipse_new(void);

EAPI void enesim_renderer_ellipse_x_set(Enesim_Renderer *p, double x);
EAPI void enesim_renderer_ellipse_y_set(Enesim_Renderer *p, double y);
EAPI void enesim_renderer_ellipse_x_radius_set(Enesim_Renderer *p, double r);
EAPI void enesim_renderer_ellipse_y_radius_set(Enesim_Renderer *p, double r);
EAPI void enesim_renderer_ellipse_center_set(Enesim_Renderer *p, double x, double y);
EAPI void enesim_renderer_ellipse_center_get(Enesim_Renderer *p, double *x, double *y);
EAPI void enesim_renderer_ellipse_radii_set(Enesim_Renderer *p, double radius_x, double radius_y);
EAPI void enesim_renderer_ellipse_radii_get(Enesim_Renderer *p, double *radius_x, double *radius_y);
/**
 * @}
 * @defgroup Enesim_Renderer_Figure_Group Figure
 * @{
 */

typedef struct _Enesim_Renderer_Figure_Polygon
{
	Eina_List *vertices;
} Enesim_Renderer_Figure_Polygon;

typedef struct _Enesim_Renderer_Figure_Vertex
{
	double x;
	double y;
} Enesim_Renderer_Figure_Vertex;

EAPI Enesim_Renderer * enesim_renderer_figure_new(void);
EAPI void enesim_renderer_figure_polygon_add(Enesim_Renderer *p);
EAPI void enesim_renderer_figure_polygon_set(Enesim_Renderer *p, Eina_List *polygons);
EAPI void enesim_renderer_figure_polygon_vertex_add(Enesim_Renderer *p, double x, double y);
EAPI void enesim_renderer_figure_clear(Enesim_Renderer *p);
/**
 * @}
 * @defgroup Enesim_Renderer_Path_Group Path
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_path_new(void);
EAPI void enesim_renderer_path_move_to(Enesim_Renderer *p, float x, float y);
EAPI void enesim_renderer_path_line_to(Enesim_Renderer *p, float x, float y);
EAPI void enesim_renderer_path_squadratic_to(Enesim_Renderer *p, float x, float y);
EAPI void enesim_renderer_path_quadratic_to(Enesim_Renderer *p, float ctrl_x,
		float ctrl_y, float x, float y);
EAPI void enesim_renderer_path_cubic_to(Enesim_Renderer *p, float ctrl_x0,
		float ctrl_y0, float ctrl_x, float ctrl_y, float x,
		float y);
EAPI void enesim_renderer_path_scubic_to(Enesim_Renderer *p, float ctrl_x, float ctrl_y,
		float x, float y);
EAPI void enesim_renderer_path_clear(Enesim_Renderer *p);
/**
 * @}
 * @}
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
EAPI void enesim_renderer_hswitch_step_set(Enesim_Renderer *r, float step);
/**
 * @}
 * @defgroup Enesim_Renderer_Image_Group Image
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_image_new(void);
EAPI void enesim_renderer_image_x_set(Enesim_Renderer *r, int x);
EAPI void enesim_renderer_image_y_set(Enesim_Renderer *r, int y);
EAPI void enesim_renderer_image_w_set(Enesim_Renderer *r, int w);
EAPI void enesim_renderer_image_h_set(Enesim_Renderer *r, int h);
EAPI void enesim_renderer_image_src_set(Enesim_Renderer *r, Enesim_Surface *src);
/**
 * @}
 * @defgroup Enesim_Renderer_Checker_Group Checker
 * @image html renderer_checker.png
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_checker_new(void);
EAPI void enesim_renderer_checker_even_color_set(Enesim_Renderer *r, Enesim_Color color);
EAPI void enesim_renderer_checker_even_color_get(Enesim_Renderer *r, Enesim_Color *color);
EAPI void enesim_renderer_checker_odd_color_set(Enesim_Renderer *r, Enesim_Color color);
EAPI void enesim_renderer_checker_odd_color_get(Enesim_Renderer *r, Enesim_Color *color);
EAPI void enesim_renderer_checker_width_set(Enesim_Renderer *r, int width);
EAPI void enesim_renderer_checker_width_get(Enesim_Renderer *r, int *width);
EAPI void enesim_renderer_checker_height_set(Enesim_Renderer *r, int height);
EAPI void enesim_renderer_checker_height_get(Enesim_Renderer *r, int *height);
/**
 * @}
 * @defgroup Enesim_Renderer_Transition_Group Transition
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_transition_new(void);
EAPI void enesim_renderer_transition_value_set(Enesim_Renderer *r, float interp_value);
EAPI void enesim_renderer_transition_source_set(Enesim_Renderer *r, Enesim_Renderer *r0);
EAPI void enesim_renderer_transition_target_set(Enesim_Renderer *r, Enesim_Renderer *r1);
EAPI void enesim_renderer_transition_offset_set(Enesim_Renderer *r, int x, int y);
/**
 * @}
 * @defgroup Enesim_Renderer_Stripes_Group Stripes
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_stripes_new(void);
EAPI void enesim_renderer_stripes_even_color_set(Enesim_Renderer *r, Enesim_Color color);
EAPI void enesim_renderer_stripes_even_color_get(Enesim_Renderer *r, Enesim_Color *color);
EAPI void enesim_renderer_stripes_even_thickness_set(Enesim_Renderer *r, double thickness);
EAPI void enesim_renderer_stripes_even_thickness_get(Enesim_Renderer *r, double *thickness);
EAPI void enesim_renderer_stripes_odd_color_set(Enesim_Renderer *r, Enesim_Color color);
EAPI void enesim_renderer_stripes_odd_color_get(Enesim_Renderer *r, Enesim_Color *color);
EAPI void enesim_renderer_stripes_odd_thickness_set(Enesim_Renderer *r,	double thickness);
EAPI void enesim_renderer_stripes_odd_thickness_get(Enesim_Renderer *r, double *thickness);
/**
 * @}
 * @defgroup Enesim_Renderer_Dispmap_Group Displacement Map
 * @{
 */

typedef enum _Enesim_Channel
{
	ENESIM_CHANNEL_RED,
	ENESIM_CHANNEL_GREEN,
	ENESIM_CHANNEL_BLUE,
	ENESIM_CHANNEL_ALPHA,
	ENESIM_CHANNELS,
} Enesim_Channel;

EAPI Enesim_Renderer * enesim_renderer_dispmap_new(void);
EAPI void enesim_renderer_dispmap_map_set(Enesim_Renderer *r, Enesim_Surface *map);
EAPI void enesim_renderer_dispmap_src_set(Enesim_Renderer *r, Enesim_Surface *src);
EAPI void enesim_renderer_dispmap_factor_set(Enesim_Renderer *r, double factor);
EAPI void enesim_renderer_dispmap_x_channel_set(Enesim_Renderer *r, Enesim_Channel channel);
EAPI void enesim_renderer_dispmap_y_channel_set(Enesim_Renderer *r, Enesim_Channel channel);
/**
 * @}
 * @defgroup Enesim_Renderer_Raddist_Group Radial Distortion
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_raddist_new(void);
EAPI void enesim_renderer_raddist_radius_set(Enesim_Renderer *r, double radius);
EAPI void enesim_renderer_raddist_factor_set(Enesim_Renderer *r, double factor);
EAPI void enesim_renderer_raddist_src_set(Enesim_Renderer *r, Enesim_Surface *src);
EAPI void enesim_renderer_raddist_x_set(Enesim_Renderer *r, int ox);
EAPI void enesim_renderer_raddist_y_set(Enesim_Renderer *r, int oy);
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
EAPI void enesim_renderer_perlin_persistence_set(Enesim_Renderer *r, float persistence);
EAPI void enesim_renderer_perlin_xfrequency_set(Enesim_Renderer *r, float freq);
EAPI void enesim_renderer_perlin_yfrequency_set(Enesim_Renderer *r, float freq);


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
} Enesim_Renderer_Gradient_Mode;

typedef struct _Enesim_Renderer_Gradient_Stop
{
	Enesim_Color color;
	double pos;
} Enesim_Renderer_Gradient_Stop;

EAPI void enesim_renderer_gradient_stop_add(Enesim_Renderer *r, Enesim_Color c,
		double pos);
EAPI void enesim_renderer_gradient_clear(Enesim_Renderer *r);
EAPI void enesim_renderer_gradient_stop_set(Enesim_Renderer *r,
		Eina_List *list);
EAPI void enesim_renderer_gradient_mode_set(Enesim_Renderer *r, Enesim_Renderer_Gradient_Mode mode);
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
 * @}
 * @defgroup Enesim_Renderer_Compound_Group Compound
 * @{
 */

EAPI Enesim_Renderer * enesim_renderer_compound_new(void);
EAPI void enesim_renderer_compound_layer_add(Enesim_Renderer *r,
		Enesim_Renderer *rend);
EAPI void enesim_renderer_compound_clear(Enesim_Renderer *r);
EAPI void enesim_renderer_compound_layer_set(Enesim_Renderer *r,
		Eina_List *list);
/**
 * @}
 * @defgroup Enesim_Renderer_Grid_Group Grid
 * @{
 */
EAPI Enesim_Renderer * enesim_renderer_grid_new(void);
EAPI void enesim_renderer_grid_inside_width_set(Enesim_Renderer *r, unsigned int width);
EAPI void enesim_renderer_grid_inside_width_get(Enesim_Renderer *r, unsigned int *width);
EAPI void enesim_renderer_grid_inside_height_set(Enesim_Renderer *r, unsigned int height);
EAPI void enesim_renderer_grid_inside_height_get(Enesim_Renderer *r, unsigned int *height);
EAPI void enesim_renderer_grid_inside_color_set(Enesim_Renderer *r, Enesim_Color color);
EAPI void enesim_renderer_grid_inside_color_get(Enesim_Renderer *r, Enesim_Color *color);
EAPI void enesim_renderer_grid_border_hthickness_set(Enesim_Renderer *r, unsigned int hthickness);
EAPI void enesim_renderer_grid_border_hthickness_get(Enesim_Renderer *r, unsigned int *htickness);
EAPI void enesim_renderer_grid_border_vthickness_set(Enesim_Renderer *r, unsigned int vthickness);
EAPI void enesim_renderer_grid_border_vthickness_get(Enesim_Renderer *r, unsigned int *vthickness);
EAPI void enesim_renderer_grid_border_color_set(Enesim_Renderer *r, Enesim_Color color);
EAPI void enesim_renderer_grid_border_color_get(Enesim_Renderer *r, Enesim_Color *color);

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
 * @}
 */

#endif /*ENESIM_RENDERER_H_*/
