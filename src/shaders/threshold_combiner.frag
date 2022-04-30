uniform sampler2D texture_0;
uniform sampler2D texture_1;
uniform float threshold; // Ideally: 0.85 - a threshold between 0.4 and 0.7 looks good
uniform float alpha; // Default: 0.5

varying vec2 tex_coord;

//const float alpha = 0.5;
//const float threshold = 0.7; //0.85;
//const float squared_threshold = (threshold * threshold);

void main()
{
    float squared_threshold = (threshold * threshold);
    vec3 tex_color_0 = texture2D(texture_0, tex_coord).rgb;
    vec3 tex_color_1 = texture2D(texture_1, tex_coord).rgb;
    vec4 col = vec4(tex_color_0 * alpha + tex_color_1 * (1.0 - alpha), 1.0);

    float squared_distance = dot(col.rgb, col.rgb);
    if (squared_distance < squared_threshold) {
        gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        gl_FragColor = vec4(1.0);
    }
}
