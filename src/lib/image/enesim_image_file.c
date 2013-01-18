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
#include "enesim_private.h"

#include "enesim_main.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_image.h"
#include "enesim_image_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Enesim_Image_File_Data
{
	Enesim_Image_Callback cb;
	void *user_data;
	Enesim_Image_Data *data;
} Enesim_Image_File_Data;

static void _enesim_image_file_cb(Enesim_Surface *s, void *user_data, int error)
{
	Enesim_Image_File_Data *fdata = user_data;

	fdata->cb(s, fdata->user_data, error);
	enesim_image_data_free(fdata->data);
}

static const char * _enesim_image_file_get_extension(const char *file)
{
	char *tmp;

	tmp = strrchr(file, '.');
	if (!tmp) return NULL;
	return tmp + 1;
}

static Eina_Bool _file_save_data_get(const char *file, Enesim_Image_Data **data, const char **mime)
{
	Enesim_Image_Data *d;
	const char *m;
	const char *ext;

	ext = _enesim_image_file_get_extension(file);
	if (!ext) return EINA_FALSE;

	d = enesim_image_data_file_new(file, "wb");
	if (!d) return EINA_FALSE;

	m = enesim_image_mime_extension_from(ext);
	if (!m)
	{
		enesim_image_data_free(d);
		return EINA_FALSE;
	}
	*mime = m;
	*data = d;

	return EINA_TRUE;
}

static Eina_Bool _file_load_data_get(const char *file, Enesim_Image_Data **data, const char **mime)
{
	Enesim_Image_Data *d;
	const char *m;

	d = enesim_image_data_file_new(file, "rb");
	if (!d) return EINA_FALSE;

	m = enesim_image_mime_data_from(d);
	if (!m)
	{
		enesim_image_data_free(d);
		return EINA_FALSE;
	}
	enesim_image_data_reset(d);
	*mime = m;
	*data = d;

	return EINA_TRUE;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Loads information about an image file
 *
 * @param file The image file to load
 * @param w The image width
 * @param h The image height
 * @param sfmt The image original format
 */
EAPI Eina_Bool enesim_image_file_info_load(const char *file, int *w, int *h, Enesim_Buffer_Format *sfmt)
{
	Enesim_Image_Data *data;
	Eina_Bool ret;
	const char *mime;

	if (!_file_load_data_get(file, &data, &mime))
		return EINA_FALSE;
	ret = enesim_image_info_load(data, mime, w, h, sfmt);
	enesim_image_data_free(data);
	return ret;
}
/**
 * Load an image synchronously
 *
 * @param file The image file to load
 * @param s The surface to write the image pixels to. It must not be NULL.
 * @param f The desired format the image should be converted to
 * @param mpool The mempool that will create the surface in case the surface
 * reference is NULL
 * @param options Any option the emage provider might require
 * @return EINA_TRUE in case the image was loaded correctly. EINA_FALSE if not
 */
EAPI Eina_Bool enesim_image_file_load(const char *file, Enesim_Surface **s,
		Enesim_Format f, Enesim_Pool *mpool, const char *options)
{
	Enesim_Image_Data *data;
	Eina_Bool ret;
	const char *mime;

	if (!_file_load_data_get(file, &data, &mime))
		return EINA_FALSE;
	ret = enesim_image_load(data, mime, s, f, mpool, options);
	enesim_image_data_free(data);
	return ret;
}
/**
 * Load an image file asynchronously
 *
 * @param file The image file to load
 * @param s The surface to write the image pixels to. It must not be NULL.
 * @param f The desired format the image should be converted to
 * @param mpool The mempool that will create the surface in case the surface
 * reference is NULL
 * @param cb The function that will get called once the load is done
 * @param data User provided data
 * @param options Any option the emage provider might require
 */
EAPI void enesim_image_file_load_async(const char *file, Enesim_Surface *s,
		Enesim_Format f, Enesim_Pool *mpool,
		Enesim_Image_Callback cb, void *user_data, const char *options)
{
	Enesim_Image_Data *data;
	Enesim_Image_File_Data fdata;
	const char *mime;

	if (!_file_load_data_get(file, &data, &mime))
	{
		cb(NULL, user_data, ENESIM_IMAGE_ERROR_PROVIDER);
		return;
	}
	fdata.cb = cb;
	fdata.user_data = user_data;
	fdata.data = data;

	enesim_image_load_async(data, mime, s, f, mpool, _enesim_image_file_cb, &fdata, options);
}
/**
 * Save an image file synchronously
 *
 * @param file The image file to save
 * @param s The surface to read the image pixels from. It must not be NULL.
 * @param options Any option the emage provider might require
 * @return EINA_TRUE in case the image was saved correctly. EINA_FALSE if not
 */
EAPI Eina_Bool enesim_image_file_save(const char *file, Enesim_Surface *s, const char *options)
{
	Enesim_Image_Data *data;
	Eina_Bool ret;
	const char *mime;

	if (!_file_save_data_get(file, &data, &mime))
		return EINA_FALSE;
	ret = enesim_image_save(data, mime, s, options);
	enesim_image_data_free(data);
	return ret;
}
/**
 * Save an image file asynchronously
 *
 * @param file The image file to save
 * @param s The surface to read the image pixels from. It must not be NULL.
 * @param cb The function that will get called once the save is done
 * @param data User provided data
 * @param options Any option the emage provider might require
 *
 */
EAPI void enesim_image_file_save_async(const char *file, Enesim_Surface *s, Enesim_Image_Callback cb,
		void *user_data, const char *options)
{
	Enesim_Image_Data *data;
	Enesim_Image_File_Data fdata;
	const char *mime;

	if (!_file_save_data_get(file, &data, &mime))
	{
		cb(NULL, user_data, ENESIM_IMAGE_ERROR_PROVIDER);
		return;
	}
	fdata.cb = cb;
	fdata.user_data = user_data;
	fdata.data = data;

	enesim_image_save_async(data, mime, s, _enesim_image_file_cb, &fdata, options);
}
