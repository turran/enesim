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
#ifndef _ENESIM_PRIVATE_H
#define _ENESIM_PRIVATE_H

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

/* we need to include this pthreads headers before eina */
#ifdef BUILD_PTHREAD
# ifdef HAVE_SCHED_H
#  include <sched.h>
# endif
# ifdef HAVE_PTHREAD_H
#  include <pthread.h>
# endif
# ifdef HAVE_PTHREAD_NP_H
#  include <pthread_np.h>
# endif
# ifdef HAVE_SYS_PARAM_H
#  include <sys/param.h>
# endif
# ifdef HAVE_SYS_CPUSET_H
#  include <sys/cpuset.h>
# endif
#endif

/* the libargb needed macros */
/* SIMD intrinsics */
#ifdef EFL_HAVE_MMX
#define LIBARGB_MMX 1
#else
#define LIBARGB_MMX 0
#endif

#ifdef  EFL_HAVE_SSE
#define LIBARGB_SSE 1
#else
#define LIBARGB_SSE 0
#endif

#ifdef EFL_HAVE_SSE2
#define LIBARGB_SSE2 1
#else
#define LIBARGB_SSE2 0
#endif

#define LIBARGB_DEBUG 0

/* dependencies */
#include "Eina.h"

#ifdef BUILD_OPENGL
#include "GL/glew.h"
#endif

#include <math.h>

/* the log domains */
extern int enesim_log_global;
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

#define ERR(...) EINA_LOG_DOM_ERR(ENESIM_LOG_DEFAULT, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(ENESIM_LOG_DEFAULT, __VA_ARGS__)
#define DBG(...) EINA_LOG_DOM_DBG(ENESIM_LOG_DEFAULT, __VA_ARGS__)

/* define here every renderer magic */
#define ENESIM_RENDERER_BACKGROUND_MAGIC 0xe7e51430
#define ENESIM_RENDERER_CHECKER_MAGIC 0xe7e51431
#define ENESIM_RENDERER_CLIPPER_MAGIC 0xe7e51432
#define ENESIM_RENDERER_COMPOUND_MAGIC 0xe7e51433
#define ENESIM_RENDERER_DISPMAP_MAGIC 0xe7e51434
#define ENESIM_RENDERER_GRADIENT_MAGIC 0xe7e51435
#define ENESIM_RENDERER_GRID_MAGIC 0xe7e51436
#define ENESIM_RENDERER_HSWITCH_MAGIC 0xe7e51437
#define ENESIM_RENDERER_IMAGE_MAGIC 0xe7e51438
#define ENESIM_RENDERER_IMPORTER_MAGIC 0xe7e51438
#define ENESIM_RENDERER_PERLIN_MAGIC 0xe7e51439
#define ENESIM_RENDERER_RADDIST_MAGIC 0xe7e51439
#define ENESIM_RENDERER_SHAPE_MAGIC 0xe7e5143a
#define ENESIM_RENDERER_STRIPES_MAGIC 0xe7e5143b
#define ENESIM_RENDERER_TRANSITION_MAGIC 0xe7e5143c

#define ENESIM_RENDERER_CIRCLE_MAGIC 0xe7e51440
#define ENESIM_RENDERER_RECTANGLE_MAGIC 0xe7e51441
#define ENESIM_RENDERER_ELLIPSE_MAGIC 0xe7e51442
#define ENESIM_RENDERER_LINE_MAGIC 0xe7e51443
#define ENESIM_RENDERER_FIGURE_MAGIC 0xe7e51444
#define ENESIM_RENDERER_PATH_MAGIC 0xe7e51445

#define ENESIM_RENDERER_GRADIENT_LINEAR_MAGIC 0xe7e51450
#define ENESIM_RENDERER_GRADIENT_RADIAL_MAGIC 0xe7e51451

#define ENESIM_RASTERIZER_MAGIC 0xe7e51460
#define ENESIM_RASTERIZER_BASIC_MAGIC 0xe7e51461
#define ENESIM_RASTERIZER_BIFIGURE_MAGIC 0xe7e51462

#define ENESIM_RENDERER_PROXY_MAGIC 0xe7e51463

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
