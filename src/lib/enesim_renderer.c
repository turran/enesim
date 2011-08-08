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
/**
 * @todo
 * - Add a way to retrieve the damaged area between two different state_setup
 * i.e state_setup, state_cleanup, property_changes .... , damage_get,
 * state_setup, state_cleanup ... etc
 * something like: void (*damages)(Enesim_Renderer *r, Eina_List **damages);
 * - Add a class name to the description
 * - Add a way to get/set such description
 * - Add a multi cpu render, send each span to a different CPU. One way is to
 *   use a queue, we initialize every thread on startup and me them sleep
 *   then wakeup when the function is called and start enqueueing the span
 *   renders
 * - Add a scale tranform property, every renderer should multiply its
 *   coordinates and lengths by this multiplier
 * - Change every internal struct on Enesim to have the correct prefix
 *   looks like we are having issues with mingw
 * - We have some overlfow on the coordinates whenever we want to trasnlate or
 *   transform boundings, we need to fix the maximum and minimum for a
 *   coordinate and length
 *
 */
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#define ENESIM_MAGIC_RENDERER 0xe7e51420
#define ENESIM_MAGIC_CHECK_RENDERER(d)\
	do {\
		if (!EINA_MAGIC_CHECK(d, ENESIM_MAGIC_RENDERER))\
			EINA_MAGIC_FAIL(d, ENESIM_MAGIC_RENDERER);\
	} while(0)

static void _draw_internal(Enesim_Renderer *r, Enesim_Surface *s,
		Eina_Rectangle *area, int x, int y, Enesim_Renderer_Flag flags)
{
	Enesim_Backend b;

	b = enesim_surface_backend_get(s);
	switch (b)
	{
		case ENESIM_BACKEND_SOFTWARE:
		enesim_renderer_sw_draw(r, s, area, x, y, flags);
		break;

		case ENESIM_BACKEND_OPENCL:
#if BUILD_OPENCL
		enesim_renderer_opencl_draw(r, s, area, x, y, flags);
#endif
		break;

		default:
		WRN("Backend not supported %d", b);
		break;
	}
}

static void _draw_list_internal(Enesim_Renderer *r, Enesim_Surface *s,
		Eina_Rectangle *area,
		Eina_List *clips, int x, int y, Enesim_Renderer_Flag flags)
{
	Enesim_Backend b;

	b = enesim_surface_backend_get(s);
	switch (b)
	{
		case ENESIM_BACKEND_SOFTWARE:
		enesim_renderer_sw_draw_list(r, s, area, clips, x, y, flags);
		break;

		default:
		break;
	}
}

static inline void _surface_boundings(Enesim_Surface *s, Eina_Rectangle *boundings)
{
	boundings->x = 0;
	boundings->y = 0;
	enesim_surface_size_get(s, &boundings->w, &boundings->h);
}

/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_renderer_init(void)
{
	enesim_renderer_sw_init();
}

void enesim_renderer_shutdown(void)
{
	enesim_renderer_sw_shutdown();
}

void enesim_renderer_relative_matrix_set(Enesim_Renderer *r, Enesim_Renderer *rel,
		Enesim_Matrix *old_matrix)
{
	Enesim_Matrix rel_matrix, r_matrix;

	/* the relativeness of the matrix should be done only if both
	 * both renderers have the matrix flag set
	 */
	/* TODO should we use the f16p16 matrix? */
	/* multiply the matrix by the current transformation */
	enesim_renderer_transformation_get(r, &r_matrix);
	enesim_renderer_transformation_get(rel, old_matrix);
	enesim_matrix_compose(old_matrix, &r_matrix, &rel_matrix);
	enesim_renderer_transformation_set(rel, &rel_matrix);
}

void enesim_renderer_relative_set(Enesim_Renderer *r, Enesim_Renderer *rel,
		Enesim_Matrix *old_matrix, double *old_ox, double *old_oy)
{
	Enesim_Matrix rel_matrix, r_matrix;
	double r_ox, r_oy;
	double nox, noy;

	if (!rel) return;

	/* TODO should we use the f16p16 matrix? */
	/* multiply the matrix by the current transformation */
	enesim_renderer_transformation_get(r, &r_matrix);
	enesim_renderer_transformation_get(rel, old_matrix);
	enesim_matrix_compose(old_matrix, &r_matrix, &rel_matrix);
	enesim_renderer_transformation_set(rel, &rel_matrix);
	/* add the origin by the current origin */
	enesim_renderer_origin_get(rel, old_ox, old_oy);
	enesim_renderer_origin_get(r, &r_ox, &r_oy);
	/* FIXME what to do with the origin?
	 * - if we use the first case we are also translating the rel origin to the
	 * parent origin * old_matrix
	 */
#if 1
	enesim_matrix_point_transform(old_matrix, *old_ox + r_ox, *old_oy + r_oy, &nox, &noy);
	enesim_renderer_origin_set(rel, nox, noy);
#else
	//printf("setting new origin %g %g\n", *old_ox - r_ox, *old_oy - r_oy);
	enesim_renderer_origin_set(rel, *old_ox - r_ox, *old_oy - r_oy);
#endif
}

void enesim_renderer_relative_unset(Enesim_Renderer *r, Enesim_Renderer *rel,
		Enesim_Matrix *old_matrix, double old_ox, double old_oy)
{
	if (!rel) return;

	/* restore origin */
	enesim_renderer_origin_set(rel, old_ox, old_oy);
	/* restore original matrix */
	enesim_renderer_transformation_set(rel, old_matrix);
}

/* FIXME expor this */
void * enesim_renderer_backend_data_get(Enesim_Renderer *r, Enesim_Backend b)
{
	return r->backend_data[b];
}

void enesim_renderer_backend_data_set(Enesim_Renderer *r, Enesim_Backend b, void *data)
{
	r->backend_data[b] = data;
}
/*============================================================================*
 *                                   API                                      *
 *============================================================================*/
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Renderer * enesim_renderer_new(Enesim_Renderer_Descriptor
		*descriptor, void *data)
{
	Enesim_Renderer *r;

	if (!descriptor) return NULL;
	if (descriptor->version > ENESIM_RENDERER_API) {
		ERR("API version %d is greater than %d", descriptor->version, ENESIM_RENDERER_API);
		return NULL;
	}

	r = calloc(1, sizeof(Enesim_Renderer));
	/* first check the passed in functions */
	if (!descriptor->is_inside) WRN("No is_inside() function available");
	if (!descriptor->boundings) WRN("No bounding() function available");
	if (!descriptor->flags) WRN("No flags() function available");
	if (!descriptor->sw_setup) WRN("No sw_setup() function available");
	if (!descriptor->sw_cleanup) WRN("No sw_setup() function available");
	if (!descriptor->free) WRN("No sw_setup() function available");
	r->descriptor = descriptor;
	r->data = data;
	/* now initialize the renderer common properties */
	EINA_MAGIC_SET(r, ENESIM_MAGIC_RENDERER);
	/* common properties */
	r->ox = 0;
	r->oy = 0;
	r->color = ENESIM_COLOR_FULL;
	r->rop = ENESIM_FILL;
	enesim_f16p16_matrix_identity(&r->matrix.values);
	enesim_matrix_identity(&r->matrix.original);
	r->matrix.type = ENESIM_MATRIX_IDENTITY;
	r->prv_data = eina_hash_string_superfast_new(NULL);

	return r;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void * enesim_renderer_data_get(Enesim_Renderer *r)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);

	return r->data;
}

/**
 * Sets the transformation matrix of a renderer
 * @param[in] r The renderer to set the transformation matrix on
 * @param[in] m The transformation matrix to set
 */
EAPI void enesim_renderer_transformation_set(Enesim_Renderer *r, Enesim_Matrix *m)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);

	if (!m)
	{
		enesim_f16p16_matrix_identity(&r->matrix.values);
		enesim_matrix_identity(&r->matrix.original);
		r->matrix.type = ENESIM_MATRIX_IDENTITY;
		return;
	}
	r->matrix.original = *m;
	enesim_matrix_f16p16_matrix_to(m, &r->matrix.values);
	r->matrix.type = enesim_f16p16_matrix_type_get(&r->matrix.values);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_transformation_get(Enesim_Renderer *r, Enesim_Matrix *m)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);

	if (m) *m = r->matrix.original;
}
/**
 * Deletes a renderer
 * @param[in] r The renderer to delete
 */
EAPI void enesim_renderer_delete(Enesim_Renderer *r)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (r->descriptor->free)
		r->descriptor->free(r);
	free(r);
}


/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Eina_Bool enesim_renderer_setup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Backend b;

	b = enesim_surface_backend_get(s);
	switch (b)
	{
		case ENESIM_BACKEND_SOFTWARE:
		{
			Enesim_Renderer_Sw_Fill fill;

			if (!enesim_renderer_sw_setup(r)) return EINA_FALSE;
			fill = enesim_renderer_sw_fill_get(r);
			if (!fill)
			{
				enesim_renderer_sw_cleanup(r);
				return EINA_FALSE;
			}
			return EINA_TRUE;
		}
		break;

		case ENESIM_BACKEND_OPENCL:
#if BUILD_OPENCL
		return enesim_renderer_opencl_setup(r, s);
#endif
		break;

		default:
		break;
	}
	return EINA_FALSE;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Backend b;

	b = enesim_surface_backend_get(s);
	switch (b)
	{
		case ENESIM_BACKEND_SOFTWARE:
		enesim_renderer_sw_cleanup(r);
		break;

		default:
		break;
	}
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (r->descriptor->flags)
	{
		r->descriptor->flags(r, flags);
		return;
	}
	*flags = 0;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_origin_set(Enesim_Renderer *r, double x, double y)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	r->ox = x;
	r->oy = y;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_origin_get(Enesim_Renderer *r, double *x, double *y)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (x) *x = r->ox;
	if (y) *y = r->oy;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_x_origin_set(Enesim_Renderer *r, double x)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	r->ox = x;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_x_origin_get(Enesim_Renderer *r, double *x)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (x) *x = r->ox;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_y_origin_set(Enesim_Renderer *r, double y)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	r->oy = y;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_y_origin_get(Enesim_Renderer *r, double *y)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (y) *y = r->oy;
}
/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	r->color = color;
}
/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (color) *color = r->color;
}

/**
 * Gets the bounding box of the renderer on its own coordinate space translated
 * by the origin
 * @param[in] r The renderer to get the boundings from
 * @param[out] rect The rectangle to store the boundings
 */
EAPI void enesim_renderer_translated_boundings(Enesim_Renderer *r, Enesim_Rectangle *boundings)
{
	Enesim_Renderer_Flag flags;
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (!boundings) return;

	enesim_renderer_boundings(r, boundings);
	enesim_renderer_flags(r, &flags);
	/* move by the origin */
#if LATER
	if (flags & ENESIM_RENDERER_FLAG_TRANSLATE)
#endif
	{
		if (boundings->x != INT_MIN / 2)
			boundings->x -= r->ox;
		if (boundings->y != INT_MIN / 2)
			boundings->y -= r->oy;
	}
}

/**
 * Gets the bounding box of the renderer on its own coordinate space without
 * adding the origin translation
 * @param[in] r The renderer to get the boundings from
 * @param[out] rect The rectangle to store the boundings
 */
EAPI void enesim_renderer_boundings(Enesim_Renderer *r, Enesim_Rectangle *rect)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (!rect) return;

	if (r->descriptor->boundings)
	{
		 r->descriptor->boundings(r, rect);
	}
	else
	{
		rect->x = INT_MIN / 2;
		rect->y = INT_MIN / 2;
		rect->w = INT_MAX;
		rect->h = INT_MAX;
	}
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_destination_boundings(Enesim_Renderer *r, Eina_Rectangle *rect,
		int x, int y)
{

	Enesim_Rectangle boundings;
	Enesim_Renderer_Flag flags;

	ENESIM_MAGIC_CHECK_RENDERER(r);

	if (!rect) return;

	enesim_renderer_boundings(r, &boundings);
	enesim_renderer_flags(r, &flags);
#if LATER
	if (flags & ENESIM_RENDERER_FLAG_TRANSLATE)
#endif
	{
		if (boundings.x != INT_MIN / 2)
			boundings.x += r->ox;
		if (boundings.y != INT_MIN / 2)
			boundings.y += r->oy;
	}
#if LATER
	if (flags & (ENESIM_RENDERER_FLAG_AFFINE | ENESIM_RENDERER_FLAG_PROJECTIVE))
#endif
	{
		if (r->matrix.type != ENESIM_MATRIX_IDENTITY && boundings.w != INT_MAX && boundings.h != INT_MAX)
		{
			Enesim_Quad q;
			Enesim_Matrix m;

			enesim_matrix_inverse(&r->matrix.original, &m);
			enesim_matrix_rectangle_transform(&m, &boundings, &q);
			enesim_quad_rectangle_to(&q, &boundings);
		}
	}
	rect->x = lround(boundings.x) - x;
	rect->y = lround(boundings.y) - y;
	rect->w = lround(boundings.w);
	rect->h = lround(boundings.h);
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rop_set(Enesim_Renderer *r, Enesim_Rop rop)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	r->rop = rop;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rop_get(Enesim_Renderer *r, Enesim_Rop *rop)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (rop) *rop = r->rop;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI Eina_Bool enesim_renderer_is_inside(Enesim_Renderer *r, double x, double y)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (r->descriptor->is_inside) return r->descriptor->is_inside(r, x, y);
	return EINA_TRUE;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_private_set(Enesim_Renderer *r, const char *name, void *data)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	eina_hash_add(r->prv_data, name, data);
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void * enesim_renderer_private_get(Enesim_Renderer *r, const char *name)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	return eina_hash_find(r->prv_data, name);
}

/**
 * Draw a renderer into a surface
 * @param[in] r The renderer to draw
 * @param[in] s The surface to draw the renderer into
 * @param[in] clip The area on the destination surface to limit the drawing
 * @param[in] x The x origin of the destination surface
 * @param[in] y The y origin of the destination surface
 * TODO What about the mask?
 */
EAPI void enesim_renderer_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Eina_Rectangle *clip, int x, int y)
{
	Enesim_Renderer_Flag flags;
	Eina_Rectangle boundings;
	Eina_Rectangle final;

	ENESIM_MAGIC_CHECK_RENDERER(r);
	ENESIM_MAGIC_CHECK_SURFACE(s);

	if (!enesim_renderer_setup(r, s)) return;

	if (!clip)
	{
		_surface_boundings(s, &final);
	}
	else
	{
		Eina_Rectangle surface_size;

		final.x = clip->x;
		final.y = clip->y;
		final.w = clip->w;
		final.h = clip->h;
		_surface_boundings(s, &surface_size);
		if (!eina_rectangle_intersection(&final, &surface_size))
		{
			WRN("The renderer %p boundings does not intersect with the surface", r);
			goto end;
		}
	}
	/* clip against the destination rectangle */
	enesim_renderer_destination_boundings(r, &boundings, 0, 0);
	if (!eina_rectangle_intersection(&final, &boundings))
	{
		WRN("The renderer %p boundings does not intersect on the destination rectangle", r);
		goto end;
	}
	enesim_renderer_flags(r, &flags);
	_draw_internal(r, s, &final, x, y, flags);

	/* TODO set the format again */
end:
	enesim_renderer_cleanup(r, s);
}

/**
 * Draw a renderer into a surface
 * @param[in] r The renderer to draw
 * @param[in] s The surface to draw the renderer into
 * @param[in] clips A list of clipping areas on the destination surface to limit the drawing
 * @param[in] x The x origin of the destination surface
 * @param[in] y The y origin of the destination surface
 */
EAPI void enesim_renderer_draw_list(Enesim_Renderer *r, Enesim_Surface *s,
		Eina_List *clips, int x, int y)
{
	Enesim_Renderer_Flag flags;
	Eina_Rectangle boundings;
	Eina_Rectangle surface_size;

	if (!clips)
	{
		enesim_renderer_draw(r, s, NULL, x, y);
		return;
	}

	ENESIM_MAGIC_CHECK_RENDERER(r);
	ENESIM_MAGIC_CHECK_SURFACE(s);

	/* setup the common parameters */
	if (!enesim_renderer_setup(r, s)) return;

	_surface_boundings(s, &surface_size);
	/* clip against the destination rectangle */
	enesim_renderer_destination_boundings(r, &boundings, 0, 0);
	if (!eina_rectangle_intersection(&boundings, &surface_size))
	{
		WRN("The renderer %p boundings does not intersect on the destination rectangle", r);
		goto end;
	}
	enesim_renderer_flags(r, &flags);
	_draw_list_internal(r, s, &boundings, clips, x, y, flags);

	/* TODO set the format again */
end:
	enesim_renderer_cleanup(r, s);
}

#if 0
/* Given how renderes are being developed outside enesim, we need to define a helper function that creates
 * renderers based on others, just a wrapper for a renderer. For example, on a sw based renderer call the
 * fill function of the wrapped renderer so the developers should not provide one. Or the setup function
 * where we usually propagate the color, the origin or other properties into the wrapped renderer
 */

/* define the *must* implement functions */
typedef struct _Enesim_Renderer_Wrapper_Descriptor
{
	Enesim_Renderer_Delete free;
	Enesim_Renderer_Boundings boundings;
	Enesim_Renderer_Flags flags;
	Enesim_Renderer_Inside is_inside;
	Enesim_Renderer_Wrapper_Setup wrapper_setup;
	Enesim_Renderer_Wrapper_Cleanup wrapper_cleanup;
} Enesim_Rendere_Wrapper_Descriptor;

#if BUILD_OPENCL

static Eina_Bool _wrapper_sw_setup(Enesim_Renderer *r, Enesim_Renderer_Sw_Fill *fill)
{
	Eina_Bool ret = EINA_TRUE;

	if (r->wrapper_descriptor->setup)
		ret = r->wrapper_descriptor->setup(r);
	if (!ret)
	{
		return EINA_FALSE;
	}
	return enesim_renderer_sw_setup(r->wrapper_renderer, fill);
}

static void _wrapper_sw_cleanup(Enesim_Renderer *r)
{
	enesim_renderer_sw_cleanup(r->wrapper_renderer);
	if (r->wrapper_descriptor->cleanup)
		ret = r->wrapper_descriptor->cleanup(r);
	if (!ret)
	{
		return EINA_FALSE;
	}
}

static Eina_Bool _wrapper_opencl_setup(Enesim_Renderer *r, Enesim_Surface *s,
		const char **program_name, const char **program_source,
		size_t *program_length)
{
	Eina_Bool ret = EINA_TRUE;

	if (r->wrapper_descriptor->setup)
		ret = r->wrapper_descriptor->setup(r);
	if (!ret)
	{
		return EINA_FALSE;
	}
	return enesim_renderer_opencl_setup(r->wrapper_renderer, s,
			program_name, program_source, program_length);
}

static Eina_Bool _wrapper_opencl_kernel_setup(Enesim_Renderer *r, Enesim_Surface *s)
{
	return enesim_renderer_opencl_kernel_setup(r->wrapper_renderer, s);
}

static void _wrapper_opencl_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	enesim_renderer_opencl_cleanup(r->wrapper_renderer, s);
	if (r->wrapper_descriptor->cleanup)
		r->wrapper_descriptor->cleanup(r);
}
#endif

static void _wrapper_boundings(Enesim_Renderer *r, Enesim_Rectangle *rect)
{
	if (r->wrapper_descriptor->boundings)
		r->wrapper_descriptor->boundings(r, rect);
	else
		enesim_renderer_boundings(r->wrapper_renderer, rect);

}

static void _wrapper_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	if (r->wrapper_descriptor->flags)
		r->wrapper_descriptor->flags(r, flags);
	else
		enesim_renderer_flags(r->wrapper_renderer, flags);
}

static void _wrapper_free(Enesim_Renderer *r)
{
	if (r->wrapper_descriptor->free)
		r->wrapper_descriptor->free(r);
	enesim_renderer_delete(r->wrapper_renderer);
}

static Enesim_Renderer_Descriptor _wrapper_descriptor = {
	/* .version =               */ ENESIM_RENDERER_API,
	/* .free =                  */ _wrapper_free,
	/* .boundings =             */ _wrapper_boundings,
	/* .flags =                 */ _wrapper_flags,
	/* .is_inside =             */ _wrapper_is_inside,
	/* .sw_setup =              */ _wrapper_sw_setup,
	/* .sw_cleanup =            */ _wrapper_sw_cleanup,
#if BUILD_OPENCL
	/* .opencl_setup =          */ _wrapper_opencl_setup,
	/* .opencl_kernel_setup =   */ _wrapper_opencl_kernel_setup,
	/* .opencl_cleanup =        */ _wrapper_opencl_cleanup,
#endif
};

/**
 * @param[in] wrapped The renderer to wrap
 * @param[in] wdescriptor The wrapper descriptor definition
 */
EAPI Enesim_Renderer * enesim_renderer_new_from_renderer(Enesim_Renderer *wrapped,
		Enesim_Renderer_Wrapper_Descriptor *wdescriptor)
{
	Enesim_Renderer_Descriptor *new_descriptor;
	Enesim_Renderer *r:

	if (!wrapped) return NULL;
	/* if we use the new here, we need to refactor the way we handle
	 * inheritance
	 */
	enesim_renderer_new??
}
#endif
