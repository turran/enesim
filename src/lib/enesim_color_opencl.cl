ENESIM_OPENCL_CODE(

uchar4 enesim_color_opencl_mul_256(ushort a, uchar4 c)
{
	uchar4 ret;
	ushort4 cs = upsample((uchar4)(0, 0, 0, 0), c);

	ret = convert_uchar4((a * cs) >> 8);
	//printf("color_mul_256: a = %#hx, c = %#v4hhx, ret = %#v4hhx\n", a, c, ret);
	return ret;
}

uchar4 enesim_color_opencl_interp_256(ushort a, uchar4 c0, uchar4 c1)
{
	uchar4 ret;
	ushort4 c0s = upsample((uchar4)(0, 0, 0, 0), c0);
	ushort4 c1s = upsample((uchar4)(0, 0, 0, 0), c1);

	ret = convert_uchar4((((c0s - c1s) * a) >> 8) + c1s);
	//printf("color_interp256: a = %#hx, c0 = %#v4hhx, c1 = %#v4hhx, ret = %#v4hhx\n", a, c0, c1, ret);
	return ret;
}

uchar4 enesim_color_opencl_blend(uchar4 d, ushort a, uchar4 s)
{
	uchar4 ret;

	ret = s + enesim_color_opencl_mul_256(a, d);
	//printf("color_blend: d = %#v4hhx, a = %#hx, s = %#v4hhx, ret = %#v4hhx\n", d, a, s, ret);
	return ret;
}
)
