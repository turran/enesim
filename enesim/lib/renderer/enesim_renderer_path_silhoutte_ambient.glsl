ENESIM_OPENGL_SHADER(
uniform vec4 ambient_final_color;

void main()
{
	float r = 1.0;
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
	gl_FragColor = ambient_final_color * d;
}
)
