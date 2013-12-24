ENESIM_OPENGL_SHADER(
uniform sampler2D texture;
uniform vec4 color;

void main()
{
	vec4 texel = texture2D(texture, gl_TexCoord[0].xy);
	gl_FragColor = texel * color;
}
)

