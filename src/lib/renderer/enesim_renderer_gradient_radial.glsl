ENESIM_OPENGL_SHADER(
uniform bool radial_simple;
uniform vec2 radial_c;
uniform vec2 radial_f;
uniform float radial_scale;
uniform float radial_rad2;
uniform float radial_zf;
uniform int gradient_length;
uniform sampler1D gradient_stops;
/* 0 = restrict
 * 1 = pad
 * 2 = reflect
 * 3 = repeat
 */
uniform int gradient_repeat_mode;

vec4 gradient_restrict(float d)
{
	if (d < 0.0)
	{
		return vec4(0, 0, 0, 0);
	}
	else if (d >= float(gradient_length - 1))
	{
		return vec4(0, 0, 0, 0);
	}
	else
	{
		return texture1D(gradient_stops, d/float(gradient_length - 1));
	}
}

vec4 gradient_pad(float d)
{
	if (d < 0.0)
	{
		return texture1D(gradient_stops, 0.0);
	}
	else if (d >= float(gradient_length - 1))
	{
		return texture1D(gradient_stops, 0.99);
	}
	else
	{
		return texture1D(gradient_stops, d/float(gradient_length - 1));
	}
}

float radial_distance(float x, float y)
{
	float a;
	float b;
	float d1;
	float d2;
	float d;

	if (radial_simple)
	{
		a = x - radial_c.x;
		b = y - radial_c.y;
		d = radial_scale * length(vec2(a, b));
		return d;
	}

	a = radial_scale * (x - (radial_f.x + radial_c.x));
	b = radial_scale * (y - (radial_f.y + radial_c.y));

	d1 = (a * radial_f.y) - (b * radial_f.x);
	d2 = abs((radial_rad2 * ((a * a) + (b * b))) - (d1 * d1));
	d = ((a * radial_f.x) + (b * radial_f.y) + sqrt(d2)) * radial_zf;

	return d;
}

void main()
{
	float x;
	float y;
	float d;

	/* get the position */
 	x = gl_TexCoord[0].x;
	y = gl_TexCoord[0].y;

	/* get the distance */
	d = radial_distance(x, y);

	if (gradient_repeat_mode == 0)
		gl_FragColor = gradient_restrict(d);
	else
		gl_FragColor = gradient_pad(d);
}
)
