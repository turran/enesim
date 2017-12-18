ENESIM_OPENCL_KERNEL(
__kernel void checker(write_only image2d_t out, uchar4 even_color, uchar4 odd_color)
{
	int x = get_global_id(0);
	int y = get_global_id(1);

	write_imageui(out, (int2)(x, y), (uint4)(even_color.x, even_color.y, even_color.z, even_color.w));
}
);

