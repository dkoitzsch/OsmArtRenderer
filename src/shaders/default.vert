attribute vec4 in_position;
attribute vec2 in_tex_coord;
varying vec2 tex_coord;
void main()
{
   gl_Position = in_position;
   tex_coord = in_tex_coord;
}


/* DEFAULT: */
/*
attribute vec4 qt_Vertex;
attribute vec4 qt_MultiTexCoord0;
uniform mat4 qt_ModelViewProjectionMatrix;
varying vec4 qt_TexCoord0;

void main(void)
{
    gl_Position = qt_ModelViewProjectionMatrix * qt_Vertex;
    qt_TexCoord0 = qt_MultiTexCoord0;
}
*/
