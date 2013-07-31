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
#define ENESIM_LOG_DEFAULT enesim_log_image

static int _enesim_image_init_count = 0;
static Eina_Array *_modules = NULL;
static Eina_Hash *_providers = NULL;
static Eina_List *_finders = NULL;
static Enesim_Image_Context *_main_context = NULL;

Eina_Error ENESIM_IMAGE_ERROR_EXIST;
Eina_Error ENESIM_IMAGE_ERROR_PROVIDER;
Eina_Error ENESIM_IMAGE_ERROR_FORMAT;
Eina_Error ENESIM_IMAGE_ERROR_SIZE;
Eina_Error ENESIM_IMAGE_ERROR_ALLOCATOR;
Eina_Error ENESIM_IMAGE_ERROR_LOADING;
Eina_Error ENESIM_IMAGE_ERROR_SAVING;

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Image_Provider * enesim_image_load_info_provider_get(
		Enesim_Image_Data *data, const char *mime)
{
	Enesim_Image_Provider *p;
	Eina_List *providers;
	Eina_List *l;

	if (!mime)
	{
		mime = enesim_image_mime_data_from(data);
		enesim_image_data_reset(data);
	}
	if (!mime)
	{
		WRN("No mime type detected");
		return NULL;
	}
	providers = eina_hash_find(_providers, mime);
	EINA_LIST_FOREACH (providers, l, p)
	{
		if (p->d->info_get)
			return p;
	}
	return NULL;
}

Enesim_Image_Provider * enesim_image_load_provider_get(Enesim_Image_Data *data,
		const char *mime, Enesim_Pool *pool)
{
	Enesim_Image_Provider *p;
	Eina_List *providers;
	Eina_List *l;

	if (!mime)
	{
		mime = enesim_image_mime_data_from(data);
		enesim_image_data_reset(data);
	}
	if (!mime)
	{
		WRN("No mime type detected");
		return NULL;
	}
	providers = eina_hash_find(_providers, mime);
	/* iterate over the list of providers and check for a compatible loader */
	EINA_LIST_FOREACH (providers, l, p)
	{
		/* TODO priority loaders */
		/* check if the provider can load the image */
		if (!p->d->loadable)
			return p;

		enesim_image_data_reset(data);
		if (p->d->loadable(data, pool) == EINA_TRUE)
		{
			enesim_image_data_reset(data);
			return p;
		}
	}
	return NULL;
}

Enesim_Image_Provider * enesim_image_save_provider_get(Enesim_Buffer *b,
		const char *mime)
{
	Enesim_Image_Provider *p;
	Eina_List *providers;
	Eina_List *l;

	providers = eina_hash_find(_providers, mime);
	/* iterate over the list of providers and check for a compatible saver */
	EINA_LIST_FOREACH (providers, l, p)
	{
		/* TODO priority savers */
		/* check if the provider can save the image */
		if (!p->d->saveable)
			return p;
		if (p->d->saveable(b) == EINA_TRUE)
		{
			return p;
		}
	}
	return NULL;
}

/*
 * Initialize the image system. This function must be called before any other image
 * function
 */
int enesim_image_init(void)
{
	if (++_enesim_image_init_count != 1)
		return _enesim_image_init_count;

	/* the errors */
	ENESIM_IMAGE_ERROR_EXIST = eina_error_msg_static_register("Files does not exist");
	ENESIM_IMAGE_ERROR_PROVIDER = eina_error_msg_static_register("No provider for such file");
	ENESIM_IMAGE_ERROR_FORMAT = eina_error_msg_static_register("Wrong surface format");
	ENESIM_IMAGE_ERROR_SIZE = eina_error_msg_static_register("Size mismatch");
	ENESIM_IMAGE_ERROR_ALLOCATOR = eina_error_msg_static_register("Error allocating the surface data");
	ENESIM_IMAGE_ERROR_LOADING = eina_error_msg_static_register("Error loading the image");
	ENESIM_IMAGE_ERROR_SAVING = eina_error_msg_static_register("Error saving the image");
	/* the providers */
	_providers = eina_hash_string_superfast_new(NULL);
	/* the modules */
	_modules = eina_module_list_get(_modules, PACKAGE_LIB_DIR"/enesim/image/", 1, NULL, NULL);
	eina_module_list_load(_modules);
#if BUILD_STATIC_MODULE_PNG
	enesim_image_png_provider_init();
#endif
#if BUILD_STATIC_MODULE_JPG
	enesim_image_jpg_provider_init();
#endif
#if BUILD_STATIC_MODULE_RAW
	enesim_image_raw_provider_init();
#endif
	/* create our main context */
	_main_context = enesim_image_context_new();

	return _enesim_image_init_count;
}

/*
 * Shutdown the image systemibrary. Once you have finished using it, shut it down.
 */
int enesim_image_shutdown(void)
{
	if (--_enesim_image_init_count != 0)
		return _enesim_image_init_count;

	/* destroy our main context */
	enesim_image_context_free(_main_context);
	_main_context = NULL;

	/* unload every module */
	eina_module_list_free(_modules);
	eina_array_free(_modules);
#if BUILD_STATIC_MODULE_PNG
	enesim_image_png_provider_shutdown();
#endif
#if BUILD_STATIC_MODULE_JPG
	enesim_image_jpg_provider_shutdown();
#endif
#if BUILD_STATIC_MODULE_RAW
	enesim_image_raw_provider_shutdown();
#endif
	/* remove the finders */
	eina_list_free(_finders);
	/* remove the providers */
	eina_hash_free(_providers);

	return _enesim_image_init_count;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/

/**
 * Loads information about an image
 *
 * @param data The image data
 * @param mime The image mime
 * @param w The image width
 * @param h The image height
 * @param sfmt The image original format
 */
EAPI Eina_Bool enesim_image_info_load(Enesim_Image_Data *data, const char *mime,
		int *w, int *h, Enesim_Buffer_Format *sfmt)
{
	Enesim_Image_Provider *prov;

	prov = enesim_image_load_info_provider_get(data, mime);
	return enesim_image_provider_info_load(prov, data, w, h, sfmt);
}
/**
 * Load an image synchronously
 *
 * @param data The image data to load from
 * @param mime The image mime. It can be NULL, if so, it will be autodetected
 * from the data itself.
 * @param b The buffer to write the image pixels to. It must not be NULL.
 * @param mpool The mempool that will create the buffer in case the buffer
 * reference is NULL
 * @param options Any option the provider might require
 * @return EINA_TRUE in case the image was loaded correctly. EINA_FALSE if not
 */
EAPI Eina_Bool enesim_image_load(Enesim_Image_Data *data, const char *mime,
		Enesim_Buffer **b, Enesim_Pool *mpool, const char *options)
{
	Enesim_Image_Provider *prov;

	prov = enesim_image_load_provider_get(data, mime, mpool);
	return enesim_image_provider_load(prov, data, b, mpool, options);
}
/**
 * Load an image asynchronously
 *
 * @param data The image data to load from
 * @param mime The image mime. It can be NULL, if so, it will be autodetected
 * from the data itself.
 * @param b The buffer to write the image pixels to. It must not be NULL.
 * @param mpool The mempool that will create the buffer in case the buffer
 * reference is NULL
 * @param cb The function that will get called once the load is done
 * @param data User provided data
 * @param options Any option the provider might require
 */
EAPI void enesim_image_load_async(Enesim_Image_Data *data, const char *mime,
		Enesim_Buffer *b, Enesim_Pool *mpool, Enesim_Image_Callback cb,
		void *user_data, const char *options)
{
	enesim_image_context_load_async(_main_context, data, mime, b, mpool, cb,
			user_data, options);
}
/**
 * Save an image synchronously
 *
 * @param data The image data to save to
 * @param mime The image mime
 * @param b The buffer to read the image pixels from. It must not be NULL.
 * @param options Any option the provider might require
 * @return EINA_TRUE in case the image was saved correctly. EINA_FALSE if not
 */
EAPI Eina_Bool enesim_image_save(Enesim_Image_Data *data, const char *mime,
		Enesim_Buffer *b, const char *options)
{
	Enesim_Image_Provider *prov;

	prov = enesim_image_save_provider_get(b, mime);
	return enesim_image_provider_save(prov, data, b, options);
}

/**
 * Save an image asynchronously
 *
 * @param data The image data to save to
 * @param mime The image mime
 * @param b The buffer to read the image pixels from. It must not be NULL.
 * @param cb The function that will get called once the save is done
 * @param data User provided data
 * @param options Any option the provider might require
 */
EAPI void enesim_image_save_async(Enesim_Image_Data *data, const char *mime,
		Enesim_Buffer *b, Enesim_Image_Callback cb,
		void *user_data, const char *options)
{
	enesim_image_context_save_async(_main_context, data, mime, b, cb, user_data,
			options);
}

/**
 * @brief Dispatch every asynchronous callback set on the main context
 *
 * In case of requesting some asynchronous load or save, you must call this
 * function to get the status of such process
 */
EAPI void enesim_image_dispatch(void)
{
	enesim_image_context_dispatch(_main_context);
}

/**
 *
 */
EAPI Eina_Bool enesim_image_provider_register(Enesim_Image_Provider_Descriptor *pd,
		Enesim_Priority priority, const char *mime)
{
	Enesim_Image_Provider *p;
	Eina_List *providers;
	Eina_List *tmp;

	if (!pd)
		return EINA_FALSE;
	/* check for mandatory functions */
	if (!pd->info_get)
	{
		WRN("Provider %s doesn't provide the info_get() function", pd->name);
	}
	if (!pd->load)
	{
		WRN("Provider %s doesn't provide the load() function", pd->name);
	}

	DBG("Adding provider for mime '%s'", mime);
	p = calloc(1, sizeof(Enesim_Image_Provider));
	p->priority = priority;
	p->mime = mime;
	p->d = pd;

	providers = tmp = eina_hash_find(_providers, mime);
	/* TODO add to the list in order */
	providers = eina_list_append(providers, p);
	if (!tmp)
	{
		eina_hash_add(_providers, mime, providers);
	}
	return EINA_TRUE;
}

/**
 *
 */
EAPI void enesim_image_provider_priority_set(Enesim_Image_Provider *p,
		Enesim_Priority priority)
{
	Enesim_Image_Provider *pp;
	Eina_List *providers;
	Eina_List *l, *rel = NULL, *tmp;

	p->priority = priority;
	/* reorder the list */
	providers = eina_hash_find(_providers, p->mime);
	providers = tmp = eina_list_remove(providers, p);

	EINA_LIST_FOREACH (providers, l, pp)
	{
		if (pp->priority <= priority)
		{
			rel = l;
			break;
		}
	}
	providers = eina_list_prepend_relative_list(providers, p, rel);
	if (!tmp)
	{
		eina_hash_add(_providers, p->mime, providers);
	}
}

/**
 *
 */
EAPI void enesim_image_provider_unregister(Enesim_Image_Provider_Descriptor *pd, const char *mime)
{
	Enesim_Image_Provider *p;
	Eina_List *providers;
	Eina_List *tmp;
	Eina_List *l;

	providers = tmp = eina_hash_find(_providers, mime);
	if (!providers)
	{
		WRN("Impossible to unregister the provider %p on mime '%s'", pd, mime);
		return;
	}
	/* find the provider for this mime list */
	EINA_LIST_FOREACH(providers, l, p)
	{
		if (p->d != pd)
			continue;

		/* remove from the list of providers */
		providers = eina_list_remove(providers, p);
		if (!providers)
		{
			eina_hash_del(_providers, mime, tmp);
		}
		/* finally free the provider itself */
		free(p);

		break;
	}
}

/**
 *
 */
EAPI const char * enesim_image_mime_data_from(Enesim_Image_Data *data)
{
	Enesim_Image_Finder *f;
	Eina_List *l;
	const char *ret = NULL;

	EINA_LIST_FOREACH(_finders, l, f)
	{
		if (!f->data_from)
			continue;

		enesim_image_data_reset(data);
		ret = f->data_from(data);
		if (ret) break;
	}
	DBG("Using mime '%s'", ret);
	return ret;
}

/**
 *
 */
EAPI const char * enesim_image_mime_extension_from(const char *ext)
{
	Enesim_Image_Finder *f;
	Eina_List *l;
	const char *ret = NULL;

	EINA_LIST_FOREACH(_finders, l, f)
	{
		ret = f->extension_from(ext);
		if (ret) break;
	}
	return ret;
}

/**
 *
 */
EAPI Eina_Bool enesim_image_finder_register(Enesim_Image_Finder *f)
{
	if (!f) return EINA_FALSE;

	if (!f->data_from)
	{
		WRN("Finder %p doesn't provide the 'data_from()' function", f);
	}
	if (!f->extension_from)
	{
		WRN("Finder %p doesn't provide the 'extension_from()' function", f);
	}
	_finders = eina_list_append(_finders, f);

	return EINA_TRUE;
}

EAPI void enesim_image_finder_unregister(Enesim_Image_Finder *f)
{
	if (!f) return;
	_finders = eina_list_remove(_finders, f);
}

/**
 * The options format is:
 * option1=value1;option2=value2
 */
EAPI void enesim_image_options_parse(const char *options, Enesim_Image_Option_Cb cb, void *data)
{
	char *orig;
	char *v;
	char *sc;
	char *c;

	orig = v = strdup(options);
	/* split the value by ';' */
	while ((sc = strchr(v, ';')))
	{
		*sc = '\0';
		/* split it by ':' */
		c = strchr(v, '=');
		if (c)
		{
			char *vv;

			*c = '\0';
			vv = c + 1;
			cb(data, v, vv);
		}
		v = sc + 1;
	}
	/* do the last one */
	c = strchr(v, '=');
	if (c)
	{
		char *vv;

		*c = '\0';
		vv = c + 1;
		cb(data, v, vv);
	}
	/* and call the attr_cb */
	free(orig);
}
