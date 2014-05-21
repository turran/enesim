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
 * @defgroup Enesim_Image_Group Image
 * @brief Image loading and saving
 * @{
 */

EAPI extern Eina_Error ENESIM_IMAGE_ERROR_EXIST;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_PROVIDER;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_FORMAT;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_SIZE;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_ALLOCATOR;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_LOADING;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_SAVING;

EAPI void enesim_image_dispatch(void);

/**
 * @}
 * @defgroup Enesim_Image_Context_Group Context
 * @brief Asynchronous context
 * @ingroup Enesim_Image_Group
 * @{
 */
typedef struct _Enesim_Image_Context Enesim_Image_Context;
typedef void (*Enesim_Image_Callback)(Enesim_Buffer *b, void *data, int error); /**< Function prototype called whenever an image is loaded or saved */

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
 * @defgroup Enesim_Image_Load_Save_Group Loading and Saving
 * @brief Generic loading and saving using the main context
 * @ingroup Enesim_Image_Group
 * @{
 */

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
 * @defgroup Enesim_Image_File_Group File based loading and saving
 * @brief Generic file loading and saving using the main context
 * @ingroup Enesim_Image_Group
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
 * @}
 * @defgroup Enesim_Image_Provider_Group Providers
 * @brief Image format loader and saver provider
 * @ingroup Enesim_Image_Group
 * @{
 */

typedef struct _Enesim_Image_Provider Enesim_Image_Provider;

typedef void * (*Enesim_Image_Provider_Options_Parse_Cb)(const char *options);
typedef void (*Enesim_Image_Provider_Options_Free_Cb)(void *options);
typedef Eina_Bool (*Enesim_Image_Provider_Loadable_Cb)(Enesim_Stream *data);
typedef Eina_Bool (*Enesim_Image_Provider_Saveable_Cb)(Enesim_Buffer *b);
typedef Eina_Bool (*Enesim_Image_Provider_Info_Get_Cb)(Enesim_Stream *data, int *w, int *h, Enesim_Buffer_Format *sfmt, void *options, Eina_Error *err);
typedef int (*Enesim_Image_Provider_Formats_Get_Cb)(Enesim_Buffer_Format *formats, void *options, Eina_Error *err);
typedef Eina_Bool (*Enesim_Image_Provider_Load_Cb)(Enesim_Stream *data, Enesim_Buffer *b, void *options, Eina_Error *err);
typedef Eina_Bool (*Enesim_Image_Provider_Save_Cb)(Enesim_Stream *data, Enesim_Buffer *b, void *options, Eina_Error *err);

#define ENESIM_IMAGE_PROVIDER_DESCRIPTOR_VERSION 0

typedef struct _Enesim_Image_Provider_Descriptor
{
	int version;
	const char *name;
	Enesim_Image_Provider_Options_Parse_Cb options_parse;
	Enesim_Image_Provider_Options_Free_Cb options_free;
	Enesim_Image_Provider_Loadable_Cb loadable;
	Enesim_Image_Provider_Saveable_Cb saveable;
	Enesim_Image_Provider_Info_Get_Cb info_get;
	Enesim_Image_Provider_Formats_Get_Cb formats_get;
	Enesim_Image_Provider_Load_Cb load;
	Enesim_Image_Provider_Save_Cb save;
} Enesim_Image_Provider_Descriptor;

EAPI Eina_Bool enesim_image_provider_register(Enesim_Image_Provider_Descriptor *pd, Enesim_Priority priority, const char *mime);
EAPI void enesim_image_provider_unregister(Enesim_Image_Provider_Descriptor *pd, const char *mime);

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

/**
 * @}
 * @defgroup Enesim_Image_Finder_Group Finder
 * @brief Image identification
 * @ingroup Enesim_Image_Group
 * @{
 */

#define ENESIM_IMAGE_FINDER_DESCRIPTOR_VERSION 0

typedef const char *(*Enesim_Image_Finder_Data_From_Cb)(Enesim_Stream *data);
typedef const char *(*Enesim_Image_Finder_Extension_From_Cb)(const char *ext);

typedef struct _Enesim_Image_Finder_Descriptor
{
	int version;
	Enesim_Image_Finder_Data_From_Cb data_from;
	Enesim_Image_Finder_Extension_From_Cb extension_from;
} Enesim_Image_Finder_Descriptor;

EAPI Eina_Bool enesim_image_finder_register(Enesim_Image_Finder_Descriptor *f);
EAPI void enesim_image_finder_unregister(Enesim_Image_Finder_Descriptor *f);

/**
 * @}
 * @defgroup Enesim_Image_Misc_Group Misc
 * @brief Helper functions related to images
 * @ingroup Enesim_Image_Group
 * @{
 */

typedef void (*Enesim_Image_Option_Cb)(void *data, const char *key, const char *value);
EAPI void enesim_image_options_parse(const char *options, Enesim_Image_Option_Cb cb, void *data);

EAPI const char * enesim_image_mime_data_from(Enesim_Stream *data);
EAPI const char * enesim_image_mime_extension_from(const char *ext);

/**
 * @}
 */

#endif
