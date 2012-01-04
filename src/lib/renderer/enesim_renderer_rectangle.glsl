ENESIM_OPENGL_SHADER(
uniform float rectangle_width;
uniform float rectangle_height;
uniform float rectangle_x;
uniform float rectangle_y;

void main()
{
	//gl_Position = gl_ProjectionMatrix  * gl_Position;

	gl_Position = gl_PositionIn[0];
	gl_Position.xy = vec2(rectangle_x, rectangle_y);
	EmitVertex();

	gl_Position.xy = vec2(rectangle_x + rectangle_width, rectangle_y);
	EmitVertex();

	gl_Position.xy = vec2(rectangle_x + rectangle_width, rectangle_y + rectangle_height);
	EmitVertex();

	gl_Position.xy = vec2(rectangle_x, rectangle_y + rectangle_height);
	EmitVertex();

	EndPrimitive();
}
);
