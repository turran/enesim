ENESIM_OPENGL_SHADER(
uniform sampler2D texture_texture;
uniform vec4 texture_color;
uniform ivec2 texture_size;
uniform ivec2 texture_offset;

void main()
{
	float r = 1.0;
	vec2 pos;
	vec2 pt0 = gl_Color.rg;
	vec2 pt1 = gl_Color.ba;
	vec2 n = vec2(pt1.y - pt0.y, pt0.x - pt1.x);
	vec2 nn = normalize(n);
	vec2 p0 = vec2(pt0.x, pt0.y) + (r * nn);
	vec2 p1 = vec2(pt0.x, pt0.y) - (r * nn);
	vec2 p2 = vec2(pt1.x, pt1.y) - (r * nn);
	vec2 p3 = vec2(pt1.x, pt1.y) + (r * nn);

	float s = 1.0 / ((2.0 * r) + length(n));
	vec3 e0 = s * vec3(p0.y - p3.y, p3.x - p0.x, (p0.x * p3.y) - (p0.y * p3.x));
	vec3 e1 = s * vec3(p2.y - p1.y, p1.x - p2.x, (p2.x * p1.y) - (p2.y * p1.x));

	vec3 p = vec3(gl_FragCoord.x, gl_FragCoord.y, 1.0);
	vec2 distance = vec2(dot(e0, p), dot(e1, p));

	if (distance.x < 0.0 || distance.y < 0.0)
		discard;
	float d = min(distance.x, distance.y);

	/* the coordinates are normalized (i.e from 0 to 1) */
	pos.x = gl_TexCoord[0].x / float(texture_size.x);
	pos.y = gl_TexCoord[0].y / float(texture_size.y);
	/* also, the 0,0 texel is the lower left corner,
	 * in enesim the 0,0 texel is the top left corner
	 */
	pos.y = 1.0 - pos.y;
	vec4 texel = texture2D(texture_texture, pos);
	gl_FragColor = texel * texture_color;
	gl_FragColor.a = d;
}
)

