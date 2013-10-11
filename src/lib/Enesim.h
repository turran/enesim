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
#ifndef ENESIM_H_
#define ENESIM_H_

/**
 * @mainpage Enesim
 * @image html enesim.png
 * @section intro Introduction
 * The Enesim library provides a complete solution for drawing, allocating,
 * compositing and converting pixels. All of the systems of the library
 * are flexible enough to support drawing capabilities on any scenario
 *
 * @section backends Backends
 * - Software based
 * - @ref Enesim_OpenGL_Page (alpha)
 * - OpenCL (experimental)
 *
 * @section dependencies Dependencies
 * - Eina
 *
 * @file
 * @todo
 * - [1.1.0]
 *   - Implement OpenGL on every renderer with rop support
 *   - Export the pool API
 * @brief Enesim API
 */

#include <inttypes.h>

#include <Eina.h>

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

#ifdef __cplusplus
extern "C" {
#endif

/* core headers */
#include "enesim_log.h"
#include "enesim_main.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_pool.h"
/* object headers */
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"
/* util headers */
#include "enesim_quad.h"
#include "enesim_matrix.h"
#include "enesim_path.h"
/* main subsystems */
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_converter.h"
#include "enesim_renderer.h"
#include "enesim_stream.h"
#include "enesim_image.h"
#include "enesim_text.h"
/* renderers */
#include "enesim_renderer_background.h"
#include "enesim_renderer_blur.h"
#include "enesim_renderer_shape.h"
#include "enesim_renderer_rectangle.h"
#include "enesim_renderer_circle.h"
#include "enesim_renderer_ellipse.h"
#include "enesim_renderer_figure.h"
#include "enesim_renderer_path.h"
#include "enesim_renderer_checker.h"
#include "enesim_renderer_dispmap.h"
#include "enesim_renderer_raddist.h"
#include "enesim_renderer_grid.h"
#include "enesim_renderer_stripes.h"
#include "enesim_renderer_image.h"
#include "enesim_renderer_gradient.h"
#include "enesim_renderer_gradient_linear.h"
#include "enesim_renderer_gradient_radial.h"
#include "enesim_renderer_compound.h"
#include "enesim_renderer_importer.h"
#include "enesim_renderer_line.h"
#include "enesim_renderer_pattern.h"
#include "enesim_renderer_perlin.h"
#include "enesim_renderer_proxy.h"
#include "enesim_renderer_clipper.h"
#include "enesim_renderer_text_span.h"
#include "enesim_renderer_transition.h"

#ifdef __cplusplus
}
#endif

#endif
