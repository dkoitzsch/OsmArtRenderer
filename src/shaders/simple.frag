uniform sampler2D texture_0;
varying vec2 tex_coord;

void main()
{
    gl_FragColor = texture2D(texture_0, tex_coord);
}
