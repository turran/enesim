ENESIM_OPENCL_KERNEL(
__kernel void background(write_only image2d_t out, uint4 c)
{
	int x = get_global_id(0);
	int y = get_global_id(1);

	//write_imageui(out, (int2)(x, y), (uint4)(0, 0, 128, 255));
	write_imageui(out, (int2)(x, y), c);
}
);
