ENESIM_OPENCL_KERNEL(
__kernel void background(write_only __global image2d_t out, uint4 c)
{
	int x = get_global_id(0);
	int y = get_global_id(1);

	write_imageui(out, (int2)(x, y), c);
}
);
