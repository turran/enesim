ENESIM_OPENGL_SHADER(
uniform int checker_sw;
uniform int checker_sh;
uniform vec4 checker_even_color;
uniform vec4 checker_odd_color;

void main()
{
	vec4 colors[2];
	vec4 p0;
	vec4 p1;
	float x;
	float y;
	float fx;
	float fy;

 	// gl_FragCoord
 	x = mod(floor(gl_TexCoord[0].x), checker_sw * 2);
	y = mod(floor(gl_TexCoord[0].y), checker_sh * 2);
	fx = fract(gl_TexCoord[0].x);
	fy = fract(gl_TexCoord[0].y);
	colors[0] = checker_even_color;
	colors[1] = checker_odd_color;

	/* pick the correct color */
	if (y > checker_sh)
	{
		colors[0] = checker_odd_color;
		colors[1] = checker_even_color;
	}
	if (x > checker_sw)
	{
		p0 = colors[0];
		p1 = colors[1];
	}
	else
	{
		p0 = colors[1];
		p1 = colors[0];
	}

	/* antialias the borders */
	if (y == 0)
	{
		p0 = mix(p1, p0, fy);
	}
 	else if (y == checker_sh)
	{
		p0 = mix(p0, p1, fy);
	}

	if (x == checker_sw)
	{
		p0 = mix(p0, p1, fx);
	}
	else if (x == 0)
	{
		p0 = mix(p1, p0, fx);
	}

	gl_FragColor = p0;
}
);

