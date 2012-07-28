precision mediump float;
varying vec2 v_texCoord;
uniform sampler2D s_texture;

void main()
{
    lowp vec4 c = texture2D(s_texture, v_texCoord);
    lowp float factor = 30.0;

    c -= texture2D(s_texture, v_texCoord.xy+0.0001)*factor;
    c += texture2D(s_texture, v_texCoord.xy-0.0001)*factor;
    gl_FragColor = vec4(c.xyz, 1.0);
}
