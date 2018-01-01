ENESIM_OPENCL_KERNEL(
uchar4 enesim_color_blend_opencl_pt_none_color_none(uchar4 d, uchar4 s)
{
	ushort a;
	uchar4 ret;

	a = 256 - s.w;
	ret = enesim_color_opencl_blend(d, a, s);
	//printf("pt_blend: d = %#v4hhx, s = %#v44hx, a = %#hx, ret = %#v4hhx\n", d, s, a, ret);
	return ret;
}
)
