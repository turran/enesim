ENESIM_OPENCL_CODE(

static float _linear_distance(float2 ay, float2 c0, float scale, float2 c)
{
	float d;

	c = c - c0;
	c = ay * c;
	d = c.x + c.y;
	d = d * scale;

	return d;
}

__kernel void gradient_linear(read_write image2d_t out, int rop,
		read_only image1d_t span, sampler_t sampler,
		__constant float *matrix, float2 oxy, float2 ay,
		float2 c0, float scale)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	float xx;
	float yy;
	float d;
	uchar4 p0;

	float3 xyz = enesim_coord_opencl_projective_setup(
			(float2)(x, y), oxy,
			(float3)(matrix[0], matrix[1], matrix[2]),
			(float3)(matrix[3], matrix[4], matrix[5]),
			(float3)(matrix[6], matrix[7], matrix[8]));
	/* project */
	xx = xyz.x / xyz.z;
	yy = xyz.y / xyz.z;

	/* get the distance */
	d = _linear_distance(ay, c0, scale, (float2)(xx, yy));
	/* get the color from the 1d span */
	p0 = convert_uchar4(read_imageui(span, sampler, d));

	if (rop == ENESIM_ROP_BLEND)
	{
		uchar4 d0;

		d0 = convert_uchar4(read_imageui(out, (int2)(x, y)));
		p0 = enesim_color_blend_opencl_pt_none_color_none(d0, p0);
	}
	write_imageui(out, (int2)(x, y), (uint4)(p0.x, p0.y, p0.z, p0.w));
}
);

