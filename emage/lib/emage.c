#include "Emage.h"
#include "emage_private.h"

#ifdef _WIN32
# define pipe_write(fd, buffer, size) send((fd), (char *)(buffer), size, 0)
# define pipe_read(fd, buffer, size)  recv((fd), (char *)(buffer), size, 0)
# define pipe_close(fd)               closesocket(fd)
#else
# define pipe_write(fd, buffer, size) write((fd), buffer, size)
# define pipe_read(fd, buffer, size)  read((fd), buffer, size)
# define pipe_close(fd)               close(fd)
#endif /* ! _WIN32 */

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/

#define EMAGE_LOG_COLOR_DEFAULT EINA_COLOR_GREEN

static int _emage_init_count = 0;
static Eina_Array *_modules = NULL;
static Eina_Hash *_providers = NULL;
static Eina_List *_finders = NULL;
static int _fifo[2]; /* the communication between the main thread and the async ones */
static int emage_log_dom_global = -1;

Eina_Error EMAGE_ERROR_EXIST;
Eina_Error EMAGE_ERROR_PROVIDER;
Eina_Error EMAGE_ERROR_FORMAT;
Eina_Error EMAGE_ERROR_SIZE;
Eina_Error EMAGE_ERROR_ALLOCATOR;
Eina_Error EMAGE_ERROR_LOADING;
Eina_Error EMAGE_ERROR_SAVING;

#ifdef _WIN32
static HANDLE tid;
# define EMAGE_THREAD_CREATE(x, f, d) x = CreateThread(NULL, 0, f, d, 0, NULL);
#else
static pthread_t tid;
# define EMAGE_THREAD_CREATE(x, f, d) pthread_create(&(x), NULL, (void *)f, d)
#endif

typedef enum _Emage_Job_Type
{
	EMAGE_LOAD,
	EMAGE_SAVE,
	EMAGE_JOB_TYPES,
} Emage_Job_Type;

typedef struct _Emage_Job
{
	Emage_Provider *prov;
	Emage_Data *data;
	Emage_Callback cb;
	void *user_data;
	Eina_Error err;
	Emage_Job_Type type;
	char *options;

	union {
		struct {
			Enesim_Surface *s;
			Enesim_Format f;
			Enesim_Pool *pool;
		} load;
		struct {
			Enesim_Surface *s;
		} save;
	} op;
} Emage_Job;

static Emage_Provider * _load_provider_get(Emage_Data *data, const char *mime)
{
	Emage_Provider *p;
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

		emage_data_reset(data);
		if (p->loadable(data) == EINA_TRUE)
		{
			emage_data_reset(data);
			return p;
		}
	}
	return NULL;
}

static Emage_Provider * _save_provider_get(Emage_Data *data, const char *mime)
{
	Emage_Provider *p;
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
			emage_data_reset(data);
			return p;
		}
#endif
	}
	return NULL;
}

static void _thread_finish(Emage_Job *j)
{
	int ret;
	ret = pipe_write(_fifo[1], &j, sizeof(j));
}

#ifdef _WIN32
static DWORD WINAPI _thread_load(LPVOID data)
#else
static void * _thread_load(void *data)
#endif
{
	Emage_Job *j = data;

	if (!emage_provider_load(j->prov, j->data, &j->op.load.s,
		j->op.load.f, j->op.load.pool, j->options))
		j->err = eina_error_get();
	_thread_finish(j);

#ifdef _WIN32
	return 1;
#else
	return NULL;
#endif
}

#ifdef _WIN32
static DWORD WINAPI _thread_save(LPVOID data)
#else
static void * _thread_save(void *data)
#endif
{
	Emage_Job *j = data;

	if (!emage_provider_save(j->prov, j->data, j->op.save.s, j->options))
		j->err = eina_error_get();
	_thread_finish(j);

#ifdef _WIN32
	return 1;
#else
	return NULL;
#endif
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * Initialize emage. This function must be called before any other emage
 * function
 */
EAPI int emage_init(void)
{
	if (++_emage_init_count != 1)
		return _emage_init_count;

	if (!eina_init())
	{
		fprintf(stderr, "Emage: Eina init failed");
		return --_emage_init_count;
	}

	emage_log_dom_global = eina_log_domain_register("emage", EMAGE_LOG_COLOR_DEFAULT);
	if (emage_log_dom_global < 0)
	{
		EINA_LOG_ERR("Emage: Can not create a general log domain.");
		goto shutdown_eina;
	}

	if (!enesim_init())
	{
		ERR("Enesim init failed");
		goto unregister_log_domain;
	}

	/* the fifo */
	if (pipe(_fifo) < 0)
	{
		ERR("can not create pipe");
		goto shutdown_enesim;
	}

	fcntl(_fifo[0], F_SETFL, O_NONBLOCK);
	/* the errors */
	EMAGE_ERROR_EXIST = eina_error_msg_static_register("Files does not exist");
	EMAGE_ERROR_PROVIDER = eina_error_msg_static_register("No provider for such file");
	EMAGE_ERROR_FORMAT = eina_error_msg_static_register("Wrong surface format");
	EMAGE_ERROR_SIZE = eina_error_msg_static_register("Size mismatch");
	EMAGE_ERROR_ALLOCATOR = eina_error_msg_static_register("Error allocating the surface data");
	EMAGE_ERROR_LOADING = eina_error_msg_static_register("Error loading the image");
	EMAGE_ERROR_SAVING = eina_error_msg_static_register("Error saving the image");
	/* the providers */
	_providers = eina_hash_string_superfast_new(NULL);
	/* the modules */
	_modules = eina_module_list_get(_modules, PACKAGE_LIB_DIR"/emage/", 1, NULL, NULL);
	eina_module_list_load(_modules);
#if BUILD_STATIC_MODULE_PNG
	png_provider_init();
#endif
	/* TODO the pool of threads */

	return _emage_init_count;

  shutdown_enesim:
	enesim_shutdown();
  unregister_log_domain:
	eina_log_domain_unregister(emage_log_dom_global);
	emage_log_dom_global = -1;
  shutdown_eina:
	eina_shutdown();
	return --_emage_init_count;
}
/**
 * Shutdown emage library. Once you have finished using emage, shut it down.
 */
EAPI int emage_shutdown(void)
{
	if (--_emage_init_count != 0)
		return _emage_init_count;

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
	/* TODO what if we shutdown while some thread is active? */
	/* the fifo */
	pipe_close(_fifo[0]);
	pipe_close(_fifo[1]);
	enesim_shutdown();
	eina_log_domain_unregister(emage_log_dom_global);
	emage_log_dom_global = -1;
	eina_shutdown();

	return _emage_init_count;
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
EAPI Eina_Bool emage_info_load(Emage_Data *data, const char *mime,
		int *w, int *h, Enesim_Buffer_Format *sfmt)
{
	Emage_Provider *prov;

	prov = _load_provider_get(data, mime);
	return emage_provider_info_load(prov, data, w, h, sfmt);
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
EAPI Eina_Bool emage_load(Emage_Data *data, const char *mime,
		Enesim_Surface **s, Enesim_Format f, Enesim_Pool *mpool,
		const char *options)
{
	Emage_Provider *prov;

	prov = _load_provider_get(data, mime);
	return emage_provider_load(prov, data, s, f, mpool, options);
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
EAPI void emage_load_async(Emage_Data *data, const char *mime,
		Enesim_Surface *s, Enesim_Format f, Enesim_Pool *mpool,
		Emage_Callback cb, void *user_data, const char *options)
{
	Emage_Job *j;
	Emage_Provider *prov;

	prov = _load_provider_get(data, mime);
	if (!prov)
	{
		cb(NULL, user_data, EMAGE_ERROR_PROVIDER);
		return;
	}

	j = calloc(1, sizeof(Emage_Job));
	j->prov = prov;
	j->data = data;
	j->cb = cb;
	j->user_data = user_data;
	if (options)
		j->options = strdup(options);
	j->err = 0;
	j->type = EMAGE_LOAD;
	j->op.load.s = s;
	j->op.load.pool = mpool;
	j->op.load.f = f;
	/* create a thread that loads the image on background and sends
	 * a command into the fifo fd */
	EMAGE_THREAD_CREATE(tid, _thread_load, j);
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
EAPI Eina_Bool emage_save(Emage_Data *data, const char *mime,
		Enesim_Surface *s, const char *options)
{
	Emage_Provider *prov;

	prov = _save_provider_get(data, mime);
	return emage_provider_save(prov, data, s, options);
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
EAPI void emage_save_async(Emage_Data *data, const char *mime,
		Enesim_Surface *s, Emage_Callback cb,
		void *user_data, const char *options)
{
	Emage_Job *j;
	Emage_Provider *prov;

	prov = _save_provider_get(data, mime);
	if (!prov)
	{
		cb(NULL, user_data, EMAGE_ERROR_PROVIDER);
		return;
	}

	j = malloc(sizeof(Emage_Job));
	j->prov = prov;
	j->data = data;
	j->cb = cb;
	j->user_data = user_data;
	if (options)
		j->options = strdup(options);
	j->err = 0;
	j->type = EMAGE_SAVE;
	j->op.save.s = s;
	/* create a thread that saves the image on background and sends
	 * a command into the fifo fd */
	EMAGE_THREAD_CREATE(tid, _thread_save, j);
}

/**
 * @brief Call every asynchronous callback set
 *
 * In case emage has setup some asynchronous load, you must call this
 * function to get the status of such process
 */
EAPI void emage_dispatch(void)
{
	fd_set readset;
	struct timeval t;
	Emage_Job *j;

	/* check if there's data to read */
	FD_ZERO(&readset);
	FD_SET(_fifo[0], &readset);
	t.tv_sec = 0;
	t.tv_usec = 0;

	if (select(_fifo[0] + 1, &readset, NULL, NULL, &t) <= 0)
		return;
	/* read from the fifo fd and call the needed callbacks */
	while (pipe_read(_fifo[0], &j, sizeof(j)) > 0)
	{
		if (j->type == EMAGE_LOAD)
			j->cb(j->op.load.s, j->user_data, j->err);
		else
			j->cb(j->op.save.s, j->user_data, j->err);
		if (j->options)
			free(j->options);
		free(j);
	}
}

/**
 *
 */
EAPI Eina_Bool emage_provider_register(Emage_Provider *p, const char *mime)
{
	Eina_List *providers;
	Eina_List *tmp;

	if (!p)
		return EINA_FALSE;
	/* check for mandatory functions */
	if (!p->info_get)
	{
		WRN("Provider %s doesn't provide the info_get() function", p->name);
		goto err;
	}
	if (!p->load)
	{
		WRN("Provider %s doesn't provide the load() function", p->name);
		goto err;
	}
	providers = tmp = eina_hash_find(_providers, mime);
	providers = eina_list_append(providers, p);
	if (!tmp)
	{
		eina_hash_add(_providers, mime, providers);
	}
	return EINA_TRUE;
err:
	return EINA_FALSE;
}

/**
 *
 */
EAPI const char * emage_mime_data_from(Emage_Data *data)
{
	Emage_Finder *f;
	Eina_List *l;
	const char *ret = NULL;

	EINA_LIST_FOREACH(_finders, l, f)
	{
		emage_data_reset(data);
		ret = f->data_from(data);
		if (ret) break;
	}
	return ret;
}

/**
 *
 */
EAPI const char * emage_mime_extension_from(const char *ext)
{
	Emage_Finder *f;
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
EAPI Eina_Bool emage_finder_register(Emage_Finder *f)
{
	if (!f) return EINA_FALSE;

	if (!f->data_from)
	{
		WRN("Finder %p doesn't provide the 'data_from()' function", f);
		return EINA_FALSE;
	}
	if (!f->extension_from)
	{
		WRN("Finder %p doesn't provide the 'extension_from()' function", f);
		return EINA_FALSE;
	}
	_finders = eina_list_append(_finders, f);

	return EINA_TRUE;
}

EAPI void emage_finder_unregister(Emage_Finder *f)
{
	if (!f) return;
	_finders = eina_list_remove(_finders, f);
}

/**
 *
 */
EAPI void emage_provider_unregister(Emage_Provider *p, const char *mime)
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
EAPI void emage_options_parse(const char *options, Emage_Option_Cb cb, void *data)
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


/**
 * @brief Sets the size of the thread's pool
 * @param num The number of threads
 *
 * Sets the maximum number of threads Emage will create to dispatch asynchronous
 * calls.
 */
EAPI void emage_pool_size_set(int num)
{

}
/**
 * @brief Gets the size of the thread's pool
 *
 * @return The number of threads
 * Returns the maximum number threads of number Emage will create the dispatch
 * asynchronous calls.
 */
EAPI int emage_pool_size_get(void)
{
	return 0;
}
