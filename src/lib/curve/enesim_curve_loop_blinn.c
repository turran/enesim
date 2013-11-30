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
#include "enesim_path.h"

#include "enesim_path_private.h"
#include "enesim_vector_private.h"
#include "enesim_curve_private.h"
#include "enesim_curve_loop_blinn_private.h"
/*============================================================================*
 *                                  Local                                     *
 *============================================================================*/
#if 0
static Eina_Bool enesim_curve_loop_blinn_compute_texcoords(
		Enesim_Curve_Loop_Blinn_Classification *clas)
{
	switch (clas->type)
	{
		case ENESIM_CURVE_LOOP_BLINN_POINT:
		case ENESIM_CURVE_LOOP_BLINN_LINE:
		case ENESIM_CURVE_LOOP_BLINN_UNKNOWN:
		return EINA_FALSE;

		case ENESIM_CURVE_LOOP_BLINN_QUADRATIC:
		case ENESIM_CURVE_LOOP_BLINN_CUSP:
		case ENESIM_CURVE_LOOP_BLINN_LOOP:
		case ENESIM_CURVE_LOOP_BLINN_SERPENTINE:
		return EINA_TRUE;
	}
}
#endif
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
#if 0
	d1 = roundToZero(d1);
	d2 = roundToZero(d2);
	d3 = roundToZero(d3);
	discriminant = roundToZero(discriminant);
#endif

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
