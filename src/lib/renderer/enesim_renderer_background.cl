ENESIM_OPENCL_KERNEL(
__kernel void background(read_write image2d_t out, cl_int rop, uchar4 c)
{
	int x = get_global_id(0);
	int y = get_global_id(1);

	write_imageui(out, (int2)(x, y), (uint4)(c.x, c.y, c.z, c.w));
}
);
