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
#include "enesim_error.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_renderer.h"
#include "enesim_image.h"
#include "enesim_renderer_importer.h"
#include "enesim_image_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static void _provider_data_convert(Enesim_Buffer *buffer,
		uint32_t w, uint32_t h, Enesim_Surface *s)
{
	Enesim_Renderer *importer;

	importer = enesim_renderer_importer_new();
	enesim_renderer_importer_buffer_set(importer, buffer);
	enesim_renderer_draw(importer, s, NULL, 0, 0, NULL);
	enesim_renderer_unref(importer);
}

static Eina_Error _provider_info_load(Enesim_Image_Provider *p, Enesim_Image_Data *data,
		int *w, int *h, Enesim_Buffer_Format *sfmt, void *options)
{
	Eina_Error ret = ENESIM_IMAGE_ERROR_PROVIDER;
	int pw, ph;
	Enesim_Buffer_Format pfmt;

	/* sanitize the values */
	*w = 0;
	*h = 0;
	*sfmt = ENESIM_BUFFER_FORMATS;
	/* get the info from the image */
	if (!p->d->info_get) return ret;

	ret = p->d->info_get(data, &pw, &ph, &pfmt, options);
	if (w) *w = pw;
	if (h) *h = ph;
	if (sfmt) *sfmt = pfmt;

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

static Eina_Bool _provider_data_load(Enesim_Image_Provider *p, Enesim_Image_Data *data,
		Enesim_Surface **s, Enesim_Format f, Enesim_Pool *mpool,
		void *options,
		Eina_Error *err)
{
	Enesim_Buffer_Format cfmt;
	Enesim_Buffer *buffer;
	Enesim_Surface *ss = *s;
	Eina_Error error;
	Eina_Bool owned = EINA_FALSE;
	Eina_Bool import = EINA_FALSE;
	int w, h;

	error = _provider_info_load(p, data, &w, &h, &cfmt, options);
	if (error)
	{
		goto info_err;
	}
	if (!ss)
	{
		ss = enesim_surface_new_pool_from(f, w, h, mpool);
		if (!ss)
		{
			error = ENESIM_IMAGE_ERROR_ALLOCATOR;
			goto surface_err;
		}
		owned = EINA_TRUE;
	}

	if (cfmt == ENESIM_BUFFER_FORMAT_ARGB8888_PRE || cfmt == ENESIM_BUFFER_FORMAT_A8)
	{
		buffer = enesim_surface_buffer_get(ss);
	}
	else
	{
		/* create a buffer of format cfmt where the provider will fill */
		buffer = enesim_buffer_new_pool_from(cfmt, w, h, mpool);
		if (!buffer)
		{
			error = ENESIM_IMAGE_ERROR_ALLOCATOR;
			goto buffer_err;
		}
		import = EINA_TRUE;
	}

	/* load the data */
	enesim_image_data_reset(data);
	error = p->d->load(data, buffer, options);
	if (error)
	{
		goto load_err;
	}
	if (import)
	{
		/* convert */
		_provider_data_convert(buffer, w, h, ss);
	}

	*s = ss;
	return EINA_TRUE;

load_err:
	enesim_buffer_unref(buffer);
buffer_err:
	if (owned)
		enesim_surface_unref(ss);
surface_err:
info_err:
	*err = error;
	return EINA_FALSE;
}

static Eina_Bool _provider_data_save(Enesim_Image_Provider *p, Enesim_Image_Data *data,
		Enesim_Surface *s, Eina_Error *err)
{
	/* save the data */
	if (!p->d->save) return EINA_FALSE;

	if (p->d->save(data, s, NULL) == EINA_FALSE)
	{
		*err = ENESIM_IMAGE_ERROR_SAVING;
		return EINA_FALSE;
	}

	return EINA_TRUE;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
EAPI Eina_Bool enesim_image_provider_info_load(Enesim_Image_Provider *thiz,
	Enesim_Image_Data *data, int *w, int *h, Enesim_Buffer_Format *sfmt)
{
	Eina_Error err;
	if (!thiz)
	{
		eina_error_set(ENESIM_IMAGE_ERROR_PROVIDER);
		return EINA_FALSE;
	}
	err = _provider_info_load(thiz, data, w, h, sfmt, NULL);
	if (err)
	{
		return EINA_FALSE;
	}
	return EINA_TRUE;
}

EAPI Eina_Bool enesim_image_provider_load(Enesim_Image_Provider *thiz,
		Enesim_Image_Data *data, Enesim_Surface **s,
		Enesim_Format f, Enesim_Pool *mpool, const char *options)
{
	Eina_Error err = 0;
	Eina_Bool ret = EINA_TRUE;
	void *op = NULL;

	if (!thiz)
	{
		eina_error_set(ENESIM_IMAGE_ERROR_PROVIDER);
		return EINA_FALSE;
	}
	_provider_options_parse(thiz, options, &op);
	if (!_provider_data_load(thiz, data, s, f, mpool, op, &err))
	{
		eina_error_set(err);
		ret = EINA_FALSE;
	}
	_provider_options_free(thiz, op);
	return ret;
}

EAPI Eina_Bool enesim_image_provider_save(Enesim_Image_Provider *thiz,
		Enesim_Image_Data *data, Enesim_Surface *s,
		const char *options)
{
	Eina_Error err = 0;

	if (!thiz)
	{
		eina_error_set(ENESIM_IMAGE_ERROR_PROVIDER);
		return EINA_FALSE;
	}
	if (!_provider_data_save(thiz, data, s, &err))
	{
		eina_error_set(err);
		return EINA_FALSE;
	}
	return EINA_TRUE;
}
