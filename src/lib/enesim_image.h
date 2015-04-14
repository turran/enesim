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
#ifndef ENESIM_IMAGE_H
#define ENESIM_IMAGE_H

/**
 * @file
 * @ender_group{Enesim_Image_Definitions}
 * @ender_group{Enesim_Image}
 * @ender_group{Enesim_Image_Context}
 * @ender_group{Enesim_Image_File}
 * @ender_group{Enesim_Image_Provider_Descriptor_Definitions}
 * @ender_group{Enesim_Image_Provider_Descriptor}
 * @ender_group{Enesim_Image_Provider}
 * @ender_group{Enesim_Image_Finder}
 */

/**
 * @defgroup Enesim_Image_Definitions Definitions
 * @ingroup Enesim_Image
 * @{
 */

EAPI extern Eina_Error ENESIM_IMAGE_ERROR_EXIST;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_PROVIDER;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_FORMAT;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_SIZE;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_ALLOCATOR;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_LOADING;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_SAVING;

/**
 * Function prototype called whenever an image is loaded or saved
 * @param b The buffer where the data loadad is or where the data to write is
 * @param data The user provided data
 * @param error The error in case something went wrong
 */
typedef void (*Enesim_Image_Callback)(Enesim_Buffer *b, void *user_data,
		Eina_Bool success, Eina_Error error);

/**
 * @}
 * @defgroup Enesim_Image Image
 * @brief Image loading and saving
 * @{
 */

EAPI void enesim_image_dispatch(void);

EAPI Eina_Bool enesim_image_info_get(Enesim_Stream *data, const char *mime,
		int *w, int *h, Enesim_Buffer_Format *sfmt,
		const char *options, Eina_Error *err);
EAPI Eina_Bool enesim_image_load(Enesim_Stream *data, const char *mime,
		Enesim_Buffer **b, Enesim_Pool *mpool, const char *options,
		Eina_Error *err);
EAPI void enesim_image_load_async(Enesim_Stream *data, const char *mime,
		Enesim_Buffer *b, Enesim_Pool *mpool,
		Enesim_Image_Callback cb, void *user_data, const char *options);
EAPI Eina_Bool enesim_image_save(Enesim_Stream *data, const char *mime,
		Enesim_Buffer *b, const char *options, Eina_Error *err);
EAPI void enesim_image_save_async(Enesim_Stream *data, const char *mime,
		Enesim_Buffer *b, Enesim_Image_Callback cb, void *user_data,
		const char *options);

/**
 * @}
 * @defgroup Enesim_Image_Context Context
 * @brief Asynchronous context
 * @ingroup Enesim_Image
 * @{
 */
typedef struct _Enesim_Image_Context Enesim_Image_Context;

EAPI Enesim_Image_Context * enesim_image_context_new(void);
EAPI void enesim_image_context_free(Enesim_Image_Context *thiz);
EAPI void enesim_image_context_load_async(Enesim_Image_Context *thiz,
		Enesim_Stream *data, const char *mime, Enesim_Buffer *b,
		Enesim_Pool *mpool, Enesim_Image_Callback cb,
		void *user_data, const char *options);
EAPI void enesim_image_context_save_async(Enesim_Image_Context *thiz, Enesim_Stream *data,
		const char *mime, Enesim_Buffer *b, Enesim_Image_Callback cb,
		void *user_data, const char *options);
EAPI void enesim_image_context_dispatch(Enesim_Image_Context *thiz);

/**
 * @}
 * @defgroup Enesim_Image_File File based loading and saving
 * @brief Generic file loading and saving using the main context
 * @ingroup Enesim_Image
 * @{
 */
EAPI Eina_Bool enesim_image_file_info_get(const char *file, int *w, int *h,
		Enesim_Buffer_Format *sfmt, const char *options,
		Eina_Error *err);
EAPI Eina_Bool enesim_image_file_load(const char *file, Enesim_Buffer **b,
		Enesim_Pool *mpool, const char *options, Eina_Error *err);
EAPI void enesim_image_file_load_async(const char *file, Enesim_Buffer *b,
		Enesim_Pool *mpool, Enesim_Image_Callback cb,
		void *user_data, const char *options);
EAPI Eina_Bool enesim_image_file_save(const char *file, Enesim_Buffer *b,
		const char *options, Eina_Error *err);
EAPI void enesim_image_file_save_async(const char *file, Enesim_Buffer *b,
		Enesim_Image_Callback cb, void *user_data,
		const char *options);

/**
 * @defgroup Enesim_Image_Provider_Descriptor_Definitions Definitions
 * @ingroup Enesim_Image_Provider_Descriptor
 * @{
 */

/**
 * @ender_name{enesim.image.provider.options_parse}
 */
typedef void * (*Enesim_Image_Provider_Options_Parse)(const char *options);

/**
 * @ender_name{enesim.image.provider.options_free}
 */
typedef void (*Enesim_Image_Provider_Options_Free)(void *options);

/**
 * @ender_name{enesim.image.provider.loadable}
 */
typedef Eina_Bool (*Enesim_Image_Provider_Loadable)(Enesim_Stream *data);

/**
 * @ender_name{enesim.image.provider.saveable}
 */
typedef Eina_Bool (*Enesim_Image_Provider_Saveable)(Enesim_Buffer *b);

/**
 * @ender_name{enesim.image.provider.info_get}
 */
typedef Eina_Bool (*Enesim_Image_Provider_Info_Get)(Enesim_Stream *data, int *w, int *h, Enesim_Buffer_Format *sfmt, void *options, Eina_Error *err);

/**
 * @ender_name{enesim.image.provider.formats_get}
 */
typedef int (*Enesim_Image_Provider_Formats_Get)(Enesim_Buffer_Format *formats, void *options, Eina_Error *err);

/**
 * @ender_name{enesim.image.provider.load}
 */
typedef Eina_Bool (*Enesim_Image_Provider_Load)(Enesim_Stream *data, Enesim_Buffer *b, void *options, Eina_Error *err);

/**
 * @ender_name{enesim.image.provider.save}
 */
typedef Eina_Bool (*Enesim_Image_Provider_Save)(Enesim_Stream *data, Enesim_Buffer *b, void *options, Eina_Error *err);

/**
 * @}
 * @defgroup Enesim_Image_Provider_Descriptor Provider Descriptor
 * @brief Allows the creation of new providers
 * @ingroup Enesim_Image
 * @{
 */

#define ENESIM_IMAGE_PROVIDER_DESCRIPTOR_VERSION 0

typedef struct _Enesim_Image_Provider_Descriptor
{
	int version;
	const char *name;
	Enesim_Image_Provider_Options_Parse options_parse;
	Enesim_Image_Provider_Options_Free options_free;
	Enesim_Image_Provider_Loadable loadable;
	Enesim_Image_Provider_Saveable saveable;
	Enesim_Image_Provider_Info_Get info_get;
	Enesim_Image_Provider_Formats_Get formats_get;
	Enesim_Image_Provider_Load load;
	Enesim_Image_Provider_Save save;
} Enesim_Image_Provider_Descriptor;


/**
 * @}
 * @defgroup Enesim_Image_Provider Providers
 * @brief Image format loader and saver provider
 * @ingroup Enesim_Image
 * @{
 */

typedef struct _Enesim_Image_Provider Enesim_Image_Provider;

EAPI void enesim_image_provider_priority_set(Enesim_Image_Provider *p,
		Enesim_Priority priority);

EAPI Eina_Bool enesim_image_provider_info_get(Enesim_Image_Provider *thiz,
		Enesim_Stream *data, int *w, int *h, Enesim_Buffer_Format *sfmt,
		const char *options, Eina_Error *err);
EAPI Eina_Bool enesim_image_provider_load(Enesim_Image_Provider *thiz,
		Enesim_Stream *data, Enesim_Buffer **b,
		Enesim_Pool *mpool, const char *options, Eina_Error *err);
EAPI Eina_Bool enesim_image_provider_save(Enesim_Image_Provider *thiz,
		Enesim_Stream *data, Enesim_Buffer *b,
		const char *options, Eina_Error *err);

EAPI Eina_Bool enesim_image_provider_register(Enesim_Image_Provider_Descriptor *pd, Enesim_Priority priority, const char *mime);
EAPI void enesim_image_provider_unregister(Enesim_Image_Provider_Descriptor *pd, const char *mime);


/**
 * @}
 * @defgroup Enesim_Image_Finder Finder
 * @brief Image identification
 * @ingroup Enesim_Image
 * @{
 */

#define ENESIM_IMAGE_FINDER_DESCRIPTOR_VERSION 0

/**
 * @ender_name{enesim.image.finder.data_from}
 */
typedef const char *(*Enesim_Image_Finder_Data_From)(Enesim_Stream *data);
/**
 * @ender_name{enesim.image.finder.extension_from}
 */
typedef const char *(*Enesim_Image_Finder_Extension_From)(const char *ext);

typedef struct _Enesim_Image_Finder_Descriptor
{
	int version;
	Enesim_Image_Finder_Data_From data_from;
	Enesim_Image_Finder_Extension_From extension_from;
} Enesim_Image_Finder_Descriptor;

EAPI Eina_Bool enesim_image_finder_register(Enesim_Image_Finder_Descriptor *f);
EAPI void enesim_image_finder_unregister(Enesim_Image_Finder_Descriptor *f);

/**
 * @}
 * @defgroup Enesim_Image_Misc Misc
 * @brief Helper functions related to images
 * @ingroup Enesim_Image
 * @{
 */

typedef void (*Enesim_Image_Option)(void *data, const char *key, const char *value);
EAPI void enesim_image_options_parse(const char *options, Enesim_Image_Option cb, void *data);

EAPI const char * enesim_image_mime_data_from(Enesim_Stream *data);
EAPI const char * enesim_image_mime_extension_from(const char *ext);

/**
 * @}
 */

#endif
