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
#include "Enesim.h"
#include "enesim_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static int _enesim_init_count = 0;
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
int enesim_log_dom_global = -1;
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

	enesim_log_dom_global = eina_log_domain_register("enesim", ENESIM_DEFAULT_LOG_COLOR);
	if (enesim_log_dom_global < 0)
	{
		EINA_LOG_ERR("Enesim Can not create a general log domain.");
		goto shutdown_eina;
	}

	/* TODO Dump the information about SIMD extensions
	 * get the cpuid for this
	 */
	enesim_compositor_init();
	enesim_renderer_init();
	enesim_converter_init();
#ifdef EFL_HAVE_MMX
	/* EINA_ERROR_PINFO("MMX Drawer available\n"); */
#endif
#ifdef EFL_HAVE_SSE2
	/* EINA_ERROR_PINFO("SSE2 Drawer available\n"); */
#endif
	/* FIXME for some reason we are having several fp exaceptions
	 * better disable the inexact case always
	 */
	feenableexcept(FE_DIVBYZERO | FE_INVALID);

	return _enesim_init_count;

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
	eina_log_domain_unregister(enesim_log_dom_global);
	enesim_log_dom_global = -1;
	eina_shutdown();

	return _enesim_init_count;
}

/**
 * @brief Retrieve the Enesim version.
 *
 * @param major The major version number.
 * @param minor The minor version number.
 * @param micro The micro version number.
 *
 * This function returns the Enesim version in @p major, @p minor and
 * @p micro. These pointers can be NULL.
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
 * @param f The format to get the name from.
 * @return The name of the format.
 *
 * This function returns a string associated to the format @f, for
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
 * @param f The format.
 * @param w The width.
 * @param h The height.
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
 * @param f The format.
 * @param w The width.
 * @return The pitch in byte.
 *
 * This function returns the pitch in byte of a bitmap of format @p f and
 * width @p w. If the format is not valid or is not supported, 0 is
 * returned.
 *
 * @note This function calls enesim_format_size_get() with 1 as height.
 */
EAPI size_t enesim_format_pitch_get(Enesim_Format fmt, uint32_t w)
{
	return enesim_format_size_get(fmt, w, 1);
}
