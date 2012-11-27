#ifndef EMAGE_H
#define EMAGE_H

#include <Eina.h>
#include <Enesim.h>

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
# ifdef EMAGE_BUILD
#  ifdef DLL_EXPORT
#   define EAPI __declspec(dllexport)
#  else
#   define EAPI
#  endif /* ! DLL_EXPORT */
# else
#  define EAPI __declspec(dllimport)
# endif /* ! EFL_ESVG_BUILD */
#else
# ifdef __GNUC__
#  if __GNUC__ >= 4
#   define EAPI __attribute__ ((visibility("default")))
#  else
#   define EAPI
#  endif
# else
#  define EAPI
# endif
#endif

/**
 * @mainpage Emage
 * @section intro Introduction
 * Emage is a library that loads and saves several types of image formats
 * from and to Enesim Surfaces
 * @section dependencies Dependencies
 * - Enesim
 * - Eina
 */

EAPI extern Eina_Error EMAGE_ERROR_EXIST;
EAPI extern Eina_Error EMAGE_ERROR_PROVIDER;
EAPI extern Eina_Error EMAGE_ERROR_FORMAT;
EAPI extern Eina_Error EMAGE_ERROR_SIZE;
EAPI extern Eina_Error EMAGE_ERROR_ALLOCATOR;
EAPI extern Eina_Error EMAGE_ERROR_LOADING;
EAPI extern Eina_Error EMAGE_ERROR_SAVING;

/*
 * @defgroup Emage_Main_Group Main
 * @{
 */
EAPI int emage_init(void);
EAPI int emage_shutdown(void);
EAPI void emage_dispatch(void);

/**
 * @}
 * @defgroup Emage_Data_Group Data
 * @{
 */
typedef ssize_t (*Emage_Data_Read)(void *data, void *buffer, size_t len);
typedef ssize_t (*Emage_Data_Write)(void *data, void *buffer, size_t len);
typedef void * (*Emage_Data_Mmap)(void *data, size_t *size);
typedef void (*Emage_Data_Munmap)(void *data, void *ptr);
typedef void (*Emage_Data_Reset)(void *data);
typedef size_t (*Emage_Data_Length)(void *data);
typedef char * (*Emage_Data_Location)(void *data);
typedef void (*Emage_Data_Free)(void *data);

typedef struct _Emage_Data_Descriptor
{
	Emage_Data_Read read;
	Emage_Data_Write write;
	Emage_Data_Mmap mmap;
	Emage_Data_Munmap munmap;
	Emage_Data_Reset reset;
	Emage_Data_Length length;
	Emage_Data_Location location;
	Emage_Data_Free free;
} Emage_Data_Descriptor;

typedef struct _Emage_Data Emage_Data;
EAPI Emage_Data * emage_data_new(Emage_Data_Descriptor *d, void *data);
EAPI ssize_t emage_data_read(Emage_Data *thiz, void *buffer, size_t len);
EAPI ssize_t emage_data_write(Emage_Data *thiz, void *buffer, size_t len);
EAPI size_t emage_data_length(Emage_Data *thiz);
EAPI void * emage_data_mmap(Emage_Data *thiz, size_t *size);
EAPI void emage_data_munmap(Emage_Data *thiz, void *ptr);
EAPI void emage_data_reset(Emage_Data *thiz);
EAPI char * emage_data_location(Emage_Data *thiz);
EAPI void emage_data_free(Emage_Data *thiz);

EAPI Emage_Data * emage_data_file_new(const char *file, const char *mode);
EAPI Emage_Data * emage_data_buffer_new(void *buffer, size_t len);
EAPI Emage_Data * emage_data_base64_new(Emage_Data *d);

/**
 * @}
 * @defgroup Emage_Context Context
 * @{
 */
typedef struct _Emage_Context Emage_Context;
typedef void (*Emage_Callback)(Enesim_Surface *s, void *data, int error); /**< Function prototype called whenever an image is loaded or saved */

EAPI Emage_Context * emage_context_new(void);
EAPI void emage_context_free(Emage_Context *thiz);
EAPI void emage_context_load_async(Emage_Context *thiz,
		Emage_Data *data, const char *mime, Enesim_Surface *s,
		Enesim_Format f, Enesim_Pool *mpool, Emage_Callback cb,
		void *user_data, const char *options);
EAPI void emage_context_save_async(Emage_Context *thiz, Emage_Data *data,
		const char *mime, Enesim_Surface *s, Emage_Callback cb,
		void *user_data, const char *options);
EAPI void emage_context_dispatch(Emage_Context *thiz);
EAPI void emage_context_pool_size_set(Emage_Context *thiz, int num);
EAPI int emage_context_pool_size_get(Emage_Context *thiz);

/**
 * @}
 * @defgroup Emage_Load_Save_Group Image Loading and Saving
 * @{
 */

EAPI Eina_Bool emage_info_load(Emage_Data *data, const char *mime,
		int *w, int *h, Enesim_Buffer_Format *sfmt);
EAPI Eina_Bool emage_load(Emage_Data *data, const char *mime,
		Enesim_Surface **s, Enesim_Format f,
		Enesim_Pool *mpool, const char *options);
EAPI void emage_load_async(Emage_Data *data, const char *mime,
		Enesim_Surface *s, Enesim_Format f, Enesim_Pool *mpool,
		Emage_Callback cb, void *user_data, const char *options);
EAPI Eina_Bool emage_save(Emage_Data *data, const char *mime,
		Enesim_Surface *s, const char *options);
EAPI void emage_save_async(Emage_Data *data, const char *mime,
		Enesim_Surface *s, Emage_Callback cb, void *user_data,
		const char *options);

EAPI Eina_Bool emage_file_info_load(const char *file, int *w, int *h, Enesim_Buffer_Format *sfmt);
EAPI Eina_Bool emage_file_load(const char *file, Enesim_Surface **s,
		Enesim_Format f, Enesim_Pool *mpool, const char *options);
EAPI void emage_file_load_async(const char *file, Enesim_Surface *s,
		Enesim_Format f, Enesim_Pool *mpool,
		Emage_Callback cb, void *user_data, const char *options);
EAPI Eina_Bool emage_file_save(const char *file, Enesim_Surface *s, const char *options);
EAPI void emage_file_save_async(const char *file, Enesim_Surface *s, Emage_Callback cb,
		void *user_data, const char *options);

/**
 * @}
 * @defgroup Emage_Provider_Group Providers
 * @{
 */

/* TODO replace this with a priority system */
typedef enum _Emage_Provider_Type
{
	EMAGE_PROVIDER_SW,
	EMAGE_PROVIDER_HW,
} Emage_Provider_Type;

/**
 * TODO Add a way to parse options and receive options from caller
 */
typedef struct _Emage_Provider
{
	const char *name;
	Emage_Provider_Type type;
	void * (*options_parse)(const char *options);
	void (*options_free)(void *options);
	/* TODO also pass the backend, pool and desired format? */
	Eina_Bool (*loadable)(Emage_Data *data);
	Eina_Bool (*saveable)(const char *file); /* FIXME fix the arg */
	Eina_Error (*info_get)(Emage_Data *data, int *w, int *h, Enesim_Buffer_Format *sfmt, void *options);
	Eina_Error (*load)(Emage_Data *data, Enesim_Buffer *b, void *options);
	Eina_Bool (*save)(Emage_Data *data, Enesim_Surface *s, void *options);
} Emage_Provider;


EAPI Eina_Bool emage_provider_register(Emage_Provider *p, const char *mime);
EAPI void emage_provider_unregister(Emage_Provider *p, const char *mime);

EAPI Eina_Bool emage_provider_info_load(Emage_Provider *thiz,
	Emage_Data *data, int *w, int *h, Enesim_Buffer_Format *sfmt);
EAPI Eina_Bool emage_provider_load(Emage_Provider *thiz,
		Emage_Data *data, Enesim_Surface **s,
		Enesim_Format f, Enesim_Pool *mpool, const char *options);
EAPI Eina_Bool emage_provider_save(Emage_Provider *thiz,
		Emage_Data *data, Enesim_Surface *s,
		const char *options);

/**
 * @}
 * @defgroup Emage_Finder_Group Finder
 * @{
 */

typedef struct _Emage_Finder
{
	const char *(*data_from)(Emage_Data *data);
	const char *(*extension_from)(const char *ext);
} Emage_Finder;

EAPI Eina_Bool emage_finder_register(Emage_Finder *f);
EAPI void emage_finder_unregister(Emage_Finder *f);

/**
 * @}
 * @defgroup Emage_Misc_Group Misc
 * @{
 */

typedef void (*Emage_Option_Cb)(void *data, const char *key, const char *value);
EAPI void emage_options_parse(const char *options, Emage_Option_Cb cb, void *data);

EAPI const char * emage_mime_data_from(Emage_Data *data);
EAPI const char * emage_mime_extension_from(const char *ext);

/**
 * @}
 */

#endif
