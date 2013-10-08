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

#ifndef ENESIM_MAIN_H_
#define ENESIM_MAIN_H_

/**
 * @defgroup Enesim_Main_Group Main
 * @brief Common definitions
 * @{
 */

/**
 * Angle enumeration
 */
typedef enum _Enesim_Angle
{
	ENESIM_ANGLE_0, /**< 0 degrees angle */
	ENESIM_ANGLE_90, /**< 90 degrees angle CW */
	ENESIM_ANGLE_180, /**< 180 degrees angle CW */
	ENESIM_ANGLE_270, /**< 270 degrees angle CW */
	ENESIM_ANGLES,
} Enesim_Angle;

/**
 * Raster operations at pixel level
 */
typedef enum _Enesim_Rop
{
	ENESIM_ROP_BLEND, /**< D = S + D(1 - Sa) */
	ENESIM_ROP_FILL, /**< D = S */
	ENESIM_ROPS
} Enesim_Rop;

/**
 * Quality values
 */
typedef enum _Enesim_Quality
{
	ENESIM_QUALITY_BEST, /**< Best quality */
	ENESIM_QUALITY_GOOD, /**< Good quality */
	ENESIM_QUALITY_FAST, /**< Lower quality, fastest */
	ENESIM_QUALITIES
} Enesim_Quality;

/**
 * Priorities
 */
typedef enum _Enesim_Priority
{
	ENESIM_PRIORITY_NONE = 0,
	ENESIM_PRIORITY_MARGINAL = 64,
	ENESIM_PRIORITY_SECONDARY = 128,
	ENESIM_PRIORITY_PRIMARY = 256,
} Enesim_Priority;

/**
 * RGBA Channels
 */
typedef enum _Enesim_Channel
{
	ENESIM_CHANNEL_RED, /**< Red channel */
	ENESIM_CHANNEL_GREEN, /**< Green channel */
	ENESIM_CHANNEL_BLUE, /**< Blue channel */
	ENESIM_CHANNEL_ALPHA, /**< Alpha channel */
	ENESIM_CHANNELS, /**< The number of channels */
} Enesim_Channel;

/**
 * Surface formats
 */
typedef enum _Enesim_Format
{
	ENESIM_FORMAT_NONE,
	ENESIM_FORMAT_ARGB8888,  /**< argb32 */
	ENESIM_FORMAT_A8, /**< a8 */
	ENESIM_FORMATS /**< The number of formats */
} Enesim_Format;

/**
 * Alpha hints
 */
typedef enum _Enesim_Alpha_Hint
{
	ENESIM_ALPHA_HINT_NORMAL, /**< Alpha can be in the whole range */
	ENESIM_ALPHA_HINT_SPARSE, /**< Alpha is sparsed only, that is or 0 or 255 */
	ENESIM_ALPHA_HINT_OPAQUE, /**< Alpha is always 0 */
	ENESIM_ALPHA_HINTS /**< The number of alpha hints */
} Enesim_Alpha_Hint;

/**
 * Repeat modes
 */
typedef enum _Enesim_Repeat_Mode
{
	ENESIM_RESTRICT,
	ENESIM_PAD,
	ENESIM_REFLECT,
	ENESIM_REPEAT,
	ENESIM_REPEAT_MODES,
} Enesim_Repeat_Mode;

/**
 * The backend used for drawing
 */
typedef enum _Enesim_Backend
{
	ENESIM_BACKEND_INVALID, /**< Invalid backend */
	ENESIM_BACKEND_SOFTWARE, /**< Software based backend */
	ENESIM_BACKEND_OPENCL, /**< OpenCL based backend (not working) */
	ENESIM_BACKEND_OPENGL, /**< OpenGL based backend (experimental) */
	ENESIM_BACKENDS
} Enesim_Backend;

EAPI int enesim_init(void);
EAPI int enesim_shutdown(void);
EAPI void enesim_version_get(unsigned int *major, unsigned int *minor, unsigned int *micro);

EAPI const char * enesim_format_name_get(Enesim_Format f);
EAPI size_t enesim_format_size_get(Enesim_Format f, uint32_t w, uint32_t h);
EAPI size_t enesim_format_stride_get(Enesim_Format fmt, uint32_t w);

/** @} */

#endif /*ENESIM_MAIN_H_*/
