ENESIM_OPENGL_SHADER(
uniform vec2 linear_ay;
uniform vec2 linear_o;
uniform float linear_scale;
uniform float linear_matrix;
uniform int linear_length;
uniform sampler1D linear_stops;
/* 0 = restrict
 * 1 = pad
 * 2 = reflect
 * 3 = repeat
 */
uniform int gradient_repeat_mode;


void main()
{
	float x;
	float y;
	float a;
	float d;
	vec4 texel;

	/* get the distance */
 	x = gl_TexCoord[0].x - linear_o.x;
	y = gl_TexCoord[0].y - linear_o.y;
	d = ((linear_ay.x * x) + (linear_ay.y * y)) * linear_scale;
	
	/* TODO get the texcoord of the stops based on the repeat mode */
	if (d < 0.0)
	{
		gl_FragColor = vec4(0, 0, 0, 0);
	}
	else if (d >= float(linear_length - 1))
	{
		gl_FragColor = vec4(0, 0, 0, 0);
	}
	else
	{
		texel = texture1D(linear_stops, d/float(linear_length - 1));
		gl_FragColor = texel;
	}
}
)
