attribute highp vec4 a_vertex;
attribute mediump vec2 a_texCoord;
varying mediump vec2 UL;
varying mediump vec2 UR;
varying mediump vec2 DL;
varying mediump vec2 DR;
uniform highp mat4 u_pvmMatrix;
uniform mediump vec2 u_displaySize;

void main()
{
    gl_Position = u_pvmMatrix * a_vertex;

    mediump float dx = pow(u_displaySize.x, -1.0) * 0.5;
    mediump float dy = pow(u_displaySize.y, -1.0) * 0.5;

    UL = a_texCoord + vec2(-dx, -dy);
    UR = a_texCoord + vec2(dx, -dy);
    DL = a_texCoord + vec2(-dx, dy);
    DR = a_texCoord + vec2(dx, dy);
}
