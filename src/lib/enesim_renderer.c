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
 * - Add a way to get/set such description
 * - Change every internal struct on Enesim to have the correct prefix
 *   looks like we are having issues with mingw
 * - We have some overlfow on the coordinates whenever we want to trasnlate or
 *   transform boundings, we need to fix the maximum and minimum for a
 *   coordinate and length
 *
 * - Maybe checking the flags for every origin/value set is too much
 * another option is to make every renderer responsable of changing the origin/scale/etc
 * if the owned renderer handles that
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

typedef struct _Enesim_Renderer_Destination_Damage_Data
{
	Enesim_Renderer_Flag flags;
	Enesim_Renderer_Destination_Damage_Cb real_cb;
	void *real_data;
} Enesim_Renderer_Destination_Damage_Data;

typedef struct _Enesim_Renderer_Factory
{
	int id;
} Enesim_Renderer_Factory;

static Eina_Hash *_factories = NULL;

/*----------------------------------------------------------------------------*
 *                   Functions to transform the boundings                     *
 *----------------------------------------------------------------------------*/
static void _enesim_renderer_boundings(Enesim_Renderer *r, Enesim_Rectangle *boundings)
{
	if (r->descriptor.boundings)
	{
		 r->descriptor.boundings(r, boundings);
	}
	else
	{
		boundings->x = INT_MIN / 2;
		boundings->y = INT_MIN / 2;
		boundings->w = INT_MAX;
		boundings->h = INT_MAX;
	}
}

static void _enesim_renderer_destination_boundings(Enesim_Renderer *r,
		Enesim_Renderer_Flag flags,
		Enesim_Renderer_State *state,
		Enesim_Rectangle *boundings,
		int x, int y,
		Eina_Rectangle *destination)
{
	if (r->descriptor.destination_transform)
	{
		r->descriptor.destination_transform(r, boundings, destination);
	}
	else
	{
		/* translate */
		if (flags & ENESIM_RENDERER_FLAG_TRANSLATE)
		{
			if (boundings->x != INT_MIN / 2)
				boundings->x += state->ox;
			if (boundings->y != INT_MIN / 2)
				boundings->y += state->oy;
		}
		if (flags & (ENESIM_RENDERER_FLAG_AFFINE | ENESIM_RENDERER_FLAG_PROJECTIVE))
		{
			if (state->transformation_type != ENESIM_MATRIX_IDENTITY && boundings->w != INT_MAX && boundings->h != INT_MAX)
			{
				Enesim_Quad q;
				Enesim_Matrix m;

				enesim_matrix_inverse(&state->transformation, &m);
				enesim_matrix_rectangle_transform(&m, boundings, &q);
				enesim_quad_rectangle_to(&q, boundings);
			}
		}
		destination->x = lround(boundings->x);
		destination->y = lround(boundings->y);
		destination->w = lround(boundings->w);
		destination->h = lround(boundings->h);
	}
	destination->x -= x;
	destination->y -= y;
}

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

static void _enesim_renderer_factory_setup(Enesim_Renderer *r)
{
	Enesim_Renderer_Factory *f;
	char renderer_name[PATH_MAX];
	const char *descriptor_name = NULL;

	if (!_factories) return;
	if (r->descriptor.name)
		descriptor_name = r->descriptor.name(r);
	if (!descriptor_name)
		descriptor_name = "unknown";
	f = eina_hash_find(_factories, descriptor_name);
	if (!f)
	{
		f = calloc(1, sizeof(Enesim_Renderer_Factory));
		eina_hash_add(_factories, descriptor_name, f);
	}
	/* assign a new name for it automatically */
	snprintf(renderer_name, PATH_MAX, "%s%d", descriptor_name, f->id++);
	enesim_renderer_name_set(r, renderer_name);
}

static Eina_Bool _enesim_renderer_common_changed(Enesim_Renderer *r)
{
	/* the rop */
	if (r->current.rop != r->past.rop)
	{
		return EINA_TRUE;
	}
	/* the color */
	if (r->current.color != r->past.color)
	{
		return EINA_TRUE;
	}
	/* the origin */
	if (r->current.ox != r->past.ox || r->current.oy != r->past.oy)
	{
		return EINA_TRUE;
	}
	/* the scale */
	if (r->current.sx != r->past.sx || r->current.sy != r->past.sy)
	{
		return EINA_TRUE;
	}
	/* the transformation */
	if (r->current.transformation_type != r->past.transformation_type)
	{
		return EINA_TRUE;
	}

	if (r->current.transformation.xx != r->past.transformation.xx ||
			r->current.transformation.xy != r->past.transformation.xy ||
			r->current.transformation.xz != r->past.transformation.xz ||
			r->current.transformation.yx != r->past.transformation.yx ||
			r->current.transformation.yy != r->past.transformation.yy ||
			r->current.transformation.yz != r->past.transformation.yz ||
			r->current.transformation.zx != r->past.transformation.zx ||
			r->current.transformation.zy != r->past.transformation.zy ||
			r->current.transformation.zz != r->past.transformation.zz)
	{
		return EINA_TRUE;
	}

	return EINA_FALSE;
}

static Eina_Bool _enesim_renderer_destination_boundings_cb(Enesim_Renderer *r,
		Enesim_Rectangle *area, Eina_Bool past, void *data)
{
	Enesim_Renderer_Destination_Damage_Data *ddata = data;
	Eina_Rectangle real_area;

	/* translate, scale, transform, whatever, and send the area back */
	if (past)
	{
		_enesim_renderer_destination_boundings(r, ddata->flags,
				&r->past, area, 0, 0, &real_area);
	}
	else
	{
		_enesim_renderer_destination_boundings(r, ddata->flags,
				&r->current, area, 0, 0, &real_area);
	}

	return ddata->real_cb(r, &real_area, past, ddata->real_data);
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
void enesim_renderer_init(void)
{
	_factories = eina_hash_string_superfast_new(NULL);
	enesim_renderer_sw_init();
}

void enesim_renderer_shutdown(void)
{
	eina_hash_free(_factories);
	_factories = NULL;
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
		Enesim_Matrix *old_matrix, double *old_ox, double *old_oy,
		double *old_sx, double *old_sy)
{
	Enesim_Matrix rel_matrix, r_matrix;
	double r_ox, r_oy;
	double nox, noy;

	if (!rel) return;

	if (r->current.transformation_type != ENESIM_MATRIX_IDENTITY)
	{
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

		enesim_renderer_scale_get(rel, old_sx, old_sy);
		enesim_renderer_scale_set(rel, r->current.sx * *old_sx, r->current.sy * *old_sy);
#else
		//printf("setting new origin %g %g\n", *old_ox - r_ox, *old_oy - r_oy);
		enesim_renderer_origin_set(rel, *old_ox - r_ox, *old_oy - r_oy);
#endif
	}
	else
	{
		enesim_renderer_origin_get(rel, old_ox, old_oy);
		enesim_renderer_origin_set(rel, r->current.ox + *old_ox, r->current.oy + *old_oy);

		enesim_renderer_scale_get(rel, old_sx, old_sy);
		enesim_renderer_scale_set(rel, r->current.sx * *old_sx, r->current.sy * *old_sy);

		enesim_renderer_transformation_get(rel, old_matrix);
	}
}

void enesim_renderer_relative_unset(Enesim_Renderer *r, Enesim_Renderer *rel,
		Enesim_Matrix *old_matrix, double old_ox, double old_oy,
		double old_sx, double old_sy)
{
	if (!rel) return;

	/* restore origin */
	enesim_renderer_origin_set(rel, old_ox, old_oy);
	/* restore the scale */
	enesim_renderer_scale_set(rel, old_sx, old_sy);
	/* restore original matrix */
	enesim_renderer_transformation_set(rel, old_matrix);
}

void enesim_renderer_identity_setup(Enesim_Renderer *r, int x, int y,
		Eina_F16p16 *fpx, Eina_F16p16 *fpy)
{
	Eina_F16p16 ox, oy;

	ox = eina_f16p16_double_from(r->current.ox);
	oy = eina_f16p16_double_from(r->current.oy);

	*fpx = eina_f16p16_int_from(x);
	*fpy = eina_f16p16_int_from(y);

	*fpx = eina_f16p16_sub(*fpx, ox);
	*fpy = eina_f16p16_sub(*fpy, oy);
}

/*
 * x' = (xx * x) + (xy * y) + xz;
 * y' = (yx * x) + (yy * y) + yz;
 */
void enesim_renderer_affine_setup(Enesim_Renderer *r, int x, int y,
		Enesim_F16p16_Matrix *matrix,
		Eina_F16p16 *fpx, Eina_F16p16 *fpy)
{
	Eina_F16p16 xx, yy;
	Eina_F16p16 ox, oy;

	ox = eina_f16p16_double_from(r->current.ox);
	oy = eina_f16p16_double_from(r->current.oy);

	xx = eina_f16p16_int_from(x);
	yy = eina_f16p16_int_from(y);

	*fpx = enesim_point_f16p16_transform(xx, yy, matrix->xx,
			matrix->xy, matrix->xz);
	*fpy = enesim_point_f16p16_transform(xx, yy, matrix->yx,
			matrix->yy, matrix->yz);

	*fpx = eina_f16p16_sub(*fpx, ox);
	*fpy = eina_f16p16_sub(*fpy, oy);
}

/*
 * x' = (xx * x) + (xy * y) + xz;
 * y' = (yx * x) + (yy * y) + yz;
 * z' = (zx * x) + (zy * y) + zz;
 */
void enesim_renderer_projective_setup(Enesim_Renderer *r, int x, int y,
		Enesim_F16p16_Matrix *matrix,
		Eina_F16p16 *fpx, Eina_F16p16 *fpy, Eina_F16p16 *fpz)
{
	Eina_F16p16 xx, yy;
	Eina_F16p16 ox, oy;

	ox = eina_f16p16_double_from(r->current.ox);
	oy = eina_f16p16_double_from(r->current.oy);

	xx = eina_f16p16_int_from(x);
	yy = eina_f16p16_int_from(y);
	*fpx = enesim_point_f16p16_transform(xx, yy, matrix->xx,
			matrix->xy, matrix->xz);
	*fpy = enesim_point_f16p16_transform(xx, yy, matrix->yx,
			matrix->yy, matrix->yz);
	*fpz = enesim_point_f16p16_transform(xx, yy, matrix->zx,
			matrix->zy, matrix->zz);

	*fpx = eina_f16p16_sub(*fpx, ox);
	*fpy = eina_f16p16_sub(*fpy, oy);
}

/* FIXME export this */
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
	r->descriptor = *descriptor;
	r->data = data;
	/* now initialize the renderer common properties */
	EINA_MAGIC_SET(r, ENESIM_MAGIC_RENDERER);
	/* common properties */
	r->current.ox = r->past.ox = 0;
	r->current.oy = r->past.oy = 0;
	r->current.sx = r->past.sx = 1;
	r->current.sy = r->past.sy = 1;
	r->current.color = r->past.color = ENESIM_COLOR_FULL;
	r->current.rop = r->past.rop = ENESIM_FILL;
	enesim_matrix_identity(&r->current.transformation);
	enesim_matrix_identity(&r->past.transformation);
	r->current.transformation_type = ENESIM_MATRIX_IDENTITY;
	r->past.transformation_type = ENESIM_MATRIX_IDENTITY;
	/* private stuff */
	enesim_rectangle_coords_from(&r->past_boundings, 0, 0, 0, 0);
	r->prv_data = eina_hash_string_superfast_new(NULL);
	/* always set the first reference */
	r = enesim_renderer_ref(r);
	_enesim_renderer_factory_setup(r);

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
 * To be documented
 * FIXME: To be fixed
 */
EAPI Enesim_Renderer * enesim_renderer_ref(Enesim_Renderer *r)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	r->ref++;
	return r;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_unref(Enesim_Renderer *r)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	r->ref--;
	if (!r->ref)
	{
		if (r->descriptor.free)
			r->descriptor.free(r);
		free(r);
	}
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI Eina_Bool enesim_renderer_setup(Enesim_Renderer *r, Enesim_Surface *s, Enesim_Error **error)
{
	Enesim_Backend b;
	Eina_Bool ret = EINA_TRUE;

	ENESIM_MAGIC_CHECK_RENDERER(r);
	b = enesim_surface_backend_get(s);
	switch (b)
	{
		case ENESIM_BACKEND_SOFTWARE:
		{
			Enesim_Renderer_Sw_Fill fill;

			if (!enesim_renderer_sw_setup(r, &r->current, s, error))
			{
				ENESIM_RENDERER_ERROR(r, error, "Software setup failed");
				ret = EINA_FALSE;
				break;
			}
			fill = enesim_renderer_sw_fill_get(r);
			if (!fill)
			{
				ENESIM_RENDERER_ERROR(r, error, "Even if the setup did not failed, there's no fill function");
				enesim_renderer_sw_cleanup(r, s);
				ret = EINA_FALSE;
				break;
			}
		}
		break;

		case ENESIM_BACKEND_OPENCL:
#if BUILD_OPENCL
		if (!enesim_renderer_opencl_setup(r, &r->current, s, error))
		{
			ENESIM_RENDERER_ERROR(r, error, "OpenCL setup failed");
			ret = EINA_FALSE;
		}
#endif
		break;

		default:
		break;
	}
	enesim_renderer_boundings(r, &r->current_boundings);

	return ret;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_cleanup(Enesim_Renderer *r, Enesim_Surface *s)
{
	Enesim_Backend b;

	ENESIM_MAGIC_CHECK_RENDERER(r);
	b = enesim_surface_backend_get(s);
	switch (b)
	{
		case ENESIM_BACKEND_SOFTWARE:
		enesim_renderer_sw_cleanup(r, s);
		break;

		default:
		break;
	}
	/* swap the states */
	/* first stuff that needs to be freed */
	if (r->past.name && r->past.name != r->current.name) free(r->past.name);
	r->past = r->current;
	r->past_boundings = r->current_boundings;
	DBG("Cleaning up the renderer %s", r->current.name);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_flags(Enesim_Renderer *r, Enesim_Renderer_Flag *flags)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (r->descriptor.flags)
	{
		r->descriptor.flags(r, flags);
		return;
	}
	*flags = 0;
}

/**
 * Sets the transformation matrix of a renderer
 * @param[in] r The renderer to set the transformation matrix on
 * @param[in] m The transformation matrix to set
 */
EAPI void enesim_renderer_transformation_set(Enesim_Renderer *r, Enesim_Matrix *m)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	Enesim_Renderer_Flag flags;

	if (!m)
	{
		enesim_matrix_identity(&r->current.transformation);
		r->current.transformation_type = ENESIM_MATRIX_IDENTITY;
		return;
	}
	enesim_renderer_flags(r, &flags);
	if (!(flags & (ENESIM_RENDERER_FLAG_AFFINE | ENESIM_RENDERER_FLAG_PROJECTIVE)))
		return;
	r->current.transformation = *m;
	r->current.transformation_type = enesim_matrix_type_get(&r->current.transformation);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_transformation_get(Enesim_Renderer *r, Enesim_Matrix *m)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);

	if (m) *m = r->current.transformation;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_name_set(Enesim_Renderer *r, const char *name)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (r->current.name) free(r->current.name);
	r->current.name = strdup(name);
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_name_get(Enesim_Renderer *r, const char **name)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	*name = r->current.name;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_origin_set(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Flag flags;

	ENESIM_MAGIC_CHECK_RENDERER(r);
	enesim_renderer_flags(r, &flags);
	if (!(flags & ENESIM_RENDERER_FLAG_TRANSLATE))
		return;
	r->current.ox = x;
	r->current.oy = y;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_origin_get(Enesim_Renderer *r, double *x, double *y)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (x) *x = r->current.ox;
	if (y) *y = r->current.oy;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_x_origin_set(Enesim_Renderer *r, double x)
{
	Enesim_Renderer_Flag flags;

	ENESIM_MAGIC_CHECK_RENDERER(r);
	enesim_renderer_flags(r, &flags);
	if (!(flags & ENESIM_RENDERER_FLAG_TRANSLATE))
		return;
	r->current.ox = x;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_x_origin_get(Enesim_Renderer *r, double *x)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (x) *x = r->current.ox;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_y_origin_set(Enesim_Renderer *r, double y)
{
	Enesim_Renderer_Flag flags;

	ENESIM_MAGIC_CHECK_RENDERER(r);
	enesim_renderer_flags(r, &flags);
	if (!(flags & ENESIM_RENDERER_FLAG_TRANSLATE))
		return;
	r->current.oy = y;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_y_origin_get(Enesim_Renderer *r, double *y)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (y) *y = r->current.oy;
}

/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_scale_set(Enesim_Renderer *r, double x, double y)
{
	Enesim_Renderer_Flag flags;

	ENESIM_MAGIC_CHECK_RENDERER(r);
	enesim_renderer_flags(r, &flags);
	if (!(flags & ENESIM_RENDERER_FLAG_SCALE))
		return;
	r->current.sx = x;
	r->current.sy = y;
}
/**
 * To be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_scale_get(Enesim_Renderer *r, double *x, double *y)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (x) *x = r->current.sx;
	if (y) *y = r->current.sy;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_x_scale_set(Enesim_Renderer *r, double x)
{
	Enesim_Renderer_Flag flags;

	ENESIM_MAGIC_CHECK_RENDERER(r);
	enesim_renderer_flags(r, &flags);
	if (!(flags & ENESIM_RENDERER_FLAG_SCALE))
		return;
	r->current.sx = x;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_x_scale_get(Enesim_Renderer *r, double *x)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (x) *x = r->current.sx;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_y_scale_set(Enesim_Renderer *r, double y)
{
	Enesim_Renderer_Flag flags;

	ENESIM_MAGIC_CHECK_RENDERER(r);
	enesim_renderer_flags(r, &flags);
	if (!(flags & ENESIM_RENDERER_FLAG_SCALE))
		return;
	r->current.sy = y;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_y_scale_get(Enesim_Renderer *r, double *y)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (y) *y = r->current.sy;
}
/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_color_set(Enesim_Renderer *r, Enesim_Color color)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	r->current.color = color;
}
/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_color_get(Enesim_Renderer *r, Enesim_Color *color)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (color) *color = r->current.color;
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

	_enesim_renderer_boundings(r, rect);
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

	enesim_renderer_flags(r, &flags);
	_enesim_renderer_boundings(r, &boundings);
	_enesim_renderer_destination_boundings(r, flags, &r->current, &boundings, x, y, rect);
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rop_set(Enesim_Renderer *r, Enesim_Rop rop)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	r->current.rop = rop;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_rop_get(Enesim_Renderer *r, Enesim_Rop *rop)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (rop) *rop = r->current.rop;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI Eina_Bool enesim_renderer_is_inside(Enesim_Renderer *r, double x, double y)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (r->descriptor.is_inside) return r->descriptor.is_inside(r, x, y);
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
EAPI Eina_Bool enesim_renderer_draw(Enesim_Renderer *r, Enesim_Surface *s,
		Eina_Rectangle *clip, int x, int y, Enesim_Error **error)
{
	Enesim_Renderer_Flag flags;
	Eina_Rectangle boundings;
	Eina_Rectangle final;

	ENESIM_MAGIC_CHECK_RENDERER(r);
	ENESIM_MAGIC_CHECK_SURFACE(s);

	if (!enesim_renderer_setup(r, s, error)) return EINA_FALSE;

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
	return EINA_TRUE;
}

/**
 * Draw a renderer into a surface
 * @param[in] r The renderer to draw
 * @param[in] s The surface to draw the renderer into
 * @param[in] clips A list of clipping areas on the destination surface to limit the drawing
 * @param[in] x The x origin of the destination surface
 * @param[in] y The y origin of the destination surface
 */
EAPI Eina_Bool enesim_renderer_draw_list(Enesim_Renderer *r, Enesim_Surface *s,
		Eina_List *clips, int x, int y, Enesim_Error **error)
{
	Enesim_Renderer_Flag flags;
	Eina_Rectangle boundings;
	Eina_Rectangle surface_size;

	if (!clips)
	{
		enesim_renderer_draw(r, s, NULL, x, y, error);
		return EINA_TRUE;
	}

	ENESIM_MAGIC_CHECK_RENDERER(r);
	ENESIM_MAGIC_CHECK_SURFACE(s);

	/* setup the common parameters */
	if (!enesim_renderer_setup(r, s, error)) return EINA_FALSE;

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
	return EINA_TRUE;
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_error_add(Enesim_Renderer *r, Enesim_Error **error, const char *file,
		const char *function, int line, char *fmt, ...)
{
	va_list args;
	char str[PATH_MAX];
	int num;

	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (!error)
		return;

	va_start(args, fmt);
	num = snprintf(str, PATH_MAX, "%s:%d %s %s ", file, line, function, r->current.name);
	num += vsnprintf(str + num, PATH_MAX - num, fmt, args);
	str[num] = '\n';
	va_end(args);

	*error = enesim_error_add(*error, str);
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI Eina_Bool enesim_renderer_has_changed(Enesim_Renderer *r)
{
	Eina_Bool ret = EINA_TRUE;

	ENESIM_MAGIC_CHECK_RENDERER(r);

	/* if the renderer does not has the descriptor
	 * function we assume it is always true */
	if (r->descriptor.has_changed)
	{
		/* first check if the common properties have changed */
		if (!_enesim_renderer_common_changed(r))
		{
			ret = r->descriptor.has_changed(r);
		}
	}
	if (ret)
	{
		DBG("The renderer %s has changed", r->current.name);
	}
	return ret;
}

/**
 * To  be documented
 * FIXME: To be fixed
 * We also need the same variants as the boundings() function, that is,
 * _destination, _scaled and _translated
 */
EAPI void enesim_renderer_damages_get(Enesim_Renderer *r, Enesim_Renderer_Damage_Cb cb, void *data)
{
	ENESIM_MAGIC_CHECK_RENDERER(r);

	if (!cb) return;

	if (!enesim_renderer_has_changed(r))
		return;
	if (r->descriptor.damage)
	{
		r->descriptor.damage(r, cb, data);
	}
	else
	{
		Enesim_Rectangle current_boundings;
		/* send the old bounds and the new one */
		enesim_renderer_boundings(r, &current_boundings);
		cb(r, &current_boundings, EINA_FALSE, data);
		cb(r, &r->past_boundings, EINA_TRUE, data);
	}
}

/**
 * To  be documented
 * FIXME: To be fixed
 */
EAPI void enesim_renderer_destination_damages_get(Enesim_Renderer *r, Enesim_Renderer_Destination_Damage_Cb cb, void *data)
{
	Enesim_Renderer_Destination_Damage_Data ddata;
	Enesim_Renderer_Flag flags;

	ENESIM_MAGIC_CHECK_RENDERER(r);
	if (!cb) return;

	enesim_renderer_flags(r, &flags);

	ddata.real_data = data;
	ddata.real_cb = cb;
	ddata.flags = flags;
	enesim_renderer_damages_get(r, _enesim_renderer_destination_boundings_cb, &ddata);
}
