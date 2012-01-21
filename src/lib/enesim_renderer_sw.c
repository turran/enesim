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

/* eina includes sched.h for us, but does not set the _GNU_SOURCE
 * flag, so basically we cant use CPU_ZERO, CPU_SET, etc
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef BUILD_PTHREAD
#include <sched.h>
#include <pthread.h>
#endif

#include "Enesim.h"
#include "enesim_private.h"

/**
 * @todo
 * - In a near future we should move the API functions into global ones?
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
static inline Eina_Bool _is_sw_draw_composed(Enesim_Renderer *r,
		Enesim_Renderer_Flag flags)
{
#if 0
	if (r->current.mask)
	{
		*has_mask = EINA_TRUE;
		if (flags & ENESIM_RENDERER_FLAG_MASK)
			*needs_mask = EINA_FALSE;
		else
			*needs_mask = EINA_TRUE;
	}
	else
	{
		*has_mask = EINA_FALSE;
	}

	if (r->current.rop == ENESIM_FILL)
	{
		*has_rop = EINA_FALSE;
		if (r->current.color == ENESIM_COLOR_FULL)
		{

		}
	}
	else
	{
		*has_rop = EINA_TRUE;
	}
#endif

	/* fill rop and color is full, we use the simple draw function */
	if ((r->current.rop == ENESIM_FILL) && (r->current.color == ENESIM_COLOR_FULL))
		return EINA_FALSE;
	/* the renderer can handle different rops and the color is full, use simple draw function */
	if ((flags & ENESIM_RENDERER_FLAG_ROP) && (r->current.color == ENESIM_COLOR_FULL))
		return EINA_FALSE;
	/* the renderer can handle different rops and also the color, use simple draw function */
	if ((flags & ENESIM_RENDERER_FLAG_ROP) && (flags & ENESIM_RENDERER_FLAG_COLORIZE))
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
		Enesim_Compositor_Span mask_span,
		uint8_t *ddata, size_t stride,
		uint8_t *tmp,
		uint8_t *tmp_mask,
		size_t len,
		Eina_Rectangle *area)
{
	while (area->h--)
	{
		/* FIXME we should not memset this */
		memset(tmp_mask, 0, len);
		memset(tmp, 0, len);

		fill(r, area->x, area->y, area->w, tmp);
		mask_fill(r->current.mask, area->x, area->y, area->w, tmp_mask);
		area->y++;
		/* compose the filled and the destination spans */
		span((uint32_t *)ddata, area->w, (uint32_t *)tmp, r->current.color, (uint32_t *)tmp_mask);
		ddata += stride;
	}
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
	while (area->h--)
	{
		/* FIXME we should not memset this */
		memset(tmp, 0, len);
		fill(r, area->x, area->y, area->w, tmp);
		area->y++;
		/* compose the filled and the destination spans */
		span((uint32_t *)ddata, area->w, (uint32_t *)tmp, r->current.color, NULL);
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
/*----------------------------------------------------------------------------*
 *                            Threaded rendering                              *
 *----------------------------------------------------------------------------*/
#ifdef BUILD_PTHREAD
typedef struct _Enesim_Renderer_Thread_Operation
{
	/* common attributes */
	Enesim_Renderer *renderer;
	Enesim_Renderer_Sw_Fill fill;
	Enesim_Renderer_Sw_Fill mask_fill;
	uint8_t * dst;
	size_t stride;
	Eina_Rectangle area;
	/* in case the renderer needs to use a composer */
	Enesim_Compositor_Span span;
} Enesim_Renderer_Thread_Operation;

typedef struct _Enesim_Renderer_Thread
{
	int cpuidx;
	pthread_t tid;
} Enesim_Renderer_Thread;

static unsigned int _num_cpus;
static Enesim_Renderer_Thread *_threads;
static Enesim_Renderer_Thread_Operation _op;
static pthread_barrier_t _start;
static pthread_barrier_t _end;

static inline void _sw_surface_draw_rop_threaded(Enesim_Renderer *r,
		unsigned int thread,
		Enesim_Renderer_Sw_Fill fill, Enesim_Compositor_Span span,
		uint8_t *ddata, size_t stride,
		uint8_t *tmp, size_t len, Eina_Rectangle *area)
{
	int h = area->h;
	int y = area->y;

	while (h)
	{
		if (h % _num_cpus != thread) goto end;

		/* FIXME we should not memset this */
		memset(tmp, 0, len);
		fill(r, area->x, y, area->w, tmp);
		/* compose the filled and the destination spans */
		span((uint32_t *)ddata, area->w, (uint32_t *)tmp, r->current.color, NULL);
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

static void * _thread_run(void *data)
{
	Enesim_Renderer_Thread *thiz = data;
	Enesim_Renderer_Thread_Operation *op = &_op;

	do {
		pthread_barrier_wait(&_start);
		if (op->span)
		{
			uint8_t *tmp;
			size_t len;

			len = op->area.w * sizeof(uint32_t);
			tmp = malloc(len);
			_sw_surface_draw_rop_threaded(op->renderer,
					thiz->cpuidx,
					op->fill,
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
					op->fill,
					op->dst,
					op->stride,
					&op->area);

		}
		pthread_barrier_wait(&_end);
	} while (1);

	return NULL;
}

static void _sw_draw_threaded(Enesim_Renderer *r, Eina_Rectangle *area,
		uint8_t *ddata, size_t stride,
		Enesim_Format dfmt,
		Enesim_Renderer_Flag flags)
{
	Enesim_Renderer_Sw_Data *sw_data;

	sw_data = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_SOFTWARE);
	/* fill the data needed for every threaded renderer */
	_op.renderer = r;
	_op.dst = ddata;
	_op.stride = stride;
	_op.area = *area;
	_op.fill = sw_data->fill;
	_op.mask_fill = NULL;
	_op.span = NULL;

	if (_is_sw_draw_composed(r, flags))
	{
		Enesim_Compositor_Span span;

		span = enesim_compositor_span_get(r->current.rop, &dfmt, ENESIM_FORMAT_ARGB8888,
				r->current.color, ENESIM_FORMAT_NONE);
		if (!span)
		{
			WRN("No suitable span compositor to render %p with rop "
					"%d and color %08x", r, r->current.rop, r->current.color);
			goto end;
		}
		_op.span = span;
	}

	pthread_barrier_wait(&_start);
	pthread_barrier_wait(&_end);
end:
	return;
}
#else
/*----------------------------------------------------------------------------*
 *                          No threaded rendering                             *
 *----------------------------------------------------------------------------*/
static void _sw_draw_no_threaded(Enesim_Renderer *r,
		Eina_Rectangle *area,
		uint8_t *ddata, size_t stride, Enesim_Format dfmt,
		Enesim_Renderer_Flag flags)
{
	Enesim_Renderer_Sw_Data *sw_data;

	sw_data = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_SOFTWARE);
	if (_is_sw_draw_composed(r, flags))
	{
		Enesim_Compositor_Span span;
		uint8_t *fdata;
		size_t len;

		span = enesim_compositor_span_get(r->current.rop, &dfmt, ENESIM_FORMAT_ARGB8888,
				r->current.color, ENESIM_FORMAT_NONE);
		if (!span)
		{
			WRN("No suitable span compositor to render %p with rop "
					"%d and color %08x", r, r->current.rop, r->current.color);
			goto end;
		}
		len = area->w * sizeof(uint32_t);
		fdata = alloca(len);
		_sw_surface_draw_rop(r, sw_data->fill, span, ddata, stride, fdata, len, area);
	}
	else
	{
		_sw_surface_draw_simple(r, sw_data->fill, ddata, stride, area);
	}
end:
	return;
}
#endif
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_renderer_sw_init(void)
{
#ifdef BUILD_PTHREAD
	int i = 0;
	pthread_attr_t attr;

	_num_cpus = eina_cpu_count();
	_threads = malloc(sizeof(Enesim_Renderer_Thread) * _num_cpus);

	pthread_barrier_init(&_start, NULL, _num_cpus + 1);
	pthread_barrier_init(&_end, NULL, _num_cpus + 1);
	pthread_attr_init(&attr);
	for (i = 0; i < _num_cpus; i++)
	{
		cpu_set_t cpu;

		CPU_ZERO(&cpu);
		CPU_SET(i, &cpu);
		_threads[i].cpuidx = i;
		pthread_create(&_threads[i].tid, &attr, _thread_run, (void *)&_threads[i]);
		pthread_setaffinity_np(_threads[i].tid, sizeof(cpu_set_t), &cpu);

	}
#endif
}

void enesim_renderer_sw_shutdown(void)
{
#ifdef BUILD_PTHREAD
	/* destroy the threads */
	free(_threads);
	pthread_barrier_destroy(&_start);
	pthread_barrier_destroy(&_end);
#endif
}

void enesim_renderer_sw_draw(Enesim_Renderer *r, Enesim_Surface *s, Eina_Rectangle *area,
		int x, int y, Enesim_Renderer_Flag flags)
{
	Enesim_Format dfmt;
	uint8_t *ddata;
	size_t stride;
	size_t bpp;

	_sw_surface_setup(s, &dfmt, (void **)&ddata, &stride, &bpp);
	ddata = ddata + (area->y * stride) + (area->x * bpp);

	/* translate the origin */
	area->x -= x;
	area->y -= y;

#ifdef BUILD_PTHREAD
	_sw_draw_threaded(r, area, ddata, stride, dfmt, flags);
#else
	_sw_draw_no_threaded(r, area, ddata, stride, dfmt, flags);
#endif
}

void enesim_renderer_sw_draw_list(Enesim_Renderer *r, Enesim_Surface *s, Eina_Rectangle *area,
		Eina_List *clips, int x, int y, Enesim_Renderer_Flag flags)
{
	Enesim_Format dfmt;
	Enesim_Renderer_Sw_Data *rswdata;
	uint8_t *ddata;
	size_t stride;
	size_t bpp;

	_sw_surface_setup(s, &dfmt, (void *)&ddata, &stride, &bpp);
	ddata = ddata + (area->y * stride) + (area->x * bpp);
	rswdata = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_SOFTWARE);

	if (_is_sw_draw_composed(r, flags))
	{
		Enesim_Compositor_Span span;
		Eina_Rectangle *clip;
		Eina_List *l;

		span = enesim_compositor_span_get(r->current.rop, &dfmt, ENESIM_FORMAT_ARGB8888,
				r->current.color, ENESIM_FORMAT_NONE);
		if (!span)
		{
			WRN("No suitable span compositor to render %p with rop "
					"%d and color %08x", r, r->current.rop, r->current.color);
			goto end;
		}
		/* iterate over the list of clips */
		EINA_LIST_FOREACH(clips, l, clip)
		{
			Eina_Rectangle final;
			size_t len;
			uint8_t *fdata;
			uint8_t *rdata;

			final = *clip;
			if (!eina_rectangle_intersection(&final, area))
				continue;
			rdata = ddata + (final.y * stride) + (final.x * bpp);
			/* translate the origin */
			final.x -= x;
			final.y -= y;
			/* now render */
			len = final.w * bpp;
			fdata = alloca(len);
			_sw_surface_draw_rop(r, rswdata->fill, span, rdata, stride,
					fdata, len, &final);
		}
	}
	else
	{
		Eina_Rectangle *clip;
		Eina_List *l;

		/* iterate over the list of clips */
		EINA_LIST_FOREACH(clips, l, clip)
		{
			Eina_Rectangle final;
			uint8_t *rdata;

			final = *clip;
			if (!eina_rectangle_intersection(&final, area))
				continue;
			rdata = ddata + (final.y * stride) + (final.x * bpp);
			/* translate the origin */
			final.x -= x;
			final.y -= y;
			/* now render */
			_sw_surface_draw_simple(r, rswdata->fill, rdata, stride, &final);
		}
	}
end:
	return;
}

Eina_Bool enesim_renderer_sw_setup(Enesim_Renderer *r,
		const Enesim_Renderer_State *states[ENESIM_RENDERER_STATES],
		Enesim_Surface *s,
		Enesim_Error **error)
{
	Enesim_Renderer_Sw_Fill fill = NULL;
	Enesim_Renderer_Sw_Data *sw_data;

	/* do the setup on the mask */
	/* FIXME later this should be merged on the common renderer code */
	if (r->current.mask)
	{
		if (!enesim_renderer_setup(r->current.mask, s, error))
		{
			WRN("Mask %s setup callback on %s failed", r->current.mask->name, r->name);
			return EINA_FALSE;
		}
	}
	if (!r->descriptor.sw_setup) return EINA_TRUE;
	if (!r->descriptor.sw_setup(r, states, s, &fill, error))
	{
		WRN("Setup callback on %s failed", r->name);
		return EINA_FALSE;

	}
	if (!fill)
	{
		ENESIM_RENDERER_ERROR(r, error, "Even if the setup did not failed, there's no fill function");
		enesim_renderer_sw_cleanup(r, s);
		return EINA_FALSE;
	}

	sw_data = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_SOFTWARE);
	if (!sw_data)
	{
		sw_data = calloc(1, sizeof(Enesim_Renderer_Sw_Data));
		enesim_renderer_backend_data_set(r, ENESIM_BACKEND_SOFTWARE, sw_data);
	}
	sw_data->fill = fill;
	return EINA_TRUE;
}

void enesim_renderer_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	/* do the setup on the mask */
	/* FIXME later this should be merged on the common renderer code */
	if (r->current.mask)
	{
		enesim_renderer_cleanup(r->current.mask, s);
	}
	if (r->descriptor.sw_cleanup) r->descriptor.sw_cleanup(r, s);
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Renderer_Sw_Fill enesim_renderer_sw_fill_get(Enesim_Renderer *r)
{
	Enesim_Renderer_Sw_Data *sw_data;

	sw_data = r->backend_data[ENESIM_BACKEND_SOFTWARE];
	//ENESIM_MAGIC_CHECK_RENDERER(r);
	return sw_data->fill;
}
