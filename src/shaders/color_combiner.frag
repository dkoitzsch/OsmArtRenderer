uniform sampler2D texture_0;
uniform sampler2D texture_1;
varying vec2 tex_coord;

void main()
{
    vec4 tex_color_0 = texture2D(texture_0, tex_coord);
    vec4 tex_color_1 = texture2D(texture_1, tex_coord);

    // If the color is white at this coordinate, then use the original color.
    if (tex_color_0 == vec4(1.0)) {
        gl_FragColor = tex_color_0;
    } else {
        // If the color is not white, then use the color of the other texture.
        gl_FragColor = vec4(tex_color_1.rgb, tex_color_0.a);
    }

}
