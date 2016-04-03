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

/* Approach to the Loop & Blinn algorithm found on
 * http://research.microsoft.com/en-us/um/people/cloop/LoopBlinn05.pdf
 * Also with some ideas taken from WebCore code
 */
#include "enesim_private.h"
#include "enesim_figure.h"
#include "enesim_path.h"

#include "enesim_path_private.h"
#include "enesim_vector_private.h"
#include "enesim_curve_private.h"
#include "enesim_curve_loop_blinn_private.h"

/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
/** @cond internal */
static inline double _round_to_zero(double v, double err)
{
	if (v < err && v > -err)
		return 0;
	return v;
}
/*============================================================================*
 *                                 Global                                     *
 *============================================================================*/
/* classify the type of cubic curve so we can tesselate it correctly */
void enesim_curve_loop_blinn_classify(Enesim_Path_Cubic *c, double err,
		Enesim_Curve_Loop_Blinn_Classification *clas)
{
	Enesim_Point b0, b1, b2, b3;
	Enesim_Point tmp;
	double a1, a2, a3;
	double d1, d2, d3;
	double term0;
	double discriminant;

	/* create homogeneous points */
	enesim_point_coords_set(&b0, c->start_x, c->start_y, 1.0);
	enesim_point_coords_set(&b1, c->ctrl_x0, c->ctrl_y0, 1.0);
	enesim_point_coords_set(&b2, c->ctrl_x1, c->ctrl_y1, 1.0);
	enesim_point_coords_set(&b3, c->end_x, c->end_y, 1.0);

	/* compute a1..a3 */
	enesim_point_3d_cross(&b3, &b2, &tmp);
	a1 = enesim_point_3d_dot(&b0, &tmp);

	enesim_point_3d_cross(&b0, &b3, &tmp);
	a2 = enesim_point_3d_dot(&b1, &tmp);

	enesim_point_3d_cross(&b1, &b0, &tmp);
	a3 = enesim_point_3d_dot(&b2, &tmp);

	/* compute d1..d3 */
	d1 = a1 - 2 * a2 + 3 * a3;
	d2 = -a2 + 3 * a3;
	d3 = 3 * a3;

	/* normalize to avoid rounding errors */
	enesim_point_coords_set(&tmp, d1, d2, d3);
	enesim_point_3d_normalize(&tmp, &tmp);
	enesim_point_coords_get(&tmp, &d1, &d2, &d3);

	/* compute the discriminant. */
	/* term0 is a common term in the computation which helps decide
	 * which way to classify the cusp case: as serpentine or loop.
	 */
	term0 = (3 * d2 * d2 - 4 * d1 * d3);
	discriminant = d1 * d1 * term0;

	/* when the values are near zero, round to zero to avoid
	 * numerical errors
	 */
	d1 = _round_to_zero(d1, err);
	d2 = _round_to_zero(d2, err);
	d3 = _round_to_zero(d3, err);
	discriminant = _round_to_zero(discriminant, err);

	/* finally do the classification */
	clas->d1 = d1;
	clas->d2 = d2;
	clas->d3 = d3;
	if (enesim_point_3d_is_equal(&b0, &b1, err) &&
			enesim_point_3d_is_equal(&b0, &b2, err) &&
			enesim_point_3d_is_equal(&b0, &b3, err))
	{
		clas->type = ENESIM_CURVE_LOOP_BLINN_POINT;
	}

	if (!discriminant)
	{
		if (!d1 && !d2)
		{
			if (!d3) clas->type = ENESIM_CURVE_LOOP_BLINN_LINE;
			else clas->type = ENESIM_CURVE_LOOP_BLINN_QUADRATIC;
		}
		else if (!d1)
		{
			clas->type = ENESIM_CURVE_LOOP_BLINN_CUSP;
		}
		else
		{
			if (term0 < 0) clas->type = ENESIM_CURVE_LOOP_BLINN_LOOP;
			else clas->type = ENESIM_CURVE_LOOP_BLINN_SERPENTINE;
			
		}
	}
	else if (discriminant > 0)
	{
		clas->type = ENESIM_CURVE_LOOP_BLINN_SERPENTINE;
	}
	else
	{
		clas->type = ENESIM_CURVE_LOOP_BLINN_LOOP;
	}
}

Eina_Bool enesim_curve_loop_blinn_compute_texcoords(
		Enesim_Curve_Loop_Blinn_Classification *clas)
{
	Enesim_Point k, l, m, n;
	/* check for reverse orientation */
	Eina_Bool reverse = EINA_FALSE;
	Eina_Bool has_artifact = EINA_FALSE;
	double divide_at;
	const double one_third = 1.0 / 3.0;
	const double two_thirds = 2.0 / 3.0;

	switch (clas->type)
	{
		case ENESIM_CURVE_LOOP_BLINN_POINT:
		case ENESIM_CURVE_LOOP_BLINN_LINE:
		return EINA_FALSE;

		case ENESIM_CURVE_LOOP_BLINN_QUADRATIC:
		enesim_point_coords_set(&k, 0, 0, 0);
		enesim_point_coords_set(&l, one_third, 0, one_third);
		enesim_point_coords_set(&m, two_thirds, one_third, two_thirds);
		enesim_point_coords_set(&n, 1, 1, 1);
		if (clas->d3 < 0)
			reverse = EINA_TRUE;
		break;

		case ENESIM_CURVE_LOOP_BLINN_CUSP:{
		double ls = clas->d3;
		double lt = 3.0 * clas->d2;
		double lsMinusLt = ls - lt;
		enesim_point_coords_set(&k, ls, ls * ls * ls, 1.0);
		enesim_point_coords_set(&l, ls - one_third * lt, ls * ls * lsMinusLt, 1.0);
		enesim_point_coords_set(&m, ls - two_thirds * lt, lsMinusLt * lsMinusLt * ls, 1.0);
		enesim_point_coords_set(&n, lsMinusLt, lsMinusLt * lsMinusLt * lsMinusLt, 1.0);
		}break;

		case ENESIM_CURVE_LOOP_BLINN_LOOP:{
		double t1 = sqrt(4.0 * clas->d1 * clas->d3 - 3.0 * clas->d2 * clas->d2);
		double ls = clas->d2 - t1;
		double lt = 2.0 * clas->d1;
		double ms = clas->d2 + t1;
		double mt = lt;

		/* check for subdivision */
		double ql = ls / lt;
		double qm = ms / mt;
		if (0.0 < ql && ql < 1.0) {
		    has_artifact = EINA_TRUE;
		    divide_at = ql;
		    return EINA_TRUE;
		}

		if (0.0 < qm && qm < 1.0) {
		    has_artifact = EINA_TRUE;
		    divide_at = qm;
		    return EINA_TRUE;
		}

		double ltMinusLs = lt - ls;
		double mtMinusMs = mt - ms;
		enesim_point_coords_set(&k, ls * ms, ls * ls * ms, ls * ms * ms);
		enesim_point_coords_set(&l, one_third * (-ls * mt - lt * ms + 3.0 * ls * ms),
				-one_third * ls * (ls * (mt - 3.0 * ms) + 2.0 * lt * ms),
				-one_third * ms * (ls * (2.0 * mt - 3.0 * ms) + lt * ms));
		enesim_point_coords_set(&m, one_third * (lt * (mt - 2.0 * ms) + ls * (3.0 * ms - 2.0 * mt)),
				one_third * (lt - ls) * (ls * (2.0 * mt - 3.0 * ms) + lt * ms),
				one_third * (mt - ms) * (ls * (mt - 3.0 * ms) + 2.0 * lt * ms));
		enesim_point_coords_set(&n, ltMinusLs * mtMinusMs,
				-(ltMinusLs * ltMinusLs) * mtMinusMs,
				-ltMinusLs * mtMinusMs * mtMinusMs);
		reverse = ((clas->d1 > 0.0 && k.x < 0.0)
				   || (clas->d1 < 0.0 && k.x > 0.0));
		}break;

		case ENESIM_CURVE_LOOP_BLINN_SERPENTINE:{
		double t1 = sqrtf(9.0 * clas->d2 * clas->d2 - 12 * clas->d1 * clas->d3);
		double ls = 3.0 * clas->d2 - t1;
		double lt = 6.0 * clas->d1;
		double ms = 3.0 * clas->d2 + t1;
		double mt = lt;
		double ltMinusLs = lt - ls;
		double mtMinusMs = mt - ms;
		enesim_point_coords_set(&k, ls * ms, ls * ls * ls, ms * ms * ms);
		enesim_point_coords_set(&l, one_third * (3.0 * ls * ms - ls * mt - lt * ms),
			     	ls * ls * (ls - lt),
			     	ms * ms * (ms - mt));
		enesim_point_coords_set(&m, one_third * (lt * (mt - 2.0 * ms) + ls * (3.0 * ms - 2.0 * mt)),
			     	ltMinusLs * ltMinusLs * ls,
			     	mtMinusMs * mtMinusMs * ms);
		enesim_point_coords_set(&n, ltMinusLs * mtMinusMs,
				-(ltMinusLs * ltMinusLs * ltMinusLs),
				-(mtMinusMs * mtMinusMs * mtMinusMs));
		if (clas->d1 < 0.0)
		    reverse = EINA_TRUE;
		}break;

		default:
		return EINA_FALSE;
		break;

	}
#if 0
	if (sideToFill == LoopBlinnConstants::RightSide)
		reverse = !reverse;

	if (reverse) {
		for (int i = 0; i < 4; ++i) {
			result.klmCoordinates[i].setX(-result.klmCoordinates[i].x());
			result.klmCoordinates[i].setY(-result.klmCoordinates[i].y());
		}
	}
#endif

    return EINA_TRUE;
}
/** @endcond */
