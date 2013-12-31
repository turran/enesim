ENESIM_OPENGL_SHADER(
uniform sampler2D texture_texture;
uniform vec4 texture_color;
uniform ivec2 texture_size;
uniform ivec2 texture_offset;

void main()
{
	vec2 pos;

	/* first translate the offset */
	pos.x = gl_TexCoord[0].x - float(texture_offset.x);
	pos.y = gl_TexCoord[0].y - float(texture_offset.y);

	/* the coordinates are normalized (i.e from 0 to 1) */
	pos.x = pos.x / float(texture_size.x);
	pos.y = pos.y / float(texture_size.y);
	/* also, the 0,0 texel is the lower left corner,
	 * in enesim the 0,0 texel is the top left corner
	 */
	pos.y = 1.0 - pos.y;

	vec4 texel = texture2D(texture_texture, pos);
	gl_FragColor = texel * texture_color;
}
)
