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
#ifndef _ENESIM_PRIVATE_H
#define _ENESIM_PRIVATE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>

#ifdef WIN32
# ifndef WIN32_LEAN_AND_MEAN
#  define WIN32_LEAN_AND_MEAN
# endif
# include <windows.h>
# undef WIN32_LEAN_AND_MEAN
#else
/* we need to include this pthreads headers before eina */
# ifdef BUILD_PTHREAD
#  ifdef HAVE_SCHED_H
#   include <sched.h>
#  endif
#  ifdef HAVE_PTHREAD_H
#   include <pthread.h>
#  endif
#  ifdef HAVE_PTHREAD_NP_H
#   include <pthread_np.h>
#  endif
#  ifdef HAVE_SYS_PARAM_H
#   include <sys/param.h>
#  endif
#  ifdef HAVE_SYS_CPUSET_H
#   include <sys/cpuset.h>
#  endif
# endif
#endif

/* dependencies */
#include <Eina.h>

#ifdef BUILD_OPENGL
#include "GL/glew.h"
#endif

#ifndef MIN
#define MIN(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef MAX
#define MAX(a,b) (((a) > (b)) ? (a) : (b))
#endif

#include <math.h>

/* the log domains */
extern int enesim_log_global;
extern int enesim_log_image;
extern int enesim_log_text;
extern int enesim_log_pool;
extern int enesim_log_surface;
extern int enesim_log_buffer;
extern int enesim_log_renderer;
extern int enesim_log_renderer_image;
extern int enesim_log_renderer_compound;
extern int enesim_log_renderer_pattern;
extern int enesim_log_renderer_shape;
extern int enesim_log_renderer_gradient;
extern int enesim_log_renderer_gradient_radial;

#ifdef ENESIM_DEFAULT_LOG_COLOR
# undef ENESIM_DEFAULT_LOG_COLOR
#endif
#define ENESIM_DEFAULT_LOG_COLOR EINA_COLOR_LIGHTRED

#define CRI(...) EINA_LOG_DOM_CRIT(ENESIM_LOG_DEFAULT, __VA_ARGS__)
#define ERR(...) EINA_LOG_DOM_ERR(ENESIM_LOG_DEFAULT, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(ENESIM_LOG_DEFAULT, __VA_ARGS__)
#define INF(...) EINA_LOG_DOM_INFO(ENESIM_LOG_DEFAULT, __VA_ARGS__)
#define DBG(...) EINA_LOG_DOM_DBG(ENESIM_LOG_DEFAULT, __VA_ARGS__)

#ifdef EAPI
# undef EAPI
#endif

#ifdef _WIN32
# ifdef ENESIM_BUILD
#  ifdef DLL_EXPORT
#   define EAPI __declspec(dllexport)
#  else
#   define EAPI
#  endif /* ! DLL_EXPORT */
# else
#  define EAPI __declspec(dllimport)
# endif /* ! ENESIM_BUILD */
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

#endif
