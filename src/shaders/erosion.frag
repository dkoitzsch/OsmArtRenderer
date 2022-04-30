uniform sampler2D texture_0;
uniform float texture_width;
uniform float texture_height;
uniform int erosion_size;
varying vec2 tex_coord;

void main()
{
    float w = 1.0 / texture_width;
    float h = 1.0 / texture_height;
    vec2 tc = tex_coord.xy;
    vec2 tc_p1 = tc + vec2(1.0) * w;
    vec2 tc_p2 = tc + vec2(2.0) * w;
    vec2 tc_p3 = tc + vec2(3.0) * w;
    vec2 tc_m1 = tc + vec2(-1.0) * w;
    vec2 tc_m2 = tc + vec2(-2.0) * w;
    vec2 tc_m3 = tc + vec2(-3.0) * w;
    vec2 tc_m4 = tc + vec2(-4.0) * w;

    gl_FragColor = max(max(max(max(max(max(max(
        texture2D(texture_0, tc),
        texture2D(texture_0, tc_p2)),
        texture2D(texture_0, tc_p1)),
        texture2D(texture_0, tc_p3)),
        texture2D(texture_0, tc_m1)),
        texture2D(texture_0, tc_m2)),
        texture2D(texture_0, tc_m3)),
        texture2D(texture_0, tc_m4));

    /*int N = erosion_size;
    int i = 0, j=0;
    vec3 minValue = vec3(1.0);
    vec3 tmp = vec3(0.0);
    float step_w = 1.0 / texture_width;
    float step_h = 1.0 / texture_height;

    for (i = -(N-1)/2; i< (N+1)/2 ; i++) {
        for (j = -(N-1)/2; j< (N+1)/2; j++) {
            tmp = texture2D(texture_0, vec2 ( tex_coord.s + float(i)*step_w, tex_coord.t + float(j)*step_h ) ).rgb;
            minValue = min(tmp, minValue);
        }
    }
    gl_FragColor = vec4(minValue, 1.0);*/
}
