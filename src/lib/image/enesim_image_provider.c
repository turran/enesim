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
#include "enesim_private.h"

#include "enesim_main.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_renderer.h"
#include "enesim_stream.h"
#include "enesim_image.h"
#include "enesim_renderer_importer.h"
#include "enesim_image_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
static Eina_Bool _provider_info_get(Enesim_Image_Provider *p, Enesim_Stream *data,
		int *w, int *h, Enesim_Buffer_Format *sfmt, void *options,
		Eina_Error *err)
{
	Enesim_Buffer_Format pfmt;
	Eina_Bool ret;
	int pw, ph;

	/* sanitize the values */
	if (w) *w = 0;
	if (h) *h = 0;
	if (sfmt) *sfmt = ENESIM_BUFFER_FORMATS;

	/* get the info from the image */
	if (!p->d->info_get)
	{
		if (err) *err = ENESIM_IMAGE_ERROR_PROVIDER;
		return EINA_FALSE;
	}

	ret = p->d->info_get(data, &pw, &ph, &pfmt, options, err);
	if (ret)
	{
		if (w) *w = pw;
		if (h) *h = ph;
		if (sfmt) *sfmt = pfmt;
	}

	return ret;
}

static Eina_Bool _provider_options_parse(Enesim_Image_Provider *p, const char *options,
		void **options_data)
{
	if (!options)
		return EINA_TRUE;

	if (!p->d->options_parse)
		return EINA_TRUE;

	*options_data = p->d->options_parse(options);
	return EINA_TRUE;
}

static void _provider_options_free(Enesim_Image_Provider *p, void *options)
{
	if (p->d->options_free)
		p->d->options_free(options);
}

static Eina_Bool _provider_data_load(Enesim_Image_Provider *p,
		Enesim_Stream *data, Enesim_Buffer **b, Enesim_Pool *mpool,
		void *options, Eina_Error *err)
{
	Enesim_Buffer_Format cfmt;
	Enesim_Buffer *bb = *b;
	Eina_Bool owned = EINA_FALSE;
	Eina_Error error;
	int w, h;

	if (!_provider_info_get(p, data, &w, &h, &cfmt, options, &error))
	{
		goto info_err;
	}
	if (!bb)
	{
		/* create a new buffer in case the user does not provided
		 * one
		 */
		bb = enesim_buffer_new_pool_from(cfmt, w, h, mpool);
		if (!bb)
		{
			error = ENESIM_IMAGE_ERROR_ALLOCATOR;
			goto surface_err;
		}
		owned = EINA_TRUE;
	}
	else
	{
		Enesim_Buffer_Format fmt;
		int bw, bh;
		/* otherwise check that the provided buffer is ok */
		fmt = enesim_buffer_format_get(bb);
		if (cfmt != fmt)
		{
			error = ENESIM_IMAGE_ERROR_FORMAT;
			goto surface_err;
		}
		enesim_buffer_size_get(bb, &bw, &bh);
		if (bw != w || bh != h)
		{
			error = ENESIM_IMAGE_ERROR_SIZE;
			goto surface_err;
		}
	}

	/* load the data */
	enesim_stream_reset(data);
	if (!p->d->load(data, bb, options, &error))
	{
		goto load_err;
	}

	*b = bb;
	return EINA_TRUE;

load_err:
	if (owned)
		enesim_buffer_unref(bb);
surface_err:
info_err:
	if (err) *err = error;
	return EINA_FALSE;
}

static Eina_Bool _provider_data_save(Enesim_Image_Provider *p, Enesim_Stream *data,
		Enesim_Buffer *b, void *options, Eina_Error *err)
{
	/* save the data */
	if (!p->d->save)
	{
		*err = ENESIM_IMAGE_ERROR_PROVIDER;
		return EINA_FALSE;
	}

	if (p->d->save(data, b, options, err) == EINA_FALSE)
	{
		*err = ENESIM_IMAGE_ERROR_SAVING;
		return EINA_FALSE;
	}

	return EINA_TRUE;
}
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Gets the information of an image
 * @param[in] thiz The image provider to use
 * @param[in] data The image data to load the info from
 * @param[out] w The width of the image
 * @param[out] h The height of the image
 * @param[out] sfmt The format of the image
 * @param[out] err The error in case the info loading fails
 * @return EINA_TRUE if succeeded, EINA_FALSE otherwise.
 */
EAPI Eina_Bool enesim_image_provider_info_get(Enesim_Image_Provider *thiz,
	Enesim_Stream *data, int *w, int *h, Enesim_Buffer_Format *sfmt,
	const char *options, Eina_Error *err)
{
	Eina_Error e = 0;
	Eina_Bool ret = EINA_TRUE;
	void *op = NULL;

	if (!thiz)
	{
		if (err) *err = ENESIM_IMAGE_ERROR_PROVIDER;
		return EINA_FALSE;
	}
	_provider_options_parse(thiz, options, &op);
	if (!_provider_info_get(thiz, data, w, h, sfmt, op, &e))
	{
		if (err) *err = e;
		ret = EINA_FALSE;
	}
	_provider_options_free(thiz, op);
	return ret;
}

/**
 * @brief Loads an image
 */
EAPI Eina_Bool enesim_image_provider_load(Enesim_Image_Provider *thiz,
		Enesim_Stream *data, Enesim_Buffer **b,
		Enesim_Pool *mpool, const char *options, Eina_Error *err)
{
	Eina_Error e = 0;
	Eina_Bool ret = EINA_TRUE;
	void *op = NULL;

	if (!thiz)
	{
		if (err) *err = ENESIM_IMAGE_ERROR_PROVIDER;
		return EINA_FALSE;
	}
	_provider_options_parse(thiz, options, &op);
	if (!_provider_data_load(thiz, data, b, mpool, op, &e))
	{
		if (err) *err = e;
		ret = EINA_FALSE;
	}
	_provider_options_free(thiz, op);
	return ret;
}

/**
 * @brief Saves an image
 */
EAPI Eina_Bool enesim_image_provider_save(Enesim_Image_Provider *thiz,
		Enesim_Stream *data, Enesim_Buffer *b,
		const char *options EINA_UNUSED, Eina_Error *err)
{
	Eina_Error e = 0;
	Eina_Bool ret = EINA_TRUE;
	void *op = NULL;

	if (!thiz)
	{
		if (err) *err = ENESIM_IMAGE_ERROR_PROVIDER;
		return EINA_FALSE;
	}
	_provider_options_parse(thiz, options, &op);
	if (!_provider_data_save(thiz, data, b, op, &e))
	{
		if (err) *err = e;
		ret = EINA_FALSE;
	}
	_provider_options_free(thiz, op);
	return ret;
}
