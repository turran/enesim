ENESIM_OPENCL_CODE(
float enesim_coord_opencl_transform(float2 xy, float3 mc)
{
	return (xy.x * mc.x) + (xy.y * mc.y) + mc.z;
}

float3 enesim_coord_opencl_projective_setup(float2 xy, float2 oxy, float3 mx, float3 my, float3 mz)
{
	float3 xyz;
	xyz.x = enesim_coord_opencl_transform(xy, mx) - oxy.x;
	xyz.y = enesim_coord_opencl_transform(xy, my) - oxy.y;
	xyz.z = enesim_coord_opencl_transform(xy, mz);

	return xyz;
}
)
