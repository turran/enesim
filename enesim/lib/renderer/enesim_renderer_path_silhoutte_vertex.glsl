ENESIM_OPENGL_SHADER(
void main(void)
{
	/* this vertex shader passses as the color the texture
	 * coordinates: We pass them in the texture coordinates
	 * to avoid the glColor4f clamping. And in order
	 * to be passed to the fragment shader without
	 * clamping and interpolated we need to put
	 *  glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
	 *  glShadeModel(GL_FLAT);
	 * on the caller
	 */

	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
	gl_FrontColor.rg = gl_MultiTexCoord0.xy;
	gl_FrontColor.ba = gl_MultiTexCoord0.zw;
}
)

