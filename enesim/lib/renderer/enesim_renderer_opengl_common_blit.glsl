ENESIM_OPENGL_SHADER(
uniform sampler2D blit_texture;
uniform sampler2D blit_color;

void main()
{
	vec4 texel = texture2D(blit_texture, gl_TexCoord[0].xy);
	gl_FragColor = texel * blit_color;
}
)

