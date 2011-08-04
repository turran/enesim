ENESIM_OPENCL_KERNEL(
__kernel void background(__global image2d_t out, __global uint4 c)
{
	int x = get_global_id(0);
	int y = get_global_id(1);

	write_imagef(out, (int2)(x, y), c);
}
);
