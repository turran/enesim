ENESIM_OPENGL_SHADER(
uniform vec2 linear_ay;
uniform vec2 linear_o;
uniform float linear_scale;
uniform float linear_matrix;
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

float linear_distance(float x, float y)
{
	float d;

 	x = x - linear_o.x;
	y = y - linear_o.y;
	d = ((linear_ay.x * x) + (linear_ay.y * y)) * linear_scale;
	return d;
}

void main()
{
	float x;
	float y;
	float a;
	float d;
	vec4 texel;

	/* get the position */
 	x = gl_TexCoord[0].x;
	y = gl_TexCoord[0].y;

	/* get the distance */
	d = linear_distance(x, y);
	/* TODO get the texcoord of the stops based on the repeat mode */
	if (gradient_repeat_mode == 0)
		gl_FragColor = gradient_restrict(d);
	else
		gl_FragColor = gradient_pad(d);
}
)
