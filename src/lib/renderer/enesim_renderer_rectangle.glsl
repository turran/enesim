ENESIM_OPENGL_SHADER(
uniform float rectangle_width;
uniform float rectangle_height;
uniform float rectangle_x;
uniform float rectangle_y;

void main()
{
	gl_Position = gl_ModelViewMatrix * gl_PositionIn[0];
	gl_Position.xy = vec2(-0.5, -0.5);
	gl_TexCoord[0]  = gl_TexCoordIn[0][0] / 2;
	EmitVertex();

	gl_Position.xy = vec2(0.5, -0.5);
	gl_TexCoord[0] = vec4(1, 1, 0, 1);
	EmitVertex();

	gl_Position.xy = vec2(-0.5, 0.3);
	gl_TexCoord[0]  = gl_TexCoordIn[0][0] / 2;
	EmitVertex();

	gl_Position.xy = vec2(0.5, 0.3);
	gl_TexCoord[0]  = gl_TexCoordIn[0][0] / 2;
	EmitVertex();

	//gl_Position = gl_ProjectionMatrix  * gl_Position;
	//gl_Position = gl_in[0].gl_Position;

/*
	gl_Position = gl_PositionIn[0];
	gl_Position.xy = vec2(rectangle_x, rectangle_y);
	EmitVertex();

	gl_Position.xy = vec2(rectangle_x + rectangle_width, rectangle_y);
	EmitVertex();

	gl_Position.xy = vec2(rectangle_x, rectangle_y + rectangle_height);
	EmitVertex();

	gl_Position.xy = vec2(rectangle_x + rectangle_width, rectangle_y + rectangle_height);
	EmitVertex();
*/
	EndPrimitive();
}
);
