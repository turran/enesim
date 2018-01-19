ENESIM_OPENCL_CODE(
__kernel void compound(read_write image2d_t out, int rop,
		read_only image2d_t in)
{
	int x = get_global_id(0);
	int y = get_global_id(1);
	uchar4 p0;

	p0 = convert_uchar4(read_imageui(in, (int2)(x, y)));
	if (rop == ENESIM_ROP_BLEND)
	{
		uchar4 d0;

		d0 = convert_uchar4(read_imageui(out, (int2)(x, y)));
		p0 = enesim_color_blend_opencl_pt_none_color_none(d0, p0);
	}
	write_imageui(out, (int2)(x, y), (uint4)(p0.x, p0.y, p0.z, p0.w));
}
);
