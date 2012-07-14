ENESIM_OPENGL_SHADER(
uniform sampler2D merge_texture_0;
uniform sampler2D merge_texture_1;

void main()
{
	vec4 texel0 = texture2D(merge_texture_0, gl_TexCoord[0].xy);
	vec4 texel1 = texture2D(merge_texture_1, gl_TexCoord[0].xy);
	gl_FragColor = mix(texel1, texel0, texel0.a);
	//gl_FragColor = texel1.aaaa;
	//gl_TexCoord[0]; //vec4(1.0, 0.0, 1.0, 1.0);
}
)
