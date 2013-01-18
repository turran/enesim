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

#define CHECK_FE 0

#if CHECK_FE
#include <fenv.h>
#endif

#include "enesim_main.h"
#include "enesim_error.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_compositor.h"
#include "enesim_renderer.h"
#include "enesim_converter.h"

#include "enesim_compositor_private.h"
#include "enesim_renderer_private.h"
#include "enesim_converter_private.h"
#include "enesim_mempool_aligned_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static int _enesim_init_count = 0;

/* simple struct to initialize all the domains easily */
struct log
{
	int *d;
	const char *name;
};

static struct log logs[] = {
	{ &enesim_log_global,			"enesim" },
	{ &enesim_log_pool,			"enesim_pool" },
	{ &enesim_log_buffer,			"enesim_buffer" },
	{ &enesim_log_surface,			"enesim_surface" },
	{ &enesim_log_renderer,			"enesim_renderer" },
	{ &enesim_log_renderer_image,		"enesim_renderer_image" },
	{ &enesim_log_renderer_compound, 	"enesim_renderer_compound" },
	{ &enesim_log_renderer_pattern, 	"enesim_renderer_pattern" },
	{ &enesim_log_renderer_shape, 		"enesim_renderer_shape" },
	{ &enesim_log_renderer_gradient, 	"enesim_renderer_gradient" },
	{ &enesim_log_renderer_gradient_radial,	"enesim_renderer_gradient_radial" },
};

static Eina_Bool _register_domains(void)
{
	int count;
	int i;

	/* register every domain here */
	count = sizeof(logs)/sizeof(struct log);
	for (i = 0; i < count; i++)
	{
		/* FIXME something is wrong, is not using the correct the pointer ...? */
		*logs[i].d = eina_log_domain_register(logs[i].name, ENESIM_DEFAULT_LOG_COLOR);
		if (*logs[i].d < 0)
		{
			fprintf(stderr, "Enesim: Can not create domaing log '%s'", logs[i].name);
			goto log_error;
		}
	}
	return EINA_TRUE;
log_error:
	for (; i >= 0; i--)
	{
		struct log *l = &logs[i];
		eina_log_domain_unregister(*l->d);
	}
	return EINA_FALSE;
}

static void _unregister_domains(void)
{
	int count;
	int i;

	/* remove every log */
	count = sizeof(logs)/sizeof(struct log);
	for (i = 0; i < count; i++)
	{
		struct log *l = &logs[i];
		eina_log_domain_unregister(*l->d);
	}

}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
int enesim_log_global = -1;
int enesim_log_pool = -1;
int enesim_log_surface = -1;
int enesim_log_buffer = -1;
int enesim_log_renderer = -1;
int enesim_log_renderer_image = -1;
int enesim_log_renderer_compound = -1;
int enesim_log_renderer_pattern = -1;
int enesim_log_renderer_shape = -1;
int enesim_log_renderer_gradient = -1;
int enesim_log_renderer_gradient_radial = -1;
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * @brief Initialize the Enesim library
 *
 * @return 1 or greater on success, 0 on error.
 *
 * This function must be called before calling any other function of
 * Enesim. It sets up all the Enesim components. It returns 0 on
 * failure, otherwise it returns the number of times it has already
 * been called.
 *
 * When Enesim is not used anymore, call enesim_shutdown() to shut down
 * the Enesim library.
 */
EAPI int enesim_init(void)
{
	if (++_enesim_init_count != 1)
		return _enesim_init_count;

	if (!eina_init())
	{
		fprintf(stderr, "Enesim: Eina init failed");
		return --_enesim_init_count;
	}
	if (!eina_threads_init())
	{
		fprintf(stderr, "Enesim: Eina Threads init failed");
		goto shutdown_eina;
	}
	if (!_register_domains())
	{
		EINA_LOG_ERR("Enesim Can not create the log domains.");
		goto shutdown_eina_threads;
	}
	/* TODO Dump the information about SIMD extensions
	 * get the cpuid for this
	 */
	enesim_mempool_aligned_init();
	enesim_mempool_buddy_init();
	enesim_compositor_init();
	enesim_renderer_init();
	enesim_converter_init();
#ifdef EFL_HAVE_MMX
	/* EINA_ERROR_PINFO("MMX Drawer available\n"); */
#endif
#ifdef EFL_HAVE_SSE2
	/* EINA_ERROR_PINFO("SSE2 Drawer available\n"); */
#endif
#if CHECK_FE
	/* FIXME for some reason we are having several fp exaceptions
	 * better disable the inexact case always
	 */
	feenableexcept(FE_DIVBYZERO | FE_INVALID);
#endif
	return _enesim_init_count;

shutdown_eina_threads:
	eina_threads_shutdown();
shutdown_eina:
	eina_shutdown();
	return --_enesim_init_count;
}

/**
 * @brief Shut down the Enesim library.
 *
 * @return 0 when Enesim is shut down, 1 or greater otherwise.
 *
 * This function shuts down the Enesim library. It returns 0 when it has
 * been called the same number of times than enesim_init(). In that case
 * it shuts down all the Enesim components.
 *
 * Once this function succeeds (that is, @c 0 is returned), you must
 * not call any of the Enesim functions anymore. You must call
 * enesim_init() again to use the Enesim functions again.
 */
EAPI int enesim_shutdown(void)
{
	if (--_enesim_init_count != 0)
		return _enesim_init_count;

	enesim_converter_shutdown();
	enesim_renderer_shutdown();
	enesim_compositor_shutdown();
	enesim_mempool_aligned_shutdown();
	enesim_mempool_buddy_shutdown();
	_unregister_domains();
	eina_shutdown();

	return _enesim_init_count;
}

/**
 * @brief Retrieve the Enesim version.
 *
 * @param[out] major The major version number.
 * @param[out] minor The minor version number.
 * @param[out] micro The micro version number.
 *
 * This function returns the Enesim version in @p major, @p minor and
 * @p micro. These buffers can be NULL.
 */
EAPI void enesim_version_get(unsigned int *major, unsigned int *minor, unsigned int *micro)
{
	if (major) *major = VERSION_MAJOR;
	if (minor) *minor = VERSION_MINOR;
	if (micro) *micro = VERSION_MICRO;
}

/**
 * @brief Get the string based name of a format.
 *
 * @param[in] f The format to get the name from.
 * @return The name of the format.
 *
 * This function returns a string associated to the format @p f, for
 * convenience display of the format. If @p f is not a valid format or
 * an unsupported one, @c NULL is returned.
 */
EAPI const char * enesim_format_name_get(Enesim_Format f)
{
	switch (f)
	{
		case ENESIM_FORMAT_ARGB8888:
		return "argb8888";

		case ENESIM_FORMAT_XRGB8888:
		return "xrgb8888";

		case ENESIM_FORMAT_ARGB8888_SPARSE:
		return "argb8888sp";

		case ENESIM_FORMAT_A8:
		return "a8";

		default:
		return NULL;

	}
}

/**
 * @brief Get the total size of bytes for a given a format and a size.
 *
 * @param[in] f The format.
 * @param[in] w The width.
 * @param[in] h The height.
 * @return The size in byte of a bitmap.
 *
 * This function returns the size in byte of a bitmap of format @p f,
 * width @p w and @p height h. If the format is not valid or is not
 * supported, 0 is returned.
 */
EAPI size_t enesim_format_size_get(Enesim_Format f, uint32_t w, uint32_t h)
{
	switch (f)
	{
		case ENESIM_FORMAT_ARGB8888:
		case ENESIM_FORMAT_XRGB8888:
		case ENESIM_FORMAT_ARGB8888_SPARSE:
		return w * h * sizeof(uint32_t);

		case ENESIM_FORMAT_A8:
		return w * h * sizeof(uint8_t);

		default:
		return 0;

	}
}

/**
 * @brief Return the pitch for a given format and width.
 *
 * @param[in] fmt The format.
 * @param[in] w The width.
 * @return The pitch in byte.
 *
 * This function returns the pitch in byte of a bitmap of format @p fmt
 * and width @p w. If the format is not valid or is not supported, 0 is
 * returned.
 *
 * @note This function calls enesim_format_size_get() with 1 as height.
 */
EAPI size_t enesim_format_pitch_get(Enesim_Format fmt, uint32_t w)
{
	return enesim_format_size_get(fmt, w, 1);
}
