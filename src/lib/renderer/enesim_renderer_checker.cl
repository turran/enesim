ENESIM_OPENCL_KERNEL(

typedef enum _Enesim_Rop
{
	ENESIM_ROP_BLEND,
	ENESIM_ROP_FILL,
} Enesim_Rop;

float point_transform(float2 xy, float3 mc)
{
	return (xy.x * mc.x) + (xy.y * mc.y) + mc.z;
}

float3 projective_setup(float2 xy, float2 oxy, float3 mx, float3 my, float3 mz)
{
	float3 xyz;
	xyz.x = point_transform(xy, mx) - oxy.x;
	xyz.y = point_transform(xy, my) - oxy.y;
	xyz.z = point_transform(xy, mz);

	return xyz;
}

uchar4 color_mul_256(ushort a, uchar4 c)
{
	uchar4 ret;
	ushort4 cs = upsample((uchar4)(0, 0, 0, 0), c);

	ret = convert_uchar4((a * cs) >> 8);
	//printf("color_mul_256: a = %#hx, c = %#v4hhx, ret = %#v4hhx\n", a, c, ret);
	return ret;
}

uchar4 color_interp_256(ushort a, uchar4 c0, uchar4 c1)
{
	uchar4 ret;
	ushort4 c0s = upsample((uchar4)(0, 0, 0, 0), c0);
	ushort4 c1s = upsample((uchar4)(0, 0, 0, 0), c1);

	ret = convert_uchar4((((c0s - c1s) * a) >> 8) + c1s);
	//printf("color_interp256: a = %#hx, c0 = %#v4hhx, c1 = %#v4hhx, ret = %#v4hhx\n", a, c0, c1, ret);
	return ret;
}

uchar4 color_blend(uchar4 d, ushort a, uchar4 s)
{
	uchar4 ret;

	ret = s + color_mul_256(a, d);
	//printf("color_blend: d = %#v4hhx, a = %#hx, s = %#v4hhx, ret = %#v4hhx\n", d, a, s, ret);
	return ret;
}

uchar4 pt_blend(uchar4 d, uchar4 s)
{
	ushort a;
	uchar4 ret;

	a = 256 - s.w;
	ret = color_blend(d, a, s);
	//printf("pt_blend: d = %#v4hhx, s = %#v44hx, a = %#hx, ret = %#v4hhx\n", d, s, a, ret);
	return ret;
}

__kernel void checker(read_write image2d_t out, int rop,
		__constant float *matrix, float2 oxy, uchar4 even_color,
		uchar4 odd_color, uint w, uint h)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	float fx;
	float fy;
	float ix;
	float iy;
	float xx;
	float yy;
	uchar4 color0 = even_color;
	uchar4 color1 = odd_color;
	uchar4 p0;
	uint h2 = h * 2;
	uint w2 = w * 2;
	uchar4 d0;

	float3 xyz = projective_setup((float2)(x, y), oxy,
			(float3)(matrix[0], matrix[1], matrix[2]),
			(float3)(matrix[3], matrix[4], matrix[5]),
			(float3)(matrix[6], matrix[7], matrix[8]));
	/* project */
	xx = xyz.x / xyz.z;
	yy = xyz.y / xyz.z;
	/* normalize the coordinate */
	yy = fmod(yy, h2);
	if (yy < 0)
		yy += h2;
	xx = fmod(xx, w2);
	if (xx < 0)
		xx += w2;
	/* choose the correct color */
	if (yy >= h)
	{
		color0 = odd_color;
		color1 = even_color;
	}
	if (xx >= w)
	{
		uchar4 tmp;

		tmp = color0;
		color0 = color1;
		color1 = tmp;
	}

	p0 = color0;

	fx = fract(xx, &ix);
	fy = fract(yy, &iy);
	if (iy == 0 || iy == h)
	{
		ushort a = 1 + convert_ushort(255 * fy);
		p0 = color_interp_256(a, color0, color1);
		if (ix == 0 || ix == w)
		{
			ushort a = 1 + convert_ushort(255 * fx);
			p0 = color_interp_256(a, color1, color0);
		}
	}
	else if (ix == 0 || ix == w)
	{
		ushort a = 1 + convert_ushort(255 * fx);
		p0 = color_interp_256(a, color0, color1);
	}

	if (rop == ENESIM_ROP_BLEND)
	{
		d0 = convert_uchar4(read_imageui(out, (int2)(x, y)));
		p0 = pt_blend(d0, p0);
	}
	write_imageui(out, (int2)(x, y), (uint4)(p0.x, p0.y, p0.z, p0.w));
}
);
