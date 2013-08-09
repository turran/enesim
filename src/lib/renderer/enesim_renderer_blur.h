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
#ifndef ENESIM_RENDERER_BLUR_H_
#define ENESIM_RENDERER_BLUR_H_

/**
 * @defgroup Enesim_Renderer_Blur_Group Blur Filter
 * @brief Blur filter renderer
 * @ingroup Enesim_Renderer_Group
 * @{
 */

typedef enum _Enesim_Blur_Channel
{
	ENESIM_BLUR_CHANNEL_COLOR,
	ENESIM_BLUR_CHANNEL_ALPHA,
	ENESIM_BLUR_CHANNELS,
} Enesim_Blur_Channel;

EAPI Enesim_Renderer * enesim_renderer_blur_new(void);
EAPI void enesim_renderer_blur_src_set(Enesim_Renderer *r, Enesim_Surface *src);
EAPI Enesim_Surface * enesim_renderer_blur_src_get(Enesim_Renderer *r);

EAPI void enesim_renderer_blur_renderer_set(Enesim_Renderer *r, Enesim_Renderer *ren);
EAPI Enesim_Renderer * enesim_renderer_blur_renderer_get(Enesim_Renderer *r);

EAPI void enesim_renderer_blur_channel_set(Enesim_Renderer *r, Enesim_Blur_Channel channel);
EAPI Enesim_Blur_Channel enesim_renderer_blur_channel_get(Enesim_Renderer *r);

EAPI void enesim_renderer_blur_radius_x_set(Enesim_Renderer *r, double rx);
EAPI double enesim_renderer_blur_radius_x_get(Enesim_Renderer *r);

EAPI void enesim_renderer_blur_radius_y_set(Enesim_Renderer *r, double ry);
EAPI double enesim_renderer_blur_radius_y_get(Enesim_Renderer *r);

/**
 * @}
 */

#endif
