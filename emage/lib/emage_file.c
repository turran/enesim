#include "Emage.h"
#include "emage_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
typedef struct _Emage_File_Data
{
	Emage_Callback cb;
	void *user_data;
	Emage_Data *data;
} Emage_File_Data;

static void _emage_file_cb(Enesim_Surface *s, void *user_data, int error)
{
	Emage_File_Data *fdata = user_data;

	fdata->cb(s, fdata->user_data, error);
	emage_data_free(fdata->data);
}

static const char * _emage_file_get_extension(const char *file)
{
	char *tmp;

	tmp = strrchr(file, '.');
	if (!tmp) return NULL;
	return tmp + 1;
}

static Eina_Bool _file_save_data_get(const char *file, Emage_Data **data, const char **mime)
{
	Emage_Data *d;
	const char *m;
	const char *ext;

	ext = _emage_file_get_extension(file);
	if (!ext) return EINA_FALSE;

	d = emage_data_file_new(file, "wb");
	if (!d) return EINA_FALSE;

	m = emage_mime_extension_from(ext);
	if (!m)
	{
		emage_data_free(d);
		return EINA_FALSE;
	}
	*mime = m;
	*data = d;

	return EINA_TRUE;
}

static Eina_Bool _file_load_data_get(const char *file, Emage_Data **data, const char **mime)
{
	Emage_Data *d;
	const char *m;

	d = emage_data_file_new(file, "rb");
	if (!d) return EINA_FALSE;

	m = emage_mime_data_from(d);
	if (!m)
	{
		emage_data_free(d);
		return EINA_FALSE;
	}
	emage_data_reset(d);
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
EAPI Eina_Bool emage_file_info_load(const char *file, int *w, int *h, Enesim_Buffer_Format *sfmt)
{
	Emage_Data *data;
	Eina_Bool ret;
	const char *mime;

	if (!_file_load_data_get(file, &data, &mime))
		return EINA_FALSE;
	ret = emage_info_load(data, mime, w, h, sfmt);
	emage_data_free(data);
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
EAPI Eina_Bool emage_file_load(const char *file, Enesim_Surface **s,
		Enesim_Format f, Enesim_Pool *mpool, const char *options)
{
	Emage_Data *data;
	Eina_Bool ret;
	const char *mime;

	if (!_file_load_data_get(file, &data, &mime))
		return EINA_FALSE;
	ret = emage_load(data, mime, s, f, mpool, options);
	emage_data_free(data);
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
EAPI void emage_file_load_async(const char *file, Enesim_Surface *s,
		Enesim_Format f, Enesim_Pool *mpool,
		Emage_Callback cb, void *user_data, const char *options)
{
	Emage_Data *data;
	Emage_File_Data fdata;
	const char *mime;

	if (!_file_load_data_get(file, &data, &mime))
	{
		cb(NULL, user_data, EMAGE_ERROR_PROVIDER);
		return;
	}
	fdata.cb = cb;
	fdata.user_data = user_data;
	fdata.data = data;

	emage_load_async(data, mime, s, f, mpool, _emage_file_cb, &fdata, options);
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
	Eina_Bool ret;
	const char *mime;

	if (!_file_save_data_get(file, &data, &mime))
		return EINA_FALSE;
	ret = emage_save(data, mime, s, options);
	emage_data_free(data);
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
EAPI void emage_file_save_async(const char *file, Enesim_Surface *s, Emage_Callback cb,
		void *user_data, const char *options)
{
	Emage_Data *data;
	Emage_File_Data fdata;
	const char *mime;

	if (!_file_save_data_get(file, &data, &mime))
	{
		cb(NULL, user_data, EMAGE_ERROR_PROVIDER);
		return;
	}
	fdata.cb = cb;
	fdata.user_data = user_data;
	fdata.data = data;

	emage_save_async(data, mime, s, _emage_file_cb, &fdata, options);
}
