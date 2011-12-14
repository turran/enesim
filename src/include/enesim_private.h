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

/**
 * To be documented
 * FIXME: To be fixed
 */
static inline int enesim_hline_cut(int x, int *w, int *rx, int *rw, int cx)
{

	if ((x <= cx) && (x + *w > cx))
	{
		int x2;

		x2 = x + *w;
		*w = cx - x;
		*rx = cx;
		*rw = x2 - cx;
		return 1;
	}
	return 0;
}

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
