uniform sampler2D texture_0;
uniform sampler2D texture_1;
varying vec2 tex_coord;

void main()
{
    vec4 tex_color_0 = texture2D(texture_0, tex_coord);
    vec4 tex_color_1 = texture2D(texture_1, tex_coord);

    // If the color in the second texture is black, then replace it with white.
    if (tex_color_1.rgb == vec3(0.0)) {
        gl_FragColor = vec4(1.0);
    } else {
        // Otherwise use the original color.
        gl_FragColor = tex_color_0;
    }

}
