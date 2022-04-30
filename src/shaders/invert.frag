uniform sampler2D texture_0;
varying vec2 tex_coord;

void main()
{
    vec4 tex_color = texture2D(texture_0, tex_coord);
    gl_FragColor = vec4(1.0 - tex_color.r, 1.0 - tex_color.g, 1.0 - tex_color.b, 1.0);
}
