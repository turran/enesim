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
#ifndef ENESIM_RENDERER_CONVOLVE_H_
#define ENESIM_RENDERER_CONVOLVE_H_

/**
 * @file
 * @ender_group{Enesim_Renderer_Convolve}
 */

/**
 * @ingroup Enesim_Renderer_Convolve
 * @{
 */

/** The convolve channel enumeration */
typedef enum _Enesim_Convolve_Channel
{
	ENESIM_CONVOLVE_CHANNEL_ARGB, /**< Use all a,r,g,b channels */
	ENESIM_CONVOLVE_CHANNEL_XRGB, /**< Use only the r,g,b channels */
	ENESIM_CONVOLVE_CHANNELS, /**< Total number of channels */
} Enesim_Convolve_Channel;

/**
 * @}
 * @defgroup Enesim_Renderer_Convolve Convolution Filter
 * @brief Convolve filter renderer @ender_inherits{Enesim_Renderer}
 * @ingroup Enesim_Renderer
 * @{
 */

EAPI Enesim_Renderer * enesim_renderer_convolve_new(void);

EAPI void enesim_renderer_convolve_source_surface_set(Enesim_Renderer *r, Enesim_Surface *src);
EAPI Enesim_Surface * enesim_renderer_convolve_source_surface_get(Enesim_Renderer *r);

EAPI void enesim_renderer_convolve_source_renderer_set(Enesim_Renderer *r, Enesim_Renderer *sr);
EAPI Enesim_Renderer * enesim_renderer_convolve_source_renderer_get(Enesim_Renderer *r);

EAPI void enesim_renderer_convolve_matrix_size_set(Enesim_Renderer *r, int nrows, int ncolumns);
EAPI void enesim_renderer_convolve_matrix_size_get(Enesim_Renderer *r, int *nrows, int *ncolumns);

EAPI void enesim_renderer_convolve_matrix_value_set(Enesim_Renderer *r, double val, int i, int j);
EAPI double enesim_renderer_convolve_matrix_value_get(Enesim_Renderer *r, int i, int j);

EAPI void enesim_renderer_convolve_center_x_set(Enesim_Renderer *r, int x);
EAPI int enesim_renderer_convolve_center_x_get(Enesim_Renderer *r);

EAPI void enesim_renderer_convolve_center_y_set(Enesim_Renderer *r, int x);
EAPI int enesim_renderer_convolve_center_y_get(Enesim_Renderer *r);

EAPI void enesim_renderer_convolve_divisor_set(Enesim_Renderer *r, double d);
EAPI double enesim_renderer_convolve_divisor_get(Enesim_Renderer *r);

EAPI void enesim_renderer_convolve_channel_set(Enesim_Renderer *r, Enesim_Convolve_Channel channel);
EAPI Enesim_Convolve_Channel enesim_renderer_convolve_channel_get(Enesim_Renderer *r);

/**
 * @}
 */

#endif
