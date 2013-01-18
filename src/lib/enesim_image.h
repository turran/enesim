#ifndef ENESIM_IMAGE_H
#define ENESIM_IMAGE_H

/**
 * @mainpage Emage
 * @section intro Introduction
 * Emage is a library that loads and saves several types of image formats
 * from and to Enesim Surfaces
 * @section dependencies Dependencies
 * - Enesim
 * - Eina
 */

EAPI extern Eina_Error ENESIM_IMAGE_ERROR_EXIST;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_PROVIDER;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_FORMAT;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_SIZE;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_ALLOCATOR;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_LOADING;
EAPI extern Eina_Error ENESIM_IMAGE_ERROR_SAVING;

/*
 * @defgroup Enesim_Image_Main_Group Main
 * @{
 */
EAPI int enesim_image_init(void);
EAPI int enesim_image_shutdown(void);
EAPI void enesim_image_dispatch(void);

/**
 * @}
 * @defgroup Enesim_Image_Data_Group Data
 * @{
 */
typedef ssize_t (*Enesim_Image_Data_Read)(void *data, void *buffer, size_t len);
typedef ssize_t (*Enesim_Image_Data_Write)(void *data, void *buffer, size_t len);
typedef void * (*Enesim_Image_Data_Mmap)(void *data, size_t *size);
typedef void (*Enesim_Image_Data_Munmap)(void *data, void *ptr);
typedef void (*Enesim_Image_Data_Reset)(void *data);
typedef size_t (*Enesim_Image_Data_Length)(void *data);
typedef char * (*Enesim_Image_Data_Location)(void *data);
typedef void (*Enesim_Image_Data_Free)(void *data);

typedef struct _Enesim_Image_Data_Descriptor
{
	Enesim_Image_Data_Read read;
	Enesim_Image_Data_Write write;
	Enesim_Image_Data_Mmap mmap;
	Enesim_Image_Data_Munmap munmap;
	Enesim_Image_Data_Reset reset;
	Enesim_Image_Data_Length length;
	Enesim_Image_Data_Location location;
	Enesim_Image_Data_Free free;
} Enesim_Image_Data_Descriptor;

typedef struct _Enesim_Image_Data Enesim_Image_Data;
EAPI Enesim_Image_Data * enesim_image_data_new(Enesim_Image_Data_Descriptor *d, void *data);
EAPI ssize_t enesim_image_data_read(Enesim_Image_Data *thiz, void *buffer, size_t len);
EAPI ssize_t enesim_image_data_write(Enesim_Image_Data *thiz, void *buffer, size_t len);
EAPI size_t enesim_image_data_length(Enesim_Image_Data *thiz);
EAPI void * enesim_image_data_mmap(Enesim_Image_Data *thiz, size_t *size);
EAPI void enesim_image_data_munmap(Enesim_Image_Data *thiz, void *ptr);
EAPI void enesim_image_data_reset(Enesim_Image_Data *thiz);
EAPI char * enesim_image_data_location(Enesim_Image_Data *thiz);
EAPI void enesim_image_data_free(Enesim_Image_Data *thiz);

EAPI Enesim_Image_Data * enesim_image_data_file_new(const char *file, const char *mode);
EAPI Enesim_Image_Data * enesim_image_data_buffer_new(void *buffer, size_t len);
EAPI Enesim_Image_Data * enesim_image_data_base64_new(Enesim_Image_Data *d);

/**
 * @}
 * @defgroup Enesim_Image_Context Context
 * @{
 */
typedef struct _Enesim_Image_Context Enesim_Image_Context;
typedef void (*Enesim_Image_Callback)(Enesim_Surface *s, void *data, int error); /**< Function prototype called whenever an image is loaded or saved */

EAPI Enesim_Image_Context * enesim_image_context_new(void);
EAPI void enesim_image_context_free(Enesim_Image_Context *thiz);
EAPI void enesim_image_context_load_async(Enesim_Image_Context *thiz,
		Enesim_Image_Data *data, const char *mime, Enesim_Surface *s,
		Enesim_Format f, Enesim_Pool *mpool, Enesim_Image_Callback cb,
		void *user_data, const char *options);
EAPI void enesim_image_context_save_async(Enesim_Image_Context *thiz, Enesim_Image_Data *data,
		const char *mime, Enesim_Surface *s, Enesim_Image_Callback cb,
		void *user_data, const char *options);
EAPI void enesim_image_context_dispatch(Enesim_Image_Context *thiz);
EAPI void enesim_image_context_pool_size_set(Enesim_Image_Context *thiz, int num);
EAPI int enesim_image_context_pool_size_get(Enesim_Image_Context *thiz);

/**
 * @}
 * @defgroup Enesim_Image_Load_Save_Group Image Loading and Saving
 * @{
 */

EAPI Eina_Bool enesim_image_info_load(Enesim_Image_Data *data, const char *mime,
		int *w, int *h, Enesim_Buffer_Format *sfmt);
EAPI Eina_Bool enesim_image_load(Enesim_Image_Data *data, const char *mime,
		Enesim_Surface **s, Enesim_Format f,
		Enesim_Pool *mpool, const char *options);
EAPI void enesim_image_load_async(Enesim_Image_Data *data, const char *mime,
		Enesim_Surface *s, Enesim_Format f, Enesim_Pool *mpool,
		Enesim_Image_Callback cb, void *user_data, const char *options);
EAPI Eina_Bool enesim_image_save(Enesim_Image_Data *data, const char *mime,
		Enesim_Surface *s, const char *options);
EAPI void enesim_image_save_async(Enesim_Image_Data *data, const char *mime,
		Enesim_Surface *s, Enesim_Image_Callback cb, void *user_data,
		const char *options);

EAPI Eina_Bool enesim_image_file_info_load(const char *file, int *w, int *h, Enesim_Buffer_Format *sfmt);
EAPI Eina_Bool enesim_image_file_load(const char *file, Enesim_Surface **s,
		Enesim_Format f, Enesim_Pool *mpool, const char *options);
EAPI void enesim_image_file_load_async(const char *file, Enesim_Surface *s,
		Enesim_Format f, Enesim_Pool *mpool,
		Enesim_Image_Callback cb, void *user_data, const char *options);
EAPI Eina_Bool enesim_image_file_save(const char *file, Enesim_Surface *s, const char *options);
EAPI void enesim_image_file_save_async(const char *file, Enesim_Surface *s, Enesim_Image_Callback cb,
		void *user_data, const char *options);

/**
 * @}
 * @defgroup Enesim_Image_Provider_Group Providers
 * @{
 */

/* TODO replace this with a priority system */
typedef enum _Enesim_Image_Provider_Type
{
	ENESIM_IMAGE_PROVIDER_SW,
	ENESIM_IMAGE_PROVIDER_HW,
} Enesim_Image_Provider_Type;

/**
 * TODO Add a way to parse options and receive options from caller
 */
typedef struct _Enesim_Image_Provider
{
	const char *name;
	Enesim_Image_Provider_Type type;
	void * (*options_parse)(const char *options);
	void (*options_free)(void *options);
	/* TODO also pass the backend, pool and desired format? */
	Eina_Bool (*loadable)(Enesim_Image_Data *data);
	Eina_Bool (*saveable)(const char *file); /* FIXME fix the arg */
	Eina_Error (*info_get)(Enesim_Image_Data *data, int *w, int *h, Enesim_Buffer_Format *sfmt, void *options);
	Eina_Error (*load)(Enesim_Image_Data *data, Enesim_Buffer *b, void *options);
	Eina_Bool (*save)(Enesim_Image_Data *data, Enesim_Surface *s, void *options);
} Enesim_Image_Provider;


EAPI Eina_Bool enesim_image_provider_register(Enesim_Image_Provider *p, const char *mime);
EAPI void enesim_image_provider_unregister(Enesim_Image_Provider *p, const char *mime);

EAPI Eina_Bool enesim_image_provider_info_load(Enesim_Image_Provider *thiz,
	Enesim_Image_Data *data, int *w, int *h, Enesim_Buffer_Format *sfmt);
EAPI Eina_Bool enesim_image_provider_load(Enesim_Image_Provider *thiz,
		Enesim_Image_Data *data, Enesim_Surface **s,
		Enesim_Format f, Enesim_Pool *mpool, const char *options);
EAPI Eina_Bool enesim_image_provider_save(Enesim_Image_Provider *thiz,
		Enesim_Image_Data *data, Enesim_Surface *s,
		const char *options);

/**
 * @}
 * @defgroup Enesim_Image_Finder_Group Finder
 * @{
 */

typedef struct _Enesim_Image_Finder
{
	const char *(*data_from)(Enesim_Image_Data *data);
	const char *(*extension_from)(const char *ext);
} Enesim_Image_Finder;

EAPI Eina_Bool enesim_image_finder_register(Enesim_Image_Finder *f);
EAPI void enesim_image_finder_unregister(Enesim_Image_Finder *f);

/**
 * @}
 * @defgroup Enesim_Image_Misc_Group Misc
 * @{
 */

typedef void (*Enesim_Image_Option_Cb)(void *data, const char *key, const char *value);
EAPI void enesim_image_options_parse(const char *options, Enesim_Image_Option_Cb cb, void *data);

EAPI const char * enesim_image_mime_data_from(Enesim_Image_Data *data);
EAPI const char * enesim_image_mime_extension_from(const char *ext);

/**
 * @}
 */

#endif
