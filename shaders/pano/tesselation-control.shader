/**
 * @brief TesselationControl shader for pano.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::pano
 * @class TesselationControlShader
 */

#version 440
// EXTENSION shading_language_420pack
 
layout(vertices = 3) out;

in vec3 vertex_color[];
in vec3 vertex_normal[];
in vec2 vertex_texcoord[];
out vec3 tvertex_color[];
out vec3 tvertex_normal[];
out vec2 tvertex_texcoord[];

void main(void)
{
 gl_TessLevelInner[0] = 5.0;
 gl_TessLevelOuter[0] = 8.0;
 gl_TessLevelOuter[1] = 8.0;
 gl_TessLevelOuter[2] = 8.0;
 
 gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;
 tvertex_color[gl_InvocationID] = vertex_color[gl_InvocationID];
 tvertex_normal[gl_InvocationID] = vertex_normal[gl_InvocationID];
 tvertex_texcoord[gl_InvocationID] = vertex_texcoord[gl_InvocationID];
}
