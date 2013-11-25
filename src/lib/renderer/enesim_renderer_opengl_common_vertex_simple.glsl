ENESIM_OPENGL_SHADER(
/* For several renderers the position can be transformed by using
 * an offset for x and y through the enesim_renderer_draw() arguments
 * Even so, the texture coordinates always match the original position
 */
void main(void)
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_Vertex;
}
)
