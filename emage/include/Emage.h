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
 * @defgroup Emage_Load_Save_Group Image Loading and Saving
 * @{
 */
typedef void (*Emage_Callback)(Enesim_Surface *s, void *data, int error); /**< Function prototype called whenever an image is loaded or saved */

EAPI Eina_Bool emage_info_load(const char *file, int *w, int *h,
		Enesim_Buffer_Format *sfmt);
EAPI Eina_Bool emage_load(const char *file, Enesim_Surface **s,
		Enesim_Format f, Enesim_Pool *mpool, const char *options);
EAPI void emage_load_async(const char *file, Enesim_Surface *s,
		Enesim_Format f, Enesim_Pool *mpool,
		Emage_Callback cb, void *data, const char *options);
EAPI Eina_Bool emage_save(const char *file, Enesim_Surface *s,
		const char *options);
EAPI void emage_save_async(const char *file, Enesim_Surface *s,
		Emage_Callback cb, void *data, const char *options);
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
	Eina_Bool (*loadable)(const char *file);
	Eina_Bool (*saveable)(const char *file);
	Eina_Error (*info_get)(const char *file, int *w, int *h, Enesim_Buffer_Format *sfmt, void *options);
	Eina_Error (*load)(const char *file, Enesim_Buffer *b, void *options);
	Eina_Bool (*save)(const char *file, Enesim_Surface *s, void *options);
} Emage_Provider;


EAPI Eina_Bool emage_provider_register(Emage_Provider *);
EAPI void emage_provider_unregister(Emage_Provider *);

EAPI Eina_Bool emage_provider_info_load(Emage_Provider *thiz,
	const char *file, int *w, int *h, Enesim_Buffer_Format *sfmt);
EAPI Eina_Bool emage_provider_load(Emage_Provider *thiz,
		const char *file, Enesim_Surface **s,
		Enesim_Format f, Enesim_Pool *mpool, const char *options);
EAPI Eina_Bool emage_provider_save(Emage_Provider *thiz,
		const char *file, Enesim_Surface *s,
		const char *options);

/**
 * @}
 * @defgroup Emage_Misc_Group Misc
 * @{
 */

typedef void (*Emage_Option_Cb)(void *data, const char *key, const char *value);
EAPI void emage_options_parse(const char *options, Emage_Option_Cb cb, void *data);

/**
 * @}
 */

typedef struct _Emage_Data Emage_Data;
EAPI Eina_Bool emage_data_file_from(Emage_Data *thiz, const char *file);
EAPI void emage_data_file_free(Emage_Data *thiz);

EAPI void emage_pool_size_set(int num);
EAPI int emage_pool_size_get(void);

#endif
