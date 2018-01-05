ENESIM_OPENCL_CODE(
/*----------------------------------------------------------------------------*
 *                                 Pattern                                    *
 *----------------------------------------------------------------------------*/
__constant float kiia_pattern_32[32] = {
	28.0/32.0, 13.0/32.0, 6.0/32.0, 23.0/32.0,
	0.0/32.0, 17.0/32.0, 10.0/32.0, 27.0/32.0,
	4.0/32.0, 21.0/32.0, 14.0/32.0, 31.0/32.0,
	8.0/32.0, 25.0/32.0, 18.0/32.0,	3.0/32.0,
	12.0/32.0, 29.0/32.0, 22.0/32.0, 7.0/32.0,
	16.0/32.0, 1.0/32.0, 26.0/32.0, 11.0/32.0,
	20.0/32.0, 5.0/32.0, 30.0/32.0, 15.0/32.0,
	24.0/32.0, 9.0/32.0, 2.0/32.0, 19.0/32.0
};
/*----------------------------------------------------------------------------*
 *                                 Sampling                                   *
 *----------------------------------------------------------------------------*/
void kiia_even_odd_sample(__global int *mask, int x, int m)
{
	mask[x] ^= m;
}

int kiia_even_odd_get_mask(__global int *mask, int x, int cm)
{
	int oldcm = cm;
	cm ^= mask[x];
	//printf("mask = %08x, cm = %08x, oldcm = %08x\n", mask[x], cm, oldcm);
	mask[x] = 0;
	return cm;
}

void kiia_non_zero_sample(int *mask, int x, int m)
{
	mask[x] += m;
}

int2 kiia_non_zero_get_mask(int *mask, int x, int cm, int cacc)
{
	int m;
	int rcacc;

	m = mask[x];
	mask[x] = 0;
	rcacc = cacc + m;
	cacc = abs(cacc);
	if (cacc < 32)
	{
		if (cacc == 0)
			cm = 0;
		else
			cm = cacc;
	}
	else
	{
		cm = 32;
	}
	return (int2)(cm, rcacc);
}

/*----------------------------------------------------------------------------*
 *                                 Evaluation                                 *
 *----------------------------------------------------------------------------*/
void kiia_even_odd_evaluate(__constant float *edges, int nedges,
		__global int *mask, int y, float lx)
{
	float y0 = y;
	float y1 = y + 1;
	int i;

	for (i = 0; i < nedges; i++)
	{
		float cx;
		float yy0, yy1;
		int m;
		int ii;

		__constant float *edge = &edges[i * 7];

		//printf("i: %d edge: %f %f %f %f\n", i, edge[0], edge[1], edge[2], edge[3]);
		/* y1 */
		if (y0 >= edge[3])
			continue;
		/* y0 */
		if (y1 < edge[1])
			break;

		/* make sure not overflow */
		yy1 = y1;
		if (yy1 > edge[3])
			yy1 = edge[3];
		yy0 = y0;
		if (yy0 <= edge[1])
		{
			int sample;
			float tmp;

			yy0 = edge[1];
			cx = edge[5];
			sample = 32 * fract(yy0, &tmp);
			m = 1 << sample;
		}
		else
		{
			float inc = (yy0 - edge[1]) * 32;
			cx = edge[5] + (inc * edge[4]);
			m = 1;
		}
		/* finally sample */
		/* 0.03125 = 1/32, the step on y */
		//printf("* yy0 = %f, yy1 = %f, cx = %f, m = %d\n", yy0, yy1, cx, m);
		for (ii = 0; yy0 < yy1; yy0 += 0.03125, ii++)
		{
			int mx;

			mx = (int)(cx - lx + kiia_pattern_32[ii]);
			//printf("yy0 = %f, cx = %f, lx = %f, mx = %d, m = %d\n", yy0, cx, lx, mx, m);
			kiia_even_odd_sample(mask, mx, m);
			cx += edge[4];
			m <<= 1;
		}
	}
}

void kiia_non_zero_evaluate(__constant float *edges, int nedges,
		__global int *mask, int y, float lx)
{
	float y0 = y;
	float y1 = y + 1;
	int i;

	for (i = 0; i < nedges; i++)
	{
		float cx;
		float yy0, yy1;
		int ii;

		__constant float *edge = &edges[i * 7];

		//printf("i: %d edge: %f %f %f %f\n", i, edge[0], edge[1], edge[2], edge[3]);
		/* y1 */
		if (y0 >= edge[3])
			continue;
		/* y0 */
		if (y1 < edge[1])
			break;

		/* make sure not overflow */
		yy1 = y1;
		if (yy1 > edge[3])
			yy1 = edge[3];
		yy0 = y0;
		if (yy0 <= edge[1])
		{
			yy0 = edge[1];
			cx = edge[5];
		}
		else
		{
			float inc = (yy0 - edge[1]) * 32;
			cx = edge[5] + (inc * edge[4]);
		}
		/* finally sample */
		/* 0.03125 = 1/32, the step on y */
		//printf("* yy0 = %f, yy1 = %f, cx = %f, m = %d\n", yy0, yy1, cx, m);
		for (ii = 0; yy0 < yy1; yy0 += 0.03125, ii++)
		{
			int mx;

			mx = (int)(cx - lx + kiia_pattern_32[ii]);
			//printf("yy0 = %f, cx = %f, lx = %f, mx = %d, m = %d\n", yy0, cx, lx, mx, m);
			kiia_non_zero_sample(mask, mx, edge[6]);
			cx += edge[4];
		}
	}
}
/*----------------------------------------------------------------------------*
 *                              Alpha getters                                 *
 *----------------------------------------------------------------------------*/
ushort kiia_even_odd_get_alpha(int cm)
{
	ushort coverage;

	/* use the hamming weight to know the number of bits set to 1 */
	cm = cm - ((cm >> 1) & 0x55555555);
	cm = (cm & 0x33333333) + ((cm >> 2) & 0x33333333);
	/* we use 21 instead of 24, because we need to rescale 32 -> 256 */
	coverage = (((cm + (cm >> 4)) & 0x0f0f0f0f) * 0x01010101) >> 21;

	return coverage;
}

ushort kiia_non_zero_get_alpha(int cm)
{
	return cm <<  3;
}

/*----------------------------------------------------------------------------*
 *                             Simple rendering                               *
 *----------------------------------------------------------------------------*/
uchar4 kiia_figure_color_fill(uchar4 color, ushort coverage)
{
	uchar4 p0;
	p0 = enesim_color_opencl_mul_256(coverage, color);
	return p0;
}

uchar4 kiia_figure_color_color_fill(uchar4 fcolor, ushort falpha,
		uchar4 scolor, ushort salpha)
{
	uchar4 p0;
	uchar4 q0;

	q0 = enesim_color_opencl_mul_256(falpha, fcolor);
	p0 = enesim_color_opencl_interp_256(salpha, scolor, q0);
	return p0;
}

__kernel void path_kiia(read_write image2d_t out, int rop, int fill_rule,
		__constant float *fill_edges, int nfedges, uchar4 fcolor,
		__constant float *stroke_edges, int nsedges, uchar4 scolor,
		__global int *mask, __global int *omask, int mask_len,
		int4 bounds)
{
	int y = get_global_id(0);
	int x;

	if (y < bounds.s1 || y > bounds.s3)
		return;
	else
	{
		int cm = 0;
		int cwinding = 0;
		int ocm = 0;
		int owinding = 0;

		__global int *rmask;
		__global int *romask;
		/* initialize the mask */
		if (mask && omask)
		{
 			rmask = &mask[(y - bounds.s1)* mask_len];
 			romask = &omask[(y - bounds.s1) * mask_len];
			for (x = 0; x < mask_len; x++)
			{
				rmask[x] = 0;
				romask[x] = 0;
			}
		}
		else
		{
			if (mask)
			{
 				rmask = &mask[y * mask_len];
			}
			else
			{
 				rmask = &omask[y * mask_len];
				fill_edges = stroke_edges;
				nfedges = nsedges;
				fcolor = scolor;
				fill_rule = ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO;
				omask = NULL;
			}
			for (x = 0; x < mask_len; x++)
			{
				rmask[x] = 0;
			}
		}

		if (fill_rule == ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO)
			kiia_non_zero_evaluate(fill_edges, nfedges, rmask, y, bounds.s0);
		else
			kiia_even_odd_evaluate(fill_edges, nfedges, rmask, y, bounds.s0);
		if (omask)
			kiia_non_zero_evaluate(stroke_edges, nsedges, romask, y, bounds.s0);
		//printf("x = %d, y = %d, m = %d\n", x, y, mask[0]);
		for (x = bounds.s0; x < bounds.s2; x++)
		{
			uchar4 p0;
			ushort falpha;
			ushort salpha;

			int rx = x - bounds.s0;
			if (fill_rule == ENESIM_RENDERER_SHAPE_FILL_RULE_NON_ZERO)
			{
				int2 ret;
				ret = kiia_non_zero_get_mask(rmask, rx, cm, cwinding);
				cm = ret.x;
				cwinding = ret.y;
				falpha = kiia_non_zero_get_alpha(cm);
			}
			else
			{
				cm = kiia_even_odd_get_mask(rmask, rx, cm);
				falpha = kiia_even_odd_get_alpha(cm);
			}
			if (omask)
			{
				int2 ret;
				ret = kiia_non_zero_get_mask(romask, rx, ocm, owinding);
				ocm = ret.x;
				owinding = ret.y;
				salpha = kiia_non_zero_get_alpha(ocm);

				/* compose the stroke + fill */
				p0 = kiia_figure_color_color_fill(fcolor, falpha, scolor, salpha);
			}
			else
			{
				p0 = kiia_figure_color_fill(fcolor, falpha);
			}
			if (rop == ENESIM_ROP_BLEND)
			{
				uchar4 d0;

				d0 = convert_uchar4(read_imageui(out, (int2)(x, y)));
				p0 = enesim_color_blend_opencl_pt_none_color_none(d0, p0);
			}
			write_imageui(out, (int2)(x, y), (uint4)(p0.x, p0.y, p0.z, p0.w));
		}
	}
}
);
