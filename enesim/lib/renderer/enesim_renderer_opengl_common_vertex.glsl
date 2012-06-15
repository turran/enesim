ENESIM_OPENGL_SHADER(
/* FIXME this should be ported to the new glsl 3.1 standard */
void main(void)
{
	gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;
	gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;
}
)
