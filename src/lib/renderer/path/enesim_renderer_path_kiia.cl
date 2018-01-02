ENESIM_OPENCL_CODE(
void path_kiia_even_odd_evaluate(__constant float *edges, int nedges)
{
}

__kernel void path_kiia(read_write image2d_t out, int rop,
		__constant float *fill_edges, int nfedges, uchar4 fcolor,
		__constant float *stroke_edges, int nsedges, uchar4 scolor)
{
	int y = get_global_id(0);
	int x;

	dump_figure(fill_edges, nfedges);
	for (x = 0; x < get_image_width(out); x++)
	{
		uchar4 p0 = (uchar4)(0,0,0,0);
		write_imageui(out, (int2)(x, y), (uint4)(p0.x, p0.y, p0.z, p0.w));

	}
}
);
