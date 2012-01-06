ENESIM_OPENGL_SHADER(
uniform float rectangle_width;
uniform float rectangle_height;
uniform float rectangle_x;
uniform float rectangle_y;
uniform mat3 pixel_transform;


/* generate a corner starting at x, y of radius r
 * with iters precission
 */
void round(float x, float y, float r, int iters)
{
	float deg = 90.0 / (iters + 1);
	float theta;
	float nx;
	float ny;
	int i;

	gl_Position.xyz = vec3(x - r, y, 1) * pixel_transform;
	gl_TexCoord[0]  = vec4(0,0,0,1);
	EmitVertex();

	theta = deg;;
	nx = x + r * cos(theta);
	ny = y + r * sin(theta);

	gl_Position.xyz = vec3(nx, ny, 1) * pixel_transform;
	gl_TexCoord[0]  = vec4(0,0,0,1);
	EmitVertex();

	gl_Position.xyz = vec3(x, y + r, 1) * pixel_transform;
	gl_TexCoord[0]  = vec4(0,0,0,1);
	EmitVertex();
}

void strokeless_rounded(float x1, float y1, float x2, float y2, float r, bvec4 corners)
{
}

void strokeless(float x1, float y1, float x2, float y2)
{
	gl_Position = gl_PositionIn[0];
	gl_Position.xyz = vec3(x1, y1, 1) * pixel_transform;
	gl_TexCoord[0]  = vec4(0,0,0,1);
	EmitVertex();

	gl_Position.xyz = vec3(x2, y1, 1) * pixel_transform;
	gl_TexCoord[0]  = vec4(1,0,0,1);
	EmitVertex();

	gl_Position.xyz = vec3(x1, y2,1) * pixel_transform;
	gl_TexCoord[0]  = vec4(0,1,0,1);
	EmitVertex();

	gl_Position.xyz = vec3(x2, y2, 1) * pixel_transform;
	gl_TexCoord[0]  = vec4(1,1,0,1);
	EmitVertex();

	EndPrimitive();
}

void stroke(float x1, float y1, float x2, float y2, float r)
{
	float ix1 = x1 + r;
	float iy1 = y1 + r;
	float ix2 = x2 - r;
	float iy2 = y2 - r;

	
	gl_Position = gl_PositionIn[0];
	gl_Position.xyz = vec3(x1, y1, 1) * pixel_transform;
	gl_TexCoord[0]  = vec4(0,0,0,1);
	EmitVertex();

	gl_Position.xyz = vec3(ix1, iy1, 1) * pixel_transform;
	gl_TexCoord[0]  = vec4(1,0,0,1);
	EmitVertex();

	gl_Position.xyz = vec3(x2, y1, 1) * pixel_transform;
	gl_TexCoord[0]  = vec4(0,1,0,1);
	EmitVertex();

	gl_Position.xyz = vec3(ix2, iy1, 1) * pixel_transform;
	gl_TexCoord[0]  = vec4(1,1,0,1);
	EmitVertex();

	gl_Position.xyz = vec3(x2, y2, 1) * pixel_transform;
	gl_TexCoord[0]  = vec4(1,0,0,1);
	EmitVertex();

	gl_Position.xyz = vec3(ix2, iy2, 1) * pixel_transform;
	gl_TexCoord[0]  = vec4(1,0,0,1);
	EmitVertex();

	gl_Position.xyz = vec3(x1, y2, 1) * pixel_transform;
	gl_TexCoord[0]  = vec4(1,0,0,1);
	EmitVertex();

	gl_Position.xyz = vec3(ix1, iy2, 1) * pixel_transform;
	gl_TexCoord[0]  = vec4(1,0,0,1);
	EmitVertex();

	gl_Position.xyz = vec3(x1, y1, 1) * pixel_transform;
	gl_TexCoord[0]  = vec4(1,0,0,1);
	EmitVertex();

	gl_Position.xyz = vec3(ix1, iy1, 1) * pixel_transform;
	gl_TexCoord[0]  = vec4(1,0,0,1);
	EmitVertex();

	EndPrimitive();
}

void main()
{
	float x1 = rectangle_x;
	float x2 = rectangle_x + rectangle_width;
	float y1 = rectangle_y;
	float y2 = rectangle_y + rectangle_height;

	//strokeless_rounded(x1, y1, x2, y2, 5);
	//strokeless(x1, y1, x2, y2);
	stroke(x1, y1, x2, y2, 10);
}
);
