uniform sampler2D texture_0;
uniform sampler2D texture_1;
uniform vec3 mask_color;
varying vec2 tex_coord;

// Overlays two textures in the way that the color from texture 1 is only taken,
// if the color of texture 0 has the specified "mask_color".
void main()
{
    vec3 tex_color_0 = texture2D(texture_0, tex_coord).rgb;
    vec3 tex_color_1 = texture2D(texture_1, tex_coord).rgb;
    //vec3 mask_color = vec3(1.0);
    if (tex_color_0 == mask_color) {
        gl_FragColor = vec4(tex_color_1, 1.0);
    } else {
        gl_FragColor = vec4(tex_color_0, 1.0);
    }
}
