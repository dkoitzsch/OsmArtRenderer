uniform sampler2D texture_0;
uniform float texture_width;
uniform float texture_height;
uniform int dilation_size;
varying vec2 tex_coord;

void main()
{
    float w = 1.0 / texture_width;
    float h = 1.0 / texture_height;

    vec2 tc = tex_coord.xy;

            vec2 tc_ul = tc + vec2(-1.0, 1.0) * w;
            vec2 tc_ur = tc + vec2(1.0, 1.0) * w;

            vec2 tc_dl = tc + vec2(-1.0, -1.0) * w;
            vec2 tc_dr = tc + vec2(1.0, -1.0) * w;


    vec4 ul = texture2D(texture_0, tc_ul);
            vec4 ur = texture2D(texture_0, tc_ur);
            vec4 dl = texture2D(texture_0, tc_dl);
            vec4 dr = texture2D(texture_0, tc_dr);

    gl_FragColor = min(min(ul, ur), min(dl, dr));


    /*int N = dilation_size;
    int i = 0;
    int j = 0;
    vec3 maxValue = vec3(0.0);
    vec3 tmp = vec3(0.0);
    float step_w = 1.0 / texture_width;
    float step_h = 1.0 / texture_height;

    for (i = -(N-1)/2; i< (N+1)/2 ; i++) {
        for (j = -(N-1)/2; j< (N+1)/2; j++) {
            tmp = texture2D(texture_0, vec2 ( tex_coord.s + float(i)*step_w, tex_coord.t + float(j)*step_h ) ).rgb;
            maxValue = max(tmp, maxValue);
        }
    }
    gl_FragColor = vec4(maxValue, 1.0);*/
}
