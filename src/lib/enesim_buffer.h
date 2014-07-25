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
#ifndef ENESIM_BUFFER_H_
#define ENESIM_BUFFER_H_

/**
 * @file
 * @ender_group{Enesim_Buffer_Format}
 * @ender_group{Enesim_Buffer}
 */

/**
 * @defgroup Enesim_Buffer_Format Buffer format
 * @{
 */

/*
 * ENESIM_BUFFER_FORMAT_A8
 * +---------------+----------------+
 * |     Alpha     |      Alpha     |
 * +---------------+----------------+
 *         8                8
 * <------P0------>.<------P1------>.
 *
 * ENESIM_BUFFER_FORMAT_b1A3
 * +-------+-------+--------+-------+
 * | Blink | Alpha |  Blink | Alpha |
 * +-------+-------+--------+-------+
 *     1       3        1        3
 * <------P0------>.<------P1------>.
 */

/**
 * Enumeration of the different buffer formats
 */ 
typedef enum _Enesim_Buffer_Format
{
	ENESIM_BUFFER_FORMAT_RGB565, /**< 16bpp RGB 565 */
	ENESIM_BUFFER_FORMAT_ARGB8888, /**< 32bpp ARGB 8888 */
	ENESIM_BUFFER_FORMAT_ARGB8888_PRE, /**< 32bpp ARGB premultiplied 8888*/
	ENESIM_BUFFER_FORMAT_XRGB8888, /**< 32bpp RGB 888 */
	ENESIM_BUFFER_FORMAT_RGB888, /**< 24bpp RGB 888 */
	ENESIM_BUFFER_FORMAT_BGR888, /**< 24bpp BGR 888 */
	ENESIM_BUFFER_FORMAT_A8, /**< 8bpp A 8 */
	ENESIM_BUFFER_FORMAT_GRAY, /**< 8bpp Grayscale */
	ENESIM_BUFFER_FORMAT_CMYK, /**< CMYK */
	ENESIM_BUFFER_FORMAT_CMYK_ADOBE, /**< Adobe CMYK */
	ENESIM_BUFFER_FORMATS /**< Total number of buffer formats */
} Enesim_Buffer_Format;

/**
 * Definition of a 24 bits per pixel format
 */
typedef struct _Enesim_Buffer_24bpp
{
	uint8_t *plane0; /**< The buffer data */
	int plane0_stride; /**< The stride of the buffer */
} Enesim_Buffer_24bpp;

/**
 * Definition of a 32 bits per pixel format
 */
typedef struct _Enesim_Buffer_32bpp
{
	uint32_t *plane0; /**< The buffer data */
	int plane0_stride; /**< The stride of the buffer */
} Enesim_Buffer_32bpp;

typedef struct _Enesim_Buffer_Rgb565
{
	uint16_t *plane0; /**< The buffer data */
	int plane0_stride; /**< The stride of the buffer */
} Enesim_Buffer_Rgb565;

/**
 * Definition of a 8 bits alpha only format
 */
typedef struct _Enesim_Buffer_A8
{
	uint8_t *plane0; /**< The buffer data */
	int plane0_stride; /**< The stride of the buffer */
} Enesim_Buffer_A8;

typedef Enesim_Buffer_32bpp Enesim_Buffer_Argb8888; /**< The definition associated with @ref ENESIM_BUFFER_FORMAT_ARGB8888 */
typedef Enesim_Buffer_32bpp Enesim_Buffer_Argb8888_Pre; /**< The definition associated with @ref ENESIM_BUFFER_FORMAT_ARGB8888_PRE */
typedef Enesim_Buffer_32bpp Enesim_Buffer_Xrgb8888; /**< The definition associated with @ref ENESIM_BUFFER_FORMAT_XRGB8888 */
typedef Enesim_Buffer_24bpp Enesim_Buffer_Rgb888; /**< The definition associated with @ref ENESIM_BUFFER_FORMAT_RGB888 */
typedef Enesim_Buffer_24bpp Enesim_Buffer_Bgr888; /**< The definition associated with @ref ENESIM_BUFFER_FORMAT_BGR888 */
typedef Enesim_Buffer_24bpp Enesim_Buffer_Cmyk; /**< The definition associated with @ref ENESIM_BUFFER_FORMAT_CMYK */

/**< The software data definition */
typedef union _Enesim_Buffer_Sw_Data
{
	Enesim_Buffer_Argb8888 argb8888; /**< The @ref ENESIM_BUFFER_FORMAT_ARGB8888 value */
	Enesim_Buffer_Argb8888_Pre argb8888_pre; /**< The @ref ENESIM_BUFFER_FORMAT_ARGB8888_PRE value */
	Enesim_Buffer_Xrgb8888 xrgb8888; /**< The @ref ENESIM_BUFFER_FORMAT_XRGB8888 value */
	Enesim_Buffer_Rgb565 rgb565; /**< The @ref ENESIM_BUFFER_FORMAT_RGB565 value */
	Enesim_Buffer_A8 a8; /**< The @ref ENESIM_BUFFER_FORMAT_A8 value */
	Enesim_Buffer_Rgb888 rgb888; /**< The @ref ENESIM_BUFFER_FORMAT_RGB888 value */
	Enesim_Buffer_Bgr888 bgr888; /**< The @ref ENESIM_BUFFER_FORMAT_BGR888 value */
	Enesim_Buffer_Cmyk cmyk; /**< The @ref ENESIM_BUFFER_FORMAT_CMYK values */
} Enesim_Buffer_Sw_Data;


/**
 * @}
 * @defgroup Enesim_Buffer Buffer
 * @brief Target device pixel data holder
 * @{
 */

typedef struct _Enesim_Buffer Enesim_Buffer; /**< Buffer Handle */

/**
 * @brief Free function callback
 * @param data The buffer data provided by the user
 * @param user_data The user private data
 * This function is used to free the data provided for the creation 
 * @see enesim_buffer_new_pool_and_data_from()
 * @see enesim_buffer_new_data_from()
 * @see enesim_surface_new_data_from()
 */
typedef void (*Enesim_Buffer_Free)(void *data, void *user_data);

EAPI Enesim_Buffer * enesim_buffer_new(Enesim_Buffer_Format f, uint32_t w, uint32_t h);
EAPI Enesim_Buffer * enesim_buffer_new_data_from(Enesim_Buffer_Format f,
		uint32_t w, uint32_t h, Eina_Bool copy,
		Enesim_Buffer_Sw_Data *data, Enesim_Buffer_Free free_func,
		void *free_func_data);
EAPI Enesim_Buffer * enesim_buffer_new_pool_from(Enesim_Buffer_Format f, uint32_t w, uint32_t h, Enesim_Pool *p);
EAPI Enesim_Buffer * enesim_buffer_new_pool_and_data_from(Enesim_Buffer_Format f,
		uint32_t w, uint32_t h, Enesim_Pool *p, Eina_Bool copy,
		Enesim_Buffer_Sw_Data *data, Enesim_Buffer_Free free_func,
		void *free_func_data);
EAPI Enesim_Buffer * enesim_buffer_ref(Enesim_Buffer *b);
EAPI void enesim_buffer_unref(Enesim_Buffer *b);

EAPI void enesim_buffer_size_get(const Enesim_Buffer *b, int *w, int *h);
EAPI Enesim_Buffer_Format enesim_buffer_format_get(const Enesim_Buffer *b);
EAPI Enesim_Backend enesim_buffer_backend_get(const Enesim_Buffer *b);
EAPI Enesim_Pool * enesim_buffer_pool_get(Enesim_Buffer *b);

EAPI void enesim_buffer_private_set(Enesim_Buffer *b, void *data);
EAPI void * enesim_buffer_private_get(Enesim_Buffer *b);

EAPI Eina_Bool enesim_buffer_sw_data_get(const Enesim_Buffer *b, Enesim_Buffer_Sw_Data *data);
EAPI Eina_Bool enesim_buffer_map(Enesim_Buffer *b, Enesim_Buffer_Sw_Data *data);
EAPI Eina_Bool enesim_buffer_unmap(Enesim_Buffer *b, Enesim_Buffer_Sw_Data *data, Eina_Bool written);

EAPI size_t enesim_buffer_format_size_get(Enesim_Buffer_Format fmt, uint32_t w, uint32_t h);

EAPI Eina_Bool enesim_buffer_format_rgb_components_from(
		Enesim_Buffer_Format *fmt, int depth,
		uint8_t aoffset, uint8_t alen,
		uint8_t roffset, uint8_t rlen,
		uint8_t goffset, uint8_t glen,
		uint8_t boffset, uint8_t blen, Eina_Bool premul);
EAPI Eina_Bool enesim_buffer_format_rgb_components_to(Enesim_Buffer_Format fmt,
		uint8_t *aoffset, uint8_t *alen,
		uint8_t *roffset, uint8_t *rlen,
		uint8_t *goffset, uint8_t *glen,
		uint8_t *boffset, uint8_t *blen, Eina_Bool *premul);
EAPI uint8_t enesim_buffer_format_rgb_depth_get(Enesim_Buffer_Format fmt);

EAPI void enesim_buffer_lock(Enesim_Buffer *b, Eina_Bool write);
EAPI void enesim_buffer_unlock(Enesim_Buffer *b);

/** @} */ //End of Enesim_Buffer


#endif /*ENESIM_BUFFER_H_*/
