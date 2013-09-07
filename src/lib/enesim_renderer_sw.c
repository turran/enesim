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

/* eina includes sched.h for us, but does not set the _GNU_SOURCE
 * flag, so basically we cant use CPU_ZERO, CPU_SET, etc
 */

#include "enesim_private.h"

#include "enesim_main.h"
#include "enesim_log.h"
#include "enesim_color.h"
#include "enesim_rectangle.h"
#include "enesim_matrix.h"
#include "enesim_pool.h"
#include "enesim_buffer.h"
#include "enesim_surface.h"
#include "enesim_renderer.h"
#include "enesim_object_descriptor.h"
#include "enesim_object_class.h"
#include "enesim_object_instance.h"

#include "enesim_renderer_private.h"
#include "enesim_surface_private.h"

/**
 * @todo
 * - In a near future we should move the API functions into global ones?
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_LOG_DEFAULT enesim_log_renderer

#ifdef BUILD_THREAD
static unsigned int _num_cpus;
#endif

static inline Eina_Bool _is_sw_draw_composed(Enesim_Color *color,
		Enesim_Rop *rop, Enesim_Renderer_Sw_Hint hints)
{
	if (hints & ENESIM_RENDERER_HINT_COLORIZE)
		*color = ENESIM_COLOR_FULL;
	if (hints & ENESIM_RENDERER_HINT_ROP)
		*rop = ENESIM_ROP_FILL;

	/* fill rop and color is full, we use the simple draw function */
	if ((*rop == ENESIM_ROP_FILL) && (*color == ENESIM_COLOR_FULL))
		return EINA_FALSE;
	return EINA_TRUE;
}

static inline void _sw_surface_setup(Enesim_Surface *s, Enesim_Format *dfmt, void **data, size_t *stride, size_t *bpp)
{
	Enesim_Buffer_Sw_Data *bdata;

	bdata = enesim_surface_backend_data_get(s);
	*dfmt = enesim_surface_format_get(s);
	if (*dfmt != ENESIM_FORMAT_A8)
	{
		*stride = bdata->argb8888_pre.plane0_stride;
		*data = bdata->argb8888_pre.plane0;
		*bpp = 4;
	}
	else
	{
		*stride = bdata->a8.plane0_stride;
		*data = bdata->a8.plane0;
		*bpp = 1;
	}
}

/* worst case, rop+color+mask(rop+color) */
/* rop+color+mask */
/* rop+mask */
/* color+mask */
/* mask */
/* color */
/* rop */

static inline void _sw_surface_draw_rop_mask(Enesim_Renderer *r,
		Enesim_Renderer_Sw_Fill fill,
		Enesim_Renderer_Sw_Fill mask_fill,
		Enesim_Compositor_Span span,
		Enesim_Compositor_Span mask_span EINA_UNUSED,
		uint8_t *ddata, size_t stride,
		uint8_t *tmp,
		uint8_t *tmp_mask,
		size_t len,
		Eina_Rectangle *area)
{
	Enesim_Renderer *mask;
	Enesim_Color color;

	mask = enesim_renderer_mask_get(r);
	color = enesim_renderer_color_get(r);

	while (area->h--)
	{
		/* FIXME we should not memset this */
		memset(tmp_mask, 0, len);
		memset(tmp, 0, len);

		fill(r, area->x, area->y, area->w, tmp);
		mask_fill(mask, area->x, area->y, area->w, tmp_mask);
		area->y++;
		/* compose the filled and the destination spans */
		span((uint32_t *)ddata, area->w, (uint32_t *)tmp, color, (uint32_t *)tmp_mask);
		ddata += stride;
	}
	if (mask)
		enesim_renderer_unref(mask);
}

/* rop = any (~FLAG_ROP)
 * color = any (~FLAG_COLORIZE)
 */
static inline void _sw_surface_draw_rop(Enesim_Renderer *r,
		Enesim_Renderer_Sw_Fill fill,
		Enesim_Compositor_Span span,
		uint8_t *ddata, size_t stride,
		uint8_t *tmp, size_t len,
		Eina_Rectangle *area)
{
	Enesim_Color color;

	color = enesim_renderer_color_get(r);
	while (area->h--)
	{
		/* FIXME we should not memset this */
		memset(tmp, 0, len);
		fill(r, area->x, area->y, area->w, tmp);
		area->y++;
		/* compose the filled and the destination spans */
		span((uint32_t *)ddata, area->w, (uint32_t *)tmp, color, NULL);
		ddata += stride;
	}
}

/* rop = fill | any(FLAG_ROP)
 * color = none | any(FLAG_COLORIZE)
 * mask = none
 */
static inline void _sw_surface_draw_simple(Enesim_Renderer *r,
		Enesim_Renderer_Sw_Fill fill, uint8_t *ddata,
		size_t stride, Eina_Rectangle *area)
{
	while (area->h--)
	{
		fill(r, area->x, area->y, area->w, ddata);
		area->y++;
		ddata += stride;
	}
}

static inline void _sw_clear(uint8_t *ddata, size_t stride, int bpp,
		Eina_Rectangle *area)
{
	ddata = ddata + (area->y * stride) + (area->x * bpp);
	while (area->h--)
	{
		memset(ddata, 0x00, area->w * bpp);
		ddata += stride;
	}
}

#if 0
static inline void _sw_clear2(uint8_t *ddata, size_t stride, int bpp,
		Eina_Rectangle *area)
{
	ddata = ddata + (area->y * stride) + (area->x * bpp);
	printf("clearing2 %" EINA_EXTRA_RECTANGLE_FORMAT "\n", EINA_EXTRA_RECTANGLE_ARGS(area));
	while (area->h--)
	{
		memset(ddata, 0x33, area->w * bpp);
		ddata += stride;
	}
	printf("done\n");
}
#endif
/*----------------------------------------------------------------------------*
 *                            Threaded rendering                              *
 *----------------------------------------------------------------------------*/
#ifdef BUILD_THREAD
static inline void _sw_surface_draw_rop_threaded(Enesim_Renderer *r,
		unsigned int thread,
		Enesim_Renderer_Sw_Fill fill, Enesim_Compositor_Span span,
		uint8_t *ddata, size_t stride,
		uint8_t *tmp, size_t len, Eina_Rectangle *area)
{
	Enesim_Color color;
	int h = area->h;
	int y = area->y;

	color = enesim_renderer_color_get(r);
	while (h)
	{
		if (h % _num_cpus != thread) goto end;

		/* FIXME we should not memset this */
		memset(tmp, 0, len);
		fill(r, area->x, y, area->w, tmp);
		/* compose the filled and the destination spans */
		span((uint32_t *)ddata, area->w, (uint32_t *)tmp, color, NULL);
end:
		ddata += stride;
		h--;
		y++;
	}
}

static inline void _sw_surface_draw_simple_threaded(Enesim_Renderer *r,
		unsigned int thread,
		Enesim_Renderer_Sw_Fill fill, uint8_t *ddata,
		size_t stride, Eina_Rectangle *area)
{
	int h = area->h;
	int y = area->y;

	while (h)
	{
		if (h % _num_cpus != thread) goto end;

		fill(r, area->x, y, area->w, ddata);
end:
		ddata += stride;
		h--;
		y++;
	}
}

#ifdef _WIN32
static DWORD WINAPI _thread_run(void *data)
#else
static void * _thread_run(void *data)
#endif
{
	Enesim_Renderer_Thread *thiz = data;
	Enesim_Renderer_Sw_Data *sw_data = thiz->sw_data;
	Enesim_Renderer_Thread_Operation *op = &sw_data->op;

	do
	{
		enesim_barrier_wait(&sw_data->start);
		if (thiz->done) goto end;

		if (op->span)
		{
			uint8_t *tmp;
			size_t len;

			len = op->area.w * sizeof(uint32_t);
			/* FIXME remove this malloc. or we either
			 * make the tmp buffer part of the renderer
			 * and make it grow until we reach the span len
			 * or alloca everytime
			 */
			tmp = malloc(len);
			_sw_surface_draw_rop_threaded(op->renderer,
					thiz->cpuidx,
					sw_data->fill,
					op->span,
					op->dst,
					op->stride,
					tmp,
					len,
					&op->area);
			free(tmp);
		}
		else
		{
			_sw_surface_draw_simple_threaded(op->renderer,
					thiz->cpuidx,
					sw_data->fill,
					op->dst,
					op->stride,
					&op->area);
		}
		enesim_barrier_wait(&sw_data->end);
	} while (1);

end:
#ifdef _WIN32
	return 0;
#else
	return NULL;
#endif
}

static void _sw_draw_threaded(Enesim_Renderer *r, Eina_Rectangle *area,
		uint8_t *ddata, size_t stride,
		Enesim_Format dfmt EINA_UNUSED)
{
	Enesim_Renderer_Sw_Data *sw_data;
	Enesim_Renderer_Thread_Operation *op;

	sw_data = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_SOFTWARE);
	op = &sw_data->op;
	/* fill the data needed for every threaded renderer */
	op->renderer = r;
	op->dst = ddata;
	op->stride = stride;
	op->area = *area;
	op->mask_fill = NULL;
	op->span = sw_data->span;

	enesim_barrier_wait(&sw_data->start);
	enesim_barrier_wait(&sw_data->end);
}
#else
/*----------------------------------------------------------------------------*
 *                          No threaded rendering                             *
 *----------------------------------------------------------------------------*/
static void _sw_draw_no_threaded(Enesim_Renderer *r,
		Eina_Rectangle *area,
		uint8_t *ddata, size_t stride,
		Enesim_Format dfmt EINA_UNUSED)
{
	Enesim_Renderer_Sw_Data *sw_data;

	sw_data = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_SOFTWARE);
	if (sw_data->span)
	{
		uint8_t *fdata;
		size_t len;

		len = area->w * sizeof(uint32_t);
		fdata = alloca(len);
		_sw_surface_draw_rop(r, sw_data->fill, sw_data->span, ddata, stride, fdata, len, area);
	}
	else
	{
		_sw_surface_draw_simple(r, sw_data->fill, ddata, stride, area);
	}
}
#endif
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_renderer_sw_init(void)
{
#ifdef BUILD_THREAD
	_num_cpus = eina_cpu_count();
#endif
}

void enesim_renderer_sw_shutdown(void)
{
#ifdef BUILD_THREAD
#endif
}

void enesim_renderer_sw_hints_get(Enesim_Renderer *r, Enesim_Rop rop, Enesim_Renderer_Sw_Hint *hints)
{
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS_GET(r);
	if (!hints) return;
	if (klass->sw_hints_get)
		klass->sw_hints_get(r, rop, hints);
	else
		*hints = 0;
}

void enesim_renderer_sw_draw_area(Enesim_Renderer *r, Enesim_Surface *s,
		Enesim_Rop rop, Eina_Rectangle *area, int x, int y)
{
	Enesim_Format dfmt;
	Eina_Rectangle final;
	Eina_Bool visible;
	Eina_Bool intersect;
	uint8_t *ddata;
	size_t stride;
	size_t bpp;
#ifdef BUILD_THREAD
	Enesim_Renderer_Sw_Data *sw_data;
#endif

	/* get the destination pointer */
	_sw_surface_setup(s, &dfmt, (void **)&ddata, &stride, &bpp);

	/* be sure to clip the area to the renderer bounds */
	final = r->current_destination_bounds;
	/* final translation */
	final.x -= x;
	final.y -= y;

	intersect = eina_rectangle_intersection(&final, area);
	if (rop == ENESIM_ROP_FILL)
	{
		/* just memset the whole area */
		if (!intersect)
		{
			_sw_clear(ddata, stride, bpp, area);
			return;
		}
		/* clear the difference rectangle */
		else
		{
			Eina_Rectangle subs[4];
			int i;

			eina_extra_rectangle_substract(area, &final, subs);
			for (i = 0; i < 4; i++)
			{
				if (!eina_extra_rectangle_is_valid(&subs[i]))
					continue;
				_sw_clear(ddata, stride, bpp, &subs[i]);
			}
		}
	}

	visible = enesim_renderer_visibility_get(r);
	if (!visible)
		return;
	if (!intersect || !eina_extra_rectangle_is_valid(&final))
		return;

	ddata = ddata + (final.y * stride) + (final.x * bpp);
	/* we know have the final area on surface coordinates
	 * add again the offset because the draw functions use
	 * the area on the renderer coordinate space
	 */
	final.x += x;
	final.y += y;
#ifdef BUILD_THREAD
	/* create the threads in case those are not created yet */
	sw_data = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_SOFTWARE);
	if (!sw_data->threads)
	{
		unsigned int i;

		sw_data->threads = malloc(sizeof(Enesim_Renderer_Thread) * _num_cpus);

		enesim_barrier_new(&sw_data->start, _num_cpus + 1);
		enesim_barrier_new(&sw_data->end, _num_cpus + 1);
		for (i = 0; i < _num_cpus; i++)
		{
			sw_data->threads[i].cpuidx = i;
			sw_data->threads[i].done = EINA_FALSE;
			sw_data->threads[i].sw_data = sw_data;
			enesim_thread_new(&sw_data->threads[i].tid, _thread_run, (void *)&sw_data->threads[i]);
			enesim_thread_affinity_set(sw_data->threads[i].tid, i);
		}
	}
	_sw_draw_threaded(r, &final, ddata, stride, dfmt);
#else
	_sw_draw_no_threaded(r, &final, ddata, stride, dfmt);
#endif
}

Eina_Bool enesim_renderer_sw_setup(Enesim_Renderer *r,
		Enesim_Surface *s, Enesim_Rop rop, Enesim_Log **error)
{
	Enesim_Renderer_Class *klass;
	Enesim_Renderer_Sw_Fill fill = NULL;
	Enesim_Compositor_Span span = NULL;
	Enesim_Renderer_Sw_Data *sw_data;
	Enesim_Renderer_Sw_Hint hints;
	Enesim_Renderer *mask;
	Enesim_Color color;
	const char *name;

	klass = ENESIM_RENDERER_CLASS_GET(r);
	name = enesim_renderer_name_get(r);
	mask = enesim_renderer_mask_get(r);
	color = enesim_renderer_color_get(r);

	/* do the setup on the mask */
	/* FIXME later this should be merged on the common renderer code */
	if (mask)
	{
		Eina_Bool ret;

		ret = enesim_renderer_setup(mask, s, ENESIM_ROP_FILL, error);
		enesim_renderer_unref(mask);
		if (!ret)
		{
			WRN("Mask setup callback on '%s' failed", name);
			return EINA_FALSE;
		}
	}
	if (!klass->sw_setup) return EINA_TRUE;
	if (!klass->sw_setup(r, s, rop, &fill, error))
	{
		WRN("Setup callback on '%s' failed", name);
		return EINA_FALSE;

	}
	if (!fill)
	{
		ENESIM_RENDERER_LOG(r, error, "Even if the setup did not failed, there's no fill function");
		enesim_renderer_sw_cleanup(r, s);
		return EINA_FALSE;
	}

	sw_data = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_SOFTWARE);
	if (!sw_data)
	{
		sw_data = calloc(1, sizeof(Enesim_Renderer_Sw_Data));
		enesim_renderer_backend_data_set(r, ENESIM_BACKEND_SOFTWARE, sw_data);	
	}

	enesim_renderer_sw_hints_get(r, rop, &hints);
	if (_is_sw_draw_composed(&color, &rop, hints))
	{
		Enesim_Format dfmt;

		dfmt = enesim_surface_format_get(s);
		span = enesim_compositor_span_get(rop, &dfmt, ENESIM_FORMAT_ARGB8888,
				color, ENESIM_FORMAT_NONE);
		if (!span)
		{
			WRN("No suitable span compositor to render %p with rop "
					"%d and color %08x", r, rop, color);
			return EINA_FALSE;
		}
	}

	/* TODO add a real_draw function that will compose the two ... or not :) */
	sw_data->span = span;
	sw_data->fill = fill;
	return EINA_TRUE;
}

void enesim_renderer_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer *mask;
	Enesim_Renderer_Class *klass;

	klass = ENESIM_RENDERER_CLASS_GET(r);
	mask = enesim_renderer_mask_get(r);
	/* do the setup on the mask */
	/* FIXME later this should be merged on the common renderer code */
	if (mask)
	{
		enesim_renderer_cleanup(mask, s);
		enesim_renderer_unref(mask);
	}
	if (klass->sw_cleanup) klass->sw_cleanup(r, s);
}

void enesim_renderer_sw_free(Enesim_Renderer *r)
{

	Enesim_Renderer_Sw_Data *sw_data;
#if BUILD_THREAD
	unsigned int i;
#endif

	sw_data = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_SOFTWARE);
	if (!sw_data) return;
#if BUILD_THREAD
	if (sw_data->threads)
	{
		/* first mark all the threads to leave */
		for (i = 0; i < _num_cpus; i++)
			sw_data->threads[i].done = EINA_TRUE;

		/* now increment the barrier so the threads start again */
		enesim_barrier_wait(&sw_data->start);
		/* destroy the threads */
		for (i = 0; i < _num_cpus; i++)
			enesim_thread_free(sw_data->threads[i].tid);
		free(sw_data->threads);
		enesim_barrier_free(&sw_data->start);
		enesim_barrier_free(&sw_data->end);
	}
#endif
	free(sw_data);
}

void enesim_renderer_sw_draw(Enesim_Renderer *r,  int x, int y,
		unsigned int len, uint32_t *data)
{
	Enesim_Renderer_Sw_Data *sw_data;
	Eina_Rectangle span;
	Eina_Rectangle rbounds;
	Eina_Bool visible;
	unsigned int left;

	visible = enesim_renderer_visibility_get(r);
	if (!visible) return;
	sw_data = r->backend_data[ENESIM_BACKEND_SOFTWARE];

	eina_rectangle_coords_from(&span, x, y, len, 1);
	rbounds = r->current_destination_bounds;
	if (!eina_rectangle_intersection(&rbounds, &span))
	{
		DBG("Drawing span %" EINA_EXTRA_RECTANGLE_FORMAT " and bounds %"
				EINA_EXTRA_RECTANGLE_FORMAT " do not intersect on "
				"'%s'", EINA_EXTRA_RECTANGLE_ARGS (&span),
				EINA_EXTRA_RECTANGLE_ARGS (&rbounds),
				r->name);
		return;
	}
	left = rbounds.x - span.x;

	if (sw_data->span)
	{
		Enesim_Color color;
		uint32_t *tmp;
		size_t bytes;

		color = enesim_renderer_color_get(r);
		bytes = rbounds.w * sizeof(uint32_t);
		tmp = alloca(bytes);
		/* FIXME if we remove this there is a problem in the compound renderer */
		memset(tmp, 0, bytes);
		sw_data->fill(r, rbounds.x, rbounds.y, rbounds.w, tmp);
		/* compose the filled and the destination spans */
		sw_data->span(data + left, rbounds.w, tmp, color, NULL);
	}
	else
	{
		sw_data->fill(r, rbounds.x, rbounds.y, rbounds.w, data + left);
	}

	if (r->current_rop == ENESIM_ROP_FILL)
	{
		unsigned int right;

		if (left > 0)
			memset(data, 0, left * sizeof(uint32_t));

		right = (span.x + span.w) - (rbounds.x + rbounds.w);
		if (right > 0)
			memset(data + len - right, 0, right * sizeof(uint32_t));
	}
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
