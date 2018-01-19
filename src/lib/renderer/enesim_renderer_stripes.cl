ENESIM_OPENCL_CODE(

__kernel void stripes(read_write image2d_t out, int rop,
		__constant float *matrix, float2 oxy, uchar4 even_color,
		uchar4 odd_color, float even_thickness, float odd_thickness)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	float fy;
	float iy;
	float yy;
	uchar4 color0 = even_color;
	uchar4 color1 = odd_color;
	uchar4 p0;
	float h = odd_thickness;
	float h2 = odd_thickness + even_thickness;

	float3 xyz = enesim_coord_opencl_projective_setup(
			(float2)(x + 1, y + 1), oxy,
			(float3)(matrix[0], matrix[1], matrix[2]),
			(float3)(matrix[3], matrix[4], matrix[5]),
			(float3)(matrix[6], matrix[7], matrix[8]));
	/* project */
	yy = xyz.y / xyz.z;
	/* normalize the coordinate */
	yy = fmod(yy, h2);
	if (yy < 0)
		yy += h2;
	/* choose the correct color */
	if (yy >= h)
	{
		color0 = odd_color;
		color1 = even_color;
	}

	p0 = color0;

	fy = fract(yy, &iy);
	if (iy == 0 || iy == h)
	{
		ushort a = 1 + convert_ushort(255 * fy);
		p0 = enesim_color_opencl_interp_256(a, color0, color1);
	}

	if (rop == ENESIM_ROP_BLEND)
	{
		uchar4 d0;

		d0 = convert_uchar4(read_imageui(out, (int2)(x, y)));
		p0 = enesim_color_blend_opencl_pt_none_color_none(d0, p0);
	}
	write_imageui(out, (int2)(x, y), (uint4)(p0.x, p0.y, p0.z, p0.w));
}
);
