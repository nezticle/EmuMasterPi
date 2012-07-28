precision mediump float;
varying vec2 v_texCoord;
uniform sampler2D s_texture;

void main()
{
    vec3 ink = vec3(0.58, 0.58, 0.58);
    vec3 c = texture2D(s_texture, v_texCoord.xy).rgb;
    float sat = floor(256.0*length(c))/256.0;

    gl_FragColor = vec4(sat*ink, 1);
}
