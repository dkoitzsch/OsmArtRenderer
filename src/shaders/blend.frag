uniform sampler2D texture_0;
uniform sampler2D texture_1;
uniform float alpha; // Default: 0.5

varying vec2 tex_coord;

void main()
{
    vec3 tex_color_0 = texture2D(texture_0, tex_coord).rgb;
    vec3 tex_color_1 = texture2D(texture_1, tex_coord).rgb;
    //const float alpha = 0.8;
    vec4 col = vec4(tex_color_0 * alpha + tex_color_1 * (1.0-alpha), 1.0);
    gl_FragColor = col;
}
