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

#ifdef _WIN32
# define pipe_write(fd, buffer, size) send((fd), (char *)(buffer), size, 0)
# define pipe_read(fd, buffer, size)  recv((fd), (char *)(buffer), size, 0)
# define pipe_close(fd)               closesocket(fd)
# define ENESIM_IMAGE_THREAD_CREATE(x, f, d) x = CreateThread(NULL, 0, f, d, 0, NULL);
#else
# define pipe_write(fd, buffer, size) write((fd), buffer, size)
# define pipe_read(fd, buffer, size)  read((fd), buffer, size)
# define pipe_close(fd)               close(fd)
# define ENESIM_IMAGE_THREAD_CREATE(x, f, d) pthread_create(&(x), NULL, (void *)f, d)
#endif /* ! _WIN32 */

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_image

struct _Enesim_Image_Context
{
	/* the communication between the main thread and the async ones */
	int fifo[2];
#ifdef _WIN32
	HANDLE tid;
#else
	pthread_t tid;
#endif
};

typedef enum _Enesim_Image_Job_Type
{
	ENESIM_IMAGE_LOAD,
	ENESIM_IMAGE_SAVE,
	ENESIM_IMAGE_JOB_TYPES,
} Enesim_Image_Job_Type;

typedef struct _Enesim_Image_Job
{
	Enesim_Image_Context *thiz;
	Enesim_Image_Provider *prov;
	Enesim_Image_Data *data;
	Enesim_Image_Callback cb;
	void *user_data;
	Eina_Error err;
	Enesim_Image_Job_Type type;
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
} Enesim_Image_Job;

/*----------------------------------------------------------------------------*
 *                        Thread related functions                            *
 *----------------------------------------------------------------------------*/
static void _thread_finish(Enesim_Image_Job *j)
{
	int ret;
	ret = pipe_write(j->thiz->fifo[1], &j, sizeof(j));
}

#ifdef _WIN32
static DWORD WINAPI _thread_load(LPVOID data)
#else
static void * _thread_load(void *data)
#endif
{
	Enesim_Image_Job *j = data;

	if (!enesim_image_provider_load(j->prov, j->data, &j->op.load.s,
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
	Enesim_Image_Job *j = data;

	if (!enesim_image_provider_save(j->prov, j->data, j->op.save.s, j->options))
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
 * @brief Create a new context
 *
 * Create a new context. A context is the holder of every asynchronous
 * operation done on emage. 
 */
EAPI Enesim_Image_Context * enesim_image_context_new(void)
{
	Enesim_Image_Context *thiz;

	thiz = calloc(1, sizeof(Enesim_Image_Context));
	/* the fifo */
	if (pipe(thiz->fifo) < 0)
	{
		ERR("can not create pipe");
		free(thiz);
		return NULL;
	}

	fcntl(thiz->fifo[0], F_SETFL, O_NONBLOCK);
	/* TODO the pool of threads */
	return thiz;
}
	
/**
 * @brief Free a context
 *
 */
EAPI void enesim_image_context_free(Enesim_Image_Context *thiz)
{
	/* TODO what if we shutdown while some thread is active? */
	/* the fifo */
	pipe_close(thiz->fifo[0]);
	pipe_close(thiz->fifo[1]);
	free(thiz);
}

/**
 * Load an image asynchronously
 *
 * @param thiz The context to use for loading
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
EAPI void enesim_image_context_load_async(Enesim_Image_Context *thiz, Enesim_Image_Data *data,
		const char *mime, Enesim_Surface *s, Enesim_Format f,
		Enesim_Pool *mpool, Enesim_Image_Callback cb, void *user_data,
		const char *options)
{
	Enesim_Image_Job *j;
	Enesim_Image_Provider *prov;

	prov = enesim_image_load_provider_get(data, mime);
	if (!prov)
	{
		cb(NULL, user_data, ENESIM_IMAGE_ERROR_PROVIDER);
		return;
	}

	j = calloc(1, sizeof(Enesim_Image_Job));
	j->thiz = thiz;
	j->prov = prov;
	j->data = data;
	j->cb = cb;
	j->user_data = user_data;
	if (options)
		j->options = strdup(options);
	j->err = 0;
	j->type = ENESIM_IMAGE_LOAD;
	j->op.load.s = s;
	j->op.load.pool = mpool;
	j->op.load.f = f;
	/* FIXME we need to block the thread */
	/* create a thread that loads the image on background and sends
	 * a command into the fifo fd */
	ENESIM_IMAGE_THREAD_CREATE(thiz->tid, _thread_load, j);
}

/**
 * Save an image asynchronously
 *
 * @param thiz The context to use for loading
 * @param data The image data to load
 * @param mime The image mime
 * @param s The surface to read the image pixels from. It must not be NULL.
 * @param cb The function that will get called once the save is done
 * @param data User provided data
 * @param options Any option the emage provider might require
 *
 */
EAPI void enesim_image_context_save_async(Enesim_Image_Context *thiz, Enesim_Image_Data *data,
		const char *mime, Enesim_Surface *s, Enesim_Image_Callback cb,
		void *user_data, const char *options)
{
	Enesim_Image_Job *j;
	Enesim_Image_Provider *prov;

	prov = enesim_image_save_provider_get(data, mime);
	if (!prov)
	{
		cb(NULL, user_data, ENESIM_IMAGE_ERROR_PROVIDER);
		return;
	}

	j = malloc(sizeof(Enesim_Image_Job));
	j->thiz = thiz;
	j->prov = prov;
	j->data = data;
	j->cb = cb;
	j->user_data = user_data;
	if (options)
		j->options = strdup(options);
	j->err = 0;
	j->type = ENESIM_IMAGE_SAVE;
	j->op.save.s = s;
	/* FIXME we need to block the thread */
	/* create a thread that saves the image on background and sends
	 * a command into the fifo fd */
	ENESIM_IMAGE_THREAD_CREATE(thiz->tid, _thread_save, j);
}

/**
 * @brief Call every asynchronous callback set
 *
 * In case emage has setup some asynchronous load, you must call this
 * function to get the status of such process
 *
 * @param thiz The context to dispatch
 */
EAPI void enesim_image_context_dispatch(Enesim_Image_Context *thiz)
{
	fd_set readset;
	struct timeval t;
	Enesim_Image_Job *j;

	/* check if there's data to read */
	FD_ZERO(&readset);
	FD_SET(thiz->fifo[0], &readset);
	t.tv_sec = 0;
	t.tv_usec = 0;

	if (select(thiz->fifo[0] + 1, &readset, NULL, NULL, &t) <= 0)
		return;
	/* read from the fifo fd and call the needed callbacks */
	while (pipe_read(thiz->fifo[0], &j, sizeof(j)) > 0)
	{
		if (j->type == ENESIM_IMAGE_LOAD)
			j->cb(j->op.load.s, j->user_data, j->err);
		else
			j->cb(j->op.save.s, j->user_data, j->err);
		if (j->options)
			free(j->options);
		free(j);
	}
}

/**
 * @brief Sets the size of the thread's pool
 * @param thiz The image context
 * @param num The number of threads
 *
 * Sets the maximum number of threads the context will create to dispatch asynchronous
 * calls.
 */
EAPI void enesim_image_context_pool_size_set(Enesim_Image_Context *thiz, int num)
{

}
/**
 * @brief Gets the size of the thread's pool
 * @param thiz The image context
 *
 * @return The number of threads
 * Returns the maximum number threads the context will create the dispatch
 * asynchronous calls.
 */
EAPI int enesim_image_context_pool_size_get(Enesim_Image_Context *thiz)
{
	return 0;
}
