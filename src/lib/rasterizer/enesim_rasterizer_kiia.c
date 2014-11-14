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
#include "enesim_private.h"
#include <math.h>

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_format.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_renderer_shape.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_color_private.h"
#include "enesim_color_fill_private.h"
#include "enesim_color_mul4_sym_private.h"
#include "enesim_list_private.h"
#include "enesim_vector_private.h"
#include "enesim_renderer_private.h"
#include "enesim_rasterizer_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
#define ENESIM_RASTERIZER_KIIA(o) ENESIM_OBJECT_INSTANCE_CHECK(o,		\
		Enesim_Rasterizer_Kiia,						\
		enesim_rasterizer_kiia_descriptor_get())

/* A worker is in charge of rasterize one span */
typedef struct _Enesim_Rasterizer_Kiia_Worker
{
	/* a span of length equal to the width of the bounds */
	uint32_t *mask;
	/* keep track of the current Y this worker is doing, to ease
	 * the x increment on the edges
	 */
	int y;
} Enesim_Rasterizer_Kiia_Worker;

/* Its own definition of an edge */
typedef struct _Enesim_Rasterizer_Kiia_Edge
{
	Eina_F16p16 x0, y0, x1, y1;
	Eina_F16p16 mx;
	Eina_F16p16 slope;
	int sgn;
} Enesim_Rasterizer_Kiia_Edge;

typedef struct _Enesim_Rasterizer_Kiia
{
	Enesim_Rasterizer parent;
	/* One worker per cpu */
	Enesim_Rasterizer_Kiia_Worker *workers;
	int nworkers;
	/* private */
} Enesim_Rasterizer_Kiia;

typedef struct _Enesim_Rasterizer_Kiia_Class {
	Enesim_Rasterizer_Class parent;
} Enesim_Rasterizer_Kiia_Class;

/*----------------------------------------------------------------------------*
 *                           Rasterizer interface                             *
 *----------------------------------------------------------------------------*/
static void _kiia_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s EINA_UNUSED)
{
	Enesim_Rasterizer_Kiia *thiz;
}

/*----------------------------------------------------------------------------*
 *                    The Enesim's rasterizer interface                       *
 *----------------------------------------------------------------------------*/
static const char * _kiia_name(Enesim_Renderer *r EINA_UNUSED)
{
	return "kiia";
}

static Eina_Bool _kiia_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s EINA_UNUSED, Enesim_Rop rop EINA_UNUSED,
		Enesim_Renderer_Sw_Fill *draw, Enesim_Log **error)
{
	return EINA_TRUE;
}

static void _kiia_sw_hints(Enesim_Renderer *r EINA_UNUSED,
		Enesim_Rop rop EINA_UNUSED, Enesim_Renderer_Sw_Hint *hints)
{
	*hints = ENESIM_RENDERER_SW_HINT_COLORIZE;
}
/*----------------------------------------------------------------------------*
 *                            Object definition                               *
 *----------------------------------------------------------------------------*/
ENESIM_OBJECT_INSTANCE_BOILERPLATE(ENESIM_RASTERIZER_DESCRIPTOR,
		Enesim_Rasterizer_Kiia, Enesim_Rasterizer_Kiia_Class,
		enesim_rasterizer_kiia);

static void _enesim_rasterizer_kiia_class_init(void *k)
{
	Enesim_Renderer_Class *r_klass;
	Enesim_Rasterizer_Class *klass;

	r_klass = ENESIM_RENDERER_CLASS(k);
	r_klass->base_name_get = _kiia_name;
	r_klass->sw_setup = _kiia_sw_setup;
	r_klass->sw_hints_get = _kiia_sw_hints;

	klass = ENESIM_RASTERIZER_CLASS(k);
	klass->sw_cleanup = _kiia_sw_cleanup;
}

static void _enesim_rasterizer_kiia_instance_init(void *o EINA_UNUSED)
{
}

static void _enesim_rasterizer_kiia_instance_deinit(void *o EINA_UNUSED)
{
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
Enesim_Renderer * enesim_rasterizer_kiia_new(void)
{
	Enesim_Renderer *r;

	r = ENESIM_OBJECT_INSTANCE_NEW(enesim_rasterizer_kiia);
	return r;
}
/** @endcond */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
