ENESIM_OPENCL_KERNEL(

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


__kernel void checker(write_only image2d_t out, __constant float *matrix,
		float2 oxy, uchar4 even_color, uchar4 odd_color, uint w,
		uint h)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	uchar4 color0 = even_color;
	uchar4 color1 = odd_color;
	uchar4 p0;
	uint h2 = h * 2;
	uint w2 = w * 2;

	float3 xyz = projective_setup((float2)(x, y), oxy,
			(float3)(matrix[0], matrix[1], matrix[2]),
			(float3)(matrix[3], matrix[4], matrix[5]),
			(float3)(matrix[6], matrix[7], matrix[8]));
	/* project */
	float xx = xyz.x / xyz.z;
	float yy = xyz.y / xyz.z;
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
		p0 = color0;
	}
	else
	{
		p0 = color1;
	}
	write_imageui(out, (int2)(x, y), (uint4)(p0.x, p0.y, p0.z, p0.w));
}
);

