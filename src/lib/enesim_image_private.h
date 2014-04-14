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
#ifndef ENESIM_IMAGE_PRIVATE_H_
#define ENESIM_IMAGE_PRIVATE_H_

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#ifdef _WIN32
# include <winsock2.h>
#endif

#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>

#ifdef WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <winsock2.h>
# undef WIN32_LEAN_AND_MEAN
#else
# include <pthread.h>
#endif

int enesim_image_init(void);
int enesim_image_shutdown(void);

struct _Enesim_Image_Provider
{
	const char *mime;
	Enesim_Priority priority;
	Enesim_Image_Provider_Descriptor *d;
};

Enesim_Image_Provider * enesim_image_load_provider_get(Enesim_Stream *data, const char *mime);
Enesim_Image_Provider * enesim_image_load_info_provider_get(Enesim_Stream *data, const char *mime);
Enesim_Image_Provider * enesim_image_save_provider_get(Enesim_Buffer *b, const char *mime);
Enesim_Image_Provider * enesim_image_save_provider_get(Enesim_Buffer *b, const char *mime);

#if BUILD_STATIC_MODULE_PNG
Eina_Bool enesim_image_png_provider_init(void);
void enesim_image_png_provider_shutdown(void);
#endif

#if BUILD_STATIC_MODULE_JPG
Eina_Bool enesim_image_jpg_provider_init(void);
void enesim_image_jpg_provider_shutdown(void);
#endif

#if BUILD_STATIC_MODULE_RAW
Eina_Bool enesim_image_raw_provider_init(void);
void enesim_image_raw_provider_shutdown(void);
#endif

#endif /*ENESIM_IMAGE_PRIVATE_H_*/
