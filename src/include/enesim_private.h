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

#if BUILD_PTHREAD
#include <pthread.h>
#endif

#if HAVE_SCHED_H
#include <sched.h>
#endif

#if BUILD_OPENCL
#include "Enesim_OpenCL.h"
#endif

#if BUILD_OPENGL
#define GL_GLEXT_PROTOTYPES
#include "Enesim_OpenGL.h"
#include "GL/glext.h"
#endif

#include <string.h>
#include <limits.h>
#include <stdint.h>
/* TODO remove all assert statements */
#include <assert.h>

#ifdef ENESIM_DEFAULT_LOG_COLOR
# undef ENESIM_DEFAULT_LOG_COLOR
#endif
#define ENESIM_DEFAULT_LOG_COLOR EINA_COLOR_LIGHTRED

#define ERR(...) EINA_LOG_DOM_ERR(enesim_log_dom_global, __VA_ARGS__)
#define WRN(...) EINA_LOG_DOM_WARN(enesim_log_dom_global, __VA_ARGS__)
#define DBG(...) EINA_LOG_DOM_DBG(enesim_log_dom_global, __VA_ARGS__)
extern int enesim_log_dom_global;

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <limits.h>

#ifndef _ISOC99_SOURCE // truncf support
#define _ISOC99_SOURCE
#endif

#include <math.h>
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

#include "libargb.h"

/* define every magic herer  to track them easily */
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

#include "private/vector.h"
#include "private/curve.h"
/* now the surface format backends */
#include "private/format.h"
#include "private/format_argb8888_unpre.h"
#include "private/compositor.h"
#include "private/matrix.h"
#include "private/rasterizer.h"
#include "private/renderer.h"
#include "private/buffer.h"
#include "private/pool.h"
#include "private/surface.h"
#include "private/converter.h"

#endif
