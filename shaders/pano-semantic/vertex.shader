/**
 * @brief Vertex shader for pano-semantic.
 * @author Stefan Osswald
 * @date 2018
 * @namespace articulation::shader::pano_semantic
 * @class VertexShader
 */

#version 440

layout(location = 0) in vec3 vertex_positionIn;
layout(location = 2) in vec2 vertex_texIn;

out vec2 vertex_texcoord;

void main() {
    gl_Position = vec4(vertex_positionIn, 1.0);
    vertex_texcoord = vertex_texIn;
}
