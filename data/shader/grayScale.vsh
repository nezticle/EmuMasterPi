attribute highp vec4 a_vertex;
attribute mediump vec2 a_texCoord;
varying mediump vec2 v_texCoord;
uniform highp mat4 u_pvmMatrix;
uniform mediump vec2 u_displaySize;

void main()
{
    gl_Position = u_pvmMatrix * a_vertex;
    v_texCoord = a_texCoord;
}
