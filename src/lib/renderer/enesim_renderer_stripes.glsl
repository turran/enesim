ENESIM_OPENGL_SHADER(
uniform int stripes_even_thickness;
uniform int stripes_odd_thickness;
uniform vec4 stripes_even_color;
uniform vec4 stripes_odd_color;

void main()
{
	int y;
	float fy;
	vec4 p0;

	y = int(mod(gl_TexCoord[0].y, stripes_even_thickness + stripes_odd_thickness));
	fy = fract(gl_TexCoord[0].y);

	if (y >= stripes_even_thickness)
	{
		p0 = stripes_odd_color;
		if (y == stripes_even_thickness)
		{
			p0 = mix(stripes_even_color, p0, fy);
		}
	}
	else
	{
		p0 = stripes_even_color;
		if (y == 0)
		{
			p0 = mix(stripes_odd_color, p0, fy);
		}
	}

	gl_FragColor = p0;
}
)
