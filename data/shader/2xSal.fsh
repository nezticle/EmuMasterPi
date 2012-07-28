precision mediump float;
varying mediump vec2 UL;
varying mediump vec2 UR;
varying mediump vec2 DL;
varying mediump vec2 DR;
uniform sampler2D s_texture;

void main(void)
{
    vec3 dt = vec3(1.0, 1.0, 1.0);
    vec3 c00 = texture2D(s_texture, UL).xyz;
    vec3 c20 = texture2D(s_texture, UR).xyz;
    vec3 c02 = texture2D(s_texture, DL).xyz;
    vec3 c22 = texture2D(s_texture, DR).xyz;

    float m1 = dot(abs(c00-c22),dt)+0.001;
    float m2 = dot(abs(c02-c20),dt)+0.001;

    gl_FragColor = vec4((m1*(c02+c20)+m2*(c22+c00))/(2.0*(m1+m2)),1.0);
}
