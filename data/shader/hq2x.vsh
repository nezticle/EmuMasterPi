attribute highp vec4 a_vertex;
attribute mediump vec2 a_texCoord;
varying mediump vec4 v_texCoord[5];
uniform highp mat4 u_pvmMatrix;
uniform mediump vec2 u_displaySize;

void main()
{
    gl_Position = u_pvmMatrix * a_vertex;

    mediump float x = (1.0 / u_displaySize.x);
    mediump float y = (1.0 / u_displaySize.y);
    mediump vec2 dg1 = vec2( x, y);
    mediump vec2 dg2 = vec2(-x, y);
    mediump vec2 dx = vec2(x, 0.0);
    mediump vec2 dy = vec2(0.0, y);

    v_texCoord[0] = vec4(a_texCoord, 0.0, 0.0);
    v_texCoord[1].xy = v_texCoord[0].xy - dg1;
    v_texCoord[1].zw = v_texCoord[0].xy - dy;
    v_texCoord[2].xy = v_texCoord[0].xy - dg2;
    v_texCoord[2].zw = v_texCoord[0].xy + dx;
    v_texCoord[3].xy = v_texCoord[0].xy + dg1;
    v_texCoord[3].zw = v_texCoord[0].xy + dy;
    v_texCoord[4].xy = v_texCoord[0].xy + dg2;
    v_texCoord[4].zw = v_texCoord[0].xy - dx;
}
