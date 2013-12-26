ENESIM_OPENGL_SHADER(
uniform sampler2D texture_texture;
uniform vec4 texture_color;

void main()
{
	vec4 texel = texture2D(texture_texture, gl_TexCoord[0].xy);
	gl_FragColor = texel * texture_color;
}
)

