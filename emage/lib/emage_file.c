#include "Emage.h"
#include "emage_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/* TODO later we need to add an object (type finder?) that translates
 * filename/filedata -> mimetype
 */
static Eina_Bool  _file_mime(const char *file, const char **mime)
{
	Eina_Bool ret = EINA_TRUE;
	char *end;

	/* get the extension */
	end = strrchr(file, '.');
	if (!end) return EINA_FALSE;

	end++;
	if (!strcmp(end, "png"))
		*mime = "image/png";
	else if (!strcmp(end, "jpg"))
		*mime = "image/jpg";
	else
		ret = EINA_FALSE;

	return ret;
}

static Eina_Bool _file_save_data_get(const char *file, Emage_Data **data, const char **mime)
{
	if (!_file_mime(file, mime))
		return EINA_FALSE;
	*data = emage_data_file_new(file, "wb");
	if (!*data) return EINA_FALSE;
	return EINA_TRUE;
}

static Eina_Bool _file_load_data_get(const char *file, Emage_Data **data, const char **mime)
{
	if (!_file_mime(file, mime))
		return EINA_FALSE;
	*data = emage_data_file_new(file, "rb");
	if (!*data) return EINA_FALSE;
	return EINA_TRUE;
}

#if 0
static Eina_Bool _file_load_mime(const char *file, const char **mime)
{
	struct stat stmp;
	if ((!file) || (stat(file, &stmp) < 0))
		return EINA_FALSE;
}
#endif
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
EAPI Eina_Bool emage_file_info_load(const char *file, int *w, int *h, Enesim_Buffer_Format *sfmt)
{
	Emage_Data *data;
	const char *mime;
	
	if (!_file_load_data_get(file, &data, &mime))
		return EINA_FALSE;
	return emage_info_load(data, mime, w, h, sfmt);
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
EAPI Eina_Bool emage_file_load(const char *file, Enesim_Surface **s,
		Enesim_Format f, Enesim_Pool *mpool, const char *options)
{
	Emage_Data *data;
	const char *mime;
	
	if (!_file_load_data_get(file, &data, &mime))
		return EINA_FALSE;
	return emage_load(data, mime, s, f, mpool, options);
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
EAPI void emage_file_load_async(const char *file, Enesim_Surface *s,
		Enesim_Format f, Enesim_Pool *mpool,
		Emage_Callback cb, void *user_data, const char *options)
{
	Emage_Data *data;
	const char *mime;
	
	if (!_file_load_data_get(file, &data, &mime))
	{
		cb(NULL, user_data, EMAGE_ERROR_PROVIDER);
		return;
	}
	emage_load_async(data, mime, s, f, mpool, cb, user_data, options);
}
/**
 * Save an image file synchronously
 *
 * @param file The image file to save
 * @param s The surface to read the image pixels from. It must not be NULL.
 * @param options Any option the emage provider might require
 * @return EINA_TRUE in case the image was saved correctly. EINA_FALSE if not
 */
EAPI Eina_Bool emage_file_save(const char *file, Enesim_Surface *s, const char *options)
{
	Emage_Data *data;
	const char *mime;
	
	if (!_file_save_data_get(file, &data, &mime))
		return EINA_FALSE;
	return emage_save(data, mime, s, options);
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
EAPI void emage_file_save_async(const char *file, Enesim_Surface *s, Emage_Callback cb,
		void *user_data, const char *options)
{
	Emage_Data *data;
	const char *mime;
	
	if (!_file_save_data_get(file, &data, &mime))
	{
		cb(NULL, user_data, EMAGE_ERROR_PROVIDER);
		return;
	}
	emage_save_async(data, mime, s, cb, user_data, options);
}
