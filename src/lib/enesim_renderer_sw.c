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
#include "enesim_format.h"
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
/** @cond internal */
#define ENESIM_LOG_DEFAULT enesim_log_renderer

#ifdef BUILD_MULTI_CORE
static unsigned int _num_cpus;
#endif

static inline Eina_Bool _is_sw_draw_composed(Enesim_Color *color,
		Enesim_Rop *rop, Enesim_Renderer **mask,
		Enesim_Renderer_Sw_Hint hints)
{
	if ((hints & ENESIM_RENDERER_SW_HINT_MASK) && mask)
	{
		enesim_renderer_unref(*mask);
		*mask = NULL;
	}
	if (hints & ENESIM_RENDERER_SW_HINT_COLORIZE)
		*color = ENESIM_COLOR_FULL;
	if (hints & ENESIM_RENDERER_SW_HINT_ROP)
		*rop = ENESIM_ROP_FILL;

	/* fill rop, color is full and no mask, we use the simple draw function */
	if ((*rop == ENESIM_ROP_FILL) && (*color == ENESIM_COLOR_FULL) && (!*mask))
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
		Enesim_Compositor_Span span,
		uint8_t *ddata, size_t stride,
		uint8_t *tmp,
		uint8_t *tmp_mask,
		size_t len,
		Eina_Rectangle *area)
{
	Enesim_Renderer *mask;
	Enesim_Color color;

	/* FIXME do not use this properties, use the generated properties after the _is_sw_draw_composed() */
	mask = enesim_renderer_mask_get(r);
	color = enesim_renderer_color_get(r);

	while (area->h--)
	{
		/* FIXME we should not memset this */
		memset(tmp_mask, 0, len);
		memset(tmp, 0, len);

		fill(r, area->x, area->y, area->w, tmp);
		enesim_renderer_sw_draw(mask, area->x, area->y, area->w, (uint32_t *)tmp_mask);
		area->y++;
		/* compose the filled and the destination spans */
		span((uint32_t *)ddata, area->w, (uint32_t *)tmp, color, (uint32_t *)tmp_mask);
		ddata += stride;
	}
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

	/* FIXME do not use the renderer color, use the generated color after the _is_sw_draw_composed() */
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
/*----------------------------------------------------------------------------*
 *                            Threaded rendering                              *
 *----------------------------------------------------------------------------*/
#ifdef BUILD_MULTI_CORE
static inline void _sw_surface_draw_rop_mask_threaded(Enesim_Renderer *r,
		unsigned int thread,
		Enesim_Renderer_Sw_Fill fill, Enesim_Compositor_Span span,
		uint8_t *ddata, size_t stride,
		uint8_t *tmp, uint8_t *mtmp, size_t len, Eina_Rectangle *area)
{
	Enesim_Color color;
	Enesim_Renderer *mask;
	int h = area->h;
	int y = area->y;

	/* FIXME do not use this properties, use the generated properties after the _is_sw_draw_composed() */
	color = enesim_renderer_color_get(r);
	mask = enesim_renderer_mask_get(r);

	while (h)
	{
		if (h % _num_cpus != thread) goto end;

		/* FIXME we should not memset this */
		memset(tmp, 0, len);
		memset(mtmp, 0, len);
		fill(r, area->x, y, area->w, tmp);
		enesim_renderer_sw_draw(mask, area->x, y, area->w, (uint32_t *)mtmp);
		/* compose the filled and the destination spans */
		span((uint32_t *)ddata, area->w, (uint32_t *)tmp, color, (uint32_t *)mtmp);
end:
		ddata += stride;
		h--;
		y++;
	}
	enesim_renderer_unref(mask);
}

static inline void _sw_surface_draw_rop_threaded(Enesim_Renderer *r,
		unsigned int thread,
		Enesim_Renderer_Sw_Fill fill, Enesim_Compositor_Span span,
		uint8_t *ddata, size_t stride,
		uint8_t *tmp, size_t len, Eina_Rectangle *area)
{
	Enesim_Color color;
	int h = area->h;
	int y = area->y;

	/* FIXME do not use the renderer color, use the generated color after the _is_sw_draw_composed() */
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

		if (sw_data->span)
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
			if (sw_data->use_mask)
			{
				uint8_t *mtmp;
				/* FIXME remove this malloc. or we either
				 * make the tmp buffer part of the renderer
				 * and make it grow until we reach the span len
				 * or alloca everytime
				 */
				mtmp = malloc(len);
				_sw_surface_draw_rop_mask_threaded(op->renderer,
						thiz->cpuidx,
						sw_data->fill,
						sw_data->span,
						op->dst,
						op->stride,
						tmp,
						mtmp,
						len,
						&op->area);
				free(mtmp);
			}
			else
			{
				_sw_surface_draw_rop_threaded(op->renderer,
						thiz->cpuidx,
						sw_data->fill,
						sw_data->span,
						op->dst,
						op->stride,
						tmp,
						len,
						&op->area);
			}
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
		if (sw_data->use_mask)
		{
			uint8_t *mdata;
			mdata = alloca(len);

			_sw_surface_draw_rop_mask(r, sw_data->fill, sw_data->span,
					ddata, stride, fdata, mdata, len, area);
		}
		else
		{
			_sw_surface_draw_rop(r, sw_data->fill, sw_data->span,
					ddata, stride, fdata, len, area);
		}
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
#ifdef BUILD_MULTI_CORE
	_num_cpus = eina_cpu_count();
#endif
}

void enesim_renderer_sw_shutdown(void)
{
#ifdef BUILD_MULTI_CORE
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
#ifdef BUILD_MULTI_CORE
	Enesim_Renderer_Sw_Data *sw_data;
#endif

	/* TODO in case of a mask, first intersect the mask bounds with the
	 * renderer bounds, if they do not intersect return
	 */

	/* get the destination pointer */
	_sw_surface_setup(s, &dfmt, (void **)&ddata, &stride, &bpp);

	/* be sure to clip the area to the renderer bounds */
	final = r->current_destination_bounds;
	/* final translation */
	final.x += x;
	final.y += y;

	intersect = eina_rectangle_intersection(&final, area);
	/* when filling be sure to clear the area that we dont draw */
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

			eina_rectangle_subtract(area, &final, subs);
			for (i = 0; i < 4; i++)
			{
				if (!eina_rectangle_is_valid(&subs[i]))
					continue;
				_sw_clear(ddata, stride, bpp, &subs[i]);
			}
		}
	}

	visible = enesim_renderer_visibility_get(r);
	if (!visible)
		return;
	if (!intersect || !eina_rectangle_is_valid(&final))
		return;

	ddata = ddata + (final.y * stride) + (final.x * bpp);
	/* we know have the final area on surface coordinates
	 * add again the offset because the draw functions use
	 * the area on the renderer coordinate space
	 */
	final.x -= x;
	final.y -= y;
#ifdef BUILD_MULTI_CORE
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
	Eina_Bool use_mask = EINA_FALSE;

	/* First the setup on the renderer itself */
	klass = ENESIM_RENDERER_CLASS_GET(r);
	if (!klass->sw_setup)
		return EINA_FALSE;

	if (!klass->sw_setup(r, s, rop, &fill, error))
	{
		WRN("Setup callback on '%s' failed", r->name);
		return EINA_FALSE;
	}
	if (!fill)
	{
		ENESIM_RENDERER_LOG(r, error, "Even if the setup did not failed, there's no fill function");
		enesim_renderer_sw_cleanup(r, s);
		return EINA_FALSE;
	}

	/* do the setup on the mask */
	mask = enesim_renderer_mask_get(r);
	/* FIXME later this should be merged on the common renderer code */
	if (mask)
	{
		Eina_Bool ret;

		ret = enesim_renderer_setup(mask, s, ENESIM_ROP_FILL, error);
		if (!ret)
		{
			WRN("Mask setup callback on '%s' failed", r->name);
			enesim_renderer_unref(mask);
			return EINA_FALSE;
		}
	}

	sw_data = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_SOFTWARE);
	if (!sw_data)
	{
		sw_data = calloc(1, sizeof(Enesim_Renderer_Sw_Data));
		enesim_renderer_backend_data_set(r, ENESIM_BACKEND_SOFTWARE, sw_data);	
	}

	color = enesim_renderer_color_get(r);
	enesim_renderer_sw_hints_get(r, rop, &hints);

	if (_is_sw_draw_composed(&color, &rop, &mask, hints))
	{
		Enesim_Format dfmt;
		Enesim_Format mfmt = ENESIM_FORMAT_NONE;

		if (mask)
		{
			mfmt = ENESIM_FORMAT_ARGB8888;
			use_mask = EINA_TRUE;
		}

		dfmt = enesim_surface_format_get(s);
		span = enesim_compositor_span_get(rop, &dfmt, ENESIM_FORMAT_ARGB8888,
				color, mfmt);
		if (!span)
		{
			WRN("No suitable span compositor to render %p with rop "
					"%d and color %08x", r, rop, color);
			enesim_renderer_unref(mask);
			return EINA_FALSE;
		}
	}

	/* TODO add a real_draw function that will compose the two ... or not :) */
	sw_data->span = span;
	sw_data->fill = fill;
	sw_data->use_mask = use_mask;
	enesim_renderer_unref(mask);

	return EINA_TRUE;
}

void enesim_renderer_sw_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Renderer *mask;
	Enesim_Renderer_Class *klass;

	/* First do the cleanup on the renderer */
	klass = ENESIM_RENDERER_CLASS_GET(r);
	if (klass->sw_cleanup) klass->sw_cleanup(r, s);

	/* Now the cleanup on the mask */
	mask = enesim_renderer_mask_get(r);
	/* FIXME later this should be merged on the common renderer code */
	if (mask)
	{
		enesim_renderer_cleanup(mask, s);
		enesim_renderer_unref(mask);
	}
}

void enesim_renderer_sw_free(Enesim_Renderer *r)
{

	Enesim_Renderer_Sw_Data *sw_data;
#if BUILD_MULTI_CORE
	unsigned int i;
#endif

	sw_data = enesim_renderer_backend_data_get(r, ENESIM_BACKEND_SOFTWARE);
	if (!sw_data) return;
#if BUILD_MULTI_CORE
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
		int len, uint32_t *data)
{
	Enesim_Renderer_Sw_Data *sw_data;
	Eina_Rectangle span;
	Eina_Rectangle rbounds, mbounds;
	Eina_Bool visible;
	unsigned int left;

	if (!len) return;
	visible = enesim_renderer_visibility_get(r);
	if (!visible) return;
	sw_data = r->backend_data[ENESIM_BACKEND_SOFTWARE];

	eina_rectangle_coords_from(&span, x, y, len, 1);
	/* intersect the span against the renderer bounds */
	rbounds = r->current_destination_bounds;
	if (!eina_rectangle_intersection(&rbounds, &span))
	{
		DBG("Drawing span %" EINA_RECTANGLE_FORMAT " and bounds %"
				EINA_RECTANGLE_FORMAT " do not intersect on "
				"'%s'", EINA_RECTANGLE_ARGS (&span),
				EINA_RECTANGLE_ARGS (&rbounds),
				r->name);
		if (r->current_rop == ENESIM_ROP_FILL)
		{
			memset(data, 0, len * sizeof(uint32_t));
		}
		return;
	}

	/* intersect the span against the mask renderer bounds */
	/* FIXME use the use_mask or check the actual mask renderer presence? */
	if (!sw_data->use_mask)
		goto mask_done;

	mbounds = r->current_destination_bounds;
	if (!eina_rectangle_intersection(&rbounds, &mbounds))
	{
		DBG("Renderer bounds %" EINA_RECTANGLE_FORMAT " and mask bounds %"
				EINA_RECTANGLE_FORMAT " do not intersect on "
				"'%s'", EINA_RECTANGLE_ARGS (&rbounds),
				EINA_RECTANGLE_ARGS (&mbounds),
				r->name);
		if (r->current_rop == ENESIM_ROP_FILL)
		{
			memset(data, 0, len * sizeof(uint32_t));
		}
		return;
	}
mask_done:
	/* get the data offset */
	left = rbounds.x - span.x;

	DBG("Drawing span %" EINA_RECTANGLE_FORMAT " in '%s' with bounds %"
			EINA_RECTANGLE_FORMAT, EINA_RECTANGLE_ARGS (&span),
			r->name,  EINA_RECTANGLE_ARGS (&rbounds));

	if (sw_data->span)
	{
		Enesim_Color color;
		uint32_t *tmp;
		size_t bytes;

		color = enesim_renderer_color_get(r);
		bytes = rbounds.w * sizeof(uint32_t);
		tmp = alloca(bytes);

		/* We dont need to zero the buffer given that a fill will
		 * draw every pixel in case the span is inside the bounds
		 */
		sw_data->fill(r, rbounds.x, rbounds.y, rbounds.w, tmp);
		/* compose the filled and the destination spans */
		if (sw_data->use_mask)
		{
			uint32_t *mtmp;

			/* We assume it is 32bpp mask, later we can use the a8 variant */
			mtmp = alloca(bytes);
			enesim_renderer_sw_draw(r->state.current.mask, rbounds.x, rbounds.y, rbounds.w, mtmp);
			sw_data->span(data + left, rbounds.w, tmp, color, mtmp);
		}
		else
		{
			sw_data->span(data + left, rbounds.w, tmp, color, NULL);
		}
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
/** @endcond */
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
