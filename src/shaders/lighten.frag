uniform sampler2D texture_0;
varying vec2 tex_coord;

void main() {
    vec4 col = texture2D(texture_0, tex_coord);
    gl_FragColor = vec4(col.rgb + vec3(0.2), col.a);
}
