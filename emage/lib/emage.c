#include "Enesim_Image.h"
#include "enesim_image_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

#define ENESIM_IMAGE_LOG_COLOR_DEFAULT EINA_COLOR_GREEN

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
int enesim_image_log_dom_global = -1;

Enesim_Image_Provider * enesim_image_load_provider_get(Enesim_Image_Data *data, const char *mime)
{
	Enesim_Image_Provider *p;
	Eina_List *providers;
	Eina_List *l;

	providers = eina_hash_find(_providers, mime);
	/* iterate over the list of providers and check for a compatible loader */
	EINA_LIST_FOREACH (providers, l, p)
	{
		/* TODO priority loaders */
		/* check if the provider can load the image */
		if (!p->loadable)
			return p;

		enesim_image_data_reset(data);
		if (p->loadable(data) == EINA_TRUE)
		{
			enesim_image_data_reset(data);
			return p;
		}
	}
	return NULL;
}

Enesim_Image_Provider * enesim_image_save_provider_get(Enesim_Image_Data *data, const char *mime)
{
	Enesim_Image_Provider *p;
	Eina_List *providers;
	Eina_List *l;

	providers = eina_hash_find(_providers, mime);
	/* iterate over the list of providers and check for a compatible saver */
	EINA_LIST_FOREACH (providers, l, p)
	{
		/* TODO priority savers */
		return p;
#if 0
		/* check if the provider can save the image */
		if (!p->saveable)
			return p;
		if (p->saveable(data) == EINA_TRUE)
		{
			enesim_image_data_reset(data);
			return p;
		}
#endif
	}
	return NULL;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Initialize emage. This function must be called before any other emage
 * function
 */
EAPI int enesim_image_init(void)
{
	if (++_enesim_image_init_count != 1)
		return _enesim_image_init_count;

	if (!eina_init())
	{
		fprintf(stderr, "Emage: Eina init failed");
		return --_enesim_image_init_count;
	}

	enesim_image_log_dom_global = eina_log_domain_register("emage", ENESIM_IMAGE_LOG_COLOR_DEFAULT);
	if (enesim_image_log_dom_global < 0)
	{
		EINA_LOG_ERR("Emage: Can not create a general log domain.");
		goto shutdown_eina;
	}

	if (!enesim_init())
	{
		ERR("Enesim init failed");
		goto unregister_log_domain;
	}

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
	_modules = eina_module_list_get(_modules, PACKAGE_LIB_DIR"/emage/", 1, NULL, NULL);
	eina_module_list_load(_modules);
#if BUILD_STATIC_MODULE_PNG
	png_provider_init();
#endif
	/* create our main context */
	_main_context = enesim_image_context_new();

	return _enesim_image_init_count;

unregister_log_domain:
	eina_log_domain_unregister(enesim_image_log_dom_global);
	enesim_image_log_dom_global = -1;
shutdown_eina:
	eina_shutdown();
	return --_enesim_image_init_count;
}
/**
 * Shutdown emage library. Once you have finished using emage, shut it down.
 */
EAPI int enesim_image_shutdown(void)
{
	if (--_enesim_image_init_count != 0)
		return _enesim_image_init_count;

	/* destroy our main context */
	enesim_image_context_free(_main_context);
	_main_context = NULL;

	/* unload every module */
	eina_module_list_free(_modules);
#if BUILD_STATIC_MODULE_PNG
	png_provider_shutdown();
#endif
	/* remove the finders */
	eina_list_free(_finders);
	/* remove the providers */
	eina_hash_free(_providers);
	/* shutdown every provider */
	enesim_shutdown();
	eina_log_domain_unregister(enesim_image_log_dom_global);
	enesim_image_log_dom_global = -1;
	eina_shutdown();

	return _enesim_image_init_count;
}
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

	prov = enesim_image_load_provider_get(data, mime);
	return enesim_image_provider_info_load(prov, data, w, h, sfmt);
}
/**
 * Load an image synchronously
 *
 * @param data The image data to load
 * @param mime The image mime
 * @param s The surface to write the image pixels to. It must not be NULL.
 * @param f The desired format the image should be converted to
 * @param mpool The mempool that will create the surface in case the surface
 * reference is NULL
 * @param options Any option the emage provider might require
 * @return EINA_TRUE in case the image was loaded correctly. EINA_FALSE if not
 */
EAPI Eina_Bool enesim_image_load(Enesim_Image_Data *data, const char *mime,
		Enesim_Surface **s, Enesim_Format f, Enesim_Pool *mpool,
		const char *options)
{
	Enesim_Image_Provider *prov;

	prov = enesim_image_load_provider_get(data, mime);
	return enesim_image_provider_load(prov, data, s, f, mpool, options);
}
/**
 * Load an image asynchronously
 *
 * @param data The image data to load
 * @param mime The image mime
 * @param s The surface to write the image pixels to. It must not be NULL.
 * @param f The desired format the image should be converted to
 * @param mpool The mempool that will create the surface in case the surface
 * reference is NULL
 * @param cb The function that will get called once the load is done
 * @param data User provided data
 * @param options Any option the emage provider might require
 */
EAPI void enesim_image_load_async(Enesim_Image_Data *data, const char *mime,
		Enesim_Surface *s, Enesim_Format f, Enesim_Pool *mpool,
		Enesim_Image_Callback cb, void *user_data, const char *options)
{
	enesim_image_context_load_async(_main_context, data, mime, s, f, mpool, cb,
			user_data, options);
}
/**
 * Save an image synchronously
 *
 * @param data The image data to load
 * @param mime The image mime
 * @param s The surface to read the image pixels from. It must not be NULL.
 * @param options Any option the emage provider might require
 * @return EINA_TRUE in case the image was saved correctly. EINA_FALSE if not
 */
EAPI Eina_Bool enesim_image_save(Enesim_Image_Data *data, const char *mime,
		Enesim_Surface *s, const char *options)
{
	Enesim_Image_Provider *prov;

	prov = enesim_image_save_provider_get(data, mime);
	return enesim_image_provider_save(prov, data, s, options);
}

/**
 * Save an image asynchronously
 *
 * @param data The image data to load
 * @param mime The image mime
 * @param s The surface to read the image pixels from. It must not be NULL.
 * @param cb The function that will get called once the save is done
 * @param data User provided data
 * @param options Any option the emage provider might require
 *
 */
EAPI void enesim_image_save_async(Enesim_Image_Data *data, const char *mime,
		Enesim_Surface *s, Enesim_Image_Callback cb,
		void *user_data, const char *options)
{
	enesim_image_context_save_async(_main_context, data, mime, s, cb, user_data,
			options);
}

/**
 * @brief Call every asynchronous callback set
 *
 * In case emage has setup some asynchronous load, you must call this
 * function to get the status of such process
 */
EAPI void enesim_image_dispatch(void)
{
	enesim_image_context_dispatch(_main_context);
}

/**
 *
 */
EAPI Eina_Bool enesim_image_provider_register(Enesim_Image_Provider *p, const char *mime)
{
	Eina_List *providers;
	Eina_List *tmp;

	if (!p)
		return EINA_FALSE;
	/* check for mandatory functions */
	if (!p->info_get)
	{
		WRN("Provider %s doesn't provide the info_get() function", p->name);
	}
	if (!p->load)
	{
		WRN("Provider %s doesn't provide the load() function", p->name);
	}
	providers = tmp = eina_hash_find(_providers, mime);
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
 *
 */
EAPI void enesim_image_provider_unregister(Enesim_Image_Provider *p, const char *mime)
{
	Eina_List *providers;
	Eina_List *tmp;

	providers = tmp = eina_hash_find(_providers, mime);
	if (!providers)
	{
		WRN("Impossible to unregister the provider %p on mime '%s'", p, mime);
		return;
	}

	/* remove from the list of providers */
	providers = eina_list_remove(providers, p);
	if (!providers)
	{
		eina_hash_del(_providers, mime, tmp);
	}
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
